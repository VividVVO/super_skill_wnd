static void __fastcall hkMountedDemonJumpMoveB1DB10(
    void *thisPtr,
    void * /*edxUnused*/,
    int a2,
    int a3)
{
    MountedDemonJumpLateLocalStateSnapshot beforeLocalState = {};
    const bool hasBeforeLocalState =
        thisPtr &&
        TryReadMountedDemonJumpLateLocalStateSnapshot(
            thisPtr,
            &beforeLocalState);
    int beforeBranchState = 0;
    int beforeHorizontalTarget = 0;
    int beforeVerticalTarget = 0;
    const bool hasBeforeBranchState =
        thisPtr &&
        TryReadMountedFlightBranchState(thisPtr, &beforeBranchState);
    const bool hasBeforeTargets =
        hasBeforeBranchState &&
        TryReadMountedFlightControlTargets(
            thisPtr,
            &beforeHorizontalTarget,
            &beforeVerticalTarget);
    int mountItemId = 0;
    int rootSkillId = 0;
    int currentSkillId = 0;
    bool hasRecentIntent = false;
    const bool shouldLog = ShouldObserveMountedDemonJumpLatePath(
        &mountItemId,
        &rootSkillId,
        &currentSkillId,
        &hasRecentIntent);
    if (shouldLog)
    {
        static LONG s_mountedDemonJumpMoveB1DB10LogBudget = 64;
        if (InterlockedDecrement(&s_mountedDemonJumpMoveB1DB10LogBudget) >= 0)
        {
            WriteLogFmt(
                "[MountDemonJumpLate] B1DB10 enter this=0x%08X a2=%d a3=0x%08X mount=%d recent=%d root=%d current=%d branch=%d h=%d v=%d",
                (DWORD)(uintptr_t)thisPtr,
                a2,
                a3,
                mountItemId,
                hasRecentIntent ? 1 : 0,
                rootSkillId,
                currentSkillId,
                hasBeforeBranchState ? beforeBranchState : -1,
                hasBeforeTargets ? beforeHorizontalTarget : -1,
                hasBeforeTargets ? beforeVerticalTarget : -1);
            if (hasBeforeLocalState && beforeLocalState.valid)
            {
                WriteMountedDemonJumpLateLocalStateLog(
                    "B1DB10",
                    "enter",
                    thisPtr,
                    beforeLocalState);
            }
        }
    }

    if (oMountedDemonJumpMoveB1DB10)
    {
        oMountedDemonJumpMoveB1DB10(thisPtr, a2, a3);
    }

    if (shouldLog)
    {
        int afterBranchState = 0;
        int afterHorizontalTarget = 0;
        int afterVerticalTarget = 0;
        MountedDemonJumpLateLocalStateSnapshot afterLocalState = {};
        const bool hasAfterBranchState =
            thisPtr &&
            TryReadMountedFlightBranchState(thisPtr, &afterBranchState);
        const bool hasAfterTargets =
            hasAfterBranchState &&
            TryReadMountedFlightControlTargets(
                thisPtr,
                &afterHorizontalTarget,
                &afterVerticalTarget);
        const bool hasAfterLocalState =
            thisPtr &&
            TryReadMountedDemonJumpLateLocalStateSnapshot(
                thisPtr,
                &afterLocalState);
        static LONG s_mountedDemonJumpMoveB1DB10LeaveLogBudget = 64;
        if (InterlockedDecrement(&s_mountedDemonJumpMoveB1DB10LeaveLogBudget) >= 0)
        {
            WriteLogFmt(
                "[MountDemonJumpLate] B1DB10 leave this=0x%08X a2=%d a3=0x%08X mount=%d recent=%d root=%d current=%d branch=%d->%d h=%d->%d v=%d->%d",
                (DWORD)(uintptr_t)thisPtr,
                a2,
                a3,
                mountItemId,
                hasRecentIntent ? 1 : 0,
                rootSkillId,
                currentSkillId,
                hasBeforeBranchState ? beforeBranchState : -1,
                hasAfterBranchState ? afterBranchState : -1,
                hasBeforeTargets ? beforeHorizontalTarget : -1,
                hasAfterTargets ? afterHorizontalTarget : -1,
                hasBeforeTargets ? beforeVerticalTarget : -1,
                hasAfterTargets ? afterVerticalTarget : -1);
            if (hasAfterLocalState && afterLocalState.valid)
            {
                WriteMountedDemonJumpLateLocalStateLog(
                    "B1DB10",
                    "leave",
                    thisPtr,
                    afterLocalState);
            }
        }

        if (hasAfterLocalState &&
            afterLocalState.valid &&
            (afterLocalState.state5D7C != 0 ||
             afterLocalState.state5F2C != 0))
        {
            TryClearMountedDemonJumpLateLocalLockState(
                thisPtr,
                mountItemId,
                rootSkillId,
                currentSkillId,
                "B1DB10-leave",
                (DWORD)(uintptr_t)_ReturnAddress());
        }

        int liveRootSkillId = rootSkillId;
        int liveCurrentSkillId = currentSkillId;
        if (TryReadMountedDemonJumpContextState(
                &liveRootSkillId,
                &liveCurrentSkillId,
                nullptr))
        {
            // The descending cleanup must track the live native context only.
            // Using the effective context here lets the short recent-child cache
            // masquerade as an already-active 30010183 child, which can clear
            // the first mounted up-jump tail before the native chain finishes
            // consuming it.
        }

        TryForceClearMountedDemonJumpDescendingUpState(
            thisPtr,
            mountItemId,
            liveRootSkillId,
            liveCurrentSkillId,
            "B1DB10-descending-up",
            (DWORD)(uintptr_t)_ReturnAddress());
        if (mountItemId > 0 &&
            (liveCurrentSkillId == 30010184 ||
             liveCurrentSkillId == 30010186))
        {
            TryForceFinalizeMountedDemonJumpUpStateAfterPacket(
                thisPtr,
                mountItemId,
                liveCurrentSkillId,
                liveCurrentSkillId == 30010184
                    ? "B1DB10-side-late-finalize"
                    : "B1DB10-glide-late-finalize",
                (DWORD)(uintptr_t)_ReturnAddress());
        }

    }
}

