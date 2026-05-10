#include "runtime_feature_mounted_double_jump_runtime_release_classifier.inl"

static bool IsExtendedMountActionGateMount(int mountItemId)
{
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

static bool IsMountedDemonJumpContextClearActive()
{
    return InterlockedCompareExchange(
               &g_mountedDemonJumpContextClearDepth,
               0,
               0) > 0;
}

static bool IsMountedDemonJumpResolveSafePrimeActive()
{
    return InterlockedCompareExchange(
               &g_mountedDemonJumpResolveSafePrimeDepth,
               0,
               0) > 0;
}

static void ObserveExtendedMountContext(int mountItemId)
{
    if (!IsExtendedMountSoaringContextMount(mountItemId))
    {
        return;
    }
    InterlockedExchange(&g_recentExtendedMountContextItemId, mountItemId);
    InterlockedExchange(&g_recentExtendedMountContextTick, static_cast<LONG>(GetTickCount()));
}

static void ClearExtendedMountContext()
{
    InterlockedExchange(&g_recentExtendedMountContextItemId, 0);
    InterlockedExchange(&g_recentExtendedMountContextTick, 0);
}

static void ObserveMountedDemonJumpTerminalClear(
    int mountItemId,
    const char *reasonTag)
{
    if (!kEnableMountedDoubleJumpRuntimeHooks || mountItemId <= 0)
    {
        return;
    }

    InterlockedExchange(
        &g_recentMountedDemonJumpTerminalClearMountItemId,
        mountItemId);
    InterlockedExchange(
        &g_recentMountedDemonJumpTerminalClearTick,
        static_cast<LONG>(GetTickCount()));

    static LONG s_mountedDemonJumpTerminalClearLogBudget = 48;
    if (InterlockedDecrement(
            &s_mountedDemonJumpTerminalClearLogBudget) >= 0)
    {
        WriteLogFmt(
            "[MountDemonJumpClear] arm terminal-clear mount=%d reason=%s",
            mountItemId,
            reasonTag ? reasonTag : "unknown");
    }
}

static void ClearMountedDemonJumpTerminalClear(
    int mountItemId,
    const char *reasonTag)
{
    const LONG recentMountItemId = InterlockedCompareExchange(
        &g_recentMountedDemonJumpTerminalClearMountItemId,
        0,
        0);
    const LONG recentTick = InterlockedCompareExchange(
        &g_recentMountedDemonJumpTerminalClearTick,
        0,
        0);
    if (recentMountItemId <= 0 ||
        recentTick <= 0 ||
        (mountItemId > 0 && recentMountItemId != mountItemId))
    {
        return;
    }

    InterlockedExchange(&g_recentMountedDemonJumpTerminalClearMountItemId, 0);
    InterlockedExchange(&g_recentMountedDemonJumpTerminalClearTick, 0);

    static LONG s_mountedDemonJumpTerminalClearReleaseLogBudget = 48;
    if (InterlockedDecrement(
            &s_mountedDemonJumpTerminalClearReleaseLogBudget) >= 0)
    {
        WriteLogFmt(
            "[MountDemonJumpClear] release terminal-clear mount=%d reason=%s",
            mountItemId > 0 ? mountItemId : static_cast<int>(recentMountItemId),
            reasonTag ? reasonTag : "unknown");
    }
}

static bool ShouldReleaseMountedDemonJumpTerminalClearForReason(
    const char *reasonTag)
{
    if (!reasonTag || !reasonTag[0])
    {
        return false;
    }

    if (strcmp(reasonTag, "B22630-raw-up-commit") == 0 ||
        strcmp(reasonTag, "AFB710-post-context") == 0)
    {
        return true;
    }

    return strncmp(reasonTag, "B22630-late-child-", 19) == 0;
}

static bool HasRecentMountedDemonJumpTerminalClear(
    int mountItemId,
    DWORD maxAgeMs)
{
    if (!kEnableMountedDoubleJumpRuntimeHooks)
    {
        return false;
    }

    const LONG recentMountItemId = InterlockedCompareExchange(
        &g_recentMountedDemonJumpTerminalClearMountItemId,
        0,
        0);
    const LONG recentTick = InterlockedCompareExchange(
        &g_recentMountedDemonJumpTerminalClearTick,
        0,
        0);
    if (recentMountItemId <= 0 || recentTick <= 0)
    {
        return false;
    }

    if (mountItemId > 0 && recentMountItemId != mountItemId)
    {
        return false;
    }

    const DWORD nowTick = GetTickCount();
    const DWORD allowedAgeMs =
        maxAgeMs > 0 ? maxAgeMs : kMountedDemonJumpTerminalClearSuppressMs;
    return nowTick - static_cast<DWORD>(recentTick) <= allowedAgeMs;
}

static void ObserveMountedRuntimeSkillIntent(
    MountedRuntimeSkillKind kind,
    int mountItemId)
{
    if (!kEnableMountedDoubleJumpRuntimeHooks)
    {
        return;
    }

    if (mountItemId <= 0)
    {
        return;
    }
    InterlockedExchange(&g_recentMountedRuntimeSkillIntentItemId[kind], mountItemId);
    InterlockedExchange(
        &g_recentMountedRuntimeSkillIntentTick[kind],
        static_cast<LONG>(GetTickCount()));
}

static void ObserveMountedDemonJumpGateProbe(
    int mountItemId,
    const char *reasonTag,
    DWORD callerRet)
{
    if (!kEnableMountedDoubleJumpRuntimeHooks ||
        mountItemId <= 0 ||
        ResolveMountedRuntimeSkillIdForKind(
            MountedRuntimeSkillKind_DemonJump,
            mountItemId) != 30010110)
    {
        return;
    }

    int preferredChildSkillId = 0;
    if (reasonTag &&
        (strcmp(reasonTag, "42DE20") == 0 ||
         strcmp(reasonTag, "42DE20-no-intent") == 0 ||
         strcmp(reasonTag, "B844D0-up-seed") == 0 ||
         strcmp(reasonTag, "B22630-raw-up-commit") == 0) &&
        SkillOverlayBridgeCanUseMountedDemonJumpRuntimeSkill(
            mountItemId,
            30010183))
    {
        preferredChildSkillId = 30010183;
    }

    if (ShouldReleaseMountedDemonJumpTerminalClearForReason(reasonTag))
    {
        ClearMountedDemonJumpTerminalClear(
            mountItemId,
            reasonTag);
    }

    InterlockedExchange(&g_recentMountedDemonJumpGateProbeItemId, mountItemId);
    InterlockedExchange(
        &g_recentMountedDemonJumpGateProbeChildSkillId,
        preferredChildSkillId);
    InterlockedExchange(
        &g_recentMountedDemonJumpGateProbeTick,
        static_cast<LONG>(GetTickCount()));

    static LONG s_mountedDemonJumpGateProbeLogBudget = 48;
    if (InterlockedDecrement(&s_mountedDemonJumpGateProbeLogBudget) >= 0)
    {
        WriteLogFmt(
            "[MountDemonJumpProbe] arm reason=%s caller=0x%08X mount=%d",
            reasonTag ? reasonTag : "unknown",
            callerRet,
            mountItemId);
    }
}

static bool TryGetRecentMountedDemonJumpGateProbeMountItemId(
    int *mountItemIdOut,
    DWORD maxAgeMs)
{
    if (mountItemIdOut)
    {
        *mountItemIdOut = 0;
    }

    const LONG mountItemId =
        InterlockedCompareExchange(&g_recentMountedDemonJumpGateProbeItemId, 0, 0);
    const LONG tick =
        InterlockedCompareExchange(&g_recentMountedDemonJumpGateProbeTick, 0, 0);
    if (mountItemId <= 0 || tick <= 0)
    {
        return false;
    }

    const DWORD nowTick = GetTickCount();
    if (nowTick - static_cast<DWORD>(tick) > maxAgeMs)
    {
        return false;
    }

    if (mountItemIdOut)
    {
        *mountItemIdOut = static_cast<int>(mountItemId);
    }
    return true;
}

static bool TryGetRecentMountedDemonJumpGateProbePreferredChildSkillId(
    int mountItemId,
    int *skillIdOut,
    DWORD maxAgeMs)
{
    if (skillIdOut)
    {
        *skillIdOut = 0;
    }

    int probeMountItemId = 0;
    if (!TryGetRecentMountedDemonJumpGateProbeMountItemId(
            &probeMountItemId,
            maxAgeMs) ||
        probeMountItemId <= 0 ||
        (mountItemId > 0 && probeMountItemId != mountItemId))
    {
        return false;
    }

    const int preferredChildSkillId = static_cast<int>(
        InterlockedCompareExchange(
            &g_recentMountedDemonJumpGateProbeChildSkillId,
            0,
            0));
    if (!IsMountedDemonJumpRuntimeChildSkillId(preferredChildSkillId))
    {
        return false;
    }

    if (skillIdOut)
    {
        *skillIdOut = preferredChildSkillId;
    }
    return true;
}

#include "runtime_feature_mounted_double_jump_state.inl"

static bool TryReadCurrentUserLocalPtr(void **userLocalOut)
{
    if (userLocalOut)
    {
        *userLocalOut = nullptr;
    }

    if (!userLocalOut ||
        SafeIsBadReadPtr(reinterpret_cast<void *>(ADDR_UserLocal), sizeof(DWORD)))
    {
        return false;
    }

    DWORD userLocal = 0;
    __try
    {
        userLocal = *reinterpret_cast<DWORD *>(ADDR_UserLocal);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }

    if (!userLocal)
    {
        return false;
    }

    *userLocalOut = reinterpret_cast<void *>(static_cast<uintptr_t>(userLocal));
    return true;
}

static bool TryReadMountedDemonJumpContextState(
    int *rootSkillIdOut,
    int *currentSkillIdOut,
    DWORD *userLocalOut)
{
    if (rootSkillIdOut)
    {
        *rootSkillIdOut = 0;
    }
    if (currentSkillIdOut)
    {
        *currentSkillIdOut = 0;
    }
    if (userLocalOut)
    {
        *userLocalOut = 0;
    }

    void *userLocal = nullptr;
    if (!TryReadCurrentUserLocalPtr(&userLocal))
    {
        return false;
    }

    const uintptr_t userLocalAddr = reinterpret_cast<uintptr_t>(userLocal);
    const uintptr_t contextAddr = userLocalAddr + kMountedDemonJumpContextOffset;
    const uintptr_t rootSkillAddr = userLocalAddr + kMountedDemonJumpContextRootSkillOffset;
    if (SafeIsBadReadPtr(reinterpret_cast<void *>(contextAddr), sizeof(DWORD) * 2))
    {
        return false;
    }

    DWORD contextHead = 0;
    int rootSkillId = 0;
    __try
    {
        contextHead = *reinterpret_cast<DWORD *>(contextAddr);
        rootSkillId = *reinterpret_cast<int *>(rootSkillAddr);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }

    if (!contextHead || rootSkillId <= 0)
    {
        return false;
    }

    int currentSkillId = 0;
    if (currentSkillIdOut)
    {
        tMountedDemonJumpContextCurrentSkillFn currentSkillFn =
            reinterpret_cast<tMountedDemonJumpContextCurrentSkillFn>(
                ADDR_MountedDemonJumpContextCurrentSkill4300A0);
        if (currentSkillFn)
        {
            __try
            {
                currentSkillId = currentSkillFn(
                    reinterpret_cast<void *>(contextAddr));
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                currentSkillId = 0;
            }
        }
    }

    if (rootSkillIdOut)
    {
        *rootSkillIdOut = rootSkillId;
    }
    if (currentSkillIdOut)
    {
        *currentSkillIdOut = currentSkillId > 0 ? currentSkillId : rootSkillId;
    }
    if (userLocalOut)
    {
        *userLocalOut = static_cast<DWORD>(userLocalAddr);
    }
    return true;
}

static bool HasMountedDemonJumpContextPrimedForMount(
    int mountItemId,
    int *currentSkillIdOut,
    int *rootSkillIdOut)
{
    if (currentSkillIdOut)
    {
        *currentSkillIdOut = 0;
    }
    if (rootSkillIdOut)
    {
        *rootSkillIdOut = 0;
    }

    if (mountItemId <= 0)
    {
        return false;
    }

    int currentMountItemId = 0;
    if (!TryReadCurrentUserMountItemId(&currentMountItemId) ||
        currentMountItemId != mountItemId)
    {
        return false;
    }

    const int configuredSkillId = ResolveMountedRuntimeSkillIdForKind(
        MountedRuntimeSkillKind_DemonJump,
        mountItemId);
    if (configuredSkillId <= 0)
    {
        return false;
    }

    int rootSkillId = 0;
    int currentSkillId = 0;
    if (!TryReadMountedDemonJumpContextState(
            &rootSkillId,
            &currentSkillId,
            nullptr) ||
        rootSkillId != configuredSkillId)
    {
        return false;
    }

    if (!IsMountedDemonJumpRuntimeChildSkillId(currentSkillId) &&
        (currentSkillId <= 0 ||
         currentSkillId == configuredSkillId) &&
        HasRecentMountedDemonJumpIntent(
            mountItemId,
            kMountedDemonJumpLateChildCacheMatchMaxAgeMs))
    {
        int recentChildSkillId = 0;
        if (TryGetRecentMountedDemonJumpNativeChildSkill(
                mountItemId,
                &recentChildSkillId,
                nullptr,
                kMountedDemonJumpLateChildCacheMatchMaxAgeMs) &&
            IsMountedDemonJumpRuntimeChildSkillId(recentChildSkillId) &&
            SkillOverlayBridgeCanUseMountedDemonJumpRuntimeSkill(
                mountItemId,
                recentChildSkillId))
        {
            // Evidence from v23.04 job-100 logs:
            // B300AC can already reroute root/proxy release to child=30010183,
            // but the immediate action chain still re-reads root/current as
            // 30010110/30010110 and loses the local up-jump motion.
            // Promote the fresh native child cache only inside the short
            // recent-intent window so the action/late path sees the same
            // child-shaped context as the previously successful build.
            static LONG s_mountedDemonJumpContextPromoteFreshChildLogBudget = 48;
            if (InterlockedDecrement(
                    &s_mountedDemonJumpContextPromoteFreshChildLogBudget) >= 0)
            {
                WriteLogFmt(
                    "[MountDemonJumpContext] promote fresh child mount=%d root=%d current=%d -> child=%d intent=1",
                    mountItemId,
                    rootSkillId,
                    currentSkillId,
                    recentChildSkillId);
            }
            currentSkillId = recentChildSkillId;
        }
        else
        {
            int probeChildSkillId = 0;
            if (TryGetRecentMountedDemonJumpGateProbePreferredChildSkillId(
                    mountItemId,
                    &probeChildSkillId,
                    kMountedDemonJumpIntentMaxAgeMs) &&
                probeChildSkillId == 30010183 &&
                SkillOverlayBridgeCanUseMountedDemonJumpRuntimeSkill(
                    mountItemId,
                    probeChildSkillId))
            {
                // v23.43 logs show a slow first mounted up-jump can age past
                // the short recent-child cache while the up gate-probe is still
                // fresh. Keep this fallback narrow to the verified up-child so
                // later late-route helpers still see the intended child context.
                static LONG s_mountedDemonJumpContextPromoteProbeChildLogBudget =
                    32;
                if (InterlockedDecrement(
                        &s_mountedDemonJumpContextPromoteProbeChildLogBudget) >=
                    0)
                {
                    WriteLogFmt(
                        "[MountDemonJumpContext] promote probe child mount=%d root=%d current=%d -> child=%d intent=1",
                        mountItemId,
                        rootSkillId,
                        currentSkillId,
                        probeChildSkillId);
                }
                currentSkillId = probeChildSkillId;
            }
        }
    }

    if (currentSkillIdOut)
    {
        *currentSkillIdOut = currentSkillId > 0 ? currentSkillId : rootSkillId;
    }
    if (rootSkillIdOut)
    {
        *rootSkillIdOut = rootSkillId;
    }
    return true;
}

static bool HasFreshMountedRuntimeSkillNativeReleaseRaw(
    MountedRuntimeSkillKind kind,
    int expectedSkillId,
    int expectedMountItemId,
    DWORD maxAgeMs = 450)
{
    if (!kEnableMountedDoubleJumpRuntimeHooks ||
        expectedSkillId <= 0 ||
        expectedMountItemId <= 0)
    {
        return false;
    }

    const LONG recentSkillId =
        InterlockedCompareExchange(
            &g_recentMountedRuntimeSkillNativeReleaseSkillId[kind],
            0,
            0);
    const LONG recentMountItemId =
        InterlockedCompareExchange(
            &g_recentMountedRuntimeSkillNativeReleaseItemId[kind],
            0,
            0);
    const LONG recentTick =
        InterlockedCompareExchange(
            &g_recentMountedRuntimeSkillNativeReleaseTick[kind],
            0,
            0);
    if (recentSkillId != expectedSkillId ||
        recentMountItemId != expectedMountItemId ||
        recentTick <= 0)
    {
        return false;
    }

    const DWORD nowTick = GetTickCount();
    return nowTick - static_cast<DWORD>(recentTick) <= maxAgeMs;
}

static bool TryReadMountedDemonJumpEffectiveContextState(
    int mountItemId,
    int *rootSkillIdOut,
    int *currentSkillIdOut)
{
    if (rootSkillIdOut)
    {
        *rootSkillIdOut = 0;
    }
    if (currentSkillIdOut)
    {
        *currentSkillIdOut = 0;
    }

    if (mountItemId > 0 &&
        HasMountedDemonJumpContextPrimedForMount(
            mountItemId,
            currentSkillIdOut,
            rootSkillIdOut))
    {
        return true;
    }

    return TryReadMountedDemonJumpContextState(
        rootSkillIdOut,
        currentSkillIdOut,
        nullptr);
}

static bool ShouldSuppressMountedDemonJumpMountedContextClear(
    void *contextPtr,
    DWORD callerRet,
    int *mountItemIdOut,
    int *rootSkillIdOut,
    int *currentSkillIdOut)
{
    if (mountItemIdOut)
    {
        *mountItemIdOut = 0;
    }
    if (rootSkillIdOut)
    {
        *rootSkillIdOut = 0;
    }
    if (currentSkillIdOut)
    {
        *currentSkillIdOut = 0;
    }

    const bool callerMatchesMountedClearWindow =
        callerRet == 0x00B2F4B6 ||
        callerRet == ADDR_MountedDemonJumpContextMountedClearReturn433F48 ||
        callerRet == 0x00433FEC ||
        callerRet == 0x00433EAA ||
        (callerRet >= 0x00433E80 && callerRet <= 0x00434010);
    if (!kEnableMountedDoubleJumpRuntimeHooks ||
        !contextPtr ||
        !callerMatchesMountedClearWindow)
    {
        return false;
    }

    void *userLocal = nullptr;
    if (!TryReadCurrentUserLocalPtr(&userLocal) || !userLocal)
    {
        return false;
    }

    const uintptr_t expectedContext =
        reinterpret_cast<uintptr_t>(userLocal) + kMountedDemonJumpContextOffset;
    if (reinterpret_cast<uintptr_t>(contextPtr) != expectedContext)
    {
        return false;
    }

    int mountItemId = 0;
    if (!TryReadCurrentUserMountItemId(&mountItemId) || mountItemId <= 0)
    {
        return false;
    }

    const int configuredSkillId = ResolveMountedRuntimeSkillIdForKind(
        MountedRuntimeSkillKind_DemonJump,
        mountItemId);
    if (configuredSkillId != 30010110)
    {
        return false;
    }

    int rootSkillId = 0;
    int currentSkillId = 0;
    if (!HasMountedDemonJumpContextPrimedForMount(
            mountItemId,
            &currentSkillId,
            &rootSkillId))
    {
        return false;
    }

    const bool rootIsChild =
        IsMountedDemonJumpRuntimeChildSkillId(rootSkillId);
    const bool currentIsChild =
        IsMountedDemonJumpRuntimeChildSkillId(currentSkillId);
    if (rootSkillId != configuredSkillId &&
        !rootIsChild &&
        !currentIsChild)
    {
        return false;
    }

    // Evidence from v22.25 logs:
    // mounted side child 30010184 reaches B28A00 successfully, but the
    // postpacket finalize path does not clear root/current immediately.
    // If 433FEC is still suppressed here, the native tail never gets a chance
    // to drop the remaining context/local gate, which leaves side-jump unable
    // to retrigger for a few seconds after landing. Allow only this exact
    // 433FEC side-child tail through; up-child still uses its existing
    // B28A00->433FEC finalize path and glide keeps its own native context.
    if (callerRet == 0x00433FEC &&
        rootSkillId == configuredSkillId &&
        currentSkillId == 30010184)
    {
        return false;
    }

    if (!HasRecentMountedDemonJumpIntent(
            mountItemId,
            kMountedDemonJumpContextClearProtectMs))
    {
        return false;
    }

    if (!IsMountedDemonJumpRuntimeChildSkillId(currentSkillId))
    {
        int recentChildSkillId = 0;
        if (!(currentSkillId == configuredSkillId &&
              TryGetRecentMountedDemonJumpNativeChildSkill(
                  mountItemId,
                  &recentChildSkillId,
                  nullptr,
                  kMountedDemonJumpLateChildPrimeWindowMs) &&
              IsMountedDemonJumpRuntimeChildSkillId(recentChildSkillId)))
        {
            return false;
        }
        currentSkillId = recentChildSkillId;
    }

    if (rootSkillId != configuredSkillId &&
        (rootIsChild || currentIsChild))
    {
        rootSkillId = configuredSkillId;
    }

    if (mountItemIdOut)
    {
        *mountItemIdOut = mountItemId;
    }
    if (rootSkillIdOut)
    {
        *rootSkillIdOut = rootSkillId;
    }
    if (currentSkillIdOut)
    {
        *currentSkillIdOut = currentSkillId;
    }
    return true;
}

static bool TryManualPrimeMountedDemonJumpContext(
    void *userLocal,
    int mountItemId,
    int *currentSkillIdOut,
    int *rootSkillIdOut)
{
    if (currentSkillIdOut)
    {
        *currentSkillIdOut = 0;
    }
    if (rootSkillIdOut)
    {
        *rootSkillIdOut = 0;
    }

    if (!userLocal || mountItemId <= 0)
    {
        return false;
    }

    const int configuredSkillId = ResolveMountedRuntimeSkillIdForKind(
        MountedRuntimeSkillKind_DemonJump,
        mountItemId);
    if (configuredSkillId != 30010110)
    {
        return false;
    }

    tMountedDemonJumpSkillEntryLookupFn skillEntryLookupFn =
        reinterpret_cast<tMountedDemonJumpSkillEntryLookupFn>(
            ADDR_MountedDemonJumpSkillEntryLookupAE0420);
    tMountedDemonJumpContextSeedFn seedFn =
        reinterpret_cast<tMountedDemonJumpContextSeedFn>(
            ADDR_MountedDemonJumpContextSeedAC6B00);
    if (!skillEntryLookupFn || !seedFn)
    {
        return false;
    }

    unsigned int *nativeSkillEntry = nullptr;
    signed int nativeLookupResult = 0;
    __try
    {
        nativeLookupResult = skillEntryLookupFn(configuredSkillId, &nativeSkillEntry);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        nativeLookupResult = 0;
        nativeSkillEntry = nullptr;
    }

    const uintptr_t bridgeEntryBase =
        SkillOverlayBridgeLookupSkillEntryPointer(configuredSkillId);
    const uintptr_t nativeEntryBase =
        reinterpret_cast<uintptr_t>(nativeSkillEntry);

    uintptr_t entryBase = 0;
    const char *entrySource = "none";
    if (bridgeEntryBase &&
        !SafeIsBadReadPtr(reinterpret_cast<void *>(bridgeEntryBase), 1736))
    {
        entryBase = bridgeEntryBase;
        entrySource = "bridge";
    }
    else if (nativeEntryBase &&
             !SafeIsBadReadPtr(reinterpret_cast<void *>(nativeEntryBase), 1736))
    {
        entryBase = nativeEntryBase;
        entrySource = "ae0420";
    }

    static LONG s_mountedDemonJumpSeedLookupLogBudget = 48;
    const LONG lookupBudgetAfterDecrement =
        InterlockedDecrement(&s_mountedDemonJumpSeedLookupLogBudget);
    if (lookupBudgetAfterDecrement >= 0)
    {
        WriteLogFmt(
            "[MountDemonJumpSeed] lookup mount=%d skill=%d nativeLevel=%d nativeEntry=0x%08X bridgeEntry=0x%08X source=%s",
            mountItemId,
            configuredSkillId,
            nativeLookupResult,
            static_cast<DWORD>(nativeEntryBase),
            static_cast<DWORD>(bridgeEntryBase),
            entrySource);
    }

    if (!entryBase)
    {
        return false;
    }

    DWORD hasSeedPayload = 0;
    int a4 = 0;
    int a8 = 0;
    int a10 = 0;
    int a11 = 0;
    int a12 = 0;
    BYTE a5 = 0;
    WORD a6 = 0;
    int a9 = 0;
    __try
    {
        hasSeedPayload = *reinterpret_cast<DWORD *>(entryBase + 1632);
        a4 = *reinterpret_cast<int *>(entryBase + 0);
        a5 = *reinterpret_cast<BYTE *>(entryBase + 1684);
        a6 = *reinterpret_cast<WORD *>(entryBase + 1688);
        a8 = *reinterpret_cast<int *>(entryBase + 1704);
        a9 = *reinterpret_cast<DWORD *>(entryBase + 1692) != 0 ? 1 : 0;
        a10 = *reinterpret_cast<int *>(entryBase + 1696);
        a11 = *reinterpret_cast<int *>(entryBase + 1700);
        a12 = *reinterpret_cast<int *>(entryBase + 1732);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
    if (!hasSeedPayload)
    {
        static LONG s_mountedDemonJumpSeedPayloadLogBudget = 24;
        const LONG payloadBudgetAfterDecrement =
            InterlockedDecrement(&s_mountedDemonJumpSeedPayloadLogBudget);
        if (payloadBudgetAfterDecrement >= 0)
        {
            WriteLogFmt(
                "[MountDemonJumpSeed] missing payload mount=%d skill=%d source=%s entry=0x%08X",
                mountItemId,
                configuredSkillId,
                entrySource,
                static_cast<DWORD>(entryBase));
        }
        return false;
    }

    __try
    {
        seedFn(
            userLocal,
            static_cast<int>(entryBase + 1636),
            static_cast<int>(entryBase + 1660),
            a4,
            static_cast<int>(a5),
            static_cast<int>(a6),
            static_cast<int>(entryBase + 1708),
            a8,
            a9,
            a10,
            a11,
            a12);
        *reinterpret_cast<BYTE *>(
            reinterpret_cast<uintptr_t>(userLocal) + kMountedDemonJumpReadyFlagOffset) = 0;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }

    const bool primed = HasMountedDemonJumpContextPrimedForMount(
        mountItemId,
        currentSkillIdOut,
        rootSkillIdOut);

    static LONG s_mountedDemonJumpSeedResultLogBudget = 48;
    const LONG resultBudgetAfterDecrement =
        InterlockedDecrement(&s_mountedDemonJumpSeedResultLogBudget);
    if (resultBudgetAfterDecrement >= 0)
    {
        WriteLogFmt(
            "[MountDemonJumpSeed] result mount=%d skill=%d source=%s entry=0x%08X nativeLevel=%d payload=%u primed=%d root=%d current=%d",
            mountItemId,
            configuredSkillId,
            entrySource,
            static_cast<DWORD>(entryBase),
            nativeLookupResult,
            hasSeedPayload,
            primed ? 1 : 0,
            rootSkillIdOut ? *rootSkillIdOut : 0,
            currentSkillIdOut ? *currentSkillIdOut : 0);
    }

    return primed;
}

static bool TryManualPrimeMountedDemonJumpContextResolveSafe(
    int mountItemId,
    const char *reason,
    int *currentSkillIdOut,
    int *rootSkillIdOut)
{
    if (currentSkillIdOut)
    {
        *currentSkillIdOut = 0;
    }
    if (rootSkillIdOut)
    {
        *rootSkillIdOut = 0;
    }

    if (mountItemId <= 0)
    {
        return false;
    }

    int currentMountItemId = 0;
    if (!TryReadCurrentUserMountItemId(&currentMountItemId) ||
        currentMountItemId != mountItemId)
    {
        return false;
    }

    void *userLocal = nullptr;
    if (!TryReadCurrentUserLocalPtr(&userLocal))
    {
        return false;
    }

    struct ScopedResolveSafePrimeGuard
    {
        ScopedResolveSafePrimeGuard()
        {
            InterlockedIncrement(&g_mountedDemonJumpResolveSafePrimeDepth);
        }

        ~ScopedResolveSafePrimeGuard()
        {
            InterlockedDecrement(&g_mountedDemonJumpResolveSafePrimeDepth);
        }
    } scopedResolveSafePrimeGuard;

    int currentSkillId = 0;
    int rootSkillId = 0;
    const bool primed = TryManualPrimeMountedDemonJumpContext(
        userLocal,
        mountItemId,
        &currentSkillId,
        &rootSkillId);
    if (currentSkillIdOut)
    {
        *currentSkillIdOut = currentSkillId;
    }
    if (rootSkillIdOut)
    {
        *rootSkillIdOut = rootSkillId;
    }

    static LONG s_mountedDemonJumpResolveSafePrimeLogBudget = 32;
    if (InterlockedDecrement(&s_mountedDemonJumpResolveSafePrimeLogBudget) >= 0)
    {
        WriteLogFmt(
            "[MountDemonJumpPrime] resolve-safe reason=%s mount=%d user=0x%08X root=%d current=%d primed=%d",
            reason ? reason : "unknown",
            mountItemId,
            static_cast<DWORD>(reinterpret_cast<uintptr_t>(userLocal)),
            rootSkillId,
            currentSkillId,
            primed ? 1 : 0);
    }

    return primed;
}

static bool PrimeMountedDemonJumpContextIfNeeded(
    int mountItemId,
    const char *reason,
    int *currentSkillIdOut)
{
    if (currentSkillIdOut)
    {
        *currentSkillIdOut = 0;
    }

    if (mountItemId <= 0)
    {
        return false;
    }

    int currentSkillId = 0;
    int rootSkillId = 0;
    if (HasMountedDemonJumpContextPrimedForMount(
            mountItemId,
            &currentSkillId,
            &rootSkillId))
    {
        if (currentSkillIdOut)
        {
            *currentSkillIdOut = currentSkillId;
        }
        return true;
    }

    int currentMountItemId = 0;
    if (!TryReadCurrentUserMountItemId(&currentMountItemId) ||
        currentMountItemId != mountItemId)
    {
        return false;
    }

    const bool eagerIntentPrime =
        reason != nullptr &&
        strcmp(reason, "intent") == 0;
    if (eagerIntentPrime)
    {
        // Current v23.06 runtime evidence shows the very first mounted up-jump
        // often stalls here on a failed native+manual prime:
        //   [MountDemonJumpPrime] reason=intent ... primed=0 native=1 manual=1
        // That consumes the short recent-intent window, then 42DE20 falls back
        // into the no-intent branch and the real release only succeeds after
        // several retries. Keep the early intent arm lightweight; later
        // B22630 / 42DE20 / B300AC paths still perform the real prime exactly
        // where the native child/release chain is already confirmed.
        static LONG s_mountedDemonJumpPrimeSkipIntentLogBudget = 24;
        if (InterlockedDecrement(
                &s_mountedDemonJumpPrimeSkipIntentLogBudget) >= 0)
        {
            WriteLogFmt(
                "[MountDemonJumpPrime] skip eager intent mount=%d",
                mountItemId);
        }
        return false;
    }

    void *userLocal = nullptr;
    if (!TryReadCurrentUserLocalPtr(&userLocal))
    {
        return false;
    }

    bool nativePrimeAttempted = false;
    bool manualPrimeAttempted = false;
    tMountedDemonJumpContextPrimeFn primeFn =
        reinterpret_cast<tMountedDemonJumpContextPrimeFn>(
            ADDR_MountedDemonJumpContextPrimeB00AD0);
    if (primeFn)
    {
        nativePrimeAttempted = true;
        __try
        {
            primeFn(userLocal, nullptr);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
        }
    }
    bool primed = HasMountedDemonJumpContextPrimedForMount(
        mountItemId,
        &currentSkillId,
        &rootSkillId);
    if (!primed)
    {
        manualPrimeAttempted = true;
        primed = TryManualPrimeMountedDemonJumpContext(
            userLocal,
            mountItemId,
            &currentSkillId,
            &rootSkillId);
    }
    if (currentSkillIdOut)
    {
        *currentSkillIdOut = currentSkillId;
    }

    static LONG s_mountedDemonJumpPrimeLogBudget = 48;
    const LONG budgetAfterDecrement =
        InterlockedDecrement(&s_mountedDemonJumpPrimeLogBudget);
    if (budgetAfterDecrement >= 0)
    {
        WriteLogFmt(
            "[MountDemonJumpPrime] reason=%s mount=%d user=0x%08X root=%d current=%d primed=%d native=%d manual=%d",
            reason ? reason : "unknown",
            mountItemId,
            static_cast<DWORD>(reinterpret_cast<uintptr_t>(userLocal)),
            rootSkillId,
            currentSkillId,
            primed ? 1 : 0,
            nativePrimeAttempted ? 1 : 0,
            manualPrimeAttempted ? 1 : 0);
    }

    return primed;
}

static bool ForceRefreshMountedDemonJumpContextSeed(
    void *userLocal,
    int mountItemId,
    const char *reason,
    int *currentSkillIdOut,
    int *rootSkillIdOut)
{
    if (currentSkillIdOut)
    {
        *currentSkillIdOut = 0;
    }
    if (rootSkillIdOut)
    {
        *rootSkillIdOut = 0;
    }

    if (!userLocal || mountItemId <= 0)
    {
        return false;
    }

    int currentMountItemId = 0;
    if (!TryReadCurrentUserMountItemId(&currentMountItemId) ||
        currentMountItemId != mountItemId)
    {
        return false;
    }

    bool nativePrimeAttempted = false;
    bool manualPrimeAttempted = false;
    tMountedDemonJumpContextPrimeFn primeFn =
        reinterpret_cast<tMountedDemonJumpContextPrimeFn>(
            ADDR_MountedDemonJumpContextPrimeB00AD0);
    if (primeFn)
    {
        nativePrimeAttempted = true;
        __try
        {
            primeFn(userLocal, nullptr);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
        }
    }

    int currentSkillId = 0;
    int rootSkillId = 0;
    bool primed = HasMountedDemonJumpContextPrimedForMount(
        mountItemId,
        &currentSkillId,
        &rootSkillId);
    if (!primed)
    {
        manualPrimeAttempted = true;
        primed = TryManualPrimeMountedDemonJumpContext(
            userLocal,
            mountItemId,
            &currentSkillId,
            &rootSkillId);
    }

    if (currentSkillIdOut)
    {
        *currentSkillIdOut = currentSkillId;
    }
    if (rootSkillIdOut)
    {
        *rootSkillIdOut = rootSkillId;
    }

    static LONG s_mountedDemonJumpForceSeedLogBudget = 48;
    if (InterlockedDecrement(&s_mountedDemonJumpForceSeedLogBudget) >= 0)
    {
        WriteLogFmt(
            "[MountDemonJumpPrime] force reason=%s mount=%d user=0x%08X root=%d current=%d primed=%d native=%d manual=%d",
            reason ? reason : "unknown",
            mountItemId,
            static_cast<DWORD>(reinterpret_cast<uintptr_t>(userLocal)),
            rootSkillId,
            currentSkillId,
            primed ? 1 : 0,
            nativePrimeAttempted ? 1 : 0,
            manualPrimeAttempted ? 1 : 0);
    }

    return primed;
}

static bool TryReadCurrentUserMountItemId(int *mountItemIdOut)
{
    if (!mountItemIdOut || SafeIsBadReadPtr(reinterpret_cast<void *>(ADDR_UserLocal), sizeof(DWORD)))
    {
        return false;
    }

    DWORD userLocal = 0;
    __try
    {
        userLocal = *reinterpret_cast<DWORD *>(ADDR_UserLocal);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
    if (!userLocal)
    {
        return false;
    }

    const uintptr_t mountItemIdAddr = static_cast<uintptr_t>(userLocal) + 0x454;
    if (SafeIsBadReadPtr(reinterpret_cast<void *>(mountItemIdAddr), sizeof(DWORD)))
    {
        return false;
    }

    int mountItemId = 0;
    __try
    {
        mountItemId = *reinterpret_cast<int *>(mountItemIdAddr);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }

    if (mountItemId <= 0)
    {
        return false;
    }
    *mountItemIdOut = mountItemId;
    return true;
}

static bool TryGetActiveMountedSoaringFlightItemId(int *mountItemIdOut, DWORD maxAgeMs = 0)
{
    if (!mountItemIdOut)
    {
        return false;
    }

    const LONG activeMountItemId =
        InterlockedCompareExchange(&g_activeMountedSoaringFlightItemId, 0, 0);
    if (activeMountItemId <= 0)
    {
        return false;
    }

    const LONG activeTick =
        InterlockedCompareExchange(&g_activeMountedSoaringFlightTick, 0, 0);
    if (activeTick <= 0)
    {
        return false;
    }

    const DWORD nowTick = GetTickCount();
    const DWORD allowedAgeMs =
        maxAgeMs > 0 ? maxAgeMs : 10000;
    if (nowTick - static_cast<DWORD>(activeTick) > allowedAgeMs)
    {
        return false;
    }

    *mountItemIdOut = static_cast<int>(activeMountItemId);
    return true;
}

static bool TryGetRecentMountedFlightScaleSampleMountItemId(
    int *mountItemIdOut,
    DWORD maxAgeMs = 10000)
{
    if (!mountItemIdOut)
    {
        return false;
    }

    const MountedFlightPhysicsScaleSample sample = g_MountedFlightPhysicsScaleSample;
    if (sample.mountItemId <= 0 || sample.tick == 0)
    {
        return false;
    }

    const DWORD nowTick = GetTickCount();
    const DWORD allowedAgeMs = maxAgeMs > 0 ? maxAgeMs : 10000;
    if (nowTick - sample.tick > allowedAgeMs)
    {
        return false;
    }

    *mountItemIdOut = sample.mountItemId;
    return true;
}

static void ClearMountedSoaringRuntimeFallbackState();

static bool TryResolveCurrentUserMountItemIdWithFallback(
    int *mountItemIdOut,
    const char **sourceOut)
{
    if (mountItemIdOut)
    {
        *mountItemIdOut = 0;
    }
    if (sourceOut)
    {
        *sourceOut = nullptr;
    }

    int mountItemId = 0;
    const char *source = nullptr;
    bool shouldObserveExtendedMountContext = false;
    if (TryReadCurrentUserMountItemId(&mountItemId))
    {
        source = "user";
        shouldObserveExtendedMountContext = true;
    }
    else if (TryGetRecentExtendedMountContext(&mountItemId) &&
             IsExtendedMountSoaringContextMount(mountItemId))
    {
        source = "recent-extended";
    }
    else if (TryGetActiveMountedSoaringFlightItemId(
                 &mountItemId,
                 kMountedSoaringFallbackGraceMs) &&
             IsExtendedMountSoaringContextMount(mountItemId))
    {
        source = "active-soaring";
    }
    else
    {
        // Do not let the cached scale baseline masquerade as a live mount
        // source. That cache is useful to preserve the next legitimate flight
        // boost sample, but using it as "current mount" resurrects stale mount
        // context after dismount.
        ClearMountedSoaringRuntimeFallbackState();
        return false;
    }

    if (shouldObserveExtendedMountContext &&
        IsExtendedMountSoaringContextMount(mountItemId))
    {
        ObserveExtendedMountContext(mountItemId);
    }

    if (mountItemIdOut)
    {
        *mountItemIdOut = mountItemId;
    }
    if (sourceOut)
    {
        *sourceOut = source;
    }
    return true;
}

#include "runtime_feature_mounted_double_jump_resolve.inl"

static bool TryResolveMountItemIdFromContextPointer(void *mountContext, int *mountItemIdOut)
{
    if (!mountContext || !mountItemIdOut)
    {
        return false;
    }

    if (SafeIsBadReadPtr(mountContext, sizeof(DWORD)))
    {
        return false;
    }

    tMountContextGetItemIdFn getItemIdFn =
        reinterpret_cast<tMountContextGetItemIdFn>(ADDR_7D4CA0);
    if (!getItemIdFn)
    {
        return false;
    }

    int mountItemId = 0;
    __try
    {
        mountItemId = getItemIdFn(mountContext);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }

    if (mountItemId <= 0)
    {
        return false;
    }

    *mountItemIdOut = mountItemId;
    return true;
}

static bool TryReadMountItemIdFromPlayerObject(void *playerObj, int *mountItemIdOut)
{
    int mountItemId = 0;
    if (!TryReadMountItemIdFromPlayerObjectRaw(playerObj, &mountItemId) ||
        mountItemId <= 0)
    {
        return false;
    }

    *mountItemIdOut = mountItemId;
    return true;
}

static bool TryReadMountItemIdFromPlayerObjectRaw(void *playerObj, int *mountItemIdOut)
{
    if (!playerObj || !mountItemIdOut)
    {
        return false;
    }

    const uintptr_t mountItemIdAddr = reinterpret_cast<uintptr_t>(playerObj) + 0x454;
    if (SafeIsBadReadPtr(reinterpret_cast<void *>(mountItemIdAddr), sizeof(DWORD)))
    {
        return false;
    }

    int mountItemId = 0;
    __try
    {
        mountItemId = *reinterpret_cast<int *>(mountItemIdAddr);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }

    *mountItemIdOut = mountItemId;
    return true;
}

static bool TryResolveMountedMovementDataKeyFromMountItemId(int mountItemId, int *dataKeyOut)
{
    if (!dataKeyOut)
    {
        return false;
    }
    *dataKeyOut = 0;

    if (mountItemId <= 0)
    {
        return false;
    }

    tMountItemInfoLookupFn lookupFn =
        reinterpret_cast<tMountItemInfoLookupFn>(ADDR_6545A0);
    tMountItemInfoDataKeyFn dataKeyFn =
        reinterpret_cast<tMountItemInfoDataKeyFn>(ADDR_B22500);
    if (!lookupFn || !dataKeyFn ||
        SafeIsBadReadPtr(reinterpret_cast<void *>(static_cast<uintptr_t>(ADDR_F59D34)), sizeof(void *)))
    {
        return false;
    }

    void *mountInfoTable = nullptr;
    void *mountInfo = nullptr;
    int dataKey = 0;
    __try
    {
        mountInfoTable =
            *reinterpret_cast<void **>(static_cast<uintptr_t>(ADDR_F59D34));
        if (!mountInfoTable)
        {
            return false;
        }

        // Follow B92D10's native chain exactly:
        //   mountItemId -> sub_6545A0(dword_F59D34, itemId) -> sub_B22500(info)
        // This yields the same runtime data key later passed to 888B30, so we
        // can prime flight speed immediately instead of waiting for the first
        // natural mounted movement lookup to arrive a few seconds later.
        mountInfo = lookupFn(mountInfoTable, mountItemId);
        if (!mountInfo)
        {
            return false;
        }

        dataKey = dataKeyFn(mountInfo);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }

    if (dataKey <= 0)
    {
        return false;
    }

    *dataKeyOut = dataKey;

    static LONG s_mountDataKeyResolveLogBudget = 24;
    if (InterlockedDecrement(&s_mountDataKeyResolveLogBudget) >= 0)
    {
        WriteLogFmt(
            "[MountDataKeyResolve] mount=%d key=%d source=B92D10-native",
            mountItemId,
            dataKey);
    }
    return true;
}

