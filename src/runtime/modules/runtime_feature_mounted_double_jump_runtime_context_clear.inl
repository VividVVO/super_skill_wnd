static int __fastcall hkMountedDemonJumpContextClear433380Impl(
    void *contextPtr,
    void * /*edxUnused*/)
{
    if (IsMountedDemonJumpResolveSafePrimeActive())
    {
        static LONG s_mountedDemonJumpContextClearResolveSafeBypassLogBudget = 24;
        if (InterlockedDecrement(
                &s_mountedDemonJumpContextClearResolveSafeBypassLogBudget) >= 0)
        {
            WriteLogFmt(
                "[MountDemonJumpContext] 433380 resolve-safe bypass context=0x%08X caller=0x%08X",
                (DWORD)(uintptr_t)contextPtr,
                (DWORD)(uintptr_t)_ReturnAddress());
        }
        return oMountedDemonJumpContextClear433380
                   ? oMountedDemonJumpContextClear433380(contextPtr)
                   : 0;
    }

    const DWORD callerRet = (DWORD)(uintptr_t)_ReturnAddress();
    int passMountItemId = 0;
    int passRootSkillId = 0;
    int passCurrentSkillId = 0;
    int passRecentChildSkillId = 0;
    int passTerminalProbeMountItemId = 0;
    int passTransientClearMountItemId = 0;
    bool shouldClearLateLocalLock = false;
    bool shouldProbeTerminalTransientClear = false;
    int mountItemId = 0;
    int rootSkillId = 0;
    int currentSkillId = 0;
    if (ShouldSuppressMountedDemonJumpMountedContextClear(
            contextPtr,
            callerRet,
            &mountItemId,
            &rootSkillId,
            &currentSkillId))
    {
        static LONG s_mountedDemonJumpContextClearSuppressLogBudget = 48;
        const LONG budgetAfterDecrement =
            InterlockedDecrement(
                &s_mountedDemonJumpContextClearSuppressLogBudget);
        if (budgetAfterDecrement >= 0)
        {
            WriteLogFmt(
                "[MountDemonJumpContext] 433380 suppress clear caller=0x%08X mount=%d root=%d current=%d",
                callerRet,
                mountItemId,
                rootSkillId,
                currentSkillId);
        }
        return 0;
    }

    static LONG s_mountedDemonJumpContextClearPassLogBudget = 64;
    const bool shouldLogPassClear =
        InterlockedDecrement(&s_mountedDemonJumpContextClearPassLogBudget) >= 0;
    const bool shouldLogNativeClearTailState =
        shouldLogPassClear &&
        (callerRet == 0x00433EAA || callerRet == 0x00433FEC);
    struct MountedDemonJumpContextTailState
    {
        void *playerObj;
        DWORD state4A4;
        int state3DDC;
        int state4DE4;
        int state5D7C;
        int state5F2C;
        BYTE gateMode;
        BYTE downLatch;
        BYTE upLatch;
        int slot;
        bool valid;
    };
    MountedDemonJumpContextTailState nativeClearBeforeState = {};
    MountedDemonJumpContextTailState nativeClearAfterState = {};
    bool hasNativeClearBeforeState = false;
    bool hasNativeClearAfterState = false;
    auto ReadMountedDemonJumpContextTailState =
        [](MountedDemonJumpContextTailState *outState) -> bool
    {
        if (!outState)
        {
            return false;
        }

        memset(outState, 0, sizeof(*outState));
        void *playerObj = nullptr;
        if (!TryReadMountedDemonJumpPrimePlayerObject(&playerObj) ||
            !playerObj ||
            SafeIsBadReadPtr(playerObj, 0x5F30))
        {
            return false;
        }

        const uintptr_t playerValue =
            reinterpret_cast<uintptr_t>(playerObj);
        __try
        {
            outState->playerObj = playerObj;
            outState->state4A4 =
                *reinterpret_cast<DWORD *>(playerValue + 0x4A4);
            outState->state3DDC =
                *reinterpret_cast<int *>(playerValue + 0x3DDC);
            outState->state4DE4 =
                *reinterpret_cast<int *>(playerValue + 0x4DE4);
            outState->state5D7C =
                *reinterpret_cast<int *>(playerValue + 0x5D7C);
            outState->state5F2C =
                *reinterpret_cast<int *>(playerValue + 0x5F2C);
            outState->gateMode =
                *reinterpret_cast<BYTE *>(playerValue + 0x5E84);
            outState->downLatch =
                *reinterpret_cast<BYTE *>(playerValue + 0x5E85);
            outState->upLatch =
                *reinterpret_cast<BYTE *>(playerValue + 0x5EE4);
            outState->slot =
                *reinterpret_cast<int *>(playerValue + 0x5E8C);
            outState->valid = true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            memset(outState, 0, sizeof(*outState));
            outState->valid = false;
            return false;
        }

        return true;
    };
    {
        void *userLocal = nullptr;
        const bool hasUserLocal =
            TryReadCurrentUserLocalPtr(&userLocal) && userLocal;
        const uintptr_t expectedContext =
            hasUserLocal
                ? reinterpret_cast<uintptr_t>(userLocal) +
                      kMountedDemonJumpContextOffset
                : 0;
        const bool contextMatches =
            expectedContext != 0 &&
            reinterpret_cast<uintptr_t>(contextPtr) == expectedContext;

        int liveMountItemId = 0;
        const bool hasMountItem =
            TryReadCurrentUserMountItemId(&liveMountItemId) &&
            liveMountItemId > 0;
        const int configuredSkillId =
            hasMountItem
                ? ResolveMountedRuntimeSkillIdForKind(
                      MountedRuntimeSkillKind_DemonJump,
                      liveMountItemId)
                : 0;

        int primedCurrentSkillId = 0;
        int primedRootSkillId = 0;
        const bool hasPrimedContext =
            hasMountItem &&
            HasMountedDemonJumpContextPrimedForMount(
                liveMountItemId,
                &primedCurrentSkillId,
                &primedRootSkillId);
        const bool hasRecentIntent =
            hasMountItem &&
            HasRecentMountedDemonJumpIntent(
                liveMountItemId,
                kMountedDemonJumpContextClearProtectMs);
        int freshProbeMountItemId = 0;
        const bool hasFreshProbe =
            hasMountItem &&
            TryGetRecentMountedDemonJumpGateProbeMountItemId(
                &freshProbeMountItemId,
                kMountedDemonJumpContextClearProtectMs) &&
            freshProbeMountItemId == liveMountItemId;
        int freshRecentChildSkillId = 0;
        const bool hasFreshRecentUpChild =
            hasMountItem &&
            TryGetRecentMountedDemonJumpNativeChildSkill(
                liveMountItemId,
                &freshRecentChildSkillId,
                nullptr,
                kMountedDemonJumpLateChildPrimeWindowMs) &&
            freshRecentChildSkillId == 30010183;

        const bool shouldSuppressInModuleResolveSafePassClear =
            IsAddressInCurrentModule(callerRet) &&
            contextMatches &&
            configuredSkillId == 30010110 &&
            liveMountItemId > 0 &&
            hasRecentIntent &&
            hasFreshProbe &&
            hasPrimedContext &&
            primedRootSkillId == 30010110 &&
            (primedCurrentSkillId == 30010183 ||
             (primedCurrentSkillId == 30010110 &&
              hasFreshRecentUpChild));

        if (shouldSuppressInModuleResolveSafePassClear)
        {
            const int suppressCurrentSkillId =
                primedCurrentSkillId == 30010110 &&
                        hasFreshRecentUpChild
                    ? 30010183
                    : primedCurrentSkillId;
            static LONG s_mountedDemonJumpContextClearInModuleSuppressLogBudget =
                32;
            if (InterlockedDecrement(
                    &s_mountedDemonJumpContextClearInModuleSuppressLogBudget) >=
                0)
            {
                WriteLogFmt(
                    "[MountDemonJumpContext] 433380 in-module suppress caller=0x%08X context=0x%08X mount=%d recent=%d probe=%d recentChild=%d root=%d current=%d",
                    callerRet,
                    (DWORD)(uintptr_t)contextPtr,
                    liveMountItemId,
                    hasRecentIntent ? 1 : 0,
                    hasFreshProbe ? 1 : 0,
                    hasFreshRecentUpChild ? freshRecentChildSkillId : 0,
                    primedRootSkillId,
                    suppressCurrentSkillId);
            }

            void *playerObj = nullptr;
            if (TryReadMountedDemonJumpPrimePlayerObject(&playerObj) &&
                playerObj)
            {
                TryClearMountedDemonJumpLateLocalLockState(
                    playerObj,
                    liveMountItemId,
                    primedRootSkillId,
                    suppressCurrentSkillId,
                    "433380-inmodule-suppress",
                    callerRet);
            }
            return 0;
        }

        if (configuredSkillId == 30010110 &&
            liveMountItemId > 0 &&
            !hasPrimedContext)
        {
            passTerminalProbeMountItemId = liveMountItemId;
            shouldProbeTerminalTransientClear = true;
        }

        if (configuredSkillId == 30010110 &&
            liveMountItemId > 0 &&
            !hasRecentIntent &&
            !hasPrimedContext)
        {
            passTransientClearMountItemId = liveMountItemId;
        }

        if (shouldLogPassClear &&
            (configuredSkillId == 30010110 || hasPrimedContext || contextMatches))
        {
            WriteLogFmt(
                "[MountDemonJumpContext] 433380 pass clear caller=0x%08X context=0x%08X expected=0x%08X match=%d mount=%d configured=%d recent=%d primed=%d root=%d current=%d",
                callerRet,
                (DWORD)(uintptr_t)contextPtr,
                (DWORD)expectedContext,
                contextMatches ? 1 : 0,
                liveMountItemId,
                configuredSkillId,
                hasRecentIntent ? 1 : 0,
                hasPrimedContext ? 1 : 0,
                primedRootSkillId,
                primedCurrentSkillId);
        }

        if (configuredSkillId == 30010110 &&
            liveMountItemId > 0 &&
            (primedRootSkillId == 30010110 ||
             primedCurrentSkillId == 30010110 ||
             IsMountedDemonJumpRuntimeChildSkillId(primedCurrentSkillId)))
        {
            passMountItemId = liveMountItemId;
            passRootSkillId =
                primedRootSkillId > 0 ? primedRootSkillId : 30010110;
            passCurrentSkillId = primedCurrentSkillId;
            shouldClearLateLocalLock = true;
        }
        else if (configuredSkillId == 30010110 &&
                 liveMountItemId > 0 &&
                 !hasPrimedContext &&
                 TryGetRecentMountedDemonJumpNativeChildSkill(
                     liveMountItemId,
                     &passRecentChildSkillId,
                     nullptr,
                     1500) &&
                 IsMountedDemonJumpRuntimeChildSkillId(passRecentChildSkillId))
        {
            passMountItemId = liveMountItemId;
            passRootSkillId = 30010110;
            passCurrentSkillId = passRecentChildSkillId;
            shouldClearLateLocalLock = true;
        }
    }

    if (shouldLogNativeClearTailState)
    {
        hasNativeClearBeforeState =
            ReadMountedDemonJumpContextTailState(
                &nativeClearBeforeState) &&
            nativeClearBeforeState.valid;
    }

    const int result = oMountedDemonJumpContextClear433380
                           ? oMountedDemonJumpContextClear433380(contextPtr)
                           : 0;

    if (shouldLogNativeClearTailState)
    {
        hasNativeClearAfterState =
            ReadMountedDemonJumpContextTailState(
                &nativeClearAfterState) &&
            nativeClearAfterState.valid;

        if (hasNativeClearBeforeState)
        {
            WriteLogFmt(
                "[MountDemonJumpContextTail] %s this=0x%08X 4A4=0x%08X 3DDC=%d 4DE4=%d 5D7C=%d 5F2C=%d gate=%u/%u/%u slot=0x%08X",
                callerRet == 0x00433FEC ? "433FEC-before" : "433EAA-before",
                (DWORD)(uintptr_t)nativeClearBeforeState.playerObj,
                nativeClearBeforeState.state4A4,
                nativeClearBeforeState.state3DDC,
                nativeClearBeforeState.state4DE4,
                nativeClearBeforeState.state5D7C,
                nativeClearBeforeState.state5F2C,
                static_cast<unsigned int>(
                    nativeClearBeforeState.gateMode),
                static_cast<unsigned int>(
                    nativeClearBeforeState.downLatch),
                static_cast<unsigned int>(
                    nativeClearBeforeState.upLatch),
                nativeClearBeforeState.slot);
        }
        if (hasNativeClearAfterState)
        {
            WriteLogFmt(
                "[MountDemonJumpContextTail] %s this=0x%08X 4A4=0x%08X 3DDC=%d 4DE4=%d 5D7C=%d 5F2C=%d gate=%u/%u/%u slot=0x%08X",
                callerRet == 0x00433FEC ? "433FEC-after" : "433EAA-after",
                (DWORD)(uintptr_t)nativeClearAfterState.playerObj,
                nativeClearAfterState.state4A4,
                nativeClearAfterState.state3DDC,
                nativeClearAfterState.state4DE4,
                nativeClearAfterState.state5D7C,
                nativeClearAfterState.state5F2C,
                static_cast<unsigned int>(
                    nativeClearAfterState.gateMode),
                static_cast<unsigned int>(
                    nativeClearAfterState.downLatch),
                static_cast<unsigned int>(
                    nativeClearAfterState.upLatch),
                nativeClearAfterState.slot);
        }
    }

    if (callerRet == 0x00433FEC)
    {
        int tailMountItemId = 0;
        if (TryReadCurrentUserMountItemId(&tailMountItemId) &&
            tailMountItemId > 0 &&
            ResolveMountedRuntimeSkillIdForKind(
                MountedRuntimeSkillKind_DemonJump,
                tailMountItemId) == 30010110)
        {
            void *playerObj = hasNativeClearAfterState &&
                                      nativeClearAfterState.valid
                                  ? nativeClearAfterState.playerObj
                                  : nullptr;
            if (!playerObj)
            {
                TryReadMountedDemonJumpPrimePlayerObject(&playerObj);
            }
            if (playerObj)
            {
                if (hasNativeClearBeforeState &&
                    hasNativeClearAfterState &&
                    nativeClearBeforeState.valid &&
                    nativeClearAfterState.valid &&
                    nativeClearBeforeState.slot != 0 &&
                    nativeClearAfterState.slot == 0 &&
                    (nativeClearAfterState.gateMode != 0 ||
                     nativeClearAfterState.downLatch != 0 ||
                     nativeClearAfterState.upLatch != 0) &&
                    nativeClearAfterState.state5D7C == 0 &&
                    nativeClearAfterState.state5F2C == 0)
                {
                    TryClearMountedDemonJumpTailLocalGateState(
                        playerObj,
                        tailMountItemId,
                        "433380-tail-local-gate",
                        callerRet);
                }

                TryForceFinalizeMountedDemonJumpNativeActionTailState(
                    playerObj,
                    tailMountItemId,
                    "433380-tail-native-action",
                    callerRet);
                TryClearMountedDemonJump35121005CarrierTailState(
                    playerObj,
                    tailMountItemId,
                    "433380-tail-carrier-tail",
                    callerRet);
            }
        }
    }

    if (shouldClearLateLocalLock)
    {
        void *playerObj = nullptr;
        if (TryReadMountedDemonJumpPrimePlayerObject(&playerObj) &&
            playerObj)
        {
            TryClearMountedDemonJumpLateLocalLockState(
                playerObj,
                passMountItemId,
                passRootSkillId,
                passCurrentSkillId,
                "433380-pass",
                callerRet);
        }
    }

    if (passTransientClearMountItemId > 0)
    {
        ClearMountedDemonJumpTransientRuntimeState(
            passTransientClearMountItemId,
            "433380-pass");
    }
    else if (shouldProbeTerminalTransientClear &&
             passTerminalProbeMountItemId > 0)
    {
        int afterRootSkillId = 0;
        int afterCurrentSkillId = 0;
        const bool hasAfterContext =
            TryReadMountedDemonJumpContextState(
                &afterRootSkillId,
                &afterCurrentSkillId,
                nullptr);
        const bool contextStillActive =
            hasAfterContext &&
            (afterRootSkillId == 30010110 ||
             afterCurrentSkillId == 30010110 ||
             IsMountedDemonJumpRuntimeChildSkillId(afterRootSkillId) ||
             IsMountedDemonJumpRuntimeChildSkillId(afterCurrentSkillId));

        bool lateLocalLockStillActive = false;
        void *playerObj = nullptr;
        if (TryReadMountedDemonJumpPrimePlayerObject(&playerObj) &&
            playerObj)
        {
            const uintptr_t playerValue =
                reinterpret_cast<uintptr_t>(playerObj);
            if (!SafeIsBadReadPtr(
                    reinterpret_cast<void *>(playerValue + 0x5F30),
                    sizeof(DWORD)))
            {
                __try
                {
                    lateLocalLockStillActive =
                        *reinterpret_cast<int *>(playerValue + 0x5D7C) != 0 ||
                        *reinterpret_cast<int *>(playerValue + 0x5F2C) != 0;
                }
                __except (EXCEPTION_EXECUTE_HANDLER)
                {
                    lateLocalLockStillActive = false;
                }
            }
        }

        int freshProbeMountItemId = 0;
        const bool hasFreshTerminalIntent =
            HasRecentMountedDemonJumpIntent(
                passTerminalProbeMountItemId,
                kMountedDemonJumpContextClearProtectMs);
        const bool hasFreshTerminalProbe =
            TryGetRecentMountedDemonJumpGateProbeMountItemId(
                &freshProbeMountItemId,
                kMountedDemonJumpContextClearProtectMs) &&
            freshProbeMountItemId == passTerminalProbeMountItemId;

        const bool shouldClearRecentOnlyTerminalTransient =
            !contextStillActive &&
            !lateLocalLockStillActive &&
            // A fresh mounted demon-jump retrigger can legitimately arrive
            // before the native gate probe re-arms. Do not re-arm the
            // terminal-clear blocker in that short recent-intent window, or
            // the next root 30010110 lookup gets rejected before the chain can
            // seed a new 30010183 child again.
            !hasFreshTerminalIntent &&
            !hasFreshTerminalProbe;

        if (shouldClearRecentOnlyTerminalTransient)
        {
            static LONG s_mountedDemonJumpContextTerminalClearLogBudget = 48;
            if (InterlockedDecrement(
                    &s_mountedDemonJumpContextTerminalClearLogBudget) >= 0)
            {
                WriteLogFmt(
                    "[MountDemonJumpContext] 433380 terminal clear caller=0x%08X mount=%d root=%d current=%d lateLock=%d recent=%d probe=%d",
                    callerRet,
                    passTerminalProbeMountItemId,
                    hasAfterContext ? afterRootSkillId : 0,
                    hasAfterContext ? afterCurrentSkillId : 0,
                    lateLocalLockStillActive ? 1 : 0,
                    hasFreshTerminalIntent ? 1 : 0,
                    hasFreshTerminalProbe ? 1 : 0);
            }

            ObserveMountedDemonJumpTerminalClear(
                passTerminalProbeMountItemId,
                "433380-pass-terminal");
            ClearMountedDemonJumpTransientRuntimeState(
                passTerminalProbeMountItemId,
                "433380-pass-terminal");
        }
        else if ((!contextStillActive && !lateLocalLockStillActive) &&
                 (hasFreshTerminalIntent || hasFreshTerminalProbe))
        {
            static LONG s_mountedDemonJumpContextTerminalSkipLogBudget = 48;
            if (InterlockedDecrement(
                    &s_mountedDemonJumpContextTerminalSkipLogBudget) >= 0)
            {
                WriteLogFmt(
                    "[MountDemonJumpContext] 433380 terminal keep caller=0x%08X mount=%d root=%d current=%d lateLock=%d recent=%d probe=%d",
                    callerRet,
                    passTerminalProbeMountItemId,
                    hasAfterContext ? afterRootSkillId : 0,
                    hasAfterContext ? afterCurrentSkillId : 0,
                    lateLocalLockStillActive ? 1 : 0,
                    hasFreshTerminalIntent ? 1 : 0,
                    hasFreshTerminalProbe ? 1 : 0);
            }
        }
    }

    return result;
}

