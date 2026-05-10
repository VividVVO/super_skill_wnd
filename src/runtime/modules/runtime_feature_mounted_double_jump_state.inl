static void ObserveMountedDoubleJumpIntent(int mountItemId)
{
    ObserveMountedRuntimeSkillIntent(MountedRuntimeSkillKind_DoubleJump, mountItemId);
}

static void ClearMountedRuntimeSkillTransientStateIfMatching(
    MountedRuntimeSkillKind kind,
    int mountItemId,
    const char *reasonTag,
    const char *logTag)
{
    if (!kEnableMountedDoubleJumpRuntimeHooks || mountItemId <= 0)
    {
        return;
    }

    const LONG recentIntentMountItemId = InterlockedCompareExchange(
        &g_recentMountedRuntimeSkillIntentItemId[kind],
        0,
        0);
    const LONG recentIntentTick = InterlockedCompareExchange(
        &g_recentMountedRuntimeSkillIntentTick[kind],
        0,
        0);
    const LONG recentNativeReleaseMountItemId = InterlockedCompareExchange(
        &g_recentMountedRuntimeSkillNativeReleaseItemId[kind],
        0,
        0);
    const LONG recentNativeReleaseSkillId = InterlockedCompareExchange(
        &g_recentMountedRuntimeSkillNativeReleaseSkillId[kind],
        0,
        0);
    const LONG recentNativeReleaseTick = InterlockedCompareExchange(
        &g_recentMountedRuntimeSkillNativeReleaseTick[kind],
        0,
        0);
    const bool hadIntent = recentIntentMountItemId == mountItemId;
    const bool hadNativeRelease = recentNativeReleaseMountItemId == mountItemId;
    if (!hadIntent && !hadNativeRelease)
    {
        return;
    }

    DWORD intentAgeMs = 0;
    if (hadIntent && recentIntentTick > 0)
    {
        intentAgeMs = GetTickCount() - static_cast<DWORD>(recentIntentTick);
    }
    DWORD nativeReleaseAgeMs = 0;
    if (hadNativeRelease && recentNativeReleaseTick > 0)
    {
        nativeReleaseAgeMs =
            GetTickCount() - static_cast<DWORD>(recentNativeReleaseTick);
    }

    InterlockedExchange(&g_recentMountedRuntimeSkillIntentItemId[kind], 0);
    InterlockedExchange(&g_recentMountedRuntimeSkillIntentTick[kind], 0);
    InterlockedExchange(&g_recentMountedRuntimeSkillNativeReleaseItemId[kind], 0);
    InterlockedExchange(&g_recentMountedRuntimeSkillNativeReleaseSkillId[kind], 0);
    InterlockedExchange(&g_recentMountedRuntimeSkillNativeReleaseTick[kind], 0);

    static LONG s_clearMountedRuntimeSkillTransientStateLogBudget = 96;
    if (InterlockedDecrement(
            &s_clearMountedRuntimeSkillTransientStateLogBudget) >= 0)
    {
        WriteLogFmt(
            "[%s] clear transient runtime mount=%d reason=%s hadIntent=%d intentAge=%u hadRelease=%d releaseSkill=%d releaseAge=%u",
            logTag ? logTag : "MountRuntimeSkill",
            mountItemId,
            reasonTag ? reasonTag : "unknown",
            hadIntent ? 1 : 0,
            intentAgeMs,
            hadNativeRelease ? 1 : 0,
            hadNativeRelease ? static_cast<int>(recentNativeReleaseSkillId) : 0,
            nativeReleaseAgeMs);
    }
}

static DWORD NormalizeMountedRuntimeSkillIntentAgeMs(
    MountedRuntimeSkillKind kind,
    DWORD maxAgeMs,
    DWORD defaultAgeMs)
{
    DWORD allowedAgeMs = maxAgeMs > 0 ? maxAgeMs : defaultAgeMs;
    if (kind == MountedRuntimeSkillKind_DemonJump &&
        allowedAgeMs > kMountedDemonJumpIntentMaxAgeMs)
    {
        allowedAgeMs = kMountedDemonJumpIntentMaxAgeMs;
    }
    return allowedAgeMs;
}

