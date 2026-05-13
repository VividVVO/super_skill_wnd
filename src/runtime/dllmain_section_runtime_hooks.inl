// 运行时总装配入口：
// 这里只负责按功能域拼装模块，不承载具体业务实现。
// ============================================================================
// UI / SkillWnd 核心
#include "runtime/modules/runtime_feature_ui_skill_window_runtime_core.inl"

// 运行时开关与共享 hook 状态
#include "runtime/modules/runtime_feature_runtime_hook_state.inl"
#include "runtime/modules/runtime_feature_mounted_flight_and_ability_state.inl"

// 观测 / 助手 / 诊断
#include "runtime/modules/runtime_feature_surface_and_ability_red_runtime.inl"
#include "runtime/modules/runtime_feature_status_bar_buff_slot_runtime.inl"

// 发包 / 本地补丁
#include "runtime/modules/runtime_feature_runtime_patch_and_packet_runtime.inl"

// 骑宠 / 移动 / 二段跳
#include "runtime/modules/runtime_feature_mounted_double_jump_runtime_core.inl"
#include "runtime/modules/runtime_feature_mount_movement_runtime_core.inl"
#include "runtime/modules/runtime_feature_mounted_double_jump_runtime_handlers.inl"

// 技能释放 / 展示
#include "runtime/modules/runtime_feature_skill_effect_and_release_runtime.inl"

// Overlay 后端
#include "runtime/modules/runtime_feature_ui_overlay_runtime.inl"

// 本地安装器 / 原生 hook / 延迟交互
#include "runtime/modules/runtime_feature_packet_and_local_setup.inl"
#include "runtime/modules/runtime_feature_ability_red_local_setup.inl"
#include "runtime/modules/runtime_feature_native_hook_setup.inl"
#include "runtime/modules/runtime_feature_deferred_interaction_hooks.inl"
// ============================================================================
#undef kEnableMountedDoubleJumpRuntimeHooks
#undef kEnableMountedDemonJumpRuntimeHooks
#undef kEnableMountMovementAbilityRedHooks
#undef kEnableGlobalMovementSetterProtectionHooks
#undef kEnableMountMovementCapPatches
#undef kEnableMountMovementObservationHooks
#undef kEnableMountedFlightPhysicsSpeedHooks
#undef kEnableGlobalMovementOutputClampHook
