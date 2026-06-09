#include "util/skill_config_package.h"

#include "core/Common.h"
#include "core/GameAddresses.h"
#include "hook/InlineHook.h"
#include "skill/skill_overlay_bridge.h"
#include "util/runtime_paths.h"

#include <windows.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <cwctype>
#include <map>
#include <string>
#include <vector>

namespace ssw
{
namespace skillpack
{
namespace
{
    static const wchar_t* kPackageFileName = L"skill_pack.ssp";
    static const unsigned char kHeaderMagic[4] = {'S', 'S', 'P', 'K'};
    static const unsigned char kManifestMagic[4] = {'M', 'P', 'K', '1'};
    static const unsigned int kPackageVersion = 1;
    static const unsigned int kManifestVersion = 1;
    static const unsigned int kPackageFlagsEncrypted = 1;
    static const unsigned long long kFileHashSeedSalt = 0x7A3C9D2F14A5B6C7ULL;
    static const unsigned long long kManifestHashSeedSalt = 0x2C8D71A54F3B9E11ULL;
    static const unsigned long long kMasterSeedSalt = 0x9E3779B97F4A7C15ULL;
    static const unsigned long long kManifestSeedSalt = 0xD1342543DE82EF95ULL;
    static const unsigned long long kFileSeedSalt = 0x94D049BB133111EBULL;
    static const unsigned int kRuntimePasswordXorMask = 0x42C27E06U;

    struct PackageStamp
    {
        bool exists = false;
        unsigned long long fileSize = 0;
        FILETIME lastWriteTime = {};
    };

    struct PackageEntry
    {
        std::wstring relativePath;
        bool isBinary = false;
        unsigned long long plainSize = 0;
        unsigned long long cipherSize = 0;
        unsigned long long payloadOffset = 0;
        unsigned long long fileHash = 0;
        unsigned long long pathHash = 0;
        unsigned long long fileSeed = 0;
        std::vector<unsigned char> plainBytes;
    };

    struct PackageState
    {
        bool initialized = false;
        std::wstring skillConfigDir;
        std::wstring packagePath;
        std::wstring passwordSignature;
        bool runtimePasswordPending = false;
        DWORD pendingRetryTick = 0;
        PackageStamp stamp;
        bool packageLoaded = false;
        bool packageAttempted = false;
        bool physicalFallbackBlockedLogged = false;
        std::vector<PackageEntry> entries;
        std::map<std::wstring, size_t> entryIndexByPath;
    };

    struct PasswordContext
    {
        bool hasPassword = false;
        std::wstring text;
        unsigned long long passwordKey = 0;
        unsigned long long forwardHash = 0;
        unsigned long long reverseHash = 0;
    };

    struct PasswordCandidateSet
    {
        std::vector<std::wstring> passwords;
        std::wstring signature;
        uintptr_t runtimePasswordAddress = 0;
        unsigned int runtimePasswordRaw = 0;
        bool runtimePasswordPending = false;
    };

    void AppendUniquePasswordCandidate(std::vector<std::wstring>& passwords, const std::wstring& password)
    {
        if (password.empty())
            return;
        if (std::find(passwords.begin(), passwords.end(), password) == passwords.end())
            passwords.push_back(password);
    }

    SRWLOCK g_stateLock = SRWLOCK_INIT;
    PackageState g_state;

    struct PackageStateLockGuard
    {
        PackageStateLockGuard()
        {
            ::AcquireSRWLockExclusive(&g_stateLock);
        }

        ~PackageStateLockGuard()
        {
            ::ReleaseSRWLockExclusive(&g_stateLock);
        }
    };

    std::wstring NormalizeRelativePath(std::wstring path)
    {
        for (size_t i = 0; i < path.size(); ++i)
        {
            if (path[i] == L'\\')
                path[i] = L'/';
            else
                path[i] = static_cast<wchar_t>(::towlower(path[i]));
        }

        while (!path.empty() && path[0] == L'/')
            path.erase(path.begin());
        return path;
    }

    std::wstring Utf8ToWide(const std::string& text)
    {
        if (text.empty())
            return std::wstring();

        const int required = ::MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
        if (required <= 1)
            return std::wstring();

        std::wstring result;
        result.resize(static_cast<size_t>(required - 1));
        ::MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, &result[0], required);
        return result;
    }

    std::string WideToUtf8String(const std::wstring& text)
    {
        return ssw::path::WideToUtf8(text);
    }

    std::wstring GetRelativePath(const std::wstring& root, const std::wstring& absolutePath)
    {
        const std::wstring fullRoot = ssw::path::TrimTrailingSlash(root);
        if (fullRoot.empty())
            return NormalizeRelativePath(absolutePath);

        const std::wstring trimmedPath = ssw::path::TrimTrailingSlash(absolutePath);
        if (trimmedPath.size() > fullRoot.size() &&
            _wcsnicmp(trimmedPath.c_str(), fullRoot.c_str(), fullRoot.size()) == 0 &&
            (trimmedPath[fullRoot.size()] == L'\\' || trimmedPath[fullRoot.size()] == L'/'))
        {
            return NormalizeRelativePath(trimmedPath.substr(fullRoot.size() + 1));
        }

        const size_t slashPos = trimmedPath.find_last_of(L"\\/");
        if (slashPos == std::wstring::npos)
            return NormalizeRelativePath(trimmedPath);
        return NormalizeRelativePath(trimmedPath.substr(slashPos + 1));
    }

    bool PackageFileExists(const std::wstring& skillConfigDir)
    {
        const std::wstring directory = ssw::path::TrimTrailingSlash(skillConfigDir);
        if (directory.empty())
            return false;
        return ssw::path::FileExists(ssw::path::Combine(directory, kPackageFileName));
    }

    bool HasPlainSkillConfigMarker(const std::wstring& skillConfigDir)
    {
        const std::wstring directory = ssw::path::TrimTrailingSlash(skillConfigDir);
        if (directory.empty())
            return false;
        return ssw::path::FileExists(ssw::path::Combine(directory, L"super_skills.json"));
    }

    bool ShouldPreferPackageForDirectory(const std::wstring& skillConfigDir)
    {
        return PackageFileExists(skillConfigDir);
    }

    bool ReadBinaryFilePhysical(const std::wstring& path, std::vector<unsigned char>& outBytes)
    {
        outBytes.clear();

        FILE* file = nullptr;
        if (_wfopen_s(&file, path.c_str(), L"rb") != 0 || !file)
            return false;

        if (fseek(file, 0, SEEK_END) != 0)
        {
            fclose(file);
            return false;
        }

        long size = ftell(file);
        if (size < 0)
        {
            fclose(file);
            return false;
        }

        if (fseek(file, 0, SEEK_SET) != 0)
        {
            fclose(file);
            return false;
        }

        outBytes.resize(static_cast<size_t>(size));
        const size_t readSize = size > 0
            ? fread(&outBytes[0], 1, static_cast<size_t>(size), file)
            : 0;
        fclose(file);

        if (readSize != static_cast<size_t>(size))
        {
            outBytes.clear();
            return false;
        }

        return true;
    }

    bool ReadUInt32(const std::vector<unsigned char>& bytes, size_t& offset, unsigned int& outValue)
    {
        outValue = 0;
        if (offset + 4 > bytes.size())
            return false;

        outValue =
            static_cast<unsigned int>(bytes[offset]) |
            (static_cast<unsigned int>(bytes[offset + 1]) << 8) |
            (static_cast<unsigned int>(bytes[offset + 2]) << 16) |
            (static_cast<unsigned int>(bytes[offset + 3]) << 24);
        offset += 4;
        return true;
    }