static void __fastcall hkMountedDemonJumpMoveB1C9E0(
    void *thisPtr,
    void * /*edxUnused*/,
    int a2,
    int a3)
{
    MountedDemonJumpLateLocalStateSnapshot beforeLocalState = {};
    const bool hasBeforeLocalState =
        thisPtr &&
        TryReadMountedDemonJumpLateLocalStateSnapshot(
            thisPtr,
            &beforeLocalState);
    int beforeBranchState = 0;
    int beforeHorizontalTarget = 0;
    int beforeVerticalTarget = 0;
    const bool hasBeforeBranchState =
        thisPtr &&
        TryReadMountedFlightBranchState(thisPtr, &beforeBranchState);
    const bool hasBeforeTargets =
        hasBeforeBranchState &&
        TryReadMountedFlightControlTargets(
            thisPtr,
            &beforeHorizontalTarget,
            &beforeVerticalTarget);
    int mountItemId = 0;
    int rootSkillId = 0;
    int currentSkillId = 0;
    bool hasRecentIntent = false;
    const bool shouldLog = ShouldObserveMountedDemonJumpLatePath(
        &mountItemId,
        &rootSkillId,
        &currentSkillId,
        &hasRecentIntent);
    if (shouldLog)
    {
        static LONG s_mountedDemonJumpMoveB1C9E0LogBudget = 64;
        if (InterlockedDecrement(&s_mountedDemonJumpMoveB1C9E0LogBudget) >= 0)
        {
            WriteLogFmt(
                "[MountDemonJumpLate] B1C9E0 enter this=0x%08X a2=%d a3=0x%08X mount=%d recent=%d root=%d current=%d branch=%d h=%d v=%d",
                (DWORD)(uintptr_t)thisPtr,
                a2,
                a3,
                mountItemId,
                hasRecentIntent ? 1 : 0,
                rootSkillId,
                currentSkillId,
                hasBeforeBranchState ? beforeBranchState : -1,
                hasBeforeTargets ? beforeHorizontalTarget : -1,
                hasBeforeTargets ? beforeVerticalTarget : -1);
            if (hasBeforeLocalState && beforeLocalState.valid)
            {
                WriteMountedDemonJumpLateLocalStateLog(
                    "B1C9E0",
                    "enter",
                    thisPtr,
                    beforeLocalState);
            }
        }
    }

    if (oMountedDemonJumpMoveB1C9E0)
    {
        oMountedDemonJumpMoveB1C9E0(thisPtr, a2, a3);
    }

    if (shouldLog)
    {
        int afterBranchState = 0;
        int afterHorizontalTarget = 0;
        int afterVerticalTarget = 0;
        MountedDemonJumpLateLocalStateSnapshot afterLocalState = {};
        const bool hasAfterBranchState =
            thisPtr &&
            TryReadMountedFlightBranchState(thisPtr, &afterBranchState);
        const bool hasAfterTargets =
            hasAfterBranchState &&
            TryReadMountedFlightControlTargets(
                thisPtr,
                &afterHorizontalTarget,
                &afterVerticalTarget);
        const bool hasAfterLocalState =
            thisPtr &&
            TryReadMountedDemonJumpLateLocalStateSnapshot(
                thisPtr,
                &afterLocalState);
        static LONG s_mountedDemonJumpMoveB1C9E0LeaveLogBudget = 64;
        if (InterlockedDecrement(&s_mountedDemonJumpMoveB1C9E0LeaveLogBudget) >= 0)
        {
            WriteLogFmt(
                "[MountDemonJumpLate] B1C9E0 leave this=0x%08X a2=%d a3=0x%08X mount=%d recent=%d root=%d current=%d branch=%d->%d h=%d->%d v=%d->%d",
                (DWORD)(uintptr_t)thisPtr,
                a2,
                a3,
                mountItemId,
                hasRecentIntent ? 1 : 0,
                rootSkillId,
                currentSkillId,
                hasBeforeBranchState ? beforeBranchState : -1,
                hasAfterBranchState ? afterBranchState : -1,
                hasBeforeTargets ? beforeHorizontalTarget : -1,
                hasAfterTargets ? afterHorizontalTarget : -1,
                hasBeforeTargets ? beforeVerticalTarget : -1,
                hasAfterTargets ? afterVerticalTarget : -1);
            if (hasAfterLocalState && afterLocalState.valid)
            {
                WriteMountedDemonJumpLateLocalStateLog(
                    "B1C9E0",
                    "leave",
                    thisPtr,
                    afterLocalState);
            }
        }

        int liveRootSkillId = rootSkillId;
        int liveCurrentSkillId = currentSkillId;
        if (TryReadMountedDemonJumpContextState(
                &liveRootSkillId,
                &liveCurrentSkillId,
                nullptr) &&
            mountItemId > 0 &&
            (liveCurrentSkillId == 30010184 ||
             liveCurrentSkillId == 30010186))
        {
            TryForceFinalizeMountedDemonJumpUpStateAfterPacket(
                thisPtr,
                mountItemId,
                liveCurrentSkillId,
                liveCurrentSkillId == 30010184
                    ? "B1C9E0-side-late-finalize"
                    : "B1C9E0-glide-late-finalize",
                (DWORD)(uintptr_t)_ReturnAddress());
        }
    }
}

