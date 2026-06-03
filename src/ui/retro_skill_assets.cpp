#include "retro_skill_assets.h"

#include "resource.h"
#include "skill/skill_local_data.h"
#include "util/runtime_paths.h"
#include "util/stb_image.h"

#include "core/Common.h"

static bool CreateTextureFromRgba(const RetroDeviceRef& deviceRef, const unsigned char* data, int width, int height, UITexture& outTexture)
{
    outTexture.texture = nullptr;
    if (!RetroCreateBackendTextureFromRgba(deviceRef, data, width, height, &outTexture.texture))
        return false;

    outTexture.backend = deviceRef.backend;
    outTexture.width = width;
    outTexture.height = height;
    return true;
}

static bool LoadTextureInternalFromMemory(const RetroDeviceRef& deviceRef, const unsigned char* fileData, size_t fileSize, UITexture& outTexture)
{
    int width = 0;
    int height = 0;
    int channels = 0;
    unsigned char* data = stbi_load_from_memory(fileData, static_cast<int>(fileSize), &width, &height, &channels, 4);
    if (!data)
        return false;

    const bool ok = CreateTextureFromRgba(deviceRef, data, width, height, outTexture);
    stbi_image_free(data);
    return ok;
}

static bool LoadTextureInternal(const RetroDeviceRef& deviceRef, const char* filename, UITexture& outTexture)
{
    int width = 0;
    int height = 0;
    int channels = 0;
    unsigned char* data = stbi_load(filename, &width, &height, &channels, 4);
    if (!data)
        return false;

    const bool ok = CreateTextureFromRgba(deviceRef, data, width, height, outTexture);
    stbi_image_free(data);
    return ok;
}

bool LoadRetroSkillTexture(const RetroDeviceRef& deviceRef, const char* filename, UITexture& outTexture)
{
    return LoadTextureInternal(deviceRef, filename, outTexture);
}

bool LoadRetroSkillTextureFromMemory(const RetroDeviceRef& deviceRef, const unsigned char* data, size_t size, UITexture& outTexture)
{
    return LoadTextureInternalFromMemory(deviceRef, data, size, outTexture);
}

static bool LoadAssetBytes(int resourceId, std::vector<unsigned char>& outBytes)
{
    outBytes.clear();

    if (!ssw::path::TryReadSkillExAssetBytes(resourceId, outBytes) || outBytes.empty())
    {
        WriteLogFmt("[Assets] TryReadSkillExAssetBytes(%d) FAIL", resourceId);
        return false;
    }

    return true;
}

static bool LoadTextureFromAssetFile(const RetroDeviceRef& deviceRef, int resourceId, UITexture& outTexture)
{
    std::vector<unsigned char> fileBytes;
    if (!LoadAssetBytes(resourceId, fileBytes) || fileBytes.empty())
        return false;

    return LoadTextureInternalFromMemory(deviceRef, &fileBytes[0], fileBytes.size(), outTexture);
}