    bool ReadUInt64(const std::vector<unsigned char>& bytes, size_t& offset, unsigned long long& outValue)
    {
        outValue = 0;
        if (offset + 8 > bytes.size())
            return false;

        outValue =
            static_cast<unsigned long long>(bytes[offset]) |
            (static_cast<unsigned long long>(bytes[offset + 1]) << 8) |
            (static_cast<unsigned long long>(bytes[offset + 2]) << 16) |
            (static_cast<unsigned long long>(bytes[offset + 3]) << 24) |
            (static_cast<unsigned long long>(bytes[offset + 4]) << 32) |
            (static_cast<unsigned long long>(bytes[offset + 5]) << 40) |
            (static_cast<unsigned long long>(bytes[offset + 6]) << 48) |
            (static_cast<unsigned long long>(bytes[offset + 7]) << 56);
        offset += 8;
        return true;
    }

    bool ReadByte(const std::vector<unsigned char>& bytes, size_t& offset, unsigned char& outValue)
    {
        outValue = 0;
        if (offset >= bytes.size())
            return false;

        outValue = bytes[offset++];
        return true;
    }

    bool ReadBytes(const std::vector<unsigned char>& bytes, size_t& offset, size_t count, std::vector<unsigned char>& outBytes)
    {
        outBytes.clear();
        if (count == 0)
            return true;
        if (offset + count > bytes.size())
            return false;

        outBytes.resize(count);
        memcpy(&outBytes[0], &bytes[offset], count);
        offset += count;
        return true;
    }

    bool ReadString(const std::vector<unsigned char>& bytes, size_t& offset, std::string& outValue)
    {
        outValue.clear();

        unsigned int length = 0;
        if (!ReadUInt32(bytes, offset, length))
            return false;
        if (offset + length > bytes.size())
            return false;

        if (length == 0)
        {
            outValue.clear();
            return true;
        }

        outValue.assign(
            reinterpret_cast<const char*>(bytes.data() + offset),
            reinterpret_cast<const char*>(bytes.data() + offset + length));
        offset += length;
        return true;
    }

    unsigned long long RotL64(unsigned long long value, int rot)
    {
        rot &= 63;
        if (rot == 0)
            return value;
        return (value << rot) | (value >> (64 - rot));
    }

    unsigned long long Mix64(unsigned long long value)
    {
        value ^= value >> 30;
        value *= 0xBF58476D1CE4E5B9ULL;
        value ^= value >> 27;
        value *= 0x94D049BB133111EBULL;
        value ^= value >> 31;
        return value;
    }

    unsigned long long ReadUInt64LittleEndian(const unsigned char* bytes, int index)
    {
        return static_cast<unsigned long long>(bytes[index]) |
            (static_cast<unsigned long long>(bytes[index + 1]) << 8) |
            (static_cast<unsigned long long>(bytes[index + 2]) << 16) |
            (static_cast<unsigned long long>(bytes[index + 3]) << 24) |
            (static_cast<unsigned long long>(bytes[index + 4]) << 32) |
            (static_cast<unsigned long long>(bytes[index + 5]) << 40) |
            (static_cast<unsigned long long>(bytes[index + 6]) << 48) |
            (static_cast<unsigned long long>(bytes[index + 7]) << 56);
    }

    unsigned long long PackHash64(const unsigned char* bytes, size_t length, unsigned long long seed);

    unsigned long long SecretSeedA()
    {
        const char* secret = "SuperSkillWnd.SkillPack.alpha.2026-05-11";
        return PackHash64(reinterpret_cast<const unsigned char*>(secret), strlen(secret), 0x13579BDF2468ACE0ULL);
    }

    unsigned long long SecretSeedB()
    {
        const char* secret = "SuperSkillWnd.SkillPack.beta.Data.Plugins.SS.Skill";
        return PackHash64(reinterpret_cast<const unsigned char*>(secret), strlen(secret), 0x0F1E2D3C4B5A6978ULL);
    }

    unsigned long long PackHash64(const unsigned char* bytes, size_t length, unsigned long long seed)
    {
        unsigned long long hash = 0xCBF29CE484222325ULL ^ seed;
        for (size_t i = 0; i < length; ++i)
        {
            hash ^= static_cast<unsigned long long>(bytes[i]) + 0x9E3779B97F4A7C15ULL;
            hash *= 0x100000001B3ULL;
            hash = RotL64(hash, 13) ^ (hash >> 7);
            hash += 0x94D049BB133111EBULL;
        }

        hash ^= static_cast<unsigned long long>(length) * 0xA24BAED4963EE407ULL;
        return Mix64(hash);
    }

    unsigned long long ComputePathHash(const std::wstring& normalizedRelativePath)
    {
        const std::string utf8 = WideToUtf8String(NormalizeRelativePath(normalizedRelativePath));
        return PackHash64(reinterpret_cast<const unsigned char*>(utf8.c_str()), utf8.size(), 0xC3A5C85C97CB3127ULL);
    }

    unsigned long long ComputeFileHash(const std::vector<unsigned char>& bytes, unsigned long long pathHash)
    {
        if (bytes.empty())
            return PackHash64(nullptr, 0, pathHash ^ kFileHashSeedSalt);
        return PackHash64(&bytes[0], bytes.size(), pathHash ^ kFileHashSeedSalt);
    }

    unsigned long long ComputeManifestHash(const std::vector<unsigned char>& bytes)
    {
        if (bytes.empty())
            return PackHash64(nullptr, 0, kManifestHashSeedSalt);
        return PackHash64(&bytes[0], bytes.size(), kManifestHashSeedSalt);
    }

    std::wstring TrimWhitespace(const std::wstring& text)
    {
        size_t begin = 0;
        while (begin < text.size() && std::iswspace(text[begin]) != 0)
            ++begin;

        size_t end = text.size();
        while (end > begin && std::iswspace(text[end - 1]) != 0)
            --end;

        return text.substr(begin, end - begin);
    }

    std::wstring DwordToIpStringBigEndian(unsigned int value)
    {
        wchar_t buffer[32] = {};
        swprintf_s(buffer, L"%u.%u.%u.%u",
            (value >> 24) & 0xFFU,
            (value >> 16) & 0xFFU,
            (value >> 8) & 0xFFU,
            value & 0xFFU);
        return buffer;
    }

    std::wstring DwordToIpStringLittleEndian(unsigned int value)
    {
        wchar_t buffer[32] = {};
        swprintf_s(buffer, L"%u.%u.%u.%u",
            value & 0xFFU,
            (value >> 8) & 0xFFU,
            (value >> 16) & 0xFFU,
            (value >> 24) & 0xFFU);
        return buffer;
    }

    unsigned long long BuildPasswordKey(const PasswordContext& passwordContext)
    {
        if (!passwordContext.hasPassword)
            return 0;

        const unsigned long long lengthMix = static_cast<unsigned long long>(passwordContext.text.size()) * 0x9E3779B97F4A7C15ULL;
        return Mix64(passwordContext.forwardHash ^ RotL64(passwordContext.reverseHash, 17) ^ lengthMix ^ 0xD1342543DE82EF95ULL);
    }