static const char *DescribeMountedDemonJumpUpActionAFB710Caller(
    DWORD callerRet)
{
    switch (callerRet)
    {
    case 0x00B1E02B:
        return "B1DB10";
    case 0x00B1CB35:
        return "B1C9E0";
    default:
        return "detour-or-other";
    }
}

static const char *DescribeMountedDemonJumpAfbInnerCaller(
    DWORD callerRet)
{
    switch (callerRet)
    {
    case 0x00433EAA:
        return "433EAA-mounted-clear-pre";
    case 0x00433FEC:
        return "433FEC-mounted-clear-post";
    case 0x00B2B14E:
        return "B2B14E-post-root-enter";
    case 0x00B28B36:
        return "B28B36-postpacket-check-a";
    case 0x00B28B84:
        return "B28B84-postpacket-check-b";
    case 0x00481E86:
        return "481E86-motion-a";
    case 0x00481E92:
        return "481E92-motion-b";
    case 0x00481EF6:
        return "481EF6-motion-c";
    case 0x00A9E86B:
        return "A9E86B-action-a";
    case 0x00A9C906:
        return "A9C906-action-b";
    case 0x006ADA59:
        return "6ADA59-loop-a";
    case 0x006ADA93:
        return "6ADA93-loop-b";
    case 0x00AFB759:
        return "42E170";
    case 0x00AFB769:
        return "ADB240";
    case 0x00AFB778:
        return "AD9500";
    case 0x00AFB7C2:
        return "773500-long";
    case 0x00AFB802:
        return "773500-short";
    default:
        return nullptr;
    }
}

