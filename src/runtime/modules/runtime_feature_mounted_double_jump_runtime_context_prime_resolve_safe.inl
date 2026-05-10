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