    PasswordContext BuildPasswordContext(const std::wstring& passwordText)
    {
        PasswordContext context;
        context.text = TrimWhitespace(passwordText);
        if (context.text.empty())
            return context;

        const std::string utf8 = WideToUtf8String(context.text);
        std::string reversedUtf8(utf8.rbegin(), utf8.rend());

        context.hasPassword = true;
        context.forwardHash = PackHash64(reinterpret_cast<const unsigned char*>(utf8.data()), utf8.size(), 0x6A09E667F3BCC909ULL);
        context.reverseHash = PackHash64(reinterpret_cast<const unsigned char*>(reversedUtf8.data()), reversedUtf8.size(), 0xBB67AE8584CAA73BULL);
        return context;
    }

    bool IsPackageCoveredRelativePath(const std::wstring& relativePath)
    {
        const std::wstring normalized = NormalizeRelativePath(relativePath);
        if (normalized.empty())
            return false;

        const std::wstring imgJsonSuffix = L".img.json";
        if (normalized.size() >= imgJsonSuffix.size() &&
            _wcsicmp(normalized.c_str() + normalized.size() - imgJsonSuffix.size(), imgJsonSuffix.c_str()) == 0)
            return true;
        if (_wcsicmp(normalized.c_str(), L"feature_switches.json") == 0)
            return true;
        if (_wcsicmp(normalized.c_str(), L"module_switches.json") == 0)
            return true;
        if (_wcsicmp(normalized.c_str(), L"super_skills.json") == 0)
            return true;
        if (_wcsicmp(normalized.c_str(), L"custom_skill_routes.json") == 0)
            return true;
        if (_wcsicmp(normalized.c_str(), L"native_skill_injections.json") == 0)
            return true;
        if (_wcsicmp(normalized.c_str(), L"skill_local_cache.bin") == 0)
            return true;
        if (normalized.size() > 11 &&
            normalized.compare(0, 11, L"ui/skillex/") == 0)
            return true;
        return false;
    }

    PasswordCandidateSet BuildPasswordCandidateSet()
    {
        PasswordCandidateSet result;
        WriteLog("[InitStage] enter BuildPasswordCandidateSet");

        if (SafeIsBadReadPtr((void*)ADDR_4D6A13, 8))
        {
            WriteLogFmt("[InitStage] BuildPasswordCandidateSet invalid recv root=0x%08X", ADDR_4D6A13);
            result.signature = L"recv-root-invalid";
            WriteLog("[InitStage] leave BuildPasswordCandidateSet invalid-recv-root");
            return result;
        }

        WriteLogFmt("[InitStage] BuildPasswordCandidateSet before FollowJmpChain recvRoot=0x%08X", ADDR_4D6A13);
        const BYTE* recvEntry = FollowJmpChain((void*)ADDR_4D6A13);
        WriteLogFmt("[InitStage] BuildPasswordCandidateSet after FollowJmpChain recvEntry=0x%08X",
            (unsigned int)(uintptr_t)recvEntry);
        BYTE* recvStubTarget = nullptr;
        if (recvEntry && !SafeIsBadReadPtr(recvEntry, 8))
        {
            const BYTE movOpcode = recvEntry[0];
            if (movOpcode >= 0xB8 && movOpcode <= 0xBF)
            {
                const BYTE regIndex = static_cast<BYTE>(movOpcode - 0xB8);
                if (recvEntry[5] == 0xFF && recvEntry[6] == static_cast<BYTE>(0xE0 + regIndex))
                {
                    const DWORD target = *(DWORD*)(recvEntry + 1);
                    if (target != 0 && target != (DWORD)(uintptr_t)recvEntry)
                    {
                        if (SafeIsBadReadPtr((void*)(uintptr_t)target, 8))
                        {
                            WriteLogFmt("[InitStage] BuildPasswordCandidateSet invalid recv target=0x%08X", target);
                        }
                        recvStubTarget = FollowJmpChain((void*)(uintptr_t)target);
                    }
                }
            }
        }
        else if (recvEntry)
        {
            WriteLogFmt("[InitStage] BuildPasswordCandidateSet unreadable recvEntry=0x%08X",
                (unsigned int)(uintptr_t)recvEntry);
        }

        WriteLogFmt("[InitStage] BuildPasswordCandidateSet recvStubTarget=0x%08X",
            (unsigned int)(uintptr_t)recvStubTarget);

        if (recvStubTarget && !SafeIsBadReadPtr(recvStubTarget, 0x200))
        {
            WriteLog("[InitStage] BuildPasswordCandidateSet scanning recv stub");
            for (size_t i = 0; i + 13 < 0x200; ++i)
            {
                BYTE* p = recvStubTarget + i;
                if (p[0] == 0x0F &&
                    p[1] == 0xB7 &&
                    p[2] == 0xC0 &&
                    p[3] == 0x8D &&
                    p[4] == 0x48 &&
                    p[5] == 0xF0 &&
                    p[6] == 0x60 &&
                    p[7] == 0x8B &&
                    p[8] == 0x15)
                {
                    result.runtimePasswordAddress = *(DWORD*)(p + 9);
                    break;
                }
            }
        }
        else if (recvStubTarget)
        {
            WriteLogFmt("[InitStage] BuildPasswordCandidateSet unreadable recvStubTarget=0x%08X",
                (unsigned int)(uintptr_t)recvStubTarget);
        }

        if (result.runtimePasswordAddress != 0)
        {
            SkillOverlayBridgeSetRuntimePasswordAddress(result.runtimePasswordAddress);
            WriteLogFmt("[InitStage] BuildPasswordCandidateSet found runtimePasswordAddress=0x%08X",
                (unsigned int)result.runtimePasswordAddress);
        }
        else
        {
            const uintptr_t cachedRuntimePasswordAddress = SkillOverlayBridgeGetRuntimePasswordAddress();
            if (cachedRuntimePasswordAddress != 0)
            {
                result.runtimePasswordAddress = cachedRuntimePasswordAddress;
                WriteLogFmt("[SkillPack] reuse cached runtime password address=0x%08X",
                    (unsigned int)cachedRuntimePasswordAddress);
            }
        }

        if (result.runtimePasswordAddress != 0 && !SafeIsBadReadPtr((void*)(uintptr_t)result.runtimePasswordAddress, sizeof(DWORD)))
        {
            result.runtimePasswordRaw = *(volatile DWORD*)(uintptr_t)result.runtimePasswordAddress;
        }
        else
        {
            result.runtimePasswordPending = result.runtimePasswordAddress != 0;
            result.runtimePasswordRaw = 0;
        }

        if (result.runtimePasswordAddress != 0 && result.runtimePasswordRaw == 0)
            result.runtimePasswordPending = true;

        WriteLogFmt(
            "[InitStage] BuildPasswordCandidateSet addr=0x%08X raw=0x%08X pending=%d",
            (unsigned int)result.runtimePasswordAddress,
            result.runtimePasswordRaw,
            result.runtimePasswordPending ? 1 : 0);

        if (result.runtimePasswordAddress != 0 && result.runtimePasswordRaw != 0)
        {
            const std::wstring bigEndian = DwordToIpStringBigEndian(result.runtimePasswordRaw);
            const std::wstring littleEndian = DwordToIpStringLittleEndian(result.runtimePasswordRaw);
            const unsigned int xorValue = result.runtimePasswordRaw ^ kRuntimePasswordXorMask;
            const std::wstring xorBigEndian = DwordToIpStringBigEndian(xorValue);
            const std::wstring xorLittleEndian = DwordToIpStringLittleEndian(xorValue);

            AppendUniquePasswordCandidate(result.passwords, bigEndian);
            AppendUniquePasswordCandidate(result.passwords, littleEndian);
            AppendUniquePasswordCandidate(result.passwords, xorLittleEndian);
            AppendUniquePasswordCandidate(result.passwords, xorBigEndian);
        }

        if (result.passwords.empty())
        {
            if (!result.runtimePasswordPending)
                result.passwords.push_back(L"");
        }
        else
            result.passwords.push_back(L"");

        wchar_t signatureBuffer[256] = {};
        const std::wstring first = result.passwords.size() > 0 ? result.passwords[0] : L"";
        const std::wstring second = result.passwords.size() > 1 ? result.passwords[1] : L"";
        const std::wstring third = result.passwords.size() > 2 ? result.passwords[2] : L"";
        const std::wstring fourth = result.passwords.size() > 3 ? result.passwords[3] : L"";
        swprintf_s(signatureBuffer,
            L"recv=0x%08X ptr=0x%08X raw=0x%08X pending=%d p0=%ls p1=%ls p2=%ls p3=%ls",
            (unsigned int)(uintptr_t)recvStubTarget,
            (unsigned int)result.runtimePasswordAddress,
            result.runtimePasswordRaw,
            result.runtimePasswordPending ? 1 : 0,
            first.c_str(),
            second.c_str(),
            third.c_str(),
            fourth.c_str());
        result.signature = signatureBuffer;
        WriteLogFmt("[InitStage] leave BuildPasswordCandidateSet signature=%s",
            WideToUtf8String(result.signature).c_str());

        return result;
    }

