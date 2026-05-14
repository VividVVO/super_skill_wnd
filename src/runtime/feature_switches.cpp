#include "runtime/feature_switches.h"

#include "core/Common.h"
#include "util/skill_config_package.h"
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
        {FeatureSwitchId::SkillRuntimeEnabled, "skill.runtime.enabled", true},
        {FeatureSwitchId::CorePacketHooks, "runtime.core.packetHooks", true},
        {FeatureSwitchId::CoreLocalPotentialReadHooks, "runtime.core.localPotentialReadHooks", true},
        {FeatureSwitchId::CoreLocalPotentialDisplayHooks, "runtime.core.localPotentialDisplayHooks", true},
        {FeatureSwitchId::CoreAbilityRedObservationHooks, "runtime.core.abilityRedObservationHooks", true},
        {FeatureSwitchId::CoreStatusBarBuffSlotHooks, "runtime.core.statusBarBuffSlotHooks", true},
        {FeatureSwitchId::CoreSurfaceDrawObservationHook, "runtime.core.surfaceDrawObservationHook", true},
        {FeatureSwitchId::CoreNativeCursorStateHook, "runtime.core.nativeCursorStateHook", true},
        {FeatureSwitchId::CoreSkillReleaseClassifierHooks, "runtime.core.skillReleaseClassifierHooks", true},
        {FeatureSwitchId::CoreSkillPresentationHooks, "runtime.core.skillPresentationHooks", true},
        {FeatureSwitchId::CoreSkillNativeGateHooks, "runtime.core.skillNativeGateHooks", true},
        {FeatureSwitchId::CoreSkillLevelHooks, "runtime.core.skillLevelHooks", true},
        {FeatureSwitchId::CorePassiveEffectHooks, "runtime.core.passiveEffectHooks", true},
        {FeatureSwitchId::UiOverlayHooks, "runtime.ui.overlayHooks", true},
        {FeatureSwitchId::UiNativeButtonHooks, "runtime.ui.nativeButtonHooks", false},
        {FeatureSwitchId::UiRouteBChildHooks, "runtime.ui.routeBChildHooks", false},
        {FeatureSwitchId::UiSkillWindowCoreHooks, "runtime.ui.skillWindowCoreHooks", true},
        {FeatureSwitchId::UiSkillWindowMoveHooks, "runtime.ui.skillWindowMoveHooks", false},
        {FeatureSwitchId::UiSkillWindowRefreshHooks, "runtime.ui.skillWindowRefreshHooks", false},
        {FeatureSwitchId::UiMsgHook, "runtime.ui.msgHook", true},
        {FeatureSwitchId::UiWndProcHook, "runtime.ui.wndProcHook", true},
        {FeatureSwitchId::UiInputSpoof, "runtime.ui.inputSpoof", true},
        {FeatureSwitchId::SkillReleaseNativeRouteArm, "skill.release.nativeRouteArm", false},
        {FeatureSwitchId::MountedRuntimeRouteArm, "mount.runtime.routeArm", false},
        {FeatureSwitchId::MountedMovementOverride, "mount.movement.override", false},
        {FeatureSwitchId::MountedSoaringOverride, "mount.movement.soaringOverride", false},
        {FeatureSwitchId::PacketRewritePipeline, "packet.pipeline.enabled", false},
        {FeatureSwitchId::PacketIndependentBuffCancelRewrite, "packet.independentBuff.cancelRewrite", false},
        {FeatureSwitchId::PacketSuperSkillUpgradeRewrite, "packet.superSkill.upgradeRewrite", false},
        {FeatureSwitchId::PacketPassiveAttackExpansion, "packet.passive.attackExpansion", false},
        {FeatureSwitchId::PacketPassiveDamageRewrite, "packet.passive.damageRewrite", false},
        {FeatureSwitchId::PacketActiveNativeReleaseRewrite, "packet.activeNativeRelease.rewrite", false},
        {FeatureSwitchId::PacketMountedRuntimeSpecialMoveRewrite, "packet.mountRuntime.specialMoveRewrite", false},
        {FeatureSwitchId::PacketProxyRouteRewrite, "packet.proxyRoute.rewrite", false},
        {FeatureSwitchId::MountedDoubleJumpRuntimeHooks, "runtime.mount.doubleJumpHooks", false},
        {FeatureSwitchId::MountedDemonJumpRuntimeHooks, "runtime.mount.demonJumpHooks", false},
        {FeatureSwitchId::MountedDoubleJumpBaseGateHooks, "runtime.mount.doubleJumpBaseGateHooks", true},
        {FeatureSwitchId::MountedDemonJumpCrashTraceHooks, "runtime.mount.demonJumpCrashTraceHooks", true},
        {FeatureSwitchId::MountedDemonJumpPacketObserveHooks, "runtime.mount.demonJumpPacketObserveHooks", true},
        {FeatureSwitchId::MountedDemonJumpLatePathHooks, "runtime.mount.demonJumpLatePathHooks", true},
        {FeatureSwitchId::MountedDemonJumpActionTraceHooks, "runtime.mount.demonJumpActionTraceHooks", true},
        {FeatureSwitchId::MountedDemonJumpRequirementBypassHook, "runtime.mount.demonJumpRequirementBypassHook", true},
        {FeatureSwitchId::MountedDemonJumpContextClearHook, "runtime.mount.demonJumpContextClearHook", true},
        {FeatureSwitchId::MountClimbGateHooks, "runtime.mount.climbGateHooks", false},
        {FeatureSwitchId::MountFlightMappingHooks, "runtime.mount.flightMappingHooks", false},
        {FeatureSwitchId::MountMovementAbilityRedHooks, "runtime.mount.movementAbilityRedHooks", false},
        {FeatureSwitchId::GlobalMovementSetterProtectionHooks, "runtime.movement.setterProtectionHooks", false},
        {FeatureSwitchId::GlobalMovementOutputClampHook, "runtime.movement.outputClampHook", false},
        {FeatureSwitchId::MountMovementObservationHooks, "runtime.mount.movementObservationHooks", false},
        {FeatureSwitchId::MountedFlightPhysicsSpeedHooks, "runtime.mount.flightPhysicsSpeedHooks", false},
        {FeatureSwitchId::MountMovementCapPatches, "runtime.mount.movementCapPatches", false},
        {FeatureSwitchId::FeatureSuperSkillEnabled, "feature.superSkill.enabled", true},
        {FeatureSwitchId::FeatureSuperSkillReleaseEnabled, "feature.superSkill.release.enabled", true},
        {FeatureSwitchId::FeatureSuperSkillUiEnabled, "feature.superSkill.ui.enabled", true},
        {FeatureSwitchId::FeatureSuperSkillIndependentBuffEnabled, "feature.superSkill.independentBuff.enabled", true},
        {FeatureSwitchId::FeatureSuperSkillPassiveEffectEnabled, "feature.superSkill.passiveEffect.enabled", true},
        {FeatureSwitchId::FeaturePlayerMovementEnabled, "feature.playerMovement.enabled", true},
        {FeatureSwitchId::FeatureMountDoubleJumpEnabled, "feature.mount.doubleJump.enabled", true},
        {FeatureSwitchId::FeatureMountDemonJumpEnabled, "feature.mount.demonJump.enabled", true},
        {FeatureSwitchId::FeatureMountClimbEnabled, "feature.mount.climb.enabled", true},
        {FeatureSwitchId::FeatureMountFlightEnabled, "feature.mount.flight.enabled", true},
        {FeatureSwitchId::FeatureMountMovementEnabled, "feature.mount.movement.enabled", true},
        {FeatureSwitchId::DiagnosticCrashCapture, "runtime.diagnostics.crashCapture", true},
    };

    bool g_featureSwitchesLoaded = false;
    std::vector<FeatureSwitchState> g_featureSwitchStates;
    std::wstring g_loadedFeatureSwitchPath;
    FeatureSwitchReloadCallback g_featureSwitchReloadCallbacks[8] = {};
    size_t g_featureSwitchReloadCallbackCount = 0;
    bool g_featureSwitchReloadCallbackActive = false;
    bool g_loggedFeatureSwitchPackagePending = false;

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

    bool GetRawFeatureSwitchValue(FeatureSwitchId id)
    {
        const size_t index = ToIndex(id);
        if (index >= g_featureSwitchStates.size())
            return false;
        return g_featureSwitchStates[index].value;
    }

    bool HasExplicitFeatureSwitchDisable(FeatureSwitchId id)
    {
        const size_t index = ToIndex(id);
        if (index >= g_featureSwitchStates.size())
            return false;

        const FeatureSwitchState& state = g_featureSwitchStates[index];
        return state.hasOverride && !state.value;
    }

    bool IsFeatureSwitchLocallyAllowed(FeatureSwitchId id)
    {
        const size_t index = ToIndex(id);
        if (index >= g_featureSwitchStates.size())
            return false;

        const FeatureSwitchState& state = g_featureSwitchStates[index];
        return !state.hasOverride || state.value;
    }

    void ForceFeatureSwitchDisabled(FeatureSwitchId id)
    {
        const size_t index = ToIndex(id);
        if (index >= g_featureSwitchStates.size())
            return;

        FeatureSwitchState& state = g_featureSwitchStates[index];
        state.value = false;
        state.hasOverride = true;
    }

    void ApplyPendingPackageSafetyOverrides()
    {
        ForceFeatureSwitchDisabled(FeatureSwitchId::MountedDemonJumpRuntimeHooks);
        ForceFeatureSwitchDisabled(FeatureSwitchId::MountedDemonJumpCrashTraceHooks);
        ForceFeatureSwitchDisabled(FeatureSwitchId::MountedDemonJumpPacketObserveHooks);
        ForceFeatureSwitchDisabled(FeatureSwitchId::MountedDemonJumpLatePathHooks);
        ForceFeatureSwitchDisabled(FeatureSwitchId::MountedDemonJumpActionTraceHooks);
        ForceFeatureSwitchDisabled(FeatureSwitchId::MountedDemonJumpRequirementBypassHook);
        ForceFeatureSwitchDisabled(FeatureSwitchId::MountedDemonJumpContextClearHook);
    }

    bool IsFeatureEnabledResolved(FeatureSwitchId id)
    {
        if (HasExplicitFeatureSwitchDisable(id))
            return false;

        const bool rawValue = GetRawFeatureSwitchValue(id);
        const bool locallyAllowed = IsFeatureSwitchLocallyAllowed(id);

        switch (id)
        {
        case FeatureSwitchId::FeatureSuperSkillReleaseEnabled:
        case FeatureSwitchId::FeatureSuperSkillUiEnabled:
        case FeatureSwitchId::FeatureSuperSkillIndependentBuffEnabled:
        case FeatureSwitchId::FeatureSuperSkillPassiveEffectEnabled:
            return rawValue &&
                   IsFeatureEnabledResolved(FeatureSwitchId::FeatureSuperSkillEnabled);

        case FeatureSwitchId::FeaturePlayerMovementEnabled:
        case FeatureSwitchId::FeatureMountDoubleJumpEnabled:
        case FeatureSwitchId::FeatureMountDemonJumpEnabled:
        case FeatureSwitchId::FeatureMountClimbEnabled:
        case FeatureSwitchId::FeatureMountFlightEnabled:
        case FeatureSwitchId::FeatureMountMovementEnabled:
            return rawValue;

        case FeatureSwitchId::SkillRuntimeEnabled:
            return locallyAllowed &&
                   IsFeatureEnabledResolved(FeatureSwitchId::FeatureSuperSkillReleaseEnabled);

        case FeatureSwitchId::CorePacketHooks:
        case FeatureSwitchId::PacketRewritePipeline:
            return locallyAllowed &&
                   (IsFeatureEnabledResolved(FeatureSwitchId::FeatureSuperSkillReleaseEnabled) ||
                    IsFeatureEnabledResolved(FeatureSwitchId::FeatureSuperSkillIndependentBuffEnabled) ||
                    IsFeatureEnabledResolved(FeatureSwitchId::FeatureSuperSkillPassiveEffectEnabled) ||
                    IsFeatureEnabledResolved(FeatureSwitchId::FeatureMountDoubleJumpEnabled) ||
                    IsFeatureEnabledResolved(FeatureSwitchId::FeatureMountDemonJumpEnabled));

        case FeatureSwitchId::CoreLocalPotentialReadHooks:
        case FeatureSwitchId::CoreLocalPotentialDisplayHooks:
        case FeatureSwitchId::CoreAbilityRedObservationHooks:
        case FeatureSwitchId::CoreStatusBarBuffSlotHooks:
        case FeatureSwitchId::PacketIndependentBuffCancelRewrite:
            return locallyAllowed &&
                   IsFeatureEnabledResolved(FeatureSwitchId::FeatureSuperSkillIndependentBuffEnabled);

        case FeatureSwitchId::CoreSurfaceDrawObservationHook:
        case FeatureSwitchId::CoreNativeCursorStateHook:
        case FeatureSwitchId::UiOverlayHooks:
        case FeatureSwitchId::UiNativeButtonHooks:
        case FeatureSwitchId::UiRouteBChildHooks:
        case FeatureSwitchId::UiSkillWindowCoreHooks:
        case FeatureSwitchId::UiSkillWindowMoveHooks:
        case FeatureSwitchId::UiSkillWindowRefreshHooks:
        case FeatureSwitchId::UiMsgHook:
        case FeatureSwitchId::UiWndProcHook:
        case FeatureSwitchId::UiInputSpoof:
            return locallyAllowed &&
                   IsFeatureEnabledResolved(FeatureSwitchId::FeatureSuperSkillUiEnabled);

        case FeatureSwitchId::CoreSkillReleaseClassifierHooks:
        case FeatureSwitchId::CoreSkillPresentationHooks:
        case FeatureSwitchId::CoreSkillNativeGateHooks:
        case FeatureSwitchId::CoreSkillLevelHooks:
        case FeatureSwitchId::SkillReleaseNativeRouteArm:
        case FeatureSwitchId::PacketSuperSkillUpgradeRewrite:
        case FeatureSwitchId::PacketActiveNativeReleaseRewrite:
        case FeatureSwitchId::PacketProxyRouteRewrite:
            return locallyAllowed &&
                   IsFeatureEnabledResolved(FeatureSwitchId::FeatureSuperSkillReleaseEnabled);

        case FeatureSwitchId::CorePassiveEffectHooks:
        case FeatureSwitchId::PacketPassiveAttackExpansion:
        case FeatureSwitchId::PacketPassiveDamageRewrite:
            return locallyAllowed &&
                   IsFeatureEnabledResolved(FeatureSwitchId::FeatureSuperSkillPassiveEffectEnabled);

        case FeatureSwitchId::MountedRuntimeRouteArm:
        case FeatureSwitchId::PacketMountedRuntimeSpecialMoveRewrite:
        case FeatureSwitchId::MountedDoubleJumpBaseGateHooks:
            return locallyAllowed &&
                   (IsFeatureEnabledResolved(FeatureSwitchId::FeatureMountDoubleJumpEnabled) ||
                    IsFeatureEnabledResolved(FeatureSwitchId::FeatureMountDemonJumpEnabled));

        case FeatureSwitchId::MountedDoubleJumpRuntimeHooks:
            return locallyAllowed &&
                   IsFeatureEnabledResolved(FeatureSwitchId::FeatureMountDoubleJumpEnabled);

        case FeatureSwitchId::MountedDemonJumpRuntimeHooks:
        case FeatureSwitchId::MountedDemonJumpCrashTraceHooks:
        case FeatureSwitchId::MountedDemonJumpPacketObserveHooks:
        case FeatureSwitchId::MountedDemonJumpLatePathHooks:
        case FeatureSwitchId::MountedDemonJumpActionTraceHooks:
        case FeatureSwitchId::MountedDemonJumpRequirementBypassHook:
        case FeatureSwitchId::MountedDemonJumpContextClearHook:
            return locallyAllowed &&
                   IsFeatureEnabledResolved(FeatureSwitchId::FeatureMountDemonJumpEnabled);

        case FeatureSwitchId::MountClimbGateHooks:
            return locallyAllowed &&
                   IsFeatureEnabledResolved(FeatureSwitchId::FeatureMountClimbEnabled);

        case FeatureSwitchId::MountFlightMappingHooks:
            return locallyAllowed &&
                   IsFeatureEnabledResolved(FeatureSwitchId::FeatureMountFlightEnabled);

        case FeatureSwitchId::MountedMovementOverride:
        case FeatureSwitchId::MountedSoaringOverride:
        case FeatureSwitchId::MountMovementAbilityRedHooks:
        case FeatureSwitchId::MountMovementObservationHooks:
        case FeatureSwitchId::MountedFlightPhysicsSpeedHooks:
        case FeatureSwitchId::MountMovementCapPatches:
            return locallyAllowed &&
                   IsFeatureEnabledResolved(FeatureSwitchId::FeatureMountMovementEnabled);

        case FeatureSwitchId::GlobalMovementSetterProtectionHooks:
        case FeatureSwitchId::GlobalMovementOutputClampHook:
            return locallyAllowed &&
                   (IsFeatureEnabledResolved(FeatureSwitchId::FeaturePlayerMovementEnabled) ||
                    IsFeatureEnabledResolved(FeatureSwitchId::FeatureMountMovementEnabled));

        default:
            return rawValue;
        }
    }

    bool ReadUtf8TextFile(const std::wstring& path, std::string& outText)
    {
        return ssw::skillpack::TryReadSkillConfigTextFile(path, outText);
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

        WriteLog("[InitStage] enter EnsureFeatureSwitchesLoaded");

        g_featureSwitchesLoaded = true;
        WriteLog("[InitStage] EnsureFeatureSwitchesLoaded set loaded=true");
        ResetFeatureSwitchStatesToDefaults();
        WriteLogFmt(
            "[InitStage] after ResetFeatureSwitchStatesToDefaults count=%u",
            (unsigned int)g_featureSwitchStates.size());

        const std::wstring dllDir = ssw::path::GetHookDllDirectory();
        const std::wstring rootDir = ssw::path::ResolveRootDirectoryFromHook();
        const std::wstring skillDir = ssw::path::ResolveSkillConfigDir(rootDir, dllDir);
        g_loadedFeatureSwitchPath = ResolveFeatureSwitchConfigPath();
        WriteLogFmt(
            "[InitStage] feature path dllDir=%s rootDir=%s skillDir=%s path=%s",
            ssw::path::WideToUtf8(dllDir).c_str(),
            ssw::path::WideToUtf8(rootDir).c_str(),
            ssw::path::WideToUtf8(skillDir).c_str(),
            ssw::path::WideToUtf8(g_loadedFeatureSwitchPath).c_str());

        std::string json;
        WriteLog("[InitStage] before ReadUtf8TextFile primary");
        if (!ReadUtf8TextFile(g_loadedFeatureSwitchPath, json))
        {
            WriteLog("[InitStage] ReadUtf8TextFile primary failed");
            const std::wstring legacyPath = ssw::path::Combine(skillDir, L"module_switches.json");
            WriteLogFmt(
                "[InitStage] legacy feature path=%s",
                ssw::path::WideToUtf8(legacyPath).c_str());
            WriteLog("[InitStage] before ReadUtf8TextFile legacy");
            if (_wcsicmp(legacyPath.c_str(), g_loadedFeatureSwitchPath.c_str()) != 0 &&
                ReadUtf8TextFile(legacyPath, json))
            {
                g_loadedFeatureSwitchPath = legacyPath;
                WriteLogFmt(
                    "[InitStage] ReadUtf8TextFile legacy succeeded bytes=%u",
                    (unsigned int)json.size());
            }
            else
            {
                WriteLog("[InitStage] before IsSkillConfigPackageRuntimePasswordPending");
                const bool packagePasswordPending =
                    ssw::skillpack::IsSkillConfigPackageRuntimePasswordPending(skillDir);
                WriteLogFmt(
                    "[InitStage] after IsSkillConfigPackageRuntimePasswordPending pending=%d",
                    packagePasswordPending ? 1 : 0);
                if (packagePasswordPending)
                {
                    ApplyPendingPackageSafetyOverrides();
                    if (!g_loggedFeatureSwitchPackagePending)
                    {
                        WriteLogFmt(
                            "[FeatureSwitch] package config pending runtime password, disabled demon runtime hooks path=%s",
                            ssw::path::WideToUtf8(g_loadedFeatureSwitchPath).c_str());
                        g_loggedFeatureSwitchPackagePending = true;
                    }
                    g_featureSwitchesLoaded = false;
                    WriteLog("[InitStage] leave EnsureFeatureSwitchesLoaded pending-runtime-password");
                    return;
                }

                WriteLogFmt(
                    "[FeatureSwitch] config missing, using disabled defaults path=%s",
                    ssw::path::WideToUtf8(g_loadedFeatureSwitchPath).c_str());
                WriteLog("[InitStage] leave EnsureFeatureSwitchesLoaded missing-config");
                return;
            }
        }
        else
        {
            WriteLogFmt(
                "[InitStage] ReadUtf8TextFile primary succeeded bytes=%u",
                (unsigned int)json.size());
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
        g_loggedFeatureSwitchPackagePending = false;

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
        WriteLogFmt(
            "[InitStage] leave EnsureFeatureSwitchesLoaded overrides=%d",
            overrideCount);
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
    return IsFeatureEnabledResolved(id);
}

void ReloadFeatureSwitches()
{
    WriteLog("[InitStage] enter ReloadFeatureSwitches");
    WriteLog("[InitStage] before InvalidateSkillConfigPackage");
    ssw::skillpack::InvalidateSkillConfigPackage();
    WriteLog("[InitStage] after InvalidateSkillConfigPackage");
    g_featureSwitchesLoaded = false;
    WriteLog("[InitStage] after g_featureSwitchesLoaded=false");
    g_loadedFeatureSwitchPath.clear();
    WriteLog("[InitStage] after g_loadedFeatureSwitchPath.clear");
    g_featureSwitchStates.clear();
    WriteLog("[InitStage] after g_featureSwitchStates.clear");
    EnsureFeatureSwitchesLoaded();
    WriteLog("[InitStage] after EnsureFeatureSwitchesLoaded");

    if (g_featureSwitchReloadCallbackCount > 0 && !g_featureSwitchReloadCallbackActive)
    {
        g_featureSwitchReloadCallbackActive = true;
        for (size_t i = 0; i < g_featureSwitchReloadCallbackCount; ++i)
        {
            FeatureSwitchReloadCallback callback = g_featureSwitchReloadCallbacks[i];
            if (callback)
                callback();
        }
        g_featureSwitchReloadCallbackActive = false;
    }
    WriteLog("[InitStage] leave ReloadFeatureSwitches");
}

void SetFeatureSwitchReloadCallback(FeatureSwitchReloadCallback callback)
{
    if (!callback)
    {
        for (size_t i = 0; i < sizeof(g_featureSwitchReloadCallbacks) / sizeof(g_featureSwitchReloadCallbacks[0]); ++i)
            g_featureSwitchReloadCallbacks[i] = nullptr;
        g_featureSwitchReloadCallbackCount = 0;
        return;
    }

    for (size_t i = 0; i < g_featureSwitchReloadCallbackCount; ++i)
    {
        if (g_featureSwitchReloadCallbacks[i] == callback)
            return;
    }

    if (g_featureSwitchReloadCallbackCount >= sizeof(g_featureSwitchReloadCallbacks) / sizeof(g_featureSwitchReloadCallbacks[0]))
    {
        WriteLog("[FeatureSwitch] reload callback table full");
        return;
    }

    g_featureSwitchReloadCallbacks[g_featureSwitchReloadCallbackCount++] = callback;
}

} // namespace runtime
} // namespace ssw