static int __fastcall hkMountedDemonJumpAfbState42E170(
    void *thisPtr,
    void * /*edxUnused*/)
{
    int result = oMountedDemonJumpAfbState42E170
                     ? oMountedDemonJumpAfbState42E170(thisPtr)
                     : 0;
    const DWORD callerRet = (DWORD)(uintptr_t)_ReturnAddress();
    int mountItemId = 0;
    int rootSkillId = 0;
    int currentSkillId = 0;
    bool hasRecentIntent = false;
    const char *stage = DescribeMountedDemonJumpAfbInnerCaller(callerRet);
    if (ShouldObserveMountedDemonJumpLatePath(
            &mountItemId,
            &rootSkillId,
            &currentSkillId,
            &hasRecentIntent))
    {
        static LONG s_mountedDemonJumpAfbState42E170LogBudget = 96;
        if (InterlockedDecrement(
                &s_mountedDemonJumpAfbState42E170LogBudget) >= 0)
        {
            WriteLogFmt(
                "[MountDemonJumpLateAfb] 42E170 caller=0x%08X stage=%s this=0x%08X result=%d mount=%d recent=%d root=%d current=%d",
                callerRet,
                stage,
                (DWORD)(uintptr_t)thisPtr,
                result,
                mountItemId,
                hasRecentIntent ? 1 : 0,
                rootSkillId,
                currentSkillId);

            if (callerRet == 0x00433EAA ||
                callerRet == 0x00433FEC)
            {
                void *playerObj = nullptr;
                MountedDemonJumpLateLocalStateSnapshot playerState = {};
                if (TryReadMountedDemonJumpPrimePlayerObject(&playerObj) &&
                    playerObj &&
                    TryReadMountedDemonJumpLateLocalStateSnapshot(
                        playerObj,
                        &playerState) &&
                    playerState.valid)
                {
                    WriteMountedDemonJumpLateLocalStateLog(
                        "42E170",
                        stage ? stage : "state",
                        playerObj,
                        playerState);
                }
            }
        }
    }
    return result;
}

static int __fastcall hkMountedDemonJumpAfbGateADB240(
    void *thisPtr,
    void * /*edxUnused*/)
{
    const int result = oMountedDemonJumpAfbGateADB240
                           ? oMountedDemonJumpAfbGateADB240(thisPtr)
                           : 0;
    const DWORD callerRet = (DWORD)(uintptr_t)_ReturnAddress();
    int mountItemId = 0;
    int rootSkillId = 0;
    int currentSkillId = 0;
    bool hasRecentIntent = false;
    if (ShouldObserveMountedDemonJumpLatePath(
            &mountItemId,
            &rootSkillId,
            &currentSkillId,
            &hasRecentIntent))
    {
        static LONG s_mountedDemonJumpAfbGateADB240LogBudget = 96;
        if (InterlockedDecrement(
                &s_mountedDemonJumpAfbGateADB240LogBudget) >= 0)
        {
            WriteLogFmt(
                "[MountDemonJumpLateAfb] ADB240 caller=0x%08X stage=%s this=0x%08X result=%d mount=%d recent=%d root=%d current=%d",
                callerRet,
                DescribeMountedDemonJumpAfbInnerCaller(callerRet),
                (DWORD)(uintptr_t)thisPtr,
                result,
                mountItemId,
                hasRecentIntent ? 1 : 0,
                rootSkillId,
                currentSkillId);
        }
    }
    return result;
}