    unsigned long long BuildMasterSeed(const unsigned char* salt, unsigned long long manifestCipherSize, unsigned long long payloadCipherSize, const PasswordContext& passwordContext)
    {
        const unsigned long long saltLo = ReadUInt64LittleEndian(salt, 0);
        const unsigned long long saltHi = ReadUInt64LittleEndian(salt, 8);
        const unsigned long long secret = SecretSeedA() ^ SecretSeedB() ^ BuildPasswordKey(passwordContext);
        return Mix64(secret ^ saltLo ^ saltHi ^ manifestCipherSize ^ payloadCipherSize ^ kMasterSeedSalt);
    }

    unsigned long long BuildManifestSeed(unsigned long long masterSeed, const unsigned char* salt, unsigned long long manifestCipherSize, unsigned long long payloadCipherSize, const PasswordContext& passwordContext)
    {
        const unsigned long long saltLo = ReadUInt64LittleEndian(salt, 0);
        const unsigned long long saltHi = ReadUInt64LittleEndian(salt, 8);
        const unsigned long long passwordKey = BuildPasswordKey(passwordContext);
        return Mix64(masterSeed ^ saltLo ^ saltHi ^ manifestCipherSize ^ payloadCipherSize ^ kManifestSeedSalt ^ RotL64(passwordKey, 7));
    }

    unsigned long long BuildFileSeed(unsigned long long masterSeed, unsigned long long pathHash, unsigned long long fileHash, unsigned long long plainSize, int index, const PasswordContext& passwordContext)
    {
        const unsigned long long idx = static_cast<unsigned long long>(index) + 1ULL;
        const unsigned long long passwordKey = BuildPasswordKey(passwordContext);
        return Mix64(masterSeed ^ pathHash ^ fileHash ^ plainSize ^ (idx * kFileSeedSalt) ^ passwordKey ^ RotL64(passwordKey, index & 31));
    }

    unsigned long long SeedState0(unsigned long long seed, unsigned long long passwordKey)
    {
        return Mix64(seed ^ SecretSeedA() ^ 0xA5A5A5A5A5A5A5A5ULL ^ passwordKey);
    }

    unsigned long long SeedState1(unsigned long long seed, unsigned long long passwordKey)
    {
        unsigned long long state = Mix64(seed ^ SecretSeedB() ^ 0x5A5A5A5A5A5A5A5AULL ^ RotL64(passwordKey, 31));
        return state == 0 ? 0x9E3779B97F4A7C15ULL : state;
    }

    class XorShift128Plus
    {
    public:
        XorShift128Plus(unsigned long long seed0, unsigned long long seed1)
            : m_s0(seed0 == 0 ? 0x9E3779B97F4A7C15ULL : seed0)
            , m_s1(seed1 == 0 ? 0xBF58476D1CE4E5B9ULL : seed1)
        {
        }

        unsigned char NextByte()
        {
            if (m_bufferBytesRemaining <= 0)
            {
                m_buffer = Next64();
                m_bufferBytesRemaining = 8;
            }

            const unsigned char value = static_cast<unsigned char>(m_buffer & 0xFF);
            m_buffer >>= 8;
            --m_bufferBytesRemaining;
            return value;
        }

    private:
        unsigned long long Next64()
        {
            unsigned long long x = m_s0;
            unsigned long long y = m_s1;
            m_s0 = y;
            x ^= x << 23;
            x ^= x >> 17;
            x ^= y ^ (y >> 26);
            m_s1 = x;
            return m_s0 + m_s1;
        }

        unsigned long long m_s0;
        unsigned long long m_s1;
        unsigned long long m_buffer = 0;
        int m_bufferBytesRemaining = 0;
    };

    unsigned char RotL8(unsigned char value, int rot)
    {
        rot &= 7;
        if (rot == 0)
            return value;
        return static_cast<unsigned char>(((value << rot) | (value >> (8 - rot))) & 0xFF);
    }

    unsigned char RotR8(unsigned char value, int rot)
    {
        rot &= 7;
        if (rot == 0)
            return value;
        return static_cast<unsigned char>(((value >> rot) | (value << (8 - rot))) & 0xFF);
    }

    std::vector<unsigned char> EncryptBuffer(const std::vector<unsigned char>& plain, unsigned long long seed, unsigned long long pathHash, unsigned long long fileHash, const PasswordContext& passwordContext)
    {
        if (plain.empty())
            return std::vector<unsigned char>();

        std::vector<unsigned char> buffer = plain;
        const bool reverse = (seed & 0x8000000000000000ULL) != 0;
        if (reverse)
            std::reverse(buffer.begin(), buffer.end());

        const unsigned long long passwordKey = BuildPasswordKey(passwordContext);
        XorShift128Plus prng(SeedState0(seed, passwordKey), SeedState1(seed, passwordKey));
        unsigned char prevPlain = static_cast<unsigned char>((seed ^ pathHash ^ fileHash) & 0xFF);
        unsigned char prevCipher = static_cast<unsigned char>(((seed >> 8) ^ (pathHash >> 8) ^ (fileHash >> 8)) & 0xFF);

        for (size_t i = 0; i < buffer.size(); ++i)
        {
            const unsigned char plainByte = buffer[i];
            const unsigned char ks = prng.NextByte();
            const unsigned char seedByte = static_cast<unsigned char>(seed >> ((i & 7) * 8));
            const unsigned char pathByte = static_cast<unsigned char>(pathHash >> (((i + 3) & 7) * 8));
            const unsigned char hashByte = static_cast<unsigned char>(fileHash >> (((i + 5) & 7) * 8));
            const int rot = static_cast<int>((seed >> (((i + 5) & 7) * 8)) & 7ULL) + 1;
            const unsigned char mix = static_cast<unsigned char>(prevPlain + prevCipher + seedByte + pathByte + static_cast<unsigned char>(i * 29 + 11));

            unsigned char cipherByte = plainByte;
            cipherByte ^= ks;
            cipherByte = static_cast<unsigned char>(cipherByte + mix);
            cipherByte = RotL8(cipherByte, rot);
            cipherByte ^= hashByte;
            cipherByte ^= seedByte;
            buffer[i] = cipherByte;

            prevPlain = plainByte;
            prevCipher = cipherByte;
        }

        return buffer;
    }

