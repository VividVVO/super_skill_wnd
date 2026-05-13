#pragma once

namespace ssw
{
namespace runtime
{

enum class FeatureSwitchId
{
    SkillReleaseNativeRouteArm = 0,          // 技能原生释放路由总开关
    MountedRuntimeRouteArm,                  // 骑宠运行时路由总开关
    MountedMovementOverride,                 // 骑宠移动覆盖配置
    MountedSoaringOverride,                  // 骑宠飞行/滑翔覆盖配置
    PacketRewritePipeline,                   // 发包改写总管线
    PacketIndependentBuffCancelRewrite,      // 独立 BUFF 取消包改写
    PacketSuperSkillUpgradeRewrite,          // 超级技能升级包改写
    PacketPassiveAttackExpansion,            // 被动 attackCount 扩展
    PacketPassiveDamageRewrite,              // 被动 damage 重写
    PacketActiveNativeReleaseRewrite,        // 主动技能原生 release 改写
    PacketMountedRuntimeSpecialMoveRewrite,  // 骑宠 runtime special move 改写
    PacketProxyRouteRewrite,                 // 代理路由改写
    MountedDoubleJumpRuntimeHooks,           // 骑宠二段跳 runtime 总开关
    MountedDemonJumpRuntimeHooks,            // 骑宠恶魔跳跃 runtime 独立总开关
    MountClimbGateHooks,                     // 骑宠攀爬/绳索门禁
    MountFlightMappingHooks,                 // 骑宠飞行映射
    MountMovementAbilityRedHooks,            // 骑宠移动 AbilityRed 绑定
    GlobalMovementSetterProtectionHooks,     // 全局移动 setter 防护
    GlobalMovementOutputClampHook,           // 全局移动输出 clamp
    MountMovementObservationHooks,            // 骑宠移动观测
    MountedFlightPhysicsSpeedHooks,          // 骑宠飞行物理速度
    MountMovementCapPatches,                 // 骑宠移动 cap patch
    DiagnosticCrashCapture,                  // 进程级崩溃捕获与 minidump
    Count
};

const char* GetFeatureSwitchKey(FeatureSwitchId id);
bool IsFeatureEnabled(FeatureSwitchId id);
void ReloadFeatureSwitches();

using FeatureSwitchReloadCallback = void (*)();
void SetFeatureSwitchReloadCallback(FeatureSwitchReloadCallback callback);

} // namespace runtime
} // namespace ssw