static int __fastcall hkMountedDemonJumpAfbGateAD9500(
    void *thisPtr,
    void * /*edxUnused*/)
{
    const int result = oMountedDemonJumpAfbGateAD9500
                           ? oMountedDemonJumpAfbGateAD9500(thisPtr)
                           : 0;
    const DWORD callerRet = (DWORD)(uintptr_t)_ReturnAddress();
    int mountItemId = 0;
    int rootSkillId = 0;
    int currentSkillId = 0;
    bool hasRecentIntent = false;
    if (ShouldObserveMountedDemonJumpLatePath(
            &mountItemId,
            &rootSkillId,
            &currentSkillId,
            &hasRecentIntent))
    {
        static LONG s_mountedDemonJumpAfbGateAD9500LogBudget = 96;
        if (InterlockedDecrement(
                &s_mountedDemonJumpAfbGateAD9500LogBudget) >= 0)
        {
            WriteLogFmt(
                "[MountDemonJumpLateAfb] AD9500 caller=0x%08X stage=%s this=0x%08X result=%d mount=%d recent=%d root=%d current=%d",
                callerRet,
                DescribeMountedDemonJumpAfbInnerCaller(callerRet),
                (DWORD)(uintptr_t)thisPtr,
                result,
                mountItemId,
                hasRecentIntent ? 1 : 0,
                rootSkillId,
                currentSkillId);
        }
    }
    return result;
}

