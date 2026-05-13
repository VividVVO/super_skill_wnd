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

    int currentSkillId = 0;
    int rootSkillId = 0;
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
    if (rootSkillIdOut)
    {
        *rootSkillIdOut = rootSkillId;
    }

    static LONG s_mountedDemonJumpPrimeRefreshLogBudget = 48;
    const LONG budgetAfterDecrement =
        InterlockedDecrement(&s_mountedDemonJumpPrimeRefreshLogBudget);
    if (budgetAfterDecrement >= 0)
    {
        WriteLogFmt(
            "[MountDemonJumpPrimeRefresh] reason=%s mount=%d user=0x%08X root=%d current=%d primed=%d native=%d manual=%d",
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
