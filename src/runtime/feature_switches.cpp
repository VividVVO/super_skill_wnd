#include "runtime/feature_switches.h"

#include "core/Common.h"
#include "util/runtime_paths.h"

#include <windows.h>

#include <cstdio>
#include <string>
#include <vector>

namespace ssw
{
namespace runtime
{
namespace
{
    struct FeatureSwitchDefinition
    {
        FeatureSwitchId id;
        const char* key;
        bool defaultValue;
    };

    struct FeatureSwitchState
    {
        bool value = false;
        bool hasOverride = false;
    };

    const FeatureSwitchDefinition kFeatureSwitchDefinitions[] =
    {
        {FeatureSwitchId::SkillReleaseNativeRouteArm, "skill.release.nativeRouteArm", true},
        {FeatureSwitchId::MountedRuntimeRouteArm, "mount.runtime.routeArm", true},
        {FeatureSwitchId::MountedMovementOverride, "mount.movement.override", true},
        {FeatureSwitchId::MountedSoaringOverride, "mount.movement.soaringOverride", true},
        {FeatureSwitchId::PacketRewritePipeline, "packet.pipeline.enabled", true},
        {FeatureSwitchId::PacketIndependentBuffCancelRewrite, "packet.independentBuff.cancelRewrite", true},
        {FeatureSwitchId::PacketSuperSkillUpgradeRewrite, "packet.superSkill.upgradeRewrite", true},
        {FeatureSwitchId::PacketPassiveAttackExpansion, "packet.passive.attackExpansion", true},
        {FeatureSwitchId::PacketPassiveDamageRewrite, "packet.passive.damageRewrite", true},
        {FeatureSwitchId::PacketActiveNativeReleaseRewrite, "packet.activeNativeRelease.rewrite", true},
        {FeatureSwitchId::PacketMountedRuntimeSpecialMoveRewrite, "packet.mountRuntime.specialMoveRewrite", true},
        {FeatureSwitchId::PacketProxyRouteRewrite, "packet.proxyRoute.rewrite", true},
        {FeatureSwitchId::MountedDoubleJumpRuntimeHooks, "runtime.mount.doubleJumpHooks", true},
        {FeatureSwitchId::MountClimbGateHooks, "runtime.mount.climbGateHooks", true},
        {FeatureSwitchId::MountFlightMappingHooks, "runtime.mount.flightMappingHooks", true},
        {FeatureSwitchId::MountMovementAbilityRedHooks, "runtime.mount.movementAbilityRedHooks", true},
        {FeatureSwitchId::GlobalMovementSetterProtectionHooks, "runtime.movement.setterProtectionHooks", false},
        {FeatureSwitchId::GlobalMovementOutputClampHook, "runtime.movement.outputClampHook", true},
        {FeatureSwitchId::MountMovementObservationHooks, "runtime.mount.movementObservationHooks", true},
        {FeatureSwitchId::MountedFlightPhysicsSpeedHooks, "runtime.mount.flightPhysicsSpeedHooks", true},
        {FeatureSwitchId::MountMovementCapPatches, "runtime.mount.movementCapPatches", true},
    };

    bool g_featureSwitchesLoaded = false;
    std::vector<FeatureSwitchState> g_featureSwitchStates;
    std::wstring g_loadedFeatureSwitchPath;

    size_t ToIndex(FeatureSwitchId id)
    {
        return static_cast<size_t>(id);
    }

    const FeatureSwitchDefinition* FindFeatureSwitchDefinition(FeatureSwitchId id)
    {
        const size_t index = ToIndex(id);
        if (index >= sizeof(kFeatureSwitchDefinitions) / sizeof(kFeatureSwitchDefinitions[0]))
            return nullptr;
        return &kFeatureSwitchDefinitions[index];
    }

    bool ReadUtf8TextFile(const std::wstring& path, std::string& outText)
    {
        outText.clear();

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

        rewind(file);
        outText.resize(static_cast<size_t>(size));
        const size_t readSize = size > 0
            ? fread(&outText[0], 1, static_cast<size_t>(size), file)
            : 0;
        fclose(file);

        if (readSize != static_cast<size_t>(size))
            return false;

        if (outText.size() >= 3 &&
            static_cast<unsigned char>(outText[0]) == 0xEF &&
            static_cast<unsigned char>(outText[1]) == 0xBB &&
            static_cast<unsigned char>(outText[2]) == 0xBF)
        {
            outText.erase(0, 3);
        }
        return true;
    }