static void *__fastcall hkMountedDemonJumpAfbLookup773500(
    void *thisPtr,
    void * /*edxUnused*/,
    int a2,
    int a3,
    int a4)
{
    void *result = oMountedDemonJumpAfbLookup773500
                       ? oMountedDemonJumpAfbLookup773500(
                             thisPtr,
                             a2,
                             a3,
                             a4)
                       : nullptr;
    const DWORD callerRet = (DWORD)(uintptr_t)_ReturnAddress();
    const char *stage = DescribeMountedDemonJumpAfbInnerCaller(callerRet);
    int mountItemId = 0;
    int rootSkillId = 0;
    int currentSkillId = 0;
    bool hasRecentIntent = false;
    bool usedFallbackEntry = false;
    bool retriedWideLookup = false;
    void *retryWideResult = nullptr;
    if (ShouldObserveMountedDemonJumpLatePath(
            &mountItemId,
            &rootSkillId,
            &currentSkillId,
            &hasRecentIntent))
    {
        // Do not synthesize a fallback object for AFB710's 773500 lookup.
        // IDA evidence from 00773500_sub_773500.c shows 773500 scans an
        // environment/object list by position rectangle, not a skill-entry
        // table. Feeding it a skill-entry pointer here corrupts AFB710's later
        // object-type/map checks and leaves mounted demon jump stuck in
        // "effect only" state without actual local movement.
        //
        // Latest mounted 30010183 traces consistently show the short lookup
        // path (a4=0x14) returning null, which drops straight into AFB710's
        // glide/effect-only fallback branch. Native AFB710 already has a
        // long-range sibling lookup (a4=0xC8) behind the 5BB0 prime bit. When
        // mounted demon jump misses that prime, retry the exact same native
        // lookup with the long-range radius before giving up.
        if (!result &&
            callerRet == 0x00AFB802 &&
            a4 == 0x14 &&
            mountItemId > 0 &&
            hasRecentIntent &&
            rootSkillId == 30010110 &&
            currentSkillId == 30010183 &&
            oMountedDemonJumpAfbLookup773500)
        {
            retriedWideLookup = true;
            retryWideResult = oMountedDemonJumpAfbLookup773500(
                thisPtr,
                a2,
                a3,
                0xC8);
            if (retryWideResult)
            {
                result = retryWideResult;
            }
        }

        int field4 = 0;
        int field8 = 0;
        int field1C = 0;
        int field24 = 0;
        bool hasFields = false;
        if (result &&
            !SafeIsBadReadPtr(result, 0x28))
        {
            __try
            {
                const uintptr_t resultValue =
                    reinterpret_cast<uintptr_t>(result);
                field4 = *reinterpret_cast<int *>(resultValue + 0x4);
                field8 = *reinterpret_cast<int *>(resultValue + 0x8);
                field1C = *reinterpret_cast<int *>(resultValue + 0x1C);
                field24 = *reinterpret_cast<int *>(resultValue + 0x24);
                hasFields = true;
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                hasFields = false;
            }
        }

        static LONG s_mountedDemonJumpAfbLookup773500LogBudget = 96;
        if (InterlockedDecrement(
                &s_mountedDemonJumpAfbLookup773500LogBudget) >= 0)
        {
            if (retriedWideLookup)
            {
                WriteLogFmt(
                    "[MountDemonJumpLateAfb] 773500 retry-wide caller=0x%08X stage=%s this=0x%08X a2=0x%08X a3=0x%08X a4=0x%08X->0x000000C8 retryResult=0x%08X mount=%d recent=%d root=%d current=%d",
                    callerRet,
                    stage ? stage : "other",
                    (DWORD)(uintptr_t)thisPtr,
                    a2,
                    a3,
                    a4,
                    (DWORD)(uintptr_t)retryWideResult,
                    mountItemId,
                    hasRecentIntent ? 1 : 0,
                    rootSkillId,
                    currentSkillId);
            }
            WriteLogFmt(
                "[MountDemonJumpLateAfb] 773500 caller=0x%08X stage=%s this=0x%08X a2=0x%08X a3=0x%08X a4=0x%08X result=0x%08X fields=%d[%d,%d,0x%08X,0x%08X] mount=%d recent=%d root=%d current=%d fallback=%d",
                callerRet,
                stage ? stage : "other",
                (DWORD)(uintptr_t)thisPtr,
                a2,
                a3,
                a4,
                (DWORD)(uintptr_t)result,
                hasFields ? 1 : 0,
                field4,
                field8,
                field1C,
                field24,
                mountItemId,
                hasRecentIntent ? 1 : 0,
                rootSkillId,
                currentSkillId,
                usedFallbackEntry ? 1 : 0);
        }
    }
    return result;
}

