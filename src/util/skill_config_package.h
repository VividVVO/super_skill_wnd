#pragma once

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

bool TryReadSkillConfigBinaryFile(const std::wstring& absolutePath, std::vector<unsigned char>& outBytes);
bool TryReadSkillConfigTextFile(const std::wstring& absolutePath, std::string& outText);
bool TryEnumerateSkillConfigPackageEntries(const std::wstring& skillConfigDir, std::vector<SkillConfigPackageEntryInfo>& outEntries);
void InvalidateSkillConfigPackage();
bool DoesSkillConfigPackageExist(const std::wstring& skillConfigDir);
bool IsSkillConfigPackageRuntimePasswordPending(const std::wstring& skillConfigDir);

} // namespace skillpack
} // namespace ssw