static int __fastcall hkMountedDemonJumpContextClear433380(
    void *contextPtr,
    void *edxUnused)
{
    InterlockedIncrement(&g_mountedDemonJumpContextClearDepth);
    const int result = hkMountedDemonJumpContextClear433380Impl(
        contextPtr,
        edxUnused);
    InterlockedDecrement(&g_mountedDemonJumpContextClearDepth);
    return result;
}

static bool HasMountedDemonJumpLateSustainIntent(
    int mountItemId,
    int rootSkillId,
    int currentSkillId,
    DWORD maxAgeMs)
{
    if (mountItemId <= 0 ||
        ResolveMountedRuntimeSkillIdForKind(
            MountedRuntimeSkillKind_DemonJump,
            mountItemId) != 30010110)
    {
        return false;
    }

    if (HasRecentMountedDemonJumpIntent(mountItemId, maxAgeMs))
    {
        return true;
    }

    // v23.36 latest mounted-up logs:
    //   575D60/B22630 a2=18 a3=0x00380300 recent=1
    //   575D60/B22630 a2=18 a3=0x80380000 recent=1
    //   575D60/B22630 a2=38 a3=0x01480000 recent=0 while root/current are still
    //   30010110/30010110 and the native up gate is already latched (mode=3,
    //   upLatch=1).
    //
    // IDA-derived helper ResolveMountedDemonJumpLatchedUpChildSkillId already
    // recognizes that exact gate shape as the native transition into child
    // 30010183. Reuse it here, but keep the sustain extremely narrow:
    // - only mounted demon-jump root context
    // - only before current has become a child
    // - only while the same mount still has a fresh gate probe
    if (rootSkillId == 30010110 &&
        currentSkillId == 30010110)
    {
        int gateProbeMountItemId = 0;
        const bool hasRecentMatchingGateProbe =
            TryGetRecentMountedDemonJumpGateProbeMountItemId(
                &gateProbeMountItemId,
                maxAgeMs + 250) &&
            gateProbeMountItemId == mountItemId;
        if (hasRecentMatchingGateProbe &&
            ResolveMountedDemonJumpLatchedUpChildSkillId(
                mountItemId,
                rootSkillId,
                currentSkillId) == 30010183)
        {
            return true;
        }
    }

    // Evidence captured in the knowledge base (v22.53) already narrowed the
    // only safe late-sustain shapes:
    // - side child 30010184 may reuse the fresh child cache briefly;
    // - glide child 30010186 may reuse only a real recent glide packet while
    //   mounted soaring is still actively alive.
    // Do not revive children purely because current still says 30010184/30010186.
    if (rootSkillId != 30010110)
    {
        return false;
    }

    if (currentSkillId == 30010184)
    {
        int recentChildSkillId = 0;
        return TryGetRecentMountedDemonJumpNativeChildSkill(
                   mountItemId,
                   &recentChildSkillId,
                   nullptr,
                   kMountedDemonJumpLateChildCacheMatchMaxAgeMs) &&
               recentChildSkillId == 30010184;
    }

    if (currentSkillId == 30010186)
    {
        DWORD glidePacketAgeMs = 0;
        const bool hasRecentGlidePacket =
            ShouldSuppressMountedDemonJumpRepeatedGlidePacket(
                mountItemId,
                kMountedDemonJumpGlidePacketSustainMaxAgeMs,
                &glidePacketAgeMs);
        DWORD activeAgeMs = 0;
        DWORD activeDurationMs = 0;
        const bool hasActiveSoaringFlight =
            TryGetMountedSoaringFlightTiming(
                mountItemId,
                &activeAgeMs,
                &activeDurationMs);
        return hasRecentGlidePacket && hasActiveSoaringFlight;
    }

    return false;
}

