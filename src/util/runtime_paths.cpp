#include "util/runtime_paths.h"
#include "util/skill_config_package.h"

#include "resource.h"

#include <windows.h>

#include <cstdio>
#include <vector>

namespace ssw
{
namespace path
{

std::string WideToUtf8(const std::wstring& text)
{
    if (text.empty())
        return std::string();

    const int length = ::WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (length <= 1)
        return std::string();

    std::string result;
    result.resize(static_cast<size_t>(length - 1));
    ::WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, &result[0], length, nullptr, nullptr);
    return result;
}

bool FileExists(const wchar_t* path)
{
    if (!path || !path[0])
        return false;
    const DWORD attributes = ::GetFileAttributesW(path);
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

bool FileExists(const std::wstring& path)
{
    return FileExists(path.c_str());
}

bool DirectoryExists(const std::wstring& path)
{
    if (path.empty())
        return false;
    const DWORD attributes = ::GetFileAttributesW(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

bool ReadBinaryFile(const std::wstring& path, std::vector<unsigned char>& outBytes)
{
    outBytes.clear();
    if (path.empty())
        return false;

    FILE* file = nullptr;
    if (_wfopen_s(&file, path.c_str(), L"rb") != 0 || !file)
        return false;

    if (fseek(file, 0, SEEK_END) != 0)
    {
        fclose(file);
        return false;
    }

    const long size = ftell(file);
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

std::wstring TrimTrailingSlash(std::wstring path)
{
    while (!path.empty() && (path[path.size() - 1] == L'\\' || path[path.size() - 1] == L'/'))
        path.resize(path.size() - 1);
    return path;
}

std::wstring Combine(const std::wstring& left, const wchar_t* right)
{
    if (left.empty())
        return right ? std::wstring(right) : std::wstring();
    if (!right || !right[0])
        return left;

    std::wstring result = TrimTrailingSlash(left);
    result += L"\\";
    result += right;
    return result;
}

std::wstring Parent(const std::wstring& path)
{
    const std::wstring trimmed = TrimTrailingSlash(path);
    const size_t slashPos = trimmed.find_last_of(L"\\/");
    if (slashPos == std::wstring::npos)
        return std::wstring();
    return trimmed.substr(0, slashPos);
}

std::wstring GetHookDllDirectory()
{
    HMODULE module = nullptr;
    if (!::GetModuleHandleExW(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&GetHookDllDirectory),
            &module))
    {
        return std::wstring();
    }

    wchar_t modulePath[MAX_PATH] = {};
    const DWORD length = ::GetModuleFileNameW(module, modulePath, MAX_PATH);
    if (length == 0 || length >= MAX_PATH)
        return std::wstring();

    return Parent(std::wstring(modulePath, modulePath + length));
}

bool LooksLikeSuperSkillRoot(const std::wstring& dir)
{
    if (dir.empty())
        return false;

    if (DirectoryExists(Combine(dir, L"Data\\Plugins\\SS\\Skill")))
        return true;
    if (DirectoryExists(Combine(dir, L"skill")))
        return true;
    if (DirectoryExists(Combine(dir, L"skill2")))
        return true;
    if (FileExists(Combine(dir, L"build\\Reader\\SkillImgReader.dll")))
        return true;
    if (FileExists(Combine(dir, L"Reader\\SkillImgReader.dll")))
        return true;
    if (FileExists(Combine(dir, L"build\\SkillImgReader\\SkillImgReader.dll")))
        return true;
    if (FileExists(Combine(dir, L"SkillImgReader\\SkillImgReader.dll")))
        return true;
    if (DirectoryExists(Combine(dir, L"Data")))
        return true;
    return false;
}

std::wstring ResolveRootDirectoryFromHook()
{
    const std::wstring dllDir = GetHookDllDirectory();
    if (dllDir.empty())
        return std::wstring();

    std::vector<std::wstring> candidates;
    candidates.push_back(dllDir);

    const std::wstring parent = Parent(dllDir);
    if (!parent.empty())
        candidates.push_back(parent);

    const std::wstring grandParent = parent.empty() ? std::wstring() : Parent(parent);
    if (!grandParent.empty())
        candidates.push_back(grandParent);

    for (size_t i = 0; i < candidates.size(); ++i)
    {
        if (DirectoryExists(Combine(candidates[i], L"skill")) ||
            DirectoryExists(Combine(candidates[i], L"skill2")))
            return TrimTrailingSlash(candidates[i]);
    }

    for (size_t i = 0; i < candidates.size(); ++i)
    {
        if (DirectoryExists(Combine(candidates[i], L"Data")))
            return TrimTrailingSlash(candidates[i]);
    }

    for (size_t i = 0; i < candidates.size(); ++i)
    {
        if (LooksLikeSuperSkillRoot(candidates[i]))
            return TrimTrailingSlash(candidates[i]);
    }

    return TrimTrailingSlash(dllDir);
}

std::wstring ResolveSkillConfigDir(const std::wstring& rootDir, const std::wstring& dllDir)
{
    const std::wstring primary = Combine(dllDir, L"SS\\Skill");
    if (DirectoryExists(primary))
        return TrimTrailingSlash(primary);

    const std::wstring pluginRoot = Combine(rootDir, L"Data\\Plugins\\SS\\Skill");
    if (DirectoryExists(pluginRoot))
        return TrimTrailingSlash(pluginRoot);

    const std::wstring legacyPrimary = Combine(rootDir, L"skill");
    if (DirectoryExists(legacyPrimary))
        return TrimTrailingSlash(legacyPrimary);

    const std::wstring legacy = Combine(rootDir, L"skill2");
    if (DirectoryExists(legacy))
        return TrimTrailingSlash(legacy);

    return TrimTrailingSlash(primary);
}

namespace
{
const wchar_t* GetSkillExAssetFileName(int resourceId)
{
    switch (resourceId)
    {
    case IDR_BTN_NORMAL: return L"SkillEx.surpe.normal.png";
    case IDR_BTN_HOVER: return L"SkillEx.surpe.mouseOver.png";
    case IDR_BTN_PRESSED: return L"SkillEx.surpe.pressed.png";
    case IDR_BTN_DISABLED: return L"SkillEx.surpe.disabled.png";
    case IDR_PANEL_BG: return L"SkillEx.main.png";
    case IDR_PANEL_TAB_PASSIVE_NORMAL: return L"SkillEx.Passive.0.png";
    case IDR_PANEL_TAB_PASSIVE_ACTIVE: return L"SkillEx.Passive.1.png";
    case IDR_PANEL_TAB_ACTIVE_NORMAL: return L"SkillEx.ActivePassive.0.png";
    case IDR_PANEL_TAB_ACTIVE_ACTIVE: return L"SkillEx.ActivePassive.1.png";
    case IDR_PANEL_INIT_NORMAL: return L"SkillEx.initial.normal.png";
    case IDR_PANEL_INIT_MOUSEOVER: return L"SkillEx.initial.mouseOver.png";
    case IDR_PANEL_INIT_PRESSED: return L"SkillEx.inital.pressed.png";
    case IDR_PANEL_INIT_DISABLED: return L"SkillEx.inital.disabled.png";
    case IDR_PANEL_SCROLL_NORMAL: return L"OptionMenu.scroll.2.png";
    case IDR_PANEL_SCROLL_PRESSED: return L"OptionMenu.scroll.1.png";
    case IDR_PANEL_SPUP_NORMAL: return L"SkillEx.main.BtSpUp.normal.0.png";
    case IDR_PANEL_SPUP_DISABLED: return L"SkillEx.main.BtSpUp.disabled.0.png";
    case IDR_PANEL_SPUP_PRESSED: return L"SkillEx.main.BtSpUp.pressed.0.png";
    case IDR_PANEL_SPUP_MOUSEOVER: return L"SkillEx.main.BtSpUp.mouseOver.0.png";
    case IDR_PANEL_SKILL_ROW_NORMAL: return L"Skill.main.skill0.png";
    case IDR_PANEL_SKILL_ROW_UPGRADE: return L"Skill.main.skill1.png";
    case IDR_PANEL_TYPEICON_ACTIVE: return L"SkillEx.TypeIcon.0.png";
    case IDR_PANEL_TYPEICON_UNUSED: return L"SkillEx.TypeIcon.1.png";
    case IDR_PANEL_TYPEICON_PASSIVE: return L"SkillEx.TypeIcon.2.png";
    case IDR_CURSOR_NORMAL: return L"System.mouse.normal.png";
    case IDR_CURSOR_HOVER_A: return L"System.mouse.normal.1.png";
    case IDR_CURSOR_HOVER_B: return L"System.mouse.normal.2.png";
    case IDR_CURSOR_PRESSED: return L"System.mouse.pressed.png";
    case IDR_CURSOR_DRAG: return L"System.mouse.Drag.png";
    case IDR_RESET_NOTICE_BG: return L"SkillEx.initial.backgrnd.png";
    case IDR_RESET_NOTICE_YES_NORMAL: return L"Notice.btYes.normal.0.png";
    case IDR_RESET_NOTICE_YES_MOUSEOVER: return L"Notice.btYes.mouseOver.0.png";
    case IDR_RESET_NOTICE_YES_PRESSED: return L"Notice.btYes.pressed.0.png";
    case IDR_RESET_NOTICE_YES_DISABLED: return L"Notice.btYes.disabled.0.png";
    case IDR_RESET_NOTICE_NO_NORMAL: return L"Notice.btNo.normal.0.png";
    case IDR_RESET_NOTICE_NO_MOUSEOVER: return L"Notice.btNo.mouseOver.0.png";
    case IDR_RESET_NOTICE_NO_PRESSED: return L"Notice.btNo.pressed.0.png";
    default:
        return nullptr;
    }
}

bool BuildSkillExPackageRelativePath(int resourceId, std::wstring& outRelativePath)
{
    outRelativePath.clear();

    const wchar_t* fileName = GetSkillExAssetFileName(resourceId);
    if (!fileName || !fileName[0])
        return false;

    outRelativePath = L"ui\\skillex\\";
    outRelativePath += fileName;
    return true;
}

void AppendAssetDirCandidate(std::vector<std::wstring>& candidates, const std::wstring& dir)
{
    if (dir.empty())
        return;

    const std::wstring normalized = TrimTrailingSlash(dir);
    for (size_t i = 0; i < candidates.size(); ++i)
    {
        if (_wcsicmp(candidates[i].c_str(), normalized.c_str()) == 0)
            return;
    }

    candidates.push_back(normalized);
}

void BuildSkillExAssetDirCandidates(
    const std::wstring& rootDir,
    const std::wstring& dllDir,
    std::vector<std::wstring>& outCandidates)
{
    outCandidates.clear();

    AppendAssetDirCandidate(outCandidates, Combine(dllDir, L"UI\\SkillEx"));
    AppendAssetDirCandidate(outCandidates, Combine(rootDir, L"Data\\Plugins\\SS\\UI\\SkillEx"));
    AppendAssetDirCandidate(outCandidates, Combine(rootDir, L"UI\\SkillEx"));

    if (FileExists(Combine(rootDir, L"src\\resource.rc")))
    {
        const std::wstring parent = Parent(rootDir);
        const std::wstring grandParent = parent.empty() ? std::wstring() : Parent(parent);
        AppendAssetDirCandidate(outCandidates, Combine(grandParent, L"UI\\SkillEx"));
    }

    AppendAssetDirCandidate(outCandidates, Combine(dllDir, L"超级"));
    AppendAssetDirCandidate(outCandidates, Combine(rootDir, L"Data\\Plugins\\SS\\超级"));
}
} // namespace

std::wstring ResolveSkillExAssetDir(const std::wstring& rootDir, const std::wstring& dllDir)
{
    std::vector<std::wstring> candidates;
    BuildSkillExAssetDirCandidates(rootDir, dllDir, candidates);

    for (size_t i = 0; i < candidates.size(); ++i)
    {
        if (!DirectoryExists(candidates[i]))
            continue;

        const std::wstring panelPath = Combine(candidates[i], L"SkillEx.main.png");
        if (FileExists(panelPath))
            return candidates[i];
    }

    return std::wstring();
}

bool ResolveSkillExAssetPath(int resourceId, std::wstring& outPath)
{
    outPath.clear();

    const wchar_t* fileName = GetSkillExAssetFileName(resourceId);
    if (!fileName || !fileName[0])
        return false;

    const std::wstring dllDir = GetHookDllDirectory();
    const std::wstring rootDir = ResolveRootDirectoryFromHook();
    std::vector<std::wstring> candidates;
    BuildSkillExAssetDirCandidates(rootDir, dllDir, candidates);

    for (size_t i = 0; i < candidates.size(); ++i)
    {
        if (!DirectoryExists(candidates[i]))
            continue;

        const std::wstring assetPath = Combine(candidates[i], fileName);
        if (!FileExists(assetPath))
            continue;

        outPath = assetPath;
        return true;
    }

    return false;
}

bool TryReadSkillExAssetBytes(int resourceId, std::vector<unsigned char>& outBytes)
{
    outBytes.clear();

    const std::wstring dllDir = GetHookDllDirectory();
    const std::wstring rootDir = ResolveRootDirectoryFromHook();
    const std::wstring skillConfigDir = ResolveSkillConfigDir(rootDir, dllDir);

    std::wstring packageRelativePath;
    if (BuildSkillExPackageRelativePath(resourceId, packageRelativePath) &&
        ssw::skillpack::TryReadSkillConfigPackageRelativeBinaryFile(skillConfigDir, packageRelativePath, outBytes) &&
        !outBytes.empty())
    {
        return true;
    }

    std::wstring assetPath;
    if (!ResolveSkillExAssetPath(resourceId, assetPath))
        return false;

    if (!ReadBinaryFile(assetPath, outBytes) || outBytes.empty())
    {
        outBytes.clear();
        return false;
    }

    return true;
}

} // namespace path
} // namespace ssw