    std::vector<unsigned char> DecryptBuffer(const std::vector<unsigned char>& cipher, unsigned long long seed, unsigned long long pathHash, unsigned long long fileHash, const PasswordContext& passwordContext)
    {
        if (cipher.empty())
            return std::vector<unsigned char>();

        std::vector<unsigned char> buffer = cipher;
        const unsigned long long passwordKey = BuildPasswordKey(passwordContext);
        XorShift128Plus prng(SeedState0(seed, passwordKey), SeedState1(seed, passwordKey));
        unsigned char prevPlain = static_cast<unsigned char>((seed ^ pathHash ^ fileHash) & 0xFF);
        unsigned char prevCipher = static_cast<unsigned char>(((seed >> 8) ^ (pathHash >> 8) ^ (fileHash >> 8)) & 0xFF);

        for (size_t i = 0; i < buffer.size(); ++i)
        {
            const unsigned char cipherByte = buffer[i];
            const unsigned char ks = prng.NextByte();
            const unsigned char seedByte = static_cast<unsigned char>(seed >> ((i & 7) * 8));
            const unsigned char pathByte = static_cast<unsigned char>(pathHash >> (((i + 3) & 7) * 8));
            const unsigned char hashByte = static_cast<unsigned char>(fileHash >> (((i + 5) & 7) * 8));
            const int rot = static_cast<int>((seed >> (((i + 5) & 7) * 8)) & 7ULL) + 1;
            const unsigned char mix = static_cast<unsigned char>(prevPlain + prevCipher + seedByte + pathByte + static_cast<unsigned char>(i * 29 + 11));

            unsigned char plainByte = cipherByte;
            plainByte ^= seedByte;
            plainByte ^= hashByte;
            plainByte = RotR8(plainByte, rot);
            plainByte = static_cast<unsigned char>(plainByte - mix);
            plainByte ^= ks;
            buffer[i] = plainByte;

            prevPlain = plainByte;
            prevCipher = cipherByte;
        }

        if ((seed & 0x8000000000000000ULL) != 0)
            std::reverse(buffer.begin(), buffer.end());

        return buffer;
    }

    bool ParseManifest(const std::vector<unsigned char>& manifestPlain, unsigned long long masterSeed, const PasswordContext& passwordContext, std::vector<PackageEntry>& outEntries, unsigned long long& outManifestHash)
    {
        outEntries.clear();
        outManifestHash = 0;

        if (manifestPlain.size() < 4 + 4 + 4 + 8)
            return false;

        size_t offset = 0;
        std::vector<unsigned char> magic;
        if (!ReadBytes(manifestPlain, offset, 4, magic) || magic.size() != 4 ||
            memcmp(&magic[0], kManifestMagic, 4) != 0)
        {
            return false;
        }

        unsigned int version = 0;
        if (!ReadUInt32(manifestPlain, offset, version) || version != kManifestVersion)
            return false;

        unsigned int entryCount = 0;
        if (!ReadUInt32(manifestPlain, offset, entryCount))
            return false;

        if (entryCount > 100000U)
            return false;

        outEntries.reserve(entryCount);

        for (unsigned int i = 0; i < entryCount; ++i)
        {
            std::string relativePathUtf8;
            unsigned char kind = 0;
            unsigned long long plainSize = 0;
            unsigned long long cipherSize = 0;
            unsigned long long payloadOffset = 0;
            unsigned long long fileHash = 0;

            if (!ReadString(manifestPlain, offset, relativePathUtf8) ||
                !ReadByte(manifestPlain, offset, kind) ||
                !ReadUInt64(manifestPlain, offset, plainSize) ||
                !ReadUInt64(manifestPlain, offset, cipherSize) ||
                !ReadUInt64(manifestPlain, offset, payloadOffset) ||
                !ReadUInt64(manifestPlain, offset, fileHash))
            {
                return false;
            }

            PackageEntry entry = {};
            entry.relativePath = NormalizeRelativePath(Utf8ToWide(relativePathUtf8));
            entry.isBinary = kind == 2;
            entry.plainSize = plainSize;
            entry.cipherSize = cipherSize;
            entry.payloadOffset = payloadOffset;
            entry.fileHash = fileHash;
            entry.pathHash = ComputePathHash(entry.relativePath);
            entry.fileSeed = BuildFileSeed(masterSeed, entry.pathHash, fileHash, plainSize, static_cast<int>(i), passwordContext);
            outEntries.push_back(entry);
        }

        if (!ReadUInt64(manifestPlain, offset, outManifestHash))
            return false;

        if (offset != manifestPlain.size())
            return false;

        std::vector<unsigned char> body(manifestPlain.begin(), manifestPlain.end() - 8);
        if (ComputeManifestHash(body) != outManifestHash)
            return false;

        return true;
    }