static bool ShouldObserveMountedDemonJumpLatePath(
    int *mountItemIdOut,
    int *rootSkillIdOut,
    int *currentSkillIdOut,
    bool *recentIntentOut)
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
    if (recentIntentOut)
    {
        *recentIntentOut = false;
    }

    int mountItemId = 0;
    const bool hasMount =
        TryResolveCurrentUserMountItemIdWithFallback(&mountItemId, nullptr) &&
        mountItemId > 0;
    int rootSkillId = 0;
    int currentSkillId = 0;
    const bool hasContext = TryReadMountedDemonJumpContextState(
        &rootSkillId,
        &currentSkillId,
        nullptr);
    const bool hasRecentIntent =
        hasMount &&
        HasMountedDemonJumpLateSustainIntent(
            mountItemId,
            rootSkillId,
            currentSkillId,
            kMountedDemonJumpIntentMaxAgeMs);
    const bool demonContextRelevant =
        hasContext &&
        (rootSkillId == 30010110 ||
         currentSkillId == 30010110 ||
         currentSkillId == 30010183 ||
         currentSkillId == 30010184 ||
         currentSkillId == 30010186);
    if (!hasRecentIntent && !demonContextRelevant)
    {
        return false;
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
    if (recentIntentOut)
    {
        *recentIntentOut = hasRecentIntent;
    }
    return true;
}