static void __fastcall hkMountedDemonJumpUpActionAFB710(
    void *thisPtr,
    void * /*edxUnused*/)
{
    const DWORD callerRet = (DWORD)(uintptr_t)_ReturnAddress();
    const char *callerStage =
        DescribeMountedDemonJumpUpActionAFB710Caller(callerRet);
    int mountItemId = 0;
    int rootSkillId = 0;
    int currentSkillId = 0;
    bool hasRecentIntent = false;
    const bool shouldLog =
        ShouldObserveMountedDemonJumpLatePath(
            &mountItemId,
            &rootSkillId,
            &currentSkillId,
            &hasRecentIntent);

    MountedDemonJumpLateLocalStateSnapshot beforeLocalState = {};
    const bool hasBeforeLocalState =
        shouldLog &&
        thisPtr &&
        TryReadMountedDemonJumpLateLocalStateSnapshot(
            thisPtr,
            &beforeLocalState);

    if (shouldLog)
    {
        static LONG s_mountedDemonJumpUpActionAFB710LogBudget = 48;
        if (InterlockedDecrement(&s_mountedDemonJumpUpActionAFB710LogBudget) >= 0)
        {
            WriteLogFmt(
                "[MountDemonJumpLate] AFB710 enter caller=0x%08X stage=%s this=0x%08X mount=%d recent=%d root=%d current=%d",
                callerRet,
                callerStage,
                (DWORD)(uintptr_t)thisPtr,
                mountItemId,
                hasRecentIntent ? 1 : 0,
                rootSkillId,
                currentSkillId);
            if (hasBeforeLocalState && beforeLocalState.valid)
            {
                WriteMountedDemonJumpLateLocalStateLog(
                    "AFB710",
                    "enter",
                    thisPtr,
                    beforeLocalState);
            }
        }
    }

    if (shouldLog &&
        thisPtr &&
        rootSkillId == 30010110 &&
        currentSkillId == 30010183)
    {
        static LONG s_mountedDemonJumpUpActionAFB710ObserveLogBudget = 24;
        if (InterlockedDecrement(
                &s_mountedDemonJumpUpActionAFB710ObserveLogBudget) >= 0)
        {
            WriteLogFmt(
                "[MountDemonJumpPrime] AFB710 observe native path this=0x%08X mount=%d recent=%d root=%d current=%d",
                (DWORD)(uintptr_t)thisPtr,
                mountItemId,
                hasRecentIntent ? 1 : 0,
                rootSkillId,
                currentSkillId);
        }
    }

    if (oMountedDemonJumpUpActionAFB710)
    {
        oMountedDemonJumpUpActionAFB710(thisPtr);
    }

    if (!shouldLog &&
        thisPtr &&
        (callerRet == 0x00B1E02B || callerRet == 0x00B1CB35))
    {
        int postMountItemId = 0;
        const bool resolvedPostMount =
            TryResolveMountedDemonJumpMountItemIdWithFallback(
                thisPtr,
                &postMountItemId,
                nullptr,
                1200) ||
            TryResolveMountedDemonJumpMountItemIdWithFallback(
                nullptr,
                &postMountItemId,
                nullptr,
                1200) ||
            TryReadCurrentUserMountItemId(&postMountItemId);
        int postRootSkillId = 0;
        int postCurrentSkillId = 0;
        const bool hasPostContext =
            resolvedPostMount &&
            postMountItemId > 0 &&
            ResolveMountedRuntimeSkillIdForKind(
                MountedRuntimeSkillKind_DemonJump,
                postMountItemId) == 30010110 &&
            TryReadMountedDemonJumpContextState(
                &postRootSkillId,
                &postCurrentSkillId,
                nullptr) &&
            postRootSkillId == 30010110 &&
            IsMountedDemonJumpRuntimeChildSkillId(postCurrentSkillId) &&
            SkillOverlayBridgeCanUseMountedDemonJumpRuntimeSkill(
                postMountItemId,
                postCurrentSkillId);
        if (hasPostContext &&
            !HasRecentMountedDemonJumpIntent(postMountItemId, 250))
        {
            RememberMountedDemonJumpNativeChildSkill(
                postMountItemId,
                postCurrentSkillId,
                "AFB710-post-context");
            ObserveMountedDemonJumpIntent(
                postMountItemId,
                "AFB710-post-context");

            static LONG s_mountedDemonJumpUpActionAFB710PostArmLogBudget = 32;
            if (InterlockedDecrement(
                    &s_mountedDemonJumpUpActionAFB710PostArmLogBudget) >= 0)
            {
                WriteLogFmt(
                    "[MountDemonJumpPrime] AFB710 post-context arm caller=0x%08X stage=%s this=0x%08X mount=%d root=%d current=%d intent=%d",
                    callerRet,
                    callerStage,
                    (DWORD)(uintptr_t)thisPtr,
                    postMountItemId,
                    postRootSkillId,
                    postCurrentSkillId,
                    HasRecentMountedDemonJumpIntent(postMountItemId, 250) ? 1 : 0);
            }
        }
    }

    if (shouldLog)
    {
        MountedDemonJumpLateLocalStateSnapshot afterLocalState = {};
        const bool hasAfterLocalState =
            thisPtr &&
            TryReadMountedDemonJumpLateLocalStateSnapshot(
                thisPtr,
                &afterLocalState);
        static LONG s_mountedDemonJumpUpActionAFB710LeaveLogBudget = 48;
        if (InterlockedDecrement(&s_mountedDemonJumpUpActionAFB710LeaveLogBudget) >= 0)
        {
            WriteLogFmt(
                "[MountDemonJumpLate] AFB710 leave caller=0x%08X stage=%s this=0x%08X mount=%d recent=%d root=%d current=%d",
                callerRet,
                callerStage,
                (DWORD)(uintptr_t)thisPtr,
                mountItemId,
                hasRecentIntent ? 1 : 0,
                rootSkillId,
                currentSkillId);
            if (hasAfterLocalState && afterLocalState.valid)
            {
                WriteMountedDemonJumpLateLocalStateLog(
                    "AFB710",
                    "leave",
                    thisPtr,
                    afterLocalState);
            }
        }
    }
}