    bool TryLoadPackageWithPasswordContext(
        const std::wstring& packagePath,
        const std::vector<unsigned char>& bytes,
        const PasswordContext& passwordContext,
        std::vector<PackageEntry>& outEntries,
        unsigned long long& outManifestHash,
        size_t& outTextCount,
        size_t& outBinaryCount)
    {
        outEntries.clear();
        outManifestHash = 0;
        outTextCount = 0;
        outBinaryCount = 0;

        if (bytes.size() < 4 + 4 + 4 + 16 + 8 + 8)
        {
            WriteLogFmt("[SkillPack] WARN invalid package size path=%s", WideToUtf8String(packagePath).c_str());
            return false;
        }

        size_t offset = 0;
        std::vector<unsigned char> magic;
        unsigned int version = 0;
        unsigned int flags = 0;
        std::vector<unsigned char> salt;
        unsigned long long manifestCipherSize = 0;
        unsigned long long payloadCipherSize = 0;

        if (!ReadBytes(bytes, offset, 4, magic) || magic.size() != 4 || memcmp(&magic[0], kHeaderMagic, 4) != 0 ||
            !ReadUInt32(bytes, offset, version) || version != kPackageVersion ||
            !ReadUInt32(bytes, offset, flags) || (flags & kPackageFlagsEncrypted) == 0 ||
            !ReadBytes(bytes, offset, 16, salt) || salt.size() != 16 ||
            !ReadUInt64(bytes, offset, manifestCipherSize) ||
            !ReadUInt64(bytes, offset, payloadCipherSize))
        {
            WriteLogFmt("[SkillPack] WARN invalid package header path=%s", WideToUtf8String(packagePath).c_str());
            return false;
        }

        const unsigned long long requiredSize = static_cast<unsigned long long>(offset) + manifestCipherSize + payloadCipherSize;
        if (requiredSize != static_cast<unsigned long long>(bytes.size()))
        {
            WriteLogFmt("[SkillPack] WARN invalid package layout path=%s", WideToUtf8String(packagePath).c_str());
            return false;
        }

        const unsigned long long masterSeed = BuildMasterSeed(&salt[0], manifestCipherSize, payloadCipherSize, passwordContext);
        std::vector<unsigned char> manifestCipher(bytes.begin() + static_cast<std::vector<unsigned char>::difference_type>(offset), bytes.begin() + static_cast<std::vector<unsigned char>::difference_type>(offset + manifestCipherSize));
        std::vector<unsigned char> payloadCipher(bytes.begin() + static_cast<std::vector<unsigned char>::difference_type>(offset + manifestCipherSize), bytes.end());
        std::vector<unsigned char> manifestPlain = DecryptBuffer(
            manifestCipher,
            BuildManifestSeed(masterSeed, &salt[0], manifestCipherSize, payloadCipherSize, passwordContext),
            0,
            0,
            passwordContext);

        std::vector<PackageEntry> entries;
        unsigned long long manifestHash = 0;
        if (!ParseManifest(manifestPlain, masterSeed, passwordContext, entries, manifestHash))
        {
            return false;
        }

        unsigned long long payloadCursor = 0;
        for (size_t index = 0; index < entries.size(); ++index)
        {
            PackageEntry& entry = entries[index];
            if (entry.payloadOffset != payloadCursor)
            {
                return false;
            }

            if (entry.payloadOffset + entry.cipherSize > payloadCipher.size())
            {
                return false;
            }

            std::vector<unsigned char> cipherEntry(payloadCipher.begin() + static_cast<std::vector<unsigned char>::difference_type>(entry.payloadOffset), payloadCipher.begin() + static_cast<std::vector<unsigned char>::difference_type>(entry.payloadOffset + entry.cipherSize));
            entry.plainBytes = DecryptBuffer(cipherEntry, entry.fileSeed, entry.pathHash, entry.fileHash, passwordContext);
            if (entry.plainBytes.size() != static_cast<size_t>(entry.plainSize))
            {
                return false;
            }

            if (ComputeFileHash(entry.plainBytes, entry.pathHash) != entry.fileHash)
            {
                return false;
            }

            if (entry.isBinary)
                ++outBinaryCount;
            else
                ++outTextCount;

            payloadCursor = entry.payloadOffset + entry.cipherSize;
        }

        outManifestHash = manifestHash;
        outEntries.swap(entries);
        return true;
    }

    bool LoadPackageForDirectory(const std::wstring& skillConfigDir)
    {
        const std::wstring directory = ssw::path::TrimTrailingSlash(skillConfigDir);
        const std::wstring packagePath = ssw::path::Combine(directory, kPackageFileName);
        {
            PackageStateLockGuard guard;
            if (g_state.initialized &&
                g_state.skillConfigDir == directory &&
                g_state.packagePath == packagePath &&
                g_state.packageAttempted &&
                !g_state.stamp.exists &&
                !g_state.packageLoaded)
            {
                return false;
            }
        }

        WIN32_FILE_ATTRIBUTE_DATA packageProbeData = {};
        if (packagePath.empty() ||
            !::GetFileAttributesExW(packagePath.c_str(), GetFileExInfoStandard, &packageProbeData))
        {
            {
                PackageStateLockGuard guard;
                g_state.initialized = true;
                g_state.skillConfigDir = directory;
                g_state.packagePath = packagePath;
                g_state.passwordSignature.clear();
                g_state.runtimePasswordPending = false;
                g_state.pendingRetryTick = 0;
                g_state.stamp = PackageStamp{};
                g_state.packageLoaded = false;
                g_state.packageAttempted = true;
                g_state.physicalFallbackBlockedLogged = false;
                g_state.entries.clear();
                g_state.entryIndexByPath.clear();
            }
            WriteLogFmt("[InitStage] LoadPackageForDirectory skip missing package path=%s",
                WideToUtf8String(packagePath).c_str());
            return false;
        }

        {
            PackageStateLockGuard guard;
            if (g_state.initialized &&
                g_state.skillConfigDir == directory &&
                g_state.packagePath == packagePath &&
                g_state.runtimePasswordPending &&
                !g_state.packageLoaded)
            {
                const DWORD nowTick = ::GetTickCount();
                if (g_state.pendingRetryTick != 0 &&
                    nowTick - g_state.pendingRetryTick < 5000)
                {
                    return false;
                }
                g_state.pendingRetryTick = nowTick;
            }
        }

        PasswordCandidateSet passwordCandidates = BuildPasswordCandidateSet();
        if (passwordCandidates.runtimePasswordAddress != 0 &&
            passwordCandidates.runtimePasswordRaw == 0)
        {
            {
                PackageStateLockGuard guard;
                g_state.initialized = true;
                g_state.skillConfigDir = directory;
                g_state.packagePath = packagePath;
                g_state.passwordSignature = passwordCandidates.signature;
                g_state.runtimePasswordPending = true;
                g_state.pendingRetryTick = ::GetTickCount();
                g_state.stamp = PackageStamp{};
                g_state.packageLoaded = false;
                g_state.packageAttempted = false;
                g_state.physicalFallbackBlockedLogged = false;
                g_state.entries.clear();
                g_state.entryIndexByPath.clear();
            }

            WriteLogFmt("[SkillPack] defer package load until runtime password becomes non-zero path=%s signature=%s",
                WideToUtf8String(packagePath).c_str(),
                WideToUtf8String(passwordCandidates.signature).c_str());
            return false;
        }

        WIN32_FILE_ATTRIBUTE_DATA pendingFileData = {};
        if (passwordCandidates.runtimePasswordPending &&
            !packagePath.empty() &&
            ::GetFileAttributesExW(packagePath.c_str(), GetFileExInfoStandard, &pendingFileData))
        {
            {
                PackageStateLockGuard guard;
                g_state.initialized = true;
                g_state.skillConfigDir = directory;
                g_state.packagePath = packagePath;
                g_state.passwordSignature = passwordCandidates.signature;
                g_state.runtimePasswordPending = true;
                g_state.pendingRetryTick = ::GetTickCount();
                g_state.stamp = PackageStamp{};
                g_state.packageLoaded = false;
                g_state.packageAttempted = true;
                g_state.physicalFallbackBlockedLogged = false;
                g_state.entries.clear();
                g_state.entryIndexByPath.clear();
            }

            WriteLogFmt("[SkillPack] wait for runtime password init path=%s signature=%s",
                WideToUtf8String(packagePath).c_str(),
                WideToUtf8String(passwordCandidates.signature).c_str());
            return false;
        }

        PackageState newState;
        newState.initialized = true;
        newState.skillConfigDir = directory;
        newState.packagePath = packagePath;
        newState.passwordSignature = passwordCandidates.signature;
        newState.runtimePasswordPending = false;

        WIN32_FILE_ATTRIBUTE_DATA fileData = {};
        PackageStamp currentStamp = {};
        auto storeFailedState = [&]()
        {
            PackageStateLockGuard guard;
            g_state.initialized = true;
            g_state.skillConfigDir = newState.skillConfigDir;
            g_state.packagePath = newState.packagePath;
            g_state.passwordSignature = newState.passwordSignature;
            g_state.runtimePasswordPending = false;
            g_state.pendingRetryTick = 0;
            g_state.stamp = currentStamp;
            g_state.packageLoaded = false;
            g_state.packageAttempted = true;
            g_state.physicalFallbackBlockedLogged = false;
            g_state.entries.clear();
            g_state.entryIndexByPath.clear();
        };

        if (newState.packagePath.empty() || !::GetFileAttributesExW(newState.packagePath.c_str(), GetFileExInfoStandard, &fileData))
        {
            PackageStateLockGuard guard;
            if (!g_state.initialized ||
                g_state.skillConfigDir != newState.skillConfigDir ||
                g_state.packagePath != newState.packagePath ||
                g_state.passwordSignature != newState.passwordSignature ||
                g_state.stamp.exists)
            {
                g_state.initialized = true;
                g_state.skillConfigDir = newState.skillConfigDir;
                g_state.packagePath = newState.packagePath;
                g_state.passwordSignature = newState.passwordSignature;
                g_state.runtimePasswordPending = false;
                g_state.pendingRetryTick = 0;
                g_state.stamp = PackageStamp{};
                g_state.packageLoaded = false;
                g_state.packageAttempted = true;
                g_state.physicalFallbackBlockedLogged = false;
                g_state.entries.clear();
                g_state.entryIndexByPath.clear();
            }
            return false;
        }

        currentStamp.exists = true;
        currentStamp.fileSize =
            (static_cast<unsigned long long>(fileData.nFileSizeHigh) << 32) |
            static_cast<unsigned long long>(fileData.nFileSizeLow);
        currentStamp.lastWriteTime = fileData.ftLastWriteTime;

        {
            PackageStateLockGuard guard;
            if (g_state.initialized &&
                g_state.skillConfigDir == newState.skillConfigDir &&
                g_state.packagePath == newState.packagePath &&
                g_state.passwordSignature == newState.passwordSignature &&
                g_state.stamp.exists == currentStamp.exists &&
                g_state.stamp.fileSize == currentStamp.fileSize &&
                CompareFileTime(&g_state.stamp.lastWriteTime, &currentStamp.lastWriteTime) == 0 &&
                g_state.packageAttempted)
            {
                return g_state.packageLoaded;
            }
        }

        std::vector<unsigned char> bytes;
        if (!ReadBinaryFilePhysical(newState.packagePath, bytes))
        {
            storeFailedState();
            return false;
        }

        std::vector<PackageEntry> entries;
        unsigned long long manifestHash = 0;
        size_t textCount = 0;
        size_t binaryCount = 0;
        int loadedPasswordIndex = -1;

        for (size_t i = 0; i < passwordCandidates.passwords.size(); ++i)
        {
            const PasswordContext passwordContext = BuildPasswordContext(passwordCandidates.passwords[i]);
            std::vector<PackageEntry> candidateEntries;
            unsigned long long candidateManifestHash = 0;
            size_t candidateTextCount = 0;
            size_t candidateBinaryCount = 0;
            if (!TryLoadPackageWithPasswordContext(
                    newState.packagePath,
                    bytes,
                    passwordContext,
                    candidateEntries,
                    candidateManifestHash,
                    candidateTextCount,
                    candidateBinaryCount))
            {
                continue;
            }

            entries.swap(candidateEntries);
            manifestHash = candidateManifestHash;
            textCount = candidateTextCount;
            binaryCount = candidateBinaryCount;
            loadedPasswordIndex = static_cast<int>(i);
            break;
        }

        if (loadedPasswordIndex < 0)
        {
            WriteLogFmt("[SkillPack] WARN failed to decrypt package path=%s candidates=%d signature=%s",
                WideToUtf8String(newState.packagePath).c_str(),
                static_cast<int>(passwordCandidates.passwords.size()),
                WideToUtf8String(newState.passwordSignature).c_str());
            storeFailedState();
            return false;
        }

        const int loadedEntryCount = static_cast<int>(entries.size());
        {
            PackageStateLockGuard guard;
            g_state.initialized = true;
            g_state.skillConfigDir = newState.skillConfigDir;
            g_state.packagePath = newState.packagePath;
            g_state.passwordSignature = newState.passwordSignature;
            g_state.runtimePasswordPending = false;
            g_state.pendingRetryTick = 0;
            g_state.stamp = currentStamp;
            g_state.packageLoaded = true;
            g_state.packageAttempted = true;
            g_state.physicalFallbackBlockedLogged = false;
            g_state.entries.swap(entries);
            g_state.entryIndexByPath.clear();
            for (size_t i = 0; i < g_state.entries.size(); ++i)
                g_state.entryIndexByPath[g_state.entries[i].relativePath] = i;
        }

        WriteLogFmt("[SkillPack] loaded path=%s entries=%d text=%d binary=%d manifest=0x%08X%08X passwordIndex=%d signature=%s",
            WideToUtf8String(newState.packagePath).c_str(),
            loadedEntryCount,
            static_cast<int>(textCount),
            static_cast<int>(binaryCount),
            static_cast<unsigned int>(manifestHash >> 32),
            static_cast<unsigned int>(manifestHash & 0xFFFFFFFFULL),
            loadedPasswordIndex,
            WideToUtf8String(newState.passwordSignature).c_str());
        return true;
    }

