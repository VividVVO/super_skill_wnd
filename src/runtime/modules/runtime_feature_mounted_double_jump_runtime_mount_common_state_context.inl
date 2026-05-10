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

