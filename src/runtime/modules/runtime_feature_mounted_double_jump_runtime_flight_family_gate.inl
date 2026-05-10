static BOOL CallNativeMountContextIsFlyingFamily7D4CD0(void *mountContext)
{
    typedef BOOL(__thiscall *tMountContextFamilyBaseGateFn)(void *mountContext);
    typedef void(__thiscall *tMountContextSharedRefFn)(void *sharedRefPtr);

    if (!mountContext || SafeIsBadReadPtr(mountContext, sizeof(DWORD)))
    {
        return FALSE;
    }

    tMountContextFamilyBaseGateFn baseGateFn =
        reinterpret_cast<tMountContextFamilyBaseGateFn>(ADDR_7D4C00);
    tMountContextSharedRefFn sharedRefFn =
        reinterpret_cast<tMountContextSharedRefFn>(ADDR_4010B0);
    if (!baseGateFn || !sharedRefFn)
    {
        return FALSE;
    }

    __try
    {
        if (!baseGateFn(mountContext))
        {
            return FALSE;
        }

        const uintptr_t mountInfoAddr =
            *reinterpret_cast<uintptr_t *>(reinterpret_cast<uintptr_t>(mountContext) + 0x1BF0);
        const uintptr_t kSentinelMountInfoAddr =
            static_cast<uintptr_t>(static_cast<intptr_t>(-24));
        if (!mountInfoAddr || mountInfoAddr == kSentinelMountInfoAddr)
        {
            return FALSE;
        }

        int *mountInfo = reinterpret_cast<int *>(mountInfoAddr);
        if (SafeIsBadReadPtr(mountInfo, 0x20))
        {
            return FALSE;
        }

        sharedRefFn(reinterpret_cast<void *>(mountInfoAddr + 0x18));
        if (mountInfo[7]-- == 1)
        {
            mountInfo[6] = 0;
        }

        return (mountInfo[3] / 10000) == 199 ? TRUE : FALSE;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        static LONG s_mountFamilyNativeMirrorExceptionLogBudget = 8;
        const LONG budgetAfterDecrement =
            InterlockedDecrement(&s_mountFamilyNativeMirrorExceptionLogBudget);
        if (budgetAfterDecrement >= 0)
        {
            WriteLogFmt("[MountFamily] 7D4CD0 mirror exception code=0x%08X",
                        GetExceptionCode());
        }
        return FALSE;
    }
}