static void ObserveMountedDemonJumpIntent(
    int mountItemId,
    const char *reasonTag)
{
    ObserveMountedRuntimeSkillIntent(MountedRuntimeSkillKind_DemonJump, mountItemId);
    if (ShouldReleaseMountedDemonJumpTerminalClearForReason(reasonTag))
    {
        ClearMountedDemonJumpTerminalClear(
            mountItemId,
            reasonTag);
    }
    InterlockedExchange(&g_recentMountedDemonJumpGateProbeItemId, 0);
    InterlockedExchange(&g_recentMountedDemonJumpGateProbeChildSkillId, 0);
    InterlockedExchange(&g_recentMountedDemonJumpGateProbeTick, 0);
    // Mounted demon jump and mounted double-jump share early native gates on
    // mounts that have both skills enabled. Once the demon path is confirmed,
    // clear the stale double-jump transient state for this same mount so the
    // later flight/landing branch does not keep seeing both intents at once.
    ClearMountedRuntimeSkillTransientStateIfMatching(
        MountedRuntimeSkillKind_DoubleJump,
        mountItemId,
        reasonTag ? reasonTag : "demon-intent",
        "MountDoubleJump");
    PrimeMountedDemonJumpContextIfNeeded(mountItemId, "intent");
}

static void ObserveMountedRuntimeSkillNativeRelease(
    MountedRuntimeSkillKind kind,
    int mountItemId,
    int skillId)
{
    if (!kEnableMountedDoubleJumpRuntimeHooks)
    {
        return;
    }

    if (mountItemId <= 0 || skillId <= 0)
    {
        return;
    }

    InterlockedExchange(&g_recentMountedRuntimeSkillNativeReleaseItemId[kind], mountItemId);
    InterlockedExchange(&g_recentMountedRuntimeSkillNativeReleaseSkillId[kind], skillId);
    InterlockedExchange(
        &g_recentMountedRuntimeSkillNativeReleaseTick[kind],
        static_cast<LONG>(GetTickCount()));
}

static void ObserveMountedDoubleJumpNativeRelease(int mountItemId, int skillId)
{
    ObserveMountedRuntimeSkillNativeRelease(
        MountedRuntimeSkillKind_DoubleJump,
        mountItemId,
        skillId);
}

static void ObserveMountedDemonJumpNativeRelease(int mountItemId, int skillId)
{
    ObserveMountedRuntimeSkillNativeRelease(
        MountedRuntimeSkillKind_DemonJump,
        mountItemId,
        skillId);
}

static bool HasRecentMountedRuntimeSkillIntent(
    MountedRuntimeSkillKind kind,
    int mountItemId,
    DWORD maxAgeMs)
{
    if (!kEnableMountedDoubleJumpRuntimeHooks)
    {
        return false;
    }

    if (HasRecentMountedRuntimeRouteArmForKind(kind, mountItemId, maxAgeMs))
    {
        return true;
    }

    const LONG recentMountItemId =
        InterlockedCompareExchange(&g_recentMountedRuntimeSkillIntentItemId[kind], 0, 0);
    if (recentMountItemId <= 0)
    {
        return false;
    }

    if (mountItemId > 0 && recentMountItemId != mountItemId)
    {
        return false;
    }

    const LONG recentTick =
        InterlockedCompareExchange(&g_recentMountedRuntimeSkillIntentTick[kind], 0, 0);
    if (recentTick <= 0)
    {
        return false;
    }

    const DWORD nowTick = GetTickCount();
    const DWORD ageMs = nowTick - static_cast<DWORD>(recentTick);
    const DWORD allowedAgeMs = NormalizeMountedRuntimeSkillIntentAgeMs(
        kind,
        maxAgeMs,
        400);
    return ageMs <= allowedAgeMs;
}

static bool HasRecentMountedDoubleJumpIntent(int mountItemId, DWORD maxAgeMs)
{
    return HasRecentMountedRuntimeSkillIntent(
        MountedRuntimeSkillKind_DoubleJump,
        mountItemId,
        maxAgeMs);
}

