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
    if (!kEnableMountedDemonJumpRuntimeHooks ||
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