static int __fastcall hkMountSoaringGate7DC1B0(
    void *thisPtr,
    void * /*edxUnused*/,
    int levelContext,
    void *mountContext,
    int skillId,
    unsigned int **skillEntryOut)
{
    int result = oMountSoaringGate7DC1B0
                     ? oMountSoaringGate7DC1B0(thisPtr, levelContext, mountContext, skillId, skillEntryOut)
                     : 0;

    if (result > 0 || skillId != 80001089 || !mountContext)
    {
        return result;
    }

    int mountItemId = 0;
    if (!TryResolveMountItemIdFromContextPointer(mountContext, &mountItemId))
    {
        if (!TryResolveExtendedMountContextForSoaring(&mountItemId, nullptr))
        {
            return result;
        }
    }

    ObserveExtendedMountContext(mountItemId);
    // Mounted double-jump must keep suppressing the soaring extension to avoid
    // being rerouted into the flight/glide branch. Mounted demon jump is the
    // opposite: it still needs the native soaring chain for takeoff/glide.
    if (HasRecentMountedDoubleJumpIntent(mountItemId))
    {
        return result;
    }
    const int mappedNativeFlightSkillId = ResolveExtendedMountNativeFlightSkillId(mountItemId);
    if (mappedNativeFlightSkillId <= 0)
    {
        return result;
    }

    if (IsExtendedMountFamilyGateMount(mountItemId))
    {
        if (skillEntryOut && !*skillEntryOut)
        {
            const uintptr_t soaringSkillEntry = SkillOverlayBridgeLookupSkillEntryPointer(80001089);
            if (soaringSkillEntry)
            {
                *skillEntryOut = reinterpret_cast<unsigned int *>(soaringSkillEntry);
            }
        }

        static LONG s_mountSoaringGatePostNativeForceLogBudget = 24;
        const LONG budgetAfterDecrement = InterlockedDecrement(&s_mountSoaringGatePostNativeForceLogBudget);
        if (budgetAfterDecrement >= 0)
        {
            WriteLogFmt("[MountSoaringGate] 7DC1B0 post-native force mount=%d native=%d soaring=%d donor=%d -> allow",
                        mountItemId,
                        result,
                        skillId,
                        mappedNativeFlightSkillId);
        }

        SkillOverlayBridgeObserveExtendedMountSoaringIntent(mountItemId, skillId);
        return 1;
    }

    if (SafeIsBadReadPtr(mountContext, sizeof(DWORD)))
    {
        return result;
    }

    BOOL isFlyingFamily = FALSE;
    __try
    {
        isFlyingFamily = CallNativeMountContextIsFlyingFamily7D4CD0(mountContext);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        static LONG s_mountSoaringGateFamilyExceptionLogBudget = 8;
        const LONG budgetAfterDecrement = InterlockedDecrement(&s_mountSoaringGateFamilyExceptionLogBudget);
        if (budgetAfterDecrement >= 0)
        {
            WriteLogFmt("[MountSoaringGate] 7DC1B0 family check exception code=0x%08X",
                        GetExceptionCode());
        }
        isFlyingFamily = FALSE;
    }
    if (!isFlyingFamily)
    {
        return result;
    }

    if (skillEntryOut && !*skillEntryOut)
    {
        const uintptr_t soaringSkillEntry = SkillOverlayBridgeLookupSkillEntryPointer(80001089);
        if (soaringSkillEntry)
        {
            *skillEntryOut = reinterpret_cast<unsigned int *>(soaringSkillEntry);
        }
    }

    static LONG s_mountSoaringGateLogBudget = 16;
    const LONG budgetAfterDecrement = InterlockedDecrement(&s_mountSoaringGateLogBudget);
    if (budgetAfterDecrement >= 0)
    {
        WriteLogFmt("[MountSoaringGate] 7DC1B0 extend mount=%d soaring=%d donor=%d -> allow",
                    mountItemId,
                    skillId,
                    mappedNativeFlightSkillId);
    }

    SkillOverlayBridgeObserveExtendedMountSoaringIntent(mountItemId, skillId);
    return 1;
}