static bool HasRecentMountedDemonJumpIntent(int mountItemId, DWORD maxAgeMs)
{
    return HasRecentMountedRuntimeSkillIntent(
        MountedRuntimeSkillKind_DemonJump,
        mountItemId,
        maxAgeMs);
}

static bool TryResolveRecentMountedRuntimeSkillNativeRelease(
    MountedRuntimeSkillKind kind,
    int expectedSkillId,
    int *mountItemIdOut,
    DWORD maxAgeMs = 450)
{
    if (!kEnableMountedDoubleJumpRuntimeHooks)
    {
        return false;
    }

    if (expectedSkillId <= 0)
    {
        return false;
    }

    const LONG recentSkillId =
        InterlockedCompareExchange(&g_recentMountedRuntimeSkillNativeReleaseSkillId[kind], 0, 0);
    if (recentSkillId <= 0 || recentSkillId != expectedSkillId)
    {
        return false;
    }

    const LONG recentMountItemId =
        InterlockedCompareExchange(&g_recentMountedRuntimeSkillNativeReleaseItemId[kind], 0, 0);
    if (recentMountItemId <= 0)
    {
        return false;
    }

    const LONG recentTick =
        InterlockedCompareExchange(&g_recentMountedRuntimeSkillNativeReleaseTick[kind], 0, 0);
    if (recentTick <= 0)
    {
        return false;
    }

    const DWORD nowTick = GetTickCount();
    const DWORD ageMs = nowTick - static_cast<DWORD>(recentTick);
    if (ageMs > maxAgeMs ||
        !HasRecentMountedRuntimeSkillIntent(
            kind,
            static_cast<int>(recentMountItemId),
            maxAgeMs))
    {
        return false;
    }

    if (mountItemIdOut)
    {
        *mountItemIdOut = static_cast<int>(recentMountItemId);
    }
    return true;
}

static bool TryResolveRecentMountedDemonJumpIntentRuntimeSkill(
    int runtimeSkillId,
    int *mountItemIdOut,
    int *configuredSkillIdOut,
    DWORD maxAgeMs = 1200)
{
    if (!kEnableMountedDoubleJumpRuntimeHooks || runtimeSkillId <= 0)
    {
        return false;
    }

    int mountItemId = 0;
    if (!TryGetRecentMountedDemonJumpIntentItemId(&mountItemId, maxAgeMs) ||
        mountItemId <= 0)
    {
        return false;
    }

    int currentMountItemId = 0;
    if (TryResolveCurrentUserMountItemIdWithFallback(&currentMountItemId, nullptr) &&
        currentMountItemId > 0 &&
        currentMountItemId != mountItemId)
    {
        return false;
    }

    const int configuredSkillId =
        ResolveMountedRuntimeSkillIdForKind(
            MountedRuntimeSkillKind_DemonJump,
            mountItemId);
    if (configuredSkillId <= 0 ||
        !CanUseMountedRuntimeSkillRuntimeForKind(
            MountedRuntimeSkillKind_DemonJump,
            mountItemId,
            runtimeSkillId))
    {
        return false;
    }

    if (mountItemIdOut)
    {
        *mountItemIdOut = mountItemId;
    }
    if (configuredSkillIdOut)
    {
        *configuredSkillIdOut = configuredSkillId;
    }
    return true;
}

static bool TryResolveRecentMountedDemonJumpIntent(
    int expectedSkillId,
    int *mountItemIdOut,
    DWORD maxAgeMs = 1200)
{
    int configuredSkillId = 0;
    return TryResolveRecentMountedDemonJumpIntentRuntimeSkill(
        expectedSkillId,
        mountItemIdOut,
        &configuredSkillId,
        maxAgeMs);
}

static bool TryResolveRecentMountedDoubleJumpNativeRelease(
    int expectedSkillId,
    int *mountItemIdOut,
    DWORD maxAgeMs = 450)
{
    return TryResolveRecentMountedRuntimeSkillNativeRelease(
        MountedRuntimeSkillKind_DoubleJump,
        expectedSkillId,
        mountItemIdOut,
        maxAgeMs);
}

