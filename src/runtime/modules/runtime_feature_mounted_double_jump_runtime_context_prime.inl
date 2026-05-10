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