    bool TryReadFromLoadedPackage(const std::wstring& absolutePath, std::vector<unsigned char>& outBytes)
    {
        outBytes.clear();

        const std::wstring directory = ssw::path::Parent(absolutePath);
        if (directory.empty())
            return false;

        if (!LoadPackageForDirectory(directory))
            return false;

        PackageStateLockGuard guard;
        if (!g_state.packageLoaded)
            return false;

        const std::wstring relativePath = GetRelativePath(directory, absolutePath);
        std::map<std::wstring, size_t>::const_iterator it = g_state.entryIndexByPath.find(relativePath);
        if (it == g_state.entryIndexByPath.end())
            return false;

        outBytes = g_state.entries[it->second].plainBytes;
        return true;
    }

    bool TryReadRelativeFromLoadedPackage(
        const std::wstring& skillConfigDir,
        const std::wstring& relativePath,
        std::vector<unsigned char>& outBytes)
    {
        outBytes.clear();

        const std::wstring directory = ssw::path::TrimTrailingSlash(skillConfigDir);
        const std::wstring normalizedRelativePath = NormalizeRelativePath(relativePath);
        if (directory.empty() || normalizedRelativePath.empty())
            return false;

        if (!LoadPackageForDirectory(directory))
            return false;

        PackageStateLockGuard guard;
        if (!g_state.packageLoaded)
            return false;

        std::map<std::wstring, size_t>::const_iterator it = g_state.entryIndexByPath.find(normalizedRelativePath);
        if (it == g_state.entryIndexByPath.end())
            return false;

        outBytes = g_state.entries[it->second].plainBytes;
        return true;
    }

    bool EndsWithIgnoreCase(const std::wstring& value, const wchar_t* suffix)
    {
        if (value.empty() || !suffix || !suffix[0])
            return false;

        const std::wstring suffixText(suffix);
        if (value.size() < suffixText.size())
            return false;

        const std::wstring tail = value.substr(value.size() - suffixText.size());
        return _wcsicmp(tail.c_str(), suffixText.c_str()) == 0;
    }

    void CollectPackageEntries(const std::wstring& directory, std::vector<SkillConfigPackageEntryInfo>& outEntries)
    {
        outEntries.clear();

        if (!LoadPackageForDirectory(directory))
            return;

        PackageStateLockGuard guard;
        if (!g_state.packageLoaded)
            return;

        outEntries.reserve(g_state.entries.size());
        for (size_t i = 0; i < g_state.entries.size(); ++i)
        {
            const PackageEntry& entry = g_state.entries[i];
            SkillConfigPackageEntryInfo info;
            info.relativePath = entry.relativePath;
            info.isBinary = entry.isBinary;
            info.plainSize = entry.plainSize;
            outEntries.push_back(info);
        }
    }