static void __stdcall hkMountedDemonJumpPrimeAE8F70(
    void *a1)
{
    const DWORD callerRet = (DWORD)(uintptr_t)_ReturnAddress();
    int mountItemId = 0;
    int rootSkillId = 0;
    int currentSkillId = 0;
    bool hasRecentIntent = false;
    const bool shouldLog =
        ShouldObserveMountedDemonJumpLatePath(
            &mountItemId,
            &rootSkillId,
            &currentSkillId,
            &hasRecentIntent);

    void *beforePlayerObj = nullptr;
    MountedDemonJumpLateLocalStateSnapshot beforeLocalState = {};
    const bool hasBeforeLocalState =
        shouldLog &&
        TryReadMountedDemonJumpPrimePlayerObject(&beforePlayerObj) &&
        beforePlayerObj &&
        TryReadMountedDemonJumpLateLocalStateSnapshot(
            beforePlayerObj,
            &beforeLocalState);

    if (shouldLog)
    {
        static LONG s_mountedDemonJumpPrimeAE8F70LogBudget = 32;
        if (InterlockedDecrement(&s_mountedDemonJumpPrimeAE8F70LogBudget) >= 0)
        {
            WriteLogFmt(
                "[MountDemonJumpPrime] AE8F70 enter caller=0x%08X a1=0x%08X player=0x%08X mount=%d recent=%d root=%d current=%d",
                callerRet,
                (DWORD)(uintptr_t)a1,
                (DWORD)(uintptr_t)beforePlayerObj,
                mountItemId,
                hasRecentIntent ? 1 : 0,
                rootSkillId,
                currentSkillId);
            if (hasBeforeLocalState && beforeLocalState.valid)
            {
                WriteMountedDemonJumpLateLocalStateLog(
                    "AE8F70",
                    "enter",
                    beforePlayerObj,
                    beforeLocalState);
            }
        }
    }

    if (oMountedDemonJumpPrimeAE8F70)
    {
        oMountedDemonJumpPrimeAE8F70(a1);
    }

    if (shouldLog)
    {
        void *afterPlayerObj = nullptr;
        MountedDemonJumpLateLocalStateSnapshot afterLocalState = {};
        const bool hasAfterLocalState =
            TryReadMountedDemonJumpPrimePlayerObject(&afterPlayerObj) &&
            afterPlayerObj &&
            TryReadMountedDemonJumpLateLocalStateSnapshot(
                afterPlayerObj,
                &afterLocalState);
        static LONG s_mountedDemonJumpPrimeAE8F70LeaveLogBudget = 32;
        if (InterlockedDecrement(&s_mountedDemonJumpPrimeAE8F70LeaveLogBudget) >= 0)
        {
            WriteLogFmt(
                "[MountDemonJumpPrime] AE8F70 leave caller=0x%08X a1=0x%08X player=0x%08X mount=%d recent=%d root=%d current=%d",
                callerRet,
                (DWORD)(uintptr_t)a1,
                (DWORD)(uintptr_t)afterPlayerObj,
                mountItemId,
                hasRecentIntent ? 1 : 0,
                rootSkillId,
                currentSkillId);
            if (hasAfterLocalState && afterLocalState.valid)
            {
                WriteMountedDemonJumpLateLocalStateLog(
                    "AE8F70",
                    "leave",
                    afterPlayerObj,
                    afterLocalState);
            }
        }
    }
}

