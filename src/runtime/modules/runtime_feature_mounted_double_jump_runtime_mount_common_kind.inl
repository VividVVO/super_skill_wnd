static bool IsNativeJobMountExcludedFromExtendedMountRuntime(int mountItemId)
{
    switch (mountItemId)
    {
    case 1932016: // Mechanic mech
        return true;
    default:
        // Wild Hunter jaguar color variants.
        return mountItemId >= 1932033 && mountItemId <= 1932036;
    }
}

static bool IsExtendedMountActionGateMount(int mountItemId)
{
    if (IsNativeJobMountExcludedFromExtendedMountRuntime(mountItemId))
    {
        return false;
    }

    // 客户端 sub_4069E0 仍只硬编码放行到 1992015，导致 1999xxx 自定义坐骑
    // 即使 WZ 带 ladder/rope 资源、服务端也认可攀爬，case 51/52 仍会直接回退。
    if (mountItemId >= 1932016 && mountItemId <= 1999999)
    {
        return true;
    }
    return false;
}

static bool IsExtendedMountServerValidatedSoaringMount(int mountItemId)
{
    // 扩展 193x 坐骑统一放进 80001089 / family gate 链，最终能否飞行交给服务端判定。
    // 原生 1992xxx 飞行家族保持客户端原行为，避免回归已稳定的原生骑宠链。
    if (IsNativeJobMountExcludedFromExtendedMountRuntime(mountItemId))
    {
        return false;
    }
    return mountItemId >= 1932016 && mountItemId < 1992000;
}

static int ResolveExtendedMountNativeFlightSkillId(int mountItemId)
{
    // 客户端原生只为 1992000..1992015 建了飞行技能映射。
    // 对 1999xxx 自定义坐骑统一复用一条稳定 donor 飞行链，避免“服务端允许飞行，
    // 但本地 jump+up / 二次起飞 / 80001089 链仍查不到 skillId”。
    if (IsExtendedMountServerValidatedSoaringMount(mountItemId))
    {
        // 扩展 193x 坐骑统一走 80001089；即使最终服务端不允许飞，也要先把客户端
        // 的原生 Soaring gate / 发包链接通，避免在本地提前死在 80001077 donor 分叉。
        return 80001089;
    }
    if (IsNativeJobMountExcludedFromExtendedMountRuntime(mountItemId))
    {
        return 0;
    }
    if (mountItemId >= 1932016 && mountItemId <= 1999999)
    {
        return 80001077;
    }
    return 0;
}

static bool IsExtendedMountFamilyGateMount(int mountItemId)
{
    return IsExtendedMountServerValidatedSoaringMount(mountItemId);
}

static bool IsExtendedMountSoaringContextMount(int mountItemId)
{
    return IsExtendedMountServerValidatedSoaringMount(mountItemId) ||
           mountItemId == 1992014 ||
           mountItemId == 1992018;
}

static const char *GetMountedRuntimeSkillLogTag(MountedRuntimeSkillKind kind)
{
    return kind == MountedRuntimeSkillKind_DemonJump
               ? "MountDemonJump"
               : "MountDoubleJump";
}

static int ResolveMountedRuntimeSkillIdForKind(
    MountedRuntimeSkillKind kind,
    int mountItemId)
{
    return kind == MountedRuntimeSkillKind_DemonJump
               ? SkillOverlayBridgeResolveMountedDemonJumpSkillId(mountItemId)
               : SkillOverlayBridgeResolveMountedDoubleJumpSkillId(mountItemId);
}

static bool CanUseMountedRuntimeSkillForKind(
    MountedRuntimeSkillKind kind,
    int mountItemId,
    int skillId)
{
    return kind == MountedRuntimeSkillKind_DemonJump
               ? SkillOverlayBridgeCanUseMountedDemonJumpSkill(mountItemId, skillId)
               : SkillOverlayBridgeCanUseMountedDoubleJumpSkill(mountItemId, skillId);
}

static bool CanUseMountedRuntimeSkillRuntimeForKind(
    MountedRuntimeSkillKind kind,
    int mountItemId,
    int skillId)
{
    return kind == MountedRuntimeSkillKind_DemonJump
               ? SkillOverlayBridgeCanUseMountedDemonJumpRuntimeSkill(mountItemId, skillId)
               : SkillOverlayBridgeCanUseMountedDoubleJumpRuntimeSkill(mountItemId, skillId);
}

static bool IsMountedRuntimeSkillHooksEnabledForKind(MountedRuntimeSkillKind kind)
{
    return kind == MountedRuntimeSkillKind_DemonJump
               ? kEnableMountedDemonJumpRuntimeHooks
               : kEnableMountedDoubleJumpRuntimeHooks;
}

static bool HasRecentMountedRuntimeRouteArmForKind(
    MountedRuntimeSkillKind kind,
    int mountItemId,
    DWORD maxAgeMs)
{
    return kind == MountedRuntimeSkillKind_DemonJump
               ? SkillOverlayBridgeHasRecentMountedDemonJumpRouteArm(mountItemId, maxAgeMs)
               : SkillOverlayBridgeHasRecentMountedDoubleJumpRouteArm(mountItemId, maxAgeMs);
}

static bool TryGetRecentMountedRuntimeRouteArmMountItemIdForKind(
    MountedRuntimeSkillKind kind,
    int *mountItemIdOut,
    DWORD maxAgeMs)
{
    return kind == MountedRuntimeSkillKind_DemonJump
               ? SkillOverlayBridgeTryGetRecentMountedDemonJumpRouteArmMountItemId(mountItemIdOut, maxAgeMs)
               : SkillOverlayBridgeTryGetRecentMountedDoubleJumpRouteArmMountItemId(mountItemIdOut, maxAgeMs);
}

static volatile LONG g_recentExtendedMountContextItemId = 0;
static volatile LONG g_recentExtendedMountContextTick = 0;
static volatile LONG g_recentMountedRuntimeSkillIntentItemId[MountedRuntimeSkillKind_Count] = {0};
static volatile LONG g_recentMountedRuntimeSkillIntentTick[MountedRuntimeSkillKind_Count] = {0};
static volatile LONG g_recentMountedRuntimeSkillNativeReleaseItemId[MountedRuntimeSkillKind_Count] = {0};
static volatile LONG g_recentMountedRuntimeSkillNativeReleaseSkillId[MountedRuntimeSkillKind_Count] = {0};
static volatile LONG g_recentMountedRuntimeSkillNativeReleaseTick[MountedRuntimeSkillKind_Count] = {0};
static volatile LONG g_recentMountedDemonJumpGateProbeItemId = 0;
static volatile LONG g_recentMountedDemonJumpGateProbeChildSkillId = 0;
static volatile LONG g_recentMountedDemonJumpGateProbeTick = 0;
static volatile LONG g_recentMountedDemonJumpTerminalClearMountItemId = 0;
static volatile LONG g_recentMountedDemonJumpTerminalClearTick = 0;
static volatile LONG g_mountedDemonJumpContextClearDepth = 0;
static volatile LONG g_mountedDemonJumpResolveSafePrimeDepth = 0;

