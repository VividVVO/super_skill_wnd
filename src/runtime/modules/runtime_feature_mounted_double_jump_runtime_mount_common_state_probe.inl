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