static int __fastcall hkMountedDemonJumpLateRoute575D60(
    void *thisPtr,
    void * /*edxUnused*/,
    int a2,
    int a3)
{
    int mountItemId = 0;
    int rootSkillId = 0;
    int currentSkillId = 0;
    bool hasRecentIntent = false;
    const bool shouldLog = ShouldObserveMountedDemonJumpLatePath(
        &mountItemId,
        &rootSkillId,
        &currentSkillId,
        &hasRecentIntent);
    const bool isObservedUpLatchStepSignature =
        a2 == 18 &&
        static_cast<unsigned int>(a3) == 0x80380000u;
    const bool isObservedUpCommitSignature =
        a2 == 38 &&
        (static_cast<unsigned int>(a3) == 0x01480000u ||
         static_cast<unsigned int>(a3) == 0x81480000u);
    const bool shouldTryLateRoutePreseed =
        mountItemId > 0 &&
        hasRecentIntent &&
        (isObservedUpLatchStepSignature || isObservedUpCommitSignature) &&
        ((rootSkillId == 0 && currentSkillId == 0) ||
         (rootSkillId == 30010110 && currentSkillId == 30010110));
    bool primedLateRoutePreseed = false;
    bool forcedRawLateRoutePreseed = false;
    int primedLateRouteRootSkillId = 0;
    int primedLateRouteCurrentSkillId = 0;
    if (shouldTryLateRoutePreseed)
    {
        int gateProbeChildSkillId = 0;
        const bool hasFreshUpGateProbe =
            TryGetRecentMountedDemonJumpGateProbePreferredChildSkillId(
                mountItemId,
                &gateProbeChildSkillId,
                kMountedDemonJumpIntentMaxAgeMs) &&
            gateProbeChildSkillId == 30010183;
        if (hasFreshUpGateProbe)
        {
            int primedCurrentSkillId = 0;
            if (rootSkillId == 30010110 &&
                currentSkillId == 30010110)
            {
                forcedRawLateRoutePreseed =
                    TryManualPrimeMountedDemonJumpContextResolveSafe(
                        mountItemId,
                        "575D60-preseed-root-only-up",
                        &primedCurrentSkillId,
                        &primedLateRouteRootSkillId);
                if (forcedRawLateRoutePreseed)
                {
                    primedLateRoutePreseed = true;
                    primedLateRouteCurrentSkillId = primedCurrentSkillId;
                }
            }
            if (!primedLateRoutePreseed)
            {
                primedLateRoutePreseed =
                    PrimeMountedDemonJumpContextIfNeeded(
                        mountItemId,
                        "575D60-preseed",
                        &primedCurrentSkillId);
            }
            if (primedLateRoutePreseed)
            {
                // v23.35 logs show that preseed can restore root/current for the
                // first mounted up-jump while the original short recent-intent
                // window has already expired before the first B22630 commit runs.
                // Refresh only the narrow demon intent timestamp here so the
                // first late-route commit keeps recent=1 without broadening the
                // global up/side/glide intent window.
                ObserveMountedRuntimeSkillIntent(
                    MountedRuntimeSkillKind_DemonJump,
                    mountItemId);
                if (!TryReadMountedDemonJumpEffectiveContextState(
                        mountItemId,
                        &primedLateRouteRootSkillId,
                        &primedLateRouteCurrentSkillId))
                {
                    primedLateRouteRootSkillId = 30010110;
                    if (!IsMountedDemonJumpRuntimeChildSkillId(
                            primedLateRouteCurrentSkillId))
                    {
                        primedLateRouteCurrentSkillId =
                            primedCurrentSkillId > 0
                                ? primedCurrentSkillId
                                : 30010183;
                    }
                }
                rootSkillId = primedLateRouteRootSkillId;
                currentSkillId = primedLateRouteCurrentSkillId;
            }

            static LONG s_mountedDemonJumpLateRoutePreseedLogBudget = 32;
            if (InterlockedDecrement(
                    &s_mountedDemonJumpLateRoutePreseedLogBudget) >= 0)
            {
                WriteLogFmt(
                    "[MountDemonJumpLate] 575D60 preseed a2=%d a3=0x%08X mount=%d recent=%d probeChild=%d prime=%d forceRaw=%d root=%d current=%d",
                    a2,
                    a3,
                    mountItemId,
                    hasRecentIntent ? 1 : 0,
                    gateProbeChildSkillId,
                    primedLateRoutePreseed ? 1 : 0,
                    forcedRawLateRoutePreseed ? 1 : 0,
                    primedLateRouteRootSkillId,
                    primedLateRouteCurrentSkillId);
            }
        }
    }
    if (shouldLog)
    {
        static LONG s_mountedDemonJumpLateRouteEnterLogBudget = 64;
        if (InterlockedDecrement(&s_mountedDemonJumpLateRouteEnterLogBudget) >= 0)
        {
            WriteLogFmt(
                "[MountDemonJumpLate] 575D60 enter this=0x%08X a2=%d a3=0x%08X mount=%d recent=%d root=%d current=%d",
                (DWORD)(uintptr_t)thisPtr,
                a2,
                a3,
                mountItemId,
                hasRecentIntent ? 1 : 0,
                rootSkillId,
                currentSkillId);
        }
    }

    const int result = oMountedDemonJumpLateRoute575D60
                           ? oMountedDemonJumpLateRoute575D60(thisPtr, a2, a3)
                           : 0;
    if (shouldLog)
    {
        static LONG s_mountedDemonJumpLateRouteLeaveLogBudget = 64;
        if (InterlockedDecrement(&s_mountedDemonJumpLateRouteLeaveLogBudget) >= 0)
        {
            int afterRootSkillId = 0;
            int afterCurrentSkillId = 0;
            TryReadMountedDemonJumpContextState(
                &afterRootSkillId,
                &afterCurrentSkillId,
                nullptr);
            WriteLogFmt(
                "[MountDemonJumpLate] 575D60 leave result=%d mount=%d root=%d current=%d",
                result,
                mountItemId,
                afterRootSkillId,
                afterCurrentSkillId);
        }
    }
    return result;
}

