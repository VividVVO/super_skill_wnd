static BOOL ResolveForcedNativeSkillGateAllow(int originalSkillId, int mappedSkillId)
{
    return SkillOverlayBridgeShouldForceNativeGateAllow(originalSkillId) ||
           (mappedSkillId != originalSkillId &&
            SkillOverlayBridgeShouldForceNativeGateAllow(mappedSkillId));
}

static BOOL ResolveRecentMountSoaringSkillGateAllow(
    int originalSkillId,
    int mappedSkillId,
    int *resolvedMountItemIdOut,
    bool *fromUserLocalOut)
{
    if (originalSkillId != 80001089 && mappedSkillId != 80001089)
    {
        return FALSE;
    }

    int mountItemId = 0;
    bool fromUserLocal = false;
    if (!TryResolveExtendedMountContextForSoaring(&mountItemId, &fromUserLocal))
    {
        return FALSE;
    }

    if (HasRecentMountedDoubleJumpIntent(mountItemId))
    {
        return FALSE;
    }

    if (resolvedMountItemIdOut)
    {
        *resolvedMountItemIdOut = mountItemId;
    }
    if (fromUserLocalOut)
    {
        *fromUserLocalOut = fromUserLocal;
    }
    return TRUE;
}

static BOOL ResolveMountedRuntimeSkillGateAllow(
    MountedRuntimeSkillKind kind,
    int originalSkillId,
    int mappedSkillId,
    int *resolvedMountItemIdOut)
{
    if (!IsMountedRuntimeSkillHooksEnabledForKind(kind))
    {
        return FALSE;
    }

    if (originalSkillId <= 0 && mappedSkillId <= 0)
    {
        return FALSE;
    }

    int mountItemId = 0;
    const bool resolvedMount = kind == MountedRuntimeSkillKind_DemonJump
                                   ? TryResolveMountedDemonJumpMountItemIdWithFallback(
                                         nullptr,
                                         &mountItemId,
                                         nullptr,
                                         1200)
                                   : TryResolveMountedDoubleJumpMountItemIdWithFallback(
                                         nullptr,
                                         &mountItemId,
                                         nullptr,
                                         1200);
    if (!resolvedMount)
    {
        return FALSE;
    }

    if (!CanUseMountedRuntimeSkillRuntimeForKind(kind, mountItemId, originalSkillId) &&
        !CanUseMountedRuntimeSkillRuntimeForKind(kind, mountItemId, mappedSkillId))
    {
        return FALSE;
    }

    if (kind == MountedRuntimeSkillKind_DemonJump &&
        (IsMountedDemonJumpRelatedSkillId(originalSkillId) ||
         IsMountedDemonJumpRelatedSkillId(mappedSkillId)))
    {
        const bool hasRecentIntent =
            HasRecentMountedDemonJumpIntent(mountItemId, 250);
        const bool hasRecentTerminalClear =
            HasRecentMountedDemonJumpTerminalClear(
                mountItemId,
                kMountedDemonJumpTerminalClearSuppressMs);
        int probeMountItemId = 0;
        const bool hasFreshProbe =
            TryGetRecentMountedDemonJumpGateProbeMountItemId(
                &probeMountItemId,
                250) &&
            probeMountItemId == mountItemId;
        int rootSkillId = 0;
        int currentSkillId = 0;
        const bool hasContext =
            TryReadMountedDemonJumpContextState(
                &rootSkillId,
                &currentSkillId,
                nullptr);
        const bool contextActive =
            hasContext &&
            (rootSkillId == 30010110 ||
             currentSkillId == 30010110 ||
             IsMountedDemonJumpRuntimeChildSkillId(rootSkillId) ||
             IsMountedDemonJumpRuntimeChildSkillId(currentSkillId));
        const bool isRootFallback =
            originalSkillId == 30010110 ||
            mappedSkillId == 30010110;
        if (isRootFallback &&
            hasRecentTerminalClear &&
            !contextActive &&
            // A new mounted demon-jump attempt should override the previous
            // post-release terminal-clear guard as soon as a fresh intent or
            // fresh gate probe is observed for the same mount.
            !hasRecentIntent &&
            !hasFreshProbe)
        {
            static LONG s_mountedDemonJumpGateAllowTerminalRejectLogBudget = 64;
            if (InterlockedDecrement(
                    &s_mountedDemonJumpGateAllowTerminalRejectLogBudget) >= 0)
            {
                WriteLogFmt(
                    "[MountDemonJumpGate] reject terminal-clear original=%d mapped=%d mount=%d recent=%d probe=%d root=%d current=%d",
                    originalSkillId,
                    mappedSkillId,
                    mountItemId,
                    hasRecentIntent ? 1 : 0,
                    hasFreshProbe ? 1 : 0,
                    hasContext ? rootSkillId : 0,
                    hasContext ? currentSkillId : 0);
            }
            return FALSE;
        }
        if (!hasRecentIntent &&
            !hasFreshProbe &&
            !contextActive)
        {
            static LONG s_mountedDemonJumpGateAllowRejectLogBudget = 64;
            if (InterlockedDecrement(
                    &s_mountedDemonJumpGateAllowRejectLogBudget) >= 0)
            {
                WriteLogFmt(
                    "[MountDemonJumpGate] reject fallback original=%d mapped=%d mount=%d recent=%d probe=%d root=%d current=%d",
                    originalSkillId,
                    mappedSkillId,
                    mountItemId,
                    hasRecentIntent ? 1 : 0,
                    hasFreshProbe ? 1 : 0,
                    hasContext ? rootSkillId : 0,
                    hasContext ? currentSkillId : 0);
            }
            return FALSE;
        }
    }

    if (resolvedMountItemIdOut)
    {
        *resolvedMountItemIdOut = mountItemId;
    }
    return TRUE;
}

static BOOL ResolveMountedConfiguredSkillGateAllow(
    int originalSkillId,
    int mappedSkillId,
    MountedRuntimeSkillKind *kindOut,
    int *resolvedMountItemIdOut)
{
    if (kindOut)
    {
        *kindOut = MountedRuntimeSkillKind_DoubleJump;
    }

    if (ResolveMountedRuntimeSkillGateAllow(
            MountedRuntimeSkillKind_DoubleJump,
            originalSkillId,
            mappedSkillId,
            resolvedMountItemIdOut))
    {
        if (kindOut)
        {
            *kindOut = MountedRuntimeSkillKind_DoubleJump;
        }
        return TRUE;
    }

    if (ResolveMountedRuntimeSkillGateAllow(
            MountedRuntimeSkillKind_DemonJump,
            originalSkillId,
            mappedSkillId,
            resolvedMountItemIdOut))
    {
        if (kindOut)
        {
            *kindOut = MountedRuntimeSkillKind_DemonJump;
        }
        return TRUE;
    }

    return FALSE;
}