    bool IsRuntimePasswordPendingForDirectory(const std::wstring& skillConfigDir)
    {
        const std::wstring directory = ssw::path::TrimTrailingSlash(skillConfigDir);
        PackageStateLockGuard guard;
        return g_state.initialized &&
            g_state.skillConfigDir == directory &&
            g_state.runtimePasswordPending &&
            !g_state.packageLoaded;
    }
} // namespace

void InvalidateSkillConfigPackage()
{
    const bool pristineState =
        !g_state.initialized &&
        g_state.skillConfigDir.empty() &&
        g_state.packagePath.empty() &&
        g_state.passwordSignature.empty() &&
        !g_state.runtimePasswordPending &&
        g_state.pendingRetryTick == 0 &&
        !g_state.stamp.exists &&
        !g_state.packageLoaded &&
        !g_state.packageAttempted &&
        !g_state.physicalFallbackBlockedLogged &&
        g_state.entries.empty() &&
        g_state.entryIndexByPath.empty();

    if (pristineState)
    {
        WriteLog("[InitStage] InvalidateSkillConfigPackage pristine-skip");
        return;
    }

    PackageStateLockGuard guard;
    g_state.initialized = false;
    g_state.skillConfigDir.clear();
    g_state.packagePath.clear();
    g_state.passwordSignature.clear();
    g_state.runtimePasswordPending = false;
    g_state.pendingRetryTick = 0;
    g_state.stamp = PackageStamp{};
    g_state.packageLoaded = false;
    g_state.packageAttempted = false;
    g_state.physicalFallbackBlockedLogged = false;
    g_state.entries.clear();
    g_state.entryIndexByPath.clear();
    WriteLog("[InitStage] InvalidateSkillConfigPackage cleared");
}

bool IsSkillConfigPackagePhysicalFallbackBlocked(const std::wstring& absolutePath)
{
    const std::wstring directory = ssw::path::Parent(absolutePath);
    if (directory.empty() || !ShouldPreferPackageForDirectory(directory))
        return false;

    const std::wstring relativePath = GetRelativePath(directory, absolutePath);
    return IsPackageCoveredRelativePath(relativePath);
}

bool TryReadSkillConfigBinaryFile(const std::wstring& absolutePath, std::vector<unsigned char>& outBytes)
{
    const std::wstring directory = ssw::path::Parent(absolutePath);
    const std::wstring relativePath =
        directory.empty() ? std::wstring() : GetRelativePath(directory, absolutePath);
    const bool packagePreferred =
        !directory.empty() && ShouldPreferPackageForDirectory(directory);

    if (packagePreferred &&
        _wcsicmp(relativePath.c_str(), kPackageFileName) == 0)
    {
        return ReadBinaryFilePhysical(absolutePath, outBytes);
    }

    if (TryReadFromLoadedPackage(absolutePath, outBytes))
        return true;

    if (!directory.empty() && IsRuntimePasswordPendingForDirectory(directory))
    {
        return false;
    }

    if (packagePreferred)
    {
        if (IsPackageCoveredRelativePath(relativePath))
        {
            bool shouldLog = false;
            {
                PackageStateLockGuard guard;
                if (g_state.initialized &&
                    g_state.skillConfigDir == directory &&
                    !g_state.packageLoaded &&
                    !g_state.physicalFallbackBlockedLogged)
                {
                    g_state.physicalFallbackBlockedLogged = true;
                    shouldLog = true;
                }
            }
            if (shouldLog)
            {
                WriteLogFmt("[SkillPack] ERROR package-owned physical fallback blocked path=%s relative=%ls",
                    WideToUtf8String(absolutePath).c_str(),
                    relativePath.c_str());
            }
            return false;
        }
    }
    return ReadBinaryFilePhysical(absolutePath, outBytes);
}

bool TryReadSkillConfigPackageRelativeBinaryFile(
    const std::wstring& skillConfigDir,
    const std::wstring& relativePath,
    std::vector<unsigned char>& outBytes)
{
    outBytes.clear();

    const std::wstring directory = ssw::path::TrimTrailingSlash(skillConfigDir);
    const std::wstring normalizedRelativePath = NormalizeRelativePath(relativePath);
    if (directory.empty() || normalizedRelativePath.empty())
        return false;

    if (TryReadRelativeFromLoadedPackage(directory, normalizedRelativePath, outBytes))
        return true;

    if (IsRuntimePasswordPendingForDirectory(directory))
        return false;

    if (ShouldPreferPackageForDirectory(directory) &&
        IsPackageCoveredRelativePath(normalizedRelativePath))
    {
        WriteLogFmt("[SkillPack] WARN package-owned relative read failed dir=%s relative=%ls",
            WideToUtf8String(directory).c_str(),
            normalizedRelativePath.c_str());
    }

    return false;
}

bool TryReadSkillConfigTextFile(const std::wstring& absolutePath, std::string& outText)
{
    outText.clear();

    std::vector<unsigned char> bytes;
    if (!TryReadSkillConfigBinaryFile(absolutePath, bytes))
        return false;

    if (bytes.size() >= 3 &&
        bytes[0] == 0xEF &&
        bytes[1] == 0xBB &&
        bytes[2] == 0xBF)
    {
        outText.assign(reinterpret_cast<const char*>(bytes.data() + 3), reinterpret_cast<const char*>(bytes.data() + bytes.size()));
    }
    else if (!bytes.empty())
    {
        outText.assign(reinterpret_cast<const char*>(bytes.data()), reinterpret_cast<const char*>(bytes.data() + bytes.size()));
    }

    return true;
}

bool TryEnumerateSkillConfigPackageEntries(const std::wstring& skillConfigDir, std::vector<SkillConfigPackageEntryInfo>& outEntries)
{
    CollectPackageEntries(skillConfigDir, outEntries);
    return !outEntries.empty();
}

bool DoesSkillConfigPackageExist(const std::wstring& skillConfigDir)
{
    return PackageFileExists(skillConfigDir);
}

bool QuerySkillConfigRuntimePasswordProbe(const std::wstring& skillConfigDir, SkillConfigRuntimePasswordProbe& outProbe)
{
    outProbe = SkillConfigRuntimePasswordProbe{};

    const std::wstring directory = ssw::path::TrimTrailingSlash(skillConfigDir);
    if (directory.empty())
        return false;

    outProbe.packageFileExists = PackageFileExists(directory);
    outProbe.plainConfigPresent = HasPlainSkillConfigMarker(directory);
    if (!outProbe.packageFileExists)
        return false;

    const PasswordCandidateSet passwordCandidates = BuildPasswordCandidateSet();
    outProbe.runtimePasswordAddress = passwordCandidates.runtimePasswordAddress;
    outProbe.runtimePasswordRaw = passwordCandidates.runtimePasswordRaw;
    outProbe.runtimePasswordPending = passwordCandidates.runtimePasswordPending;
    return true;
}

bool IsSkillConfigPackageRuntimePasswordPending(const std::wstring& skillConfigDir)
{
    return IsRuntimePasswordPendingForDirectory(skillConfigDir);
}

} // namespace skillpack
} // namespace ssw