bool LoadAllRetroSkillAssets(RetroSkillAssets& assets, const RetroDeviceRef& deviceRef, const char* assetPath)
{
    (void)assetPath;

    struct EmbeddedTextureSpec
    {
        const char* key;
        int resourceId;
    };

    static const EmbeddedTextureSpec kEmbeddedTextures[] = {
        { "main", IDR_PANEL_BG },
        { "Passive.0", IDR_PANEL_TAB_PASSIVE_NORMAL },
        { "Passive.1", IDR_PANEL_TAB_PASSIVE_ACTIVE },
        { "ActivePassive.0", IDR_PANEL_TAB_ACTIVE_NORMAL },
        { "ActivePassive.1", IDR_PANEL_TAB_ACTIVE_ACTIVE },
        { "initial.normal", IDR_PANEL_INIT_NORMAL },
        { "initial.mouseOver", IDR_PANEL_INIT_MOUSEOVER },
        { "initial.pressed", IDR_PANEL_INIT_PRESSED },
        { "initial.backgrnd", IDR_RESET_NOTICE_BG },
        { "Notice.btYes.normal.0", IDR_RESET_NOTICE_YES_NORMAL },
        { "Notice.btYes.mouseOver.0", IDR_RESET_NOTICE_YES_MOUSEOVER },
        { "Notice.btYes.pressed.0", IDR_RESET_NOTICE_YES_PRESSED },
        { "Notice.btYes.disabled.0", IDR_RESET_NOTICE_YES_DISABLED },
        { "Notice.btNo.normal.0", IDR_RESET_NOTICE_NO_NORMAL },
        { "Notice.btNo.mouseOver.0", IDR_RESET_NOTICE_NO_MOUSEOVER },
        { "Notice.btNo.pressed.0", IDR_RESET_NOTICE_NO_PRESSED },
        { "scroll", IDR_PANEL_SCROLL_NORMAL },
        { "scroll.pressed", IDR_PANEL_SCROLL_PRESSED },
        { "BtSpUp.normal", IDR_PANEL_SPUP_NORMAL },
        { "BtSpUp.disabled", IDR_PANEL_SPUP_DISABLED },
        { "BtSpUp.pressed", IDR_PANEL_SPUP_PRESSED },
        { "BtSpUp.mouseOver", IDR_PANEL_SPUP_MOUSEOVER },
        { "skill0", IDR_PANEL_SKILL_ROW_NORMAL },
        { "skill1", IDR_PANEL_SKILL_ROW_UPGRADE },
        { "TypeIcon.0", IDR_PANEL_TYPEICON_ACTIVE },
        { "TypeIcon.2", IDR_PANEL_TYPEICON_PASSIVE },
        { "surpe.normal", IDR_BTN_NORMAL },
        { "surpe.mouseOver", IDR_BTN_HOVER },
        { "surpe.pressed", IDR_BTN_PRESSED },
        { "surpe.disabled", IDR_BTN_DISABLED },
        { "mouse.normal", IDR_CURSOR_NORMAL },
        { "mouse.normal.1", IDR_CURSOR_HOVER_A },
        { "mouse.normal.2", IDR_CURSOR_HOVER_B },
        { "mouse.pressed", IDR_CURSOR_PRESSED },
        { "mouse.drag", IDR_CURSOR_DRAG },
    };

    auto loadOne = [&](const char* key, int resourceId) {
        UITexture texture;
        if (LoadTextureFromAssetFile(deviceRef, resourceId, texture))
            assets.textures[key] = texture;
    };

    auto loadSkillIcon = [&](int skillId) {
        std::vector<unsigned char> pngBytes;
        if (SkillLocalDataGetIconPngBytes(skillId, pngBytes) && !pngBytes.empty())
        {
            UITexture texture;
            if (LoadTextureInternalFromMemory(deviceRef, &pngBytes[0], pngBytes.size(), texture))
                assets.skillIcons[skillId] = texture;
        }

        std::vector<unsigned char> mouseOverBytes;
        if (SkillLocalDataGetIconMouseOverPngBytes(skillId, mouseOverBytes) && !mouseOverBytes.empty())
        {
            UITexture texture;
            if (LoadTextureInternalFromMemory(deviceRef, &mouseOverBytes[0], mouseOverBytes.size(), texture))
                assets.skillIconsMouseOver[skillId] = texture;
        }

        std::vector<unsigned char> disabledBytes;
        if (SkillLocalDataGetIconDisabledPngBytes(skillId, disabledBytes) && !disabledBytes.empty())
        {
            UITexture texture;
            if (LoadTextureInternalFromMemory(deviceRef, &disabledBytes[0], disabledBytes.size(), texture))
                assets.skillIconsDisabled[skillId] = texture;
        }
    };

    for (size_t i = 0; i < sizeof(kEmbeddedTextures) / sizeof(kEmbeddedTextures[0]); ++i)
        loadOne(kEmbeddedTextures[i].key, kEmbeddedTextures[i].resourceId);

    WriteLogFmt("[Assets] external textures loaded: %d/%d", (int)assets.textures.size(), (int)(sizeof(kEmbeddedTextures) / sizeof(kEmbeddedTextures[0])));

    SkillLocalDataInitialize();
    std::vector<int> skillIds;
    SkillLocalDataGetSkillIds(skillIds);
    for (size_t i = 0; i < skillIds.size(); ++i)
        loadSkillIcon(skillIds[i]);

    return true;
}

void CleanupRetroSkillAssets(RetroSkillAssets& assets)
{
    for (auto& pair : assets.textures)
    {
        if (pair.second.texture)
            RetroReleaseBackendTexture(pair.second.texture, pair.second.backend);
    }
    assets.textures.clear();

    for (auto& pair : assets.skillIcons)
    {
        if (pair.second.texture)
            RetroReleaseBackendTexture(pair.second.texture, pair.second.backend);
    }
    assets.skillIcons.clear();

    for (auto& pair : assets.skillIconsMouseOver)
    {
        if (pair.second.texture)
            RetroReleaseBackendTexture(pair.second.texture, pair.second.backend);
    }
    assets.skillIconsMouseOver.clear();

    for (auto& pair : assets.skillIconsDisabled)
    {
        if (pair.second.texture)
            RetroReleaseBackendTexture(pair.second.texture, pair.second.backend);
    }
    assets.skillIconsDisabled.clear();
}

UITexture* GetRetroSkillTexture(RetroSkillAssets& assets, const char* name)
{
    auto it = assets.textures.find(name);
    if (it != assets.textures.end())
        return &it->second;
    return nullptr;
}

UITexture* GetRetroSkillSkillIconTexture(RetroSkillAssets& assets, int skillId)
{
    std::map<int, UITexture>::iterator it = assets.skillIcons.find(skillId);
    if (it != assets.skillIcons.end())
        return &it->second;
    return nullptr;
}

UITexture* GetRetroSkillSkillIconMouseOverTexture(RetroSkillAssets& assets, int skillId)
{
    std::map<int, UITexture>::iterator it = assets.skillIconsMouseOver.find(skillId);
    if (it != assets.skillIconsMouseOver.end())
        return &it->second;
    return nullptr;
}

UITexture* GetRetroSkillSkillIconDisabledTexture(RetroSkillAssets& assets, int skillId)
{
    std::map<int, UITexture>::iterator it = assets.skillIconsDisabled.find(skillId);
    if (it != assets.skillIconsDisabled.end())
        return &it->second;
    return nullptr;
}
