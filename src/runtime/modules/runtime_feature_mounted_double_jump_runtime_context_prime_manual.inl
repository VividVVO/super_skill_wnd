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