static bool TryResolveRecentMountedDemonJumpNativeRelease(
    int expectedSkillId,
    int *mountItemIdOut,
    DWORD maxAgeMs = 450)
{
    if (TryResolveRecentMountedRuntimeSkillNativeRelease(
            MountedRuntimeSkillKind_DemonJump,
            expectedSkillId,
            mountItemIdOut,
            maxAgeMs))
    {
        return true;
    }

    return TryResolveRecentMountedDemonJumpIntent(
        expectedSkillId,
        mountItemIdOut,
        maxAgeMs);
}

static bool TryResolveRecentMountedRuntimeSkillNativeReleaseRuntimeSkill(
    MountedRuntimeSkillKind kind,
    int runtimeSkillId,
    int *mountItemIdOut,
    int *configuredSkillIdOut,
    DWORD maxAgeMs = 450)
{
    if (!kEnableMountedDoubleJumpRuntimeHooks)
    {
        return false;
    }

    if (runtimeSkillId <= 0)
    {
        return false;
    }

    const LONG recentSkillId =
        InterlockedCompareExchange(&g_recentMountedRuntimeSkillNativeReleaseSkillId[kind], 0, 0);
    if (recentSkillId <= 0)
    {
        return false;
    }

    const LONG recentMountItemId =
        InterlockedCompareExchange(&g_recentMountedRuntimeSkillNativeReleaseItemId[kind], 0, 0);
    if (recentMountItemId <= 0)
    {
        return false;
    }

    const LONG recentTick =
        InterlockedCompareExchange(&g_recentMountedRuntimeSkillNativeReleaseTick[kind], 0, 0);
    if (recentTick <= 0)
    {
        return false;
    }

    const DWORD nowTick = GetTickCount();
    const DWORD ageMs = nowTick - static_cast<DWORD>(recentTick);
    if (ageMs > maxAgeMs ||
        !HasRecentMountedRuntimeSkillIntent(
            kind,
            static_cast<int>(recentMountItemId),
            maxAgeMs))
    {
        return false;
    }

    if (!CanUseMountedRuntimeSkillRuntimeForKind(
            kind,
            static_cast<int>(recentMountItemId),
            runtimeSkillId))
    {
        return false;
    }

    if (mountItemIdOut)
    {
        *mountItemIdOut = static_cast<int>(recentMountItemId);
    }
    if (configuredSkillIdOut)
    {
        *configuredSkillIdOut = static_cast<int>(recentSkillId);
    }
    return true;
}

static bool TryResolveRecentMountedDoubleJumpNativeReleaseRuntimeSkill(
    int runtimeSkillId,
    int *mountItemIdOut,
    int *mountedDoubleJumpSkillIdOut,
    DWORD maxAgeMs = 450)
{
    return TryResolveRecentMountedRuntimeSkillNativeReleaseRuntimeSkill(
        MountedRuntimeSkillKind_DoubleJump,
        runtimeSkillId,
        mountItemIdOut,
        mountedDoubleJumpSkillIdOut,
        maxAgeMs);
}

static bool TryResolveRecentMountedDemonJumpNativeReleaseRuntimeSkill(
    int runtimeSkillId,
    int *mountItemIdOut,
    int *mountedDemonJumpSkillIdOut,
    DWORD maxAgeMs = 1200)
{
    if (TryResolveRecentMountedRuntimeSkillNativeReleaseRuntimeSkill(
            MountedRuntimeSkillKind_DemonJump,
            runtimeSkillId,
            mountItemIdOut,
            mountedDemonJumpSkillIdOut,
            maxAgeMs))
    {
        return true;
    }

    return TryResolveRecentMountedDemonJumpIntentRuntimeSkill(
        runtimeSkillId,
        mountItemIdOut,
        mountedDemonJumpSkillIdOut,
        maxAgeMs);
}

static bool TryGetRecentExtendedMountContext(int *mountItemIdOut)
{
    const LONG mountItemId = InterlockedCompareExchange(&g_recentExtendedMountContextItemId, 0, 0);
    if (mountItemId <= 0)
    {
        return false;
    }

    const LONG tick = InterlockedCompareExchange(&g_recentExtendedMountContextTick, 0, 0);
    if (tick <= 0)
    {
        return false;
    }

    const DWORD nowTick = GetTickCount();
    const DWORD elapsed = nowTick - static_cast<DWORD>(tick);
    if (elapsed > kMountedSoaringFallbackGraceMs)
    {
        ClearExtendedMountContext();
        return false;
    }

    if (mountItemIdOut)
    {
        *mountItemIdOut = static_cast<int>(mountItemId);
    }
    return true;
}

