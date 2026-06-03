#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace ssw
{
namespace skillpack
{

struct SkillConfigPackageEntryInfo
{
    std::wstring relativePath;
    bool isBinary = false;
    unsigned long long plainSize = 0;
};

struct SkillConfigRuntimePasswordProbe
{
    bool packageFileExists = false;
    bool plainConfigPresent = false;
    uintptr_t runtimePasswordAddress = 0;
    unsigned int runtimePasswordRaw = 0;
    bool runtimePasswordPending = false;
};

bool TryReadSkillConfigBinaryFile(const std::wstring& absolutePath, std::vector<unsigned char>& outBytes);
bool TryReadSkillConfigPackageRelativeBinaryFile(const std::wstring& skillConfigDir, const std::wstring& relativePath, std::vector<unsigned char>& outBytes);
bool TryReadSkillConfigTextFile(const std::wstring& absolutePath, std::string& outText);
bool TryEnumerateSkillConfigPackageEntries(const std::wstring& skillConfigDir, std::vector<SkillConfigPackageEntryInfo>& outEntries);
void InvalidateSkillConfigPackage();
bool DoesSkillConfigPackageExist(const std::wstring& skillConfigDir);
bool QuerySkillConfigRuntimePasswordProbe(const std::wstring& skillConfigDir, SkillConfigRuntimePasswordProbe& outProbe);
bool IsSkillConfigPackageRuntimePasswordPending(const std::wstring& skillConfigDir);

} // namespace skillpack
} // namespace ssw