    bool TryParseJsonBoolValue(const std::string& json, const char* key, bool& outValue)
    {
        if (!key || !key[0])
            return false;

        const std::string token = std::string("\"") + key + "\"";
        size_t pos = json.find(token);
        while (pos != std::string::npos)
        {
            pos = json.find(':', pos + token.size());
            if (pos == std::string::npos)
                return false;
            ++pos;

            while (pos < json.size() &&
                   (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\r' || json[pos] == '\n'))
            {
                ++pos;
            }

            if (pos + 4 <= json.size() && json.compare(pos, 4, "true") == 0)
            {
                outValue = true;
                return true;
            }
            if (pos + 5 <= json.size() && json.compare(pos, 5, "false") == 0)
            {
                outValue = false;
                return true;
            }
            if (pos < json.size() && json[pos] == '1')
            {
                outValue = true;
                return true;
            }
            if (pos < json.size() && json[pos] == '0')
            {
                outValue = false;
                return true;
            }

            pos = json.find(token, pos);
        }
        return false;
    }

    std::wstring ResolveFeatureSwitchConfigPath()
    {
        const std::wstring dllDir = ssw::path::GetHookDllDirectory();
        const std::wstring rootDir = ssw::path::ResolveRootDirectoryFromHook();
        const std::wstring skillDir = ssw::path::ResolveSkillConfigDir(rootDir, dllDir);

        const std::wstring primary = ssw::path::Combine(skillDir, L"feature_switches.json");
        if (ssw::path::FileExists(primary))
            return primary;

        const std::wstring legacy = ssw::path::Combine(skillDir, L"module_switches.json");
        if (ssw::path::FileExists(legacy))
            return legacy;

        return primary;
    }

    void ResetFeatureSwitchStatesToDefaults()
    {
        g_featureSwitchStates.assign(
            static_cast<size_t>(FeatureSwitchId::Count),
            FeatureSwitchState{});

        for (size_t i = 0; i < sizeof(kFeatureSwitchDefinitions) / sizeof(kFeatureSwitchDefinitions[0]); ++i)
        {
            const FeatureSwitchDefinition& def = kFeatureSwitchDefinitions[i];
            g_featureSwitchStates[ToIndex(def.id)].value = def.defaultValue;
        }
    }

    void EnsureFeatureSwitchesLoaded()
    {
        if (g_featureSwitchesLoaded)
            return;

        g_featureSwitchesLoaded = true;
        ResetFeatureSwitchStatesToDefaults();
        g_loadedFeatureSwitchPath = ResolveFeatureSwitchConfigPath();

        std::string json;
        if (!ReadUtf8TextFile(g_loadedFeatureSwitchPath, json))
        {
            WriteLogFmt(
                "[FeatureSwitch] config missing, using defaults path=%s",
                ssw::path::WideToUtf8(g_loadedFeatureSwitchPath).c_str());
            return;
        }

        int overrideCount = 0;
        for (size_t i = 0; i < sizeof(kFeatureSwitchDefinitions) / sizeof(kFeatureSwitchDefinitions[0]); ++i)
        {
            const FeatureSwitchDefinition& def = kFeatureSwitchDefinitions[i];
            bool parsedValue = false;
            if (!TryParseJsonBoolValue(json, def.key, parsedValue))
                continue;

            FeatureSwitchState& state = g_featureSwitchStates[ToIndex(def.id)];
            state.value = parsedValue;
            state.hasOverride = true;
            ++overrideCount;
        }

        WriteLogFmt(
            "[FeatureSwitch] loaded path=%s overrides=%d",
            ssw::path::WideToUtf8(g_loadedFeatureSwitchPath).c_str(),
            overrideCount);

        for (size_t i = 0; i < sizeof(kFeatureSwitchDefinitions) / sizeof(kFeatureSwitchDefinitions[0]); ++i)
        {
            const FeatureSwitchDefinition& def = kFeatureSwitchDefinitions[i];
            const FeatureSwitchState& state = g_featureSwitchStates[ToIndex(def.id)];
            if (!state.hasOverride)
                continue;

            WriteLogFmt(
                "[FeatureSwitch] %s=%d default=%d",
                def.key,
                state.value ? 1 : 0,
                def.defaultValue ? 1 : 0);
        }
    }
} // namespace

const char* GetFeatureSwitchKey(FeatureSwitchId id)
{
    const FeatureSwitchDefinition* def = FindFeatureSwitchDefinition(id);
    return def ? def->key : "";
}

bool IsFeatureEnabled(FeatureSwitchId id)
{
    EnsureFeatureSwitchesLoaded();
    const size_t index = ToIndex(id);
    if (index >= g_featureSwitchStates.size())
        return false;
    return g_featureSwitchStates[index].value;
}

void ReloadFeatureSwitches()
{
    g_featureSwitchesLoaded = false;
    g_loadedFeatureSwitchPath.clear();
    g_featureSwitchStates.clear();
    EnsureFeatureSwitchesLoaded();
}

} // namespace runtime
} // namespace ssw

