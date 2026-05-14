#pragma once

namespace ssw
{
namespace runtime
{

enum class FeatureSwitchId
{
    SkillRuntimeEnabled = 0,               // 超级技能运行时总闸（行为层）
    CorePacketHooks,                       // 发包/收包基础 hooks 总闸
    CoreLocalPotentialReadHooks,          // 本地潜能读点 hooks
    CoreLocalPotentialDisplayHooks,       // 本地潜能显示/红字 hooks
    CoreAbilityRedObservationHooks,       // AbilityRed 纯观察 hooks
    CoreStatusBarBuffSlotHooks,           // 状态栏 BUFF 槽 hooks
    CoreSurfaceDrawObservationHook,       // surface draw 观测 hook
    CoreNativeCursorStateHook,            // 原生 cursor 状态 hook
    CoreSkillReleaseClassifierHooks,       // 技能释放分类 hooks 总闸
    CoreSkillPresentationHooks,            // 技能 presentation hooks 总闸
    CoreSkillNativeGateHooks,              // 技能原生 gate hooks 总闸
    CoreSkillLevelHooks,                   // 技能等级查询 hooks 总闸
    CorePassiveEffectHooks,                // 被动 effect/getter hooks 总闸
    UiOverlayHooks,                        // D3D8/D3D9 overlay hooks 总闸
    UiNativeButtonHooks,                   // 原生按钮 hooks 总闸
    UiRouteBChildHooks,                    // route-B child hooks 总闸
    UiSkillWindowCoreHooks,                // SkillWnd init/draw/dtor/filter 总闸
    UiSkillWindowMoveHooks,                // SkillWnd move hook 总闸
    UiSkillWindowRefreshHooks,             // SkillWnd refresh hook 总闸
    UiMsgHook,                             // SkillWnd msg hook 总闸
    UiWndProcHook,                         // WndProc hook 总闸
    UiInputSpoof,                          // 输入伪装总闸
    SkillReleaseNativeRouteArm,            // 技能原生释放路由总开关
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
    MountedDoubleJumpBaseGateHooks,          // 骑宠二段跳/恶魔跳跃共享底座 gate hooks
    MountedDemonJumpCrashTraceHooks,         // 骑宠恶魔跳跃 crash trace hooks
    MountedDemonJumpPacketObserveHooks,      // 骑宠恶魔跳跃 packet observe hooks
    MountedDemonJumpLatePathHooks,           // 骑宠恶魔跳跃 late path hooks
    MountedDemonJumpActionTraceHooks,        // 骑宠恶魔跳跃 action trace hooks
    MountedDemonJumpRequirementBypassHook,   // 骑宠恶魔跳跃 requirement bypass hook
    MountedDemonJumpContextClearHook,        // 骑宠恶魔跳跃 context clear hook
    MountClimbGateHooks,                     // 骑宠攀爬/绳索门禁
    MountFlightMappingHooks,                 // 骑宠飞行映射
    MountMovementAbilityRedHooks,            // 骑宠移动 AbilityRed 绑定
    GlobalMovementSetterProtectionHooks,     // 全局移动 setter 防护
    GlobalMovementOutputClampHook,           // 全局移动输出 clamp
    MountMovementObservationHooks,            // 骑宠移动观测
    MountedFlightPhysicsSpeedHooks,          // 骑宠飞行物理速度
    MountMovementCapPatches,                 // 骑宠移动 cap patch
    FeatureSuperSkillEnabled,                // 超级技能子体系总闸（release/ui/independentBuff/passive）
    FeatureSuperSkillReleaseEnabled,         // 超级技能释放链总闸
    FeatureSuperSkillUiEnabled,              // 超级技能 UI 总闸
    FeatureSuperSkillIndependentBuffEnabled, // 独立 BUFF 功能总闸
    FeatureSuperSkillPassiveEffectEnabled,   // 被动效果功能总闸
    FeaturePlayerMovementEnabled,            // 人物移动速度/跳跃总闸
    FeatureMountDoubleJumpEnabled,           // 骑宠二段跳总闸
    FeatureMountDemonJumpEnabled,            // 骑宠恶魔跳跃总闸
    FeatureMountClimbEnabled,                // 骑宠攀爬总闸
    FeatureMountFlightEnabled,               // 骑宠飞行放行总闸
    FeatureMountMovementEnabled,             // 骑宠移动/飞行速度总闸
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
