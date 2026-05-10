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