static BOOL __fastcall hkMountContextIsFlyingFamily7D4CD0(void *thisPtr, void * /*edxUnused*/)
{
    int mountItemId = 0;
    bool fromFallbackContext = false;
    if (!TryResolveMountItemIdFromContextPointer(thisPtr, &mountItemId) &&
        TryResolveExtendedMountContextForSoaring(&mountItemId, nullptr))
    {
        fromFallbackContext = true;
    }

    if (mountItemId > 0)
    {
        ObserveExtendedMountContext(mountItemId);
    }

    const bool suppressExtendedSoaringForDoubleJump =
        HasRecentMountedDoubleJumpIntent(mountItemId);

    if (!suppressExtendedSoaringForDoubleJump &&
        IsExtendedMountFamilyGateMount(mountItemId))
    {
        ObserveMountedSoaringFlightActive(mountItemId);
        static LONG s_mountFamilyPreBypassLogBudget = 24;
        const LONG budgetAfterDecrement = InterlockedDecrement(&s_mountFamilyPreBypassLogBudget);
        if (budgetAfterDecrement >= 0)
        {
            WriteLogFmt("[MountFamily] 7D4CD0 pre-bypass mount=%d source=%s -> allow",
                        mountItemId,
                        fromFallbackContext ? "fallback" : "context");
        }
        return TRUE;
    }

    BOOL result = FALSE;
    const bool contextReadable = thisPtr && !SafeIsBadReadPtr(thisPtr, sizeof(DWORD));
    if (contextReadable)
    {
        __try
        {
            result = CallNativeMountContextIsFlyingFamily7D4CD0(thisPtr);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            static LONG s_mountFamilyExceptionLogBudget = 8;
            const LONG budgetAfterDecrement = InterlockedDecrement(&s_mountFamilyExceptionLogBudget);
            if (budgetAfterDecrement >= 0)
            {
                WriteLogFmt("[MountFamily] 7D4CD0 native exception code=0x%08X", GetExceptionCode());
            }
            result = FALSE;
        }
    }

    if (result)
    {
        ObserveMountedSoaringFlightActive(mountItemId);
        return TRUE;
    }

    if (suppressExtendedSoaringForDoubleJump)
    {
        return result;
    }

    if (!IsExtendedMountSoaringContextMount(mountItemId))
    {
        return result;
    }

    static LONG s_mountFamilyForceAllowLogBudget = 16;
    const LONG budgetAfterDecrement = InterlockedDecrement(&s_mountFamilyForceAllowLogBudget);
    if (budgetAfterDecrement >= 0)
    {
        WriteLogFmt("[MountFamily] 7D4CD0 extend mount=%d native=%d source=%s -> allow",
                    mountItemId,
                    result ? 1 : 0,
                    fromFallbackContext ? "fallback" : "context");
    }
    ObserveMountedSoaringFlightActive(mountItemId);
    return TRUE;
}

static int __fastcall hkMountFamilyGateA9AAA0(void *thisPtr, void * /*edxUnused*/)
{
    int result = 0;
    if (oMountFamilyGateA9AAA0)
    {
        __try
        {
            result = oMountFamilyGateA9AAA0(thisPtr);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            static LONG s_mountFamilyGateExceptionLogBudget = 8;
            const LONG budgetAfterDecrement = InterlockedDecrement(&s_mountFamilyGateExceptionLogBudget);
            if (budgetAfterDecrement >= 0)
            {
                WriteLogFmt("[MountFamilyGate] A9AAA0 native exception code=0x%08X", GetExceptionCode());
            }
            result = 0;
        }
    }

    int mountItemId = 0;
    bool mountFromContext = false;
    bool mountFromFallback = false;

    if (thisPtr)
    {
        const uintptr_t mountItemIdAddr = reinterpret_cast<uintptr_t>(thisPtr) + 0x3C4;
        if (!SafeIsBadReadPtr(reinterpret_cast<void *>(mountItemIdAddr), sizeof(DWORD)))
        {
            __try
            {
                mountItemId = *reinterpret_cast<int *>(mountItemIdAddr);
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                mountItemId = 0;
            }
            mountFromContext = mountItemId > 0;
        }
    }

    if (mountItemId <= 0)
    {
        if (TryResolveExtendedMountContextForSoaring(&mountItemId, nullptr))
        {
            mountFromFallback = mountItemId > 0;
        }
    }

    if (mountItemId > 0)
    {
        ObserveExtendedMountContext(mountItemId);
    }

    if (result > 0)
    {
        return result;
    }

    if (HasRecentMountedDoubleJumpIntent(mountItemId))
    {
        return result;
    }

    if (!IsExtendedMountFamilyGateMount(mountItemId))
    {
        return result;
    }

    static LONG s_mountFamilyGateLogBudget = 16;
    const LONG budgetAfterDecrement = InterlockedDecrement(&s_mountFamilyGateLogBudget);
    if (budgetAfterDecrement >= 0)
    {
        WriteLogFmt("[MountFamilyGate] A9AAA0 extend mount=%d native=%d source=%s -> allow",
                    mountItemId,
                    result,
                    mountFromContext ? "context" : (mountFromFallback ? "fallback" : "unknown"));
    }
    return 1;
}
