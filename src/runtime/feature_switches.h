#pragma once

namespace ssw
{
namespace runtime
{

enum class FeatureSwitchId
{
    SkillReleaseNativeRouteArm = 0,
    MountedRuntimeRouteArm,
    MountedMovementOverride,
    MountedSoaringOverride,
    PacketRewritePipeline,
    PacketIndependentBuffCancelRewrite,
    PacketSuperSkillUpgradeRewrite,
    PacketPassiveAttackExpansion,
    PacketPassiveDamageRewrite,
    PacketActiveNativeReleaseRewrite,
    PacketMountedRuntimeSpecialMoveRewrite,
    PacketProxyRouteRewrite,
    MountedDoubleJumpRuntimeHooks,
    MountClimbGateHooks,
    MountFlightMappingHooks,
    MountMovementAbilityRedHooks,
    GlobalMovementSetterProtectionHooks,
    GlobalMovementOutputClampHook,
    MountMovementObservationHooks,
    MountedFlightPhysicsSpeedHooks,
    MountMovementCapPatches,
    Count
};

const char* GetFeatureSwitchKey(FeatureSwitchId id);
bool IsFeatureEnabled(FeatureSwitchId id);
void ReloadFeatureSwitches();

} // namespace runtime
} // namespace ssw