static bool TryGetRecentMountedRuntimeSkillIntentItemId(
    MountedRuntimeSkillKind kind,
    int *mountItemIdOut,
    DWORD maxAgeMs = 400)
{
    if (!kEnableMountedDoubleJumpRuntimeHooks || !mountItemIdOut)
    {
        return false;
    }

    if (TryGetRecentMountedRuntimeRouteArmMountItemIdForKind(
            kind,
            mountItemIdOut,
            maxAgeMs))
    {
        return true;
    }

    const LONG recentMountItemId =
        InterlockedCompareExchange(&g_recentMountedRuntimeSkillIntentItemId[kind], 0, 0);
    if (recentMountItemId <= 0)
    {
        return false;
    }

    const LONG recentTick =
        InterlockedCompareExchange(&g_recentMountedRuntimeSkillIntentTick[kind], 0, 0);
    if (recentTick <= 0)
    {
        return false;
    }

    const DWORD nowTick = GetTickCount();
    const DWORD allowedAgeMs = NormalizeMountedRuntimeSkillIntentAgeMs(
        kind,
        maxAgeMs,
        400);
    if (nowTick - static_cast<DWORD>(recentTick) > allowedAgeMs)
    {
        return false;
    }

    *mountItemIdOut = static_cast<int>(recentMountItemId);
    return true;
}

static bool TryGetRecentMountedDoubleJumpIntentItemId(int *mountItemIdOut, DWORD maxAgeMs = 400)
{
    return TryGetRecentMountedRuntimeSkillIntentItemId(
        MountedRuntimeSkillKind_DoubleJump,
        mountItemIdOut,
        maxAgeMs);
}

static bool TryGetRecentMountedDemonJumpIntentItemId(int *mountItemIdOut, DWORD maxAgeMs)
{
    return TryGetRecentMountedRuntimeSkillIntentItemId(
        MountedRuntimeSkillKind_DemonJump,
        mountItemIdOut,
        maxAgeMs);
}

static bool TryGetRecentMountedRuntimeSkillNativeReleaseItemId(
    MountedRuntimeSkillKind kind,
    int *mountItemIdOut,
    DWORD maxAgeMs = 450)
{
    if (!kEnableMountedDoubleJumpRuntimeHooks || !mountItemIdOut)
    {
        return false;
    }

    const LONG recentMountItemId =
        InterlockedCompareExchange(&g_recentMountedRuntimeSkillNativeReleaseItemId[kind], 0, 0);
    if (recentMountItemId <= 0)
    {
        return false;
    }

    const LONG recentTick =
        InterlockedCompareExchange(&g_recentMountedRuntimeSkillNativeReleaseTick[kind], 0, 0);
    if (recentTick <= 0)
    {
        return false;
    }

    const DWORD nowTick = GetTickCount();
    const DWORD allowedAgeMs = maxAgeMs > 0 ? maxAgeMs : 450;
    if (nowTick - static_cast<DWORD>(recentTick) > allowedAgeMs)
    {
        return false;
    }

    *mountItemIdOut = static_cast<int>(recentMountItemId);
    return true;
}

static bool TryGetRecentMountedDoubleJumpNativeReleaseItemId(
    int *mountItemIdOut,
    DWORD maxAgeMs = 450)
{
    return TryGetRecentMountedRuntimeSkillNativeReleaseItemId(
        MountedRuntimeSkillKind_DoubleJump,
        mountItemIdOut,
        maxAgeMs);
}

static bool TryGetRecentMountedDemonJumpNativeReleaseItemId(
    int *mountItemIdOut,
    DWORD maxAgeMs = 450)
{
    return TryGetRecentMountedRuntimeSkillNativeReleaseItemId(
        MountedRuntimeSkillKind_DemonJump,
        mountItemIdOut,
        maxAgeMs);
}
