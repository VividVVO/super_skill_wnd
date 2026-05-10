static void __stdcall hkMountedDemonJumpLateTick576020(
    unsigned int a1,
    int a2)
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
    if (shouldLog)
    {
        static LONG s_mountedDemonJumpLateTickEnterLogBudget = 64;
        if (InterlockedDecrement(&s_mountedDemonJumpLateTickEnterLogBudget) >= 0)
        {
            WriteLogFmt(
                "[MountDemonJumpLate] 576020 enter a1=%u a2=0x%08X mount=%d recent=%d root=%d current=%d",
                a1,
                a2,
                mountItemId,
                hasRecentIntent ? 1 : 0,
                rootSkillId,
                currentSkillId);
        }
    }

    if (oMountedDemonJumpLateTick576020)
    {
        oMountedDemonJumpLateTick576020(a1, a2);
    }

    if (shouldLog)
    {
        static LONG s_mountedDemonJumpLateTickLeaveLogBudget = 64;
        if (InterlockedDecrement(&s_mountedDemonJumpLateTickLeaveLogBudget) >= 0)
        {
            int afterRootSkillId = 0;
            int afterCurrentSkillId = 0;
            TryReadMountedDemonJumpContextState(
                &afterRootSkillId,
                &afterCurrentSkillId,
                nullptr);
            WriteLogFmt(
                "[MountDemonJumpLate] 576020 leave mount=%d root=%d current=%d",
                mountItemId,
                afterRootSkillId,
                afterCurrentSkillId);
        }
    }
}

static UINT __fastcall hkMountedDemonJumpContextInputB22630(
    void *thisPtr,
    void * /*edxUnused*/,
    UINT a2,
    int a3)
{
    const DWORD callerRet = (DWORD)(uintptr_t)_ReturnAddress();
    int mountItemId = 0;
    int rootSkillId = 0;
    int currentSkillId = 0;
    bool hasRecentIntent = false;
    const bool shouldLog = ShouldObserveMountedDemonJumpLatePath(
        &mountItemId,
        &rootSkillId,
        &currentSkillId,
        &hasRecentIntent);

    BYTE beforeGateMode = 0;
    BYTE beforeDownLatch = 0;
    BYTE beforeUpLatch = 0;
    const uintptr_t userLocalAddr =
        reinterpret_cast<uintptr_t>(thisPtr);
    if (userLocalAddr)
    {
        __try
        {
            beforeGateMode =
                *reinterpret_cast<BYTE *>(userLocalAddr + 24292);
            beforeDownLatch =
                *reinterpret_cast<BYTE *>(userLocalAddr + 24196);
            beforeUpLatch =
                *reinterpret_cast<BYTE *>(userLocalAddr + 24197);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            beforeGateMode = 0;
            beforeDownLatch = 0;
            beforeUpLatch = 0;
        }
    }

    const bool isObservedGlideCommitSignature =
        a2 == 18 &&
        static_cast<unsigned int>(a3) == 0x00380300u;
    const bool isObservedUpLatchStepSignature =
        a2 == 18 &&
        static_cast<unsigned int>(a3) == 0x80380000u;

    if (thisPtr &&
        mountItemId > 0 &&
        hasRecentIntent &&
        (a2 == 39 || a2 == 38 || isObservedUpLatchStepSignature) &&
        beforeGateMode == 3 &&
        beforeDownLatch == 0 &&
        ((rootSkillId == 0 && currentSkillId == 0) ||
         (rootSkillId == 30010110 && currentSkillId == 30010110)) &&
        ResolveMountedRuntimeSkillIdForKind(
            MountedRuntimeSkillKind_DemonJump,
            mountItemId) == 30010110)
    {
        int gateProbeChildSkillId = 0;
        if (TryGetRecentMountedDemonJumpGateProbePreferredChildSkillId(
                mountItemId,
                &gateProbeChildSkillId,
                kMountedDemonJumpIntentMaxAgeMs) &&
            gateProbeChildSkillId == 30010183)
        {
            int primedEnterRootSkillId = 0;
            int primedEnterCurrentSkillId = 0;
            bool didEnterUpPreseed =
                ForceRefreshMountedDemonJumpContextSeed(
                    thisPtr,
                    mountItemId,
                    "B22630-enter-up-preseed",
                    &primedEnterCurrentSkillId,
                    &primedEnterRootSkillId);
            if (!didEnterUpPreseed &&
                TryReadMountedDemonJumpEffectiveContextState(
                    mountItemId,
                    &primedEnterRootSkillId,
                    &primedEnterCurrentSkillId) &&
                primedEnterRootSkillId == 30010110 &&
                primedEnterCurrentSkillId == 30010183)
            {
                didEnterUpPreseed = true;
            }

            if (didEnterUpPreseed)
            {
                rootSkillId = primedEnterRootSkillId > 0
                                  ? primedEnterRootSkillId
                                  : 30010110;
                currentSkillId =
                    IsMountedDemonJumpRuntimeChildSkillId(
                        primedEnterCurrentSkillId)
                        ? primedEnterCurrentSkillId
                        : 30010183;

                static LONG
                    s_mountedDemonJumpContextInputEnterPreseedLogBudget = 32;
                if (InterlockedDecrement(
                        &s_mountedDemonJumpContextInputEnterPreseedLogBudget) >=
                    0)
                {
                    WriteLogFmt(
                        "[MountDemonJumpLate] B22630 enter preseed mount=%d a2=%u a3=0x%08X gate=%u/%u/%u root=%d current=%d",
                        mountItemId,
                        a2,
                        a3,
                        static_cast<unsigned int>(beforeGateMode),
                        static_cast<unsigned int>(beforeDownLatch),
                        static_cast<unsigned int>(beforeUpLatch),
                        rootSkillId,
                        currentSkillId);
                }
            }
        }
    }
    if (shouldLog)
    {
        static LONG s_mountedDemonJumpContextInputEnterLogBudget = 96;
        if (InterlockedDecrement(&s_mountedDemonJumpContextInputEnterLogBudget) >= 0)
        {
            WriteLogFmt(
                "[MountDemonJumpLate] B22630 enter a2=%u a3=0x%08X mount=%d recent=%d root=%d current=%d mode=%u down=%u up=%u",
                a2,
                a3,
                mountItemId,
                hasRecentIntent ? 1 : 0,
                rootSkillId,
                currentSkillId,
                static_cast<unsigned int>(beforeGateMode),
                static_cast<unsigned int>(beforeDownLatch),
                static_cast<unsigned int>(beforeUpLatch));
        }
    }

    const UINT result = oMountedDemonJumpContextInputB22630
                            ? oMountedDemonJumpContextInputB22630(thisPtr, a2, a3)
                            : 0;
    int afterRootSkillId = 0;
    int afterCurrentSkillId = 0;
    bool hasAfterContext = TryReadMountedDemonJumpContextState(
        &afterRootSkillId,
        &afterCurrentSkillId,
        nullptr);
    int effectiveMountItemId = mountItemId;
    if (effectiveMountItemId <= 0)
    {
        TryResolveCurrentUserMountItemIdWithFallback(
            &effectiveMountItemId,
            nullptr);
    }
    BYTE afterGateMode = 0;
    BYTE afterDownLatch = 0;
    BYTE afterUpLatch = 0;
    if (userLocalAddr)
    {
        __try
        {
            afterGateMode =
                *reinterpret_cast<BYTE *>(userLocalAddr + 24292);
            afterDownLatch =
                *reinterpret_cast<BYTE *>(userLocalAddr + 24196);
            afterUpLatch =
                *reinterpret_cast<BYTE *>(userLocalAddr + 24197);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            afterGateMode = 0;
            afterDownLatch = 0;
            afterUpLatch = 0;
        }
    }

    const bool isMountedDemonJumpConfiguredMount =
        effectiveMountItemId > 0 &&
        ResolveMountedRuntimeSkillIdForKind(
            MountedRuntimeSkillKind_DemonJump,
            effectiveMountItemId) == 30010110;
    const bool isObservedUpCommitSignature =
        a2 == 38 &&
        (static_cast<unsigned int>(a3) == 0x01480000u ||
         static_cast<unsigned int>(a3) == 0x81480000u);
    if (isMountedDemonJumpConfiguredMount &&
        isObservedUpCommitSignature)
    {
        static LONG s_mountedDemonJumpContextInputUpCommitProbeLogBudget = 48;
        if (InterlockedDecrement(
                &s_mountedDemonJumpContextInputUpCommitProbeLogBudget) >= 0)
        {
            WriteLogFmt(
                "[MountDemonJumpLate] B22630 up probe caller=0x%08X mount=%d result=%u recent=%d root=%d current=%d gate=%u/%u/%u->%u/%u/%u a2=%u a3=0x%08X",
                callerRet,
                effectiveMountItemId,
                result,
                hasRecentIntent ? 1 : 0,
                afterRootSkillId,
                afterCurrentSkillId,
                static_cast<unsigned int>(beforeGateMode),
                static_cast<unsigned int>(beforeDownLatch),
                static_cast<unsigned int>(beforeUpLatch),
                static_cast<unsigned int>(afterGateMode),
                static_cast<unsigned int>(afterDownLatch),
                static_cast<unsigned int>(afterUpLatch),
                a2,
                a3);
        }
    }
    if (isMountedDemonJumpConfiguredMount &&
        !(hasAfterContext &&
          afterRootSkillId == 30010110 &&
          IsMountedDemonJumpRuntimeChildSkillId(afterCurrentSkillId)))
    {
        const bool hadRealRecentIntent =
            HasRecentMountedDemonJumpIntent(
                effectiveMountItemId,
                kMountedDemonJumpIntentMaxAgeMs);
        const bool hadLateSustainIntent =
            HasMountedDemonJumpLateSustainIntent(
                effectiveMountItemId,
                afterRootSkillId,
                afterCurrentSkillId,
                kMountedDemonJumpIntentMaxAgeMs);
        const bool looksLikeStrictRawUpCommit =
            isObservedUpCommitSignature &&
            result == 4 &&
            beforeGateMode == 3 &&
            beforeUpLatch != 0 &&
            afterGateMode == 3 &&
            afterUpLatch == 0;
        const bool looksLikeRelaxedRawUpCommit =
            isObservedUpCommitSignature &&
            result == 4;
        const bool looksLikeRawUpCommit =
            looksLikeStrictRawUpCommit ||
            looksLikeRelaxedRawUpCommit;
        if (!hadRealRecentIntent &&
            !hadLateSustainIntent &&
            looksLikeRawUpCommit)
        {
            RememberMountedDemonJumpNativeChildSkill(
                effectiveMountItemId,
                30010183,
                "B22630-raw-up-commit");
            ObserveMountedDemonJumpGateProbe(
                effectiveMountItemId,
                "B22630-raw-up-commit",
                callerRet);
            ObserveMountedDemonJumpIntent(
                effectiveMountItemId,
                "B22630-raw-up-commit");

            int primedCurrentSkillId = 0;
            int primedRootSkillId = 0;
            const bool primed =
                thisPtr &&
                ForceRefreshMountedDemonJumpContextSeed(
                    thisPtr,
                    effectiveMountItemId,
                    "B22630-raw-up-commit",
                    &primedCurrentSkillId,
                    &primedRootSkillId);
            if (primed)
            {
                hasAfterContext = true;
                afterRootSkillId = primedRootSkillId;
                afterCurrentSkillId = primedCurrentSkillId;
            }

            static LONG s_mountedDemonJumpContextInputRawUpCommitLogBudget = 32;
            if (InterlockedDecrement(
                    &s_mountedDemonJumpContextInputRawUpCommitLogBudget) >= 0)
            {
                WriteLogFmt(
                    "[MountDemonJumpLate] B22630 raw up commit caller=0x%08X mount=%d result=%u a2=%u a3=0x%08X gate=%u/%u/%u->%u/%u/%u strict=%d intent=1 prime=%d primedRoot=%d primedCurrent=%d",
                    callerRet,
                    effectiveMountItemId,
                    result,
                    a2,
                    a3,
                    static_cast<unsigned int>(beforeGateMode),
                    static_cast<unsigned int>(beforeDownLatch),
                    static_cast<unsigned int>(beforeUpLatch),
                    static_cast<unsigned int>(afterGateMode),
                    static_cast<unsigned int>(afterDownLatch),
                    static_cast<unsigned int>(afterUpLatch),
                    looksLikeStrictRawUpCommit ? 1 : 0,
                    primed ? 1 : 0,
                    primed ? primedRootSkillId : 0,
                    primed ? primedCurrentSkillId : 0);
            }
        }
    }

    if (effectiveMountItemId > 0 &&
        isObservedGlideCommitSignature &&
        afterRootSkillId == 30010110 &&
        afterCurrentSkillId == 30010186 &&
        beforeGateMode == 3 &&
        beforeDownLatch == 0 &&
        beforeUpLatch != 0)
    {
        int gateProbeChildSkillId = 0;
        if (TryGetRecentMountedDemonJumpGateProbePreferredChildSkillId(
                effectiveMountItemId,
                &gateProbeChildSkillId,
                kMountedDemonJumpIntentMaxAgeMs) &&
            gateProbeChildSkillId == 30010183)
        {
            int restoredRootSkillId = 0;
            int restoredCurrentSkillId = 0;
            bool restoredUpContext = false;
            if (thisPtr)
            {
                restoredUpContext =
                    ForceRefreshMountedDemonJumpContextSeed(
                        thisPtr,
                        effectiveMountItemId,
                        "B22630-glide-hijack-restore-up",
                        &restoredCurrentSkillId,
                        &restoredRootSkillId);
            }
            if (!restoredUpContext &&
                TryReadMountedDemonJumpEffectiveContextState(
                    effectiveMountItemId,
                    &restoredRootSkillId,
                    &restoredCurrentSkillId) &&
                restoredRootSkillId == 30010110 &&
                restoredCurrentSkillId == 30010183)
            {
                restoredUpContext = true;
            }
            if (restoredUpContext)
            {
                afterRootSkillId = restoredRootSkillId;
                afterCurrentSkillId = restoredCurrentSkillId;
                static LONG s_mountedDemonJumpContextInputGlideHijackRestoreLogBudget =
                    32;
                if (InterlockedDecrement(
                        &s_mountedDemonJumpContextInputGlideHijackRestoreLogBudget) >=
                    0)
                {
                    WriteLogFmt(
                        "[MountDemonJumpLate] B22630 restore up after glide hijack caller=0x%08X mount=%d a2=%u a3=0x%08X gate=%u/%u/%u->%u/%u/%u restored=%d/%d probeChild=%d",
                        callerRet,
                        effectiveMountItemId,
                        a2,
                        a3,
                        static_cast<unsigned int>(beforeGateMode),
                        static_cast<unsigned int>(beforeDownLatch),
                        static_cast<unsigned int>(beforeUpLatch),
                        static_cast<unsigned int>(afterGateMode),
                        static_cast<unsigned int>(afterDownLatch),
                        static_cast<unsigned int>(afterUpLatch),
                        afterRootSkillId,
                        afterCurrentSkillId,
                        gateProbeChildSkillId);
                }
            }
        }
    }

    if (effectiveMountItemId > 0 &&
        hasAfterContext &&
        afterRootSkillId == 30010110 &&
        IsMountedDemonJumpRuntimeChildSkillId(afterCurrentSkillId) &&
        ResolveMountedRuntimeSkillIdForKind(
            MountedRuntimeSkillKind_DemonJump,
            effectiveMountItemId) == 30010110)
    {
        const bool hadRealRecentIntent =
            HasRecentMountedDemonJumpIntent(
                effectiveMountItemId,
                kMountedDemonJumpIntentMaxAgeMs);
        const bool hadLateSustainIntent =
            HasMountedDemonJumpLateSustainIntent(
                effectiveMountItemId,
                afterRootSkillId,
                afterCurrentSkillId,
                kMountedDemonJumpIntentMaxAgeMs);
        const bool hasNonZeroGateSnapshot =
            beforeGateMode != 0 ||
            beforeDownLatch != 0 ||
            beforeUpLatch != 0;
        int gateProbeMountItemId = 0;
        const bool hasRecentMatchingGateProbe =
            afterRootSkillId == 30010110 &&
            afterCurrentSkillId == 30010183 &&
            isObservedUpCommitSignature &&
            result == 4 &&
            TryGetRecentMountedDemonJumpGateProbeMountItemId(
                &gateProbeMountItemId,
                kMountedDemonJumpIntentMaxAgeMs + 250) &&
            gateProbeMountItemId == effectiveMountItemId;
        const bool wasAlreadyOnSameRuntimeChild =
            rootSkillId == afterRootSkillId &&
            currentSkillId == afterCurrentSkillId &&
            IsMountedDemonJumpRuntimeChildSkillId(currentSkillId);
        const bool shouldArmLateUpChild =
            !hadRealRecentIntent &&
            !hadLateSustainIntent &&
            afterRootSkillId == 30010110 &&
            afterCurrentSkillId == 30010183 &&
            wasAlreadyOnSameRuntimeChild &&
            hasNonZeroGateSnapshot;
        const bool shouldArmLateUpChildFromGateProbe =
            !hadRealRecentIntent &&
            !hadLateSustainIntent &&
            afterRootSkillId == 30010110 &&
            afterCurrentSkillId == 30010183 &&
            isObservedUpCommitSignature &&
            result == 4 &&
            hasRecentMatchingGateProbe;
        if (afterCurrentSkillId == 30010186 &&
            isObservedUpLatchStepSignature &&
            beforeUpLatch == 0 &&
            afterUpLatch != 0)
        {
            static LONG s_mountedDemonJumpContextInputSkipGlideOnUpLatchLogBudget =
                24;
            if (InterlockedDecrement(
                    &s_mountedDemonJumpContextInputSkipGlideOnUpLatchLogBudget) >=
                0)
            {
                WriteLogFmt(
                    "[MountDemonJumpLate] B22630 skip glide late-child arm on up-latch mount=%d result=%u a2=%u a3=0x%08X gate=%u/%u/%u->%u/%u/%u",
                    effectiveMountItemId,
                    result,
                    a2,
                    a3,
                    static_cast<unsigned int>(beforeGateMode),
                    static_cast<unsigned int>(beforeDownLatch),
                    static_cast<unsigned int>(beforeUpLatch),
                    static_cast<unsigned int>(afterGateMode),
                    static_cast<unsigned int>(afterDownLatch),
                    static_cast<unsigned int>(afterUpLatch));
            }
        }
        else if (hadRealRecentIntent)
        {
            RememberMountedDemonJumpNativeChildSkill(
                effectiveMountItemId,
                afterCurrentSkillId,
                "B22630");
        }
        else if (hadLateSustainIntent ||
                 shouldArmLateUpChild ||
                 shouldArmLateUpChildFromGateProbe)
        {
            int recentChildSkillId = 0;
            const bool hadRecentChildCache =
                TryGetRecentMountedDemonJumpNativeChildSkill(
                    effectiveMountItemId,
                    &recentChildSkillId,
                    nullptr,
                    kMountedDemonJumpLateChildCacheMatchMaxAgeMs) &&
                IsMountedDemonJumpRuntimeChildSkillId(recentChildSkillId);
            const bool hadMatchingRecentChildCache =
                hadRecentChildCache &&
                recentChildSkillId == afterCurrentSkillId;
            const bool shouldForceRefreshMatchingUpChild =
                hadMatchingRecentChildCache &&
                afterCurrentSkillId == 30010183 &&
                isObservedUpCommitSignature &&
                result == 4;
            const bool shouldForceRefreshMatchingSideChild =
                hadMatchingRecentChildCache &&
                afterCurrentSkillId == 30010184 &&
                result == 4;
            if (!hadMatchingRecentChildCache)
            {
                RememberMountedDemonJumpNativeChildSkill(
                    effectiveMountItemId,
                    afterCurrentSkillId,
                    "B22630");
            }
            if (shouldForceRefreshMatchingUpChild)
            {
                RememberMountedDemonJumpNativeChildSkill(
                    effectiveMountItemId,
                    afterCurrentSkillId,
                    "B22630-up-refresh");
            }
            if (shouldForceRefreshMatchingSideChild)
            {
                RememberMountedDemonJumpNativeChildSkill(
                    effectiveMountItemId,
                    afterCurrentSkillId,
                    "B22630-side-refresh");
            }
            if (!hadMatchingRecentChildCache ||
                shouldForceRefreshMatchingUpChild ||
                shouldForceRefreshMatchingSideChild)
            {
                const char *refreshReason =
                    shouldArmLateUpChildFromGateProbe
                        ? "B22630-late-child-up-probe"
                        : shouldForceRefreshMatchingUpChild
                        ? "B22630-late-child-up-refresh"
                        : afterCurrentSkillId == 30010183
                        ? "B22630-late-child-up"
                        : (afterCurrentSkillId == 30010184
                               ? "B22630-late-child-side"
                               : "B22630-late-child-glide");
                ObserveMountedDemonJumpIntent(
                    effectiveMountItemId,
                    refreshReason);
                int primedCurrentSkillId = 0;
                int primedRootSkillId = 0;
                bool hasPrimedContext = false;
                if (thisPtr)
                {
                    hasPrimedContext =
                        ForceRefreshMountedDemonJumpContextSeed(
                            thisPtr,
                            effectiveMountItemId,
                            refreshReason,
                            &primedCurrentSkillId,
                            &primedRootSkillId);
                }
                if (!hasPrimedContext)
                {
                    hasPrimedContext =
                        HasMountedDemonJumpContextPrimedForMount(
                            effectiveMountItemId,
                            &primedCurrentSkillId,
                            &primedRootSkillId);
                }
                const bool hasRecentIntentAfterArm =
                    HasRecentMountedDemonJumpIntent(effectiveMountItemId, 250);
                bool syntheticSpecialMoveSent = false;
                DWORD syntheticSpecialMoveTick = 0;
                if (afterCurrentSkillId == 30010183 &&
                    !hadMatchingRecentChildCache &&
                    hasRecentIntentAfterArm)
                {
                    syntheticSpecialMoveSent =
                        SendMountedDemonJumpSyntheticSpecialMovePacket(
                            afterCurrentSkillId,
                            1,
                            &syntheticSpecialMoveTick);
                }
                static LONG s_mountedDemonJumpContextInputLateIntentLogBudget = 32;
                if (InterlockedDecrement(
                        &s_mountedDemonJumpContextInputLateIntentLogBudget) >= 0)
                {
                    WriteLogFmt(
                        "[MountDemonJumpLate] B22630 child observed without fresh intent mount=%d root=%d current=%d result=%u a2=%u a3=0x%08X reason=%s cached=%d refreshMatch=%d -> arm intent=%d prime=%d primedRoot=%d primedCurrent=%d synth93=%d tick=%u",
                        effectiveMountItemId,
                        afterRootSkillId,
                        afterCurrentSkillId,
                        result,
                        a2,
                        a3,
                        refreshReason,
                        recentChildSkillId,
                        shouldForceRefreshMatchingUpChild ||
                                shouldForceRefreshMatchingSideChild
                            ? 1
                            : 0,
                        hasRecentIntentAfterArm ? 1 : 0,
                        hasPrimedContext ? 1 : 0,
                        hasPrimedContext ? primedRootSkillId : 0,
                        hasPrimedContext ? primedCurrentSkillId : 0,
                        syntheticSpecialMoveSent ? 1 : 0,
                        syntheticSpecialMoveTick);
                }
            }
            else
            {
                static LONG s_mountedDemonJumpContextInputIgnoreLateChildLogBudget =
                    32;
                if (InterlockedDecrement(
                        &s_mountedDemonJumpContextInputIgnoreLateChildLogBudget) >=
                    0)
                {
                    WriteLogFmt(
                        "[MountDemonJumpLate] B22630 ignore stale child mount=%d root=%d current=%d cached=%d result=%u a2=%u a3=0x%08X",
                        effectiveMountItemId,
                        afterRootSkillId,
                        afterCurrentSkillId,
                        recentChildSkillId,
                        result,
                        a2,
                        a3);
                }
            }
        }
        else
        {
            static LONG s_mountedDemonJumpContextInputSkipLateChildLogBudget = 32;
            if (InterlockedDecrement(
                    &s_mountedDemonJumpContextInputSkipLateChildLogBudget) >= 0)
            {
                WriteLogFmt(
                    "[MountDemonJumpLate] B22630 skip late child without fresh intent mount=%d root=%d current=%d result=%u a2=%u a3=0x%08X",
                    effectiveMountItemId,
                    afterRootSkillId,
                    afterCurrentSkillId,
                    result,
                    a2,
                    a3);
            }
        }
    }
    if (shouldLog)
    {
        static LONG s_mountedDemonJumpContextInputLeaveLogBudget = 96;
        if (InterlockedDecrement(&s_mountedDemonJumpContextInputLeaveLogBudget) >= 0)
        {
            WriteLogFmt(
                "[MountDemonJumpLate] B22630 leave result=%u mount=%d root=%d current=%d mode=%u down=%u up=%u",
                result,
                effectiveMountItemId > 0 ? effectiveMountItemId : mountItemId,
                afterRootSkillId,
                afterCurrentSkillId,
                static_cast<unsigned int>(afterGateMode),
                static_cast<unsigned int>(afterDownLatch),
                static_cast<unsigned int>(afterUpLatch));
        }
    }
    return result;
}

static BOOL __fastcall hkMountedDemonJumpFilterBDBFD0(
    void *thisPtr,
    void * /*edxUnused*/,
    int a2,
    int a3)
{
    const BOOL result = oMountedDemonJumpFilterBDBFD0
                            ? oMountedDemonJumpFilterBDBFD0(thisPtr, a2, a3)
                            : FALSE;
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
        static LONG s_mountedDemonJumpFilterLogBudget = 64;
        if (InterlockedDecrement(&s_mountedDemonJumpFilterLogBudget) >= 0)
        {
            WriteLogFmt(
                "[MountDemonJumpLate] BDBFD0 result=%d a2=%d a3=0x%08X mount=%d recent=%d root=%d current=%d",
                result ? 1 : 0,
                a2,
                a3,
                mountItemId,
                hasRecentIntent ? 1 : 0,
                rootSkillId,
                currentSkillId);
        }
    }
    return result;
}

static int __fastcall hkMountedDemonJumpBranchADEDA0(
    void *thisPtr,
    void * /*edxUnused*/,
    int a2)
{
    const int result = oMountedDemonJumpBranchADEDA0
                           ? oMountedDemonJumpBranchADEDA0(thisPtr, a2)
                           : 0;
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
        static LONG s_mountedDemonJumpBranchLogBudget = 64;
        if (InterlockedDecrement(&s_mountedDemonJumpBranchLogBudget) >= 0)
        {
            WriteLogFmt(
                "[MountDemonJumpLate] ADEDA0 result=%d a2=0x%08X mount=%d recent=%d root=%d current=%d",
                result,
                a2,
                mountItemId,
                hasRecentIntent ? 1 : 0,
                rootSkillId,
                currentSkillId);
        }
    }
    return result;
}

struct MountedDemonJumpLateLocalStateSnapshot
{
    int state4A4;
    int state3DDC;
    int state4DE4;
    int state490;
    int state5BAC;
    int state5BB0;
    int state5BC0;
    int state20C4;
    int state5BC4;
    int state5BC8;
    int state5D7C;
    int state5DC4;
    int state5E8C;
    int state5F2C;
    BYTE state5E84;
    BYTE state5E85;
    BYTE state5EE4;
    bool valid;
};

struct MountedDemonJumpUserGateSnapshot
{
    BYTE gateMode;
    BYTE downLatch;
    BYTE upLatch;
    bool valid;
};

static bool TryReadMountedDemonJumpLateLocalStateSnapshot(
    void *thisPtr,
    MountedDemonJumpLateLocalStateSnapshot *outSnapshot)
{
    if (outSnapshot)
    {
        memset(outSnapshot, 0, sizeof(*outSnapshot));
        outSnapshot->valid = false;
    }

    if (!thisPtr || !outSnapshot)
    {
        return false;
    }

    const uintptr_t thisValue = reinterpret_cast<uintptr_t>(thisPtr);
    if (SafeIsBadReadPtr(reinterpret_cast<void *>(thisValue + 0x5F30), sizeof(DWORD)))
    {
        return false;
    }

    __try
    {
        outSnapshot->state4A4 = *reinterpret_cast<int *>(thisValue + 0x4A4);
        outSnapshot->state3DDC = *reinterpret_cast<int *>(thisValue + 0x3DDC);
        outSnapshot->state4DE4 = *reinterpret_cast<int *>(thisValue + 0x4DE4);
        outSnapshot->state490 = *reinterpret_cast<int *>(thisValue + 0x490);
        outSnapshot->state5BAC = *reinterpret_cast<int *>(thisValue + 0x5BAC);
        outSnapshot->state5BB0 = *reinterpret_cast<int *>(thisValue + 0x5BB0);
        outSnapshot->state5BC0 = *reinterpret_cast<int *>(thisValue + 0x5BC0);
        outSnapshot->state20C4 = *reinterpret_cast<int *>(thisValue + 0x20C4);
        outSnapshot->state5BC4 = *reinterpret_cast<int *>(thisValue + 0x5BC4);
        outSnapshot->state5BC8 = *reinterpret_cast<int *>(thisValue + 0x5BC8);
        outSnapshot->state5D7C = *reinterpret_cast<int *>(thisValue + 0x5D7C);
        outSnapshot->state5DC4 = *reinterpret_cast<int *>(thisValue + 0x5DC4);
        outSnapshot->state5E8C = *reinterpret_cast<int *>(thisValue + 0x5E8C);
        outSnapshot->state5F2C = *reinterpret_cast<int *>(thisValue + 0x5F2C);
        outSnapshot->state5E84 = *reinterpret_cast<BYTE *>(thisValue + 0x5E84);
        outSnapshot->state5E85 = *reinterpret_cast<BYTE *>(thisValue + 0x5E85);
        outSnapshot->state5EE4 = *reinterpret_cast<BYTE *>(thisValue + 0x5EE4);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        memset(outSnapshot, 0, sizeof(*outSnapshot));
        outSnapshot->valid = false;
        return false;
    }

    outSnapshot->valid = true;
    return true;
}

static bool TryReadMountedDemonJumpUserGateSnapshot(
    void *userLocal,
    MountedDemonJumpUserGateSnapshot *outSnapshot)
{
    if (outSnapshot)
    {
        memset(outSnapshot, 0, sizeof(*outSnapshot));
        outSnapshot->valid = false;
    }

    if (!userLocal || !outSnapshot)
    {
        return false;
    }

    const uintptr_t userLocalAddr = reinterpret_cast<uintptr_t>(userLocal);
    if (SafeIsBadReadPtr(
            reinterpret_cast<void *>(userLocalAddr + 24292),
            sizeof(BYTE)))
    {
        return false;
    }

    __try
    {
        outSnapshot->gateMode =
            *reinterpret_cast<BYTE *>(userLocalAddr + 24292);
        outSnapshot->downLatch =
            *reinterpret_cast<BYTE *>(userLocalAddr + 24196);
        outSnapshot->upLatch =
            *reinterpret_cast<BYTE *>(userLocalAddr + 24197);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        memset(outSnapshot, 0, sizeof(*outSnapshot));
        outSnapshot->valid = false;
        return false;
    }

    outSnapshot->valid = true;
    return true;
}

static bool TryClearMountedDemonJumpUserGateState(
    void *userLocal,
    MountedDemonJumpUserGateSnapshot *beforeSnapshotOut,
    MountedDemonJumpUserGateSnapshot *afterSnapshotOut)
{
    if (beforeSnapshotOut)
    {
        memset(beforeSnapshotOut, 0, sizeof(*beforeSnapshotOut));
    }
    if (afterSnapshotOut)
    {
        memset(afterSnapshotOut, 0, sizeof(*afterSnapshotOut));
    }

    if (!userLocal)
    {
        return false;
    }

    MountedDemonJumpUserGateSnapshot beforeSnapshot = {};
    if (!TryReadMountedDemonJumpUserGateSnapshot(
            userLocal,
            &beforeSnapshot) ||
        !beforeSnapshot.valid)
    {
        return false;
    }

    const uintptr_t userLocalAddr = reinterpret_cast<uintptr_t>(userLocal);
    if (SafeIsBadWritePtr(
            reinterpret_cast<void *>(userLocalAddr + 24292),
            sizeof(BYTE)))
    {
        return false;
    }

    __try
    {
        *reinterpret_cast<BYTE *>(userLocalAddr + 24292) = 0;
        *reinterpret_cast<BYTE *>(userLocalAddr + 24196) = 0;
        *reinterpret_cast<BYTE *>(userLocalAddr + 24197) = 0;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }

    MountedDemonJumpUserGateSnapshot afterSnapshot = {};
    const bool hasAfterSnapshot =
        TryReadMountedDemonJumpUserGateSnapshot(
            userLocal,
            &afterSnapshot) &&
        afterSnapshot.valid;

    if (beforeSnapshotOut)
    {
        *beforeSnapshotOut = beforeSnapshot;
    }
    if (afterSnapshotOut)
    {
        if (hasAfterSnapshot)
        {
            *afterSnapshotOut = afterSnapshot;
        }
        else
        {
            memset(afterSnapshotOut, 0, sizeof(*afterSnapshotOut));
        }
    }

    return true;
}

static void WriteMountedDemonJumpLateLocalStateLog(
    const char *label,
    const char *phase,
    void *thisPtr,
    const MountedDemonJumpLateLocalStateSnapshot &snapshot)
{
    WriteLogFmt(
        "[MountDemonJumpLateState] %s %s this=0x%08X 4A4=0x%08X 3DDC=%d 4DE4=%d 490=%d 5BAC=%d 5BB0=%d 5D7C=%d 5DC4=%d 5F2C=%d pair=%d/%d vec=%d/%d gate=%u/%u/%u slot=0x%08X",
        label ? label : "unknown",
        phase ? phase : "state",
        (DWORD)(uintptr_t)thisPtr,
        snapshot.state4A4,
        snapshot.state3DDC,
        snapshot.state4DE4,
        snapshot.state490,
        snapshot.state5BAC,
        snapshot.state5BB0,
        snapshot.state5D7C,
        snapshot.state5DC4,
        snapshot.state5F2C,
        snapshot.state5BC0,
        snapshot.state20C4,
        snapshot.state5BC4,
        snapshot.state5BC8,
        static_cast<unsigned int>(snapshot.state5E84),
        static_cast<unsigned int>(snapshot.state5E85),
        static_cast<unsigned int>(snapshot.state5EE4),
        snapshot.state5E8C);
}

static int ResolveMountedDemonJumpLateEffectiveChildSkillId(
    int mountItemId,
    int rootSkillId,
    int currentSkillId)
{
    if (mountItemId <= 0 || rootSkillId != 30010110)
    {
        return 0;
    }

    if (IsMountedDemonJumpRuntimeChildSkillId(currentSkillId))
    {
        return currentSkillId;
    }

    int recentChildSkillId = 0;
    if (TryGetRecentMountedDemonJumpNativeChildSkill(
            mountItemId,
            &recentChildSkillId,
            nullptr,
            1500) &&
        IsMountedDemonJumpRuntimeChildSkillId(recentChildSkillId))
    {
        return recentChildSkillId;
    }

    return 0;
}

static bool TryClearMountedDemonJumpLateLocalLockState(
    void *thisPtr,
    int mountItemId,
    int rootSkillId,
    int currentSkillId,
    const char *reasonTag,
    DWORD callerRet)
{
    if (!thisPtr ||
        mountItemId <= 0 ||
        ResolveMountedRuntimeSkillIdForKind(
            MountedRuntimeSkillKind_DemonJump,
            mountItemId) != 30010110)
    {
        return false;
    }

    const int effectiveChildSkillId =
        ResolveMountedDemonJumpLateEffectiveChildSkillId(
            mountItemId,
            rootSkillId,
            currentSkillId);
    if (!IsMountedDemonJumpRuntimeChildSkillId(effectiveChildSkillId))
    {
        return false;
    }

    if (effectiveChildSkillId == 30010183 &&
        !HasFreshMountedRuntimeSkillNativeReleaseRaw(
            MountedRuntimeSkillKind_DemonJump,
            30010183,
            mountItemId,
            650))
    {
        static LONG s_mountedDemonJumpLateLocalLockSkipUpPreReleaseLogBudget =
            48;
        if (InterlockedDecrement(
                &s_mountedDemonJumpLateLocalLockSkipUpPreReleaseLogBudget) >= 0)
        {
            int liveRootSkillId = 0;
            int liveCurrentSkillId = 0;
            const bool hasLiveContext =
                TryReadMountedDemonJumpContextState(
                    &liveRootSkillId,
                    &liveCurrentSkillId,
                    nullptr);
            void *userLocal = nullptr;
            MountedDemonJumpUserGateSnapshot liveGate = {};
            const bool hasLiveGate =
                TryReadCurrentUserLocalPtr(&userLocal) &&
                userLocal &&
                TryReadMountedDemonJumpUserGateSnapshot(
                    userLocal,
                    &liveGate) &&
                liveGate.valid;
            WriteLogFmt(
                "[MountDemonJumpLate] skip local lock clear before up release reason=%s caller=0x%08X mount=%d root=%d current=%d child=%d liveRoot=%d liveCurrent=%d liveGate=%u/%u/%u",
                reasonTag ? reasonTag : "unknown",
                callerRet,
                mountItemId,
                rootSkillId,
                currentSkillId,
                effectiveChildSkillId,
                hasLiveContext ? liveRootSkillId : 0,
                hasLiveContext ? liveCurrentSkillId : 0,
                hasLiveGate
                    ? static_cast<unsigned int>(liveGate.gateMode)
                    : 0,
                hasLiveGate
                    ? static_cast<unsigned int>(liveGate.downLatch)
                    : 0,
                hasLiveGate
                    ? static_cast<unsigned int>(liveGate.upLatch)
                    : 0);
        }
        return false;
    }

    MountedDemonJumpLateLocalStateSnapshot beforeState = {};
    if (!TryReadMountedDemonJumpLateLocalStateSnapshot(
            thisPtr,
            &beforeState) ||
        !beforeState.valid)
    {
        return false;
    }

    const bool shouldClear5D7C = beforeState.state5D7C != 0;
    const bool shouldClear5F2C = beforeState.state5F2C != 0;
    if (!shouldClear5D7C && !shouldClear5F2C)
    {
        return false;
    }

    const uintptr_t thisValue = reinterpret_cast<uintptr_t>(thisPtr);
    if ((shouldClear5D7C &&
         SafeIsBadWritePtr(
             reinterpret_cast<void *>(thisValue + 0x5D7C),
             sizeof(DWORD))) ||
        (shouldClear5F2C &&
         SafeIsBadWritePtr(
             reinterpret_cast<void *>(thisValue + 0x5F2C),
             sizeof(DWORD))))
    {
        return false;
    }

    __try
    {
        if (shouldClear5D7C)
        {
            *reinterpret_cast<int *>(thisValue + 0x5D7C) = 0;
        }
        if (shouldClear5F2C)
        {
            *reinterpret_cast<int *>(thisValue + 0x5F2C) = 0;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }

    MountedDemonJumpLateLocalStateSnapshot afterState = {};
    const bool hasAfterState =
        TryReadMountedDemonJumpLateLocalStateSnapshot(
            thisPtr,
            &afterState) &&
        afterState.valid;

    static LONG s_mountedDemonJumpLateLocalLockClearLogBudget = 48;
    if (InterlockedDecrement(
            &s_mountedDemonJumpLateLocalLockClearLogBudget) >= 0)
    {
        WriteLogFmt(
            "[MountDemonJumpLate] clear local lock reason=%s caller=0x%08X mount=%d root=%d current=%d child=%d 5D7C=%d->%d 5F2C=%d->%d",
            reasonTag ? reasonTag : "unknown",
            callerRet,
            mountItemId,
            rootSkillId,
            currentSkillId,
            effectiveChildSkillId,
            beforeState.state5D7C,
            hasAfterState ? afterState.state5D7C : -1,
            beforeState.state5F2C,
            hasAfterState ? afterState.state5F2C : -1);
        WriteMountedDemonJumpLateLocalStateLog(
            "LateLock",
            "before-clear",
            thisPtr,
            beforeState);
        if (hasAfterState)
        {
            WriteMountedDemonJumpLateLocalStateLog(
                "LateLock",
                "after-clear",
                thisPtr,
                afterState);
        }
    }

    return true;
}

static bool TryClearMountedDemonJumpTailLocalGateState(
    void *thisPtr,
    int mountItemId,
    const char *reasonTag,
    DWORD callerRet)
{
    if (!thisPtr ||
        mountItemId <= 0 ||
        ResolveMountedRuntimeSkillIdForKind(
            MountedRuntimeSkillKind_DemonJump,
            mountItemId) != 30010110)
    {
        return false;
    }

    MountedDemonJumpLateLocalStateSnapshot beforeState = {};
    if (!TryReadMountedDemonJumpLateLocalStateSnapshot(
            thisPtr,
            &beforeState) ||
        !beforeState.valid)
    {
        return false;
    }

    const bool shouldClearLocalGate =
        beforeState.state5E84 != 0 ||
        beforeState.state5E85 != 0 ||
        beforeState.state5EE4 != 0;
    if (!shouldClearLocalGate ||
        beforeState.state5D7C != 0 ||
        beforeState.state5F2C != 0 ||
        beforeState.state5E8C != 0)
    {
        return false;
    }

    const uintptr_t thisValue = reinterpret_cast<uintptr_t>(thisPtr);
    if (SafeIsBadWritePtr(
            reinterpret_cast<void *>(thisValue + 0x5E84),
            sizeof(BYTE)) ||
        SafeIsBadWritePtr(
            reinterpret_cast<void *>(thisValue + 0x5E85),
            sizeof(BYTE)) ||
        SafeIsBadWritePtr(
            reinterpret_cast<void *>(thisValue + 0x5EE4),
            sizeof(BYTE)))
    {
        return false;
    }

    __try
    {
        *reinterpret_cast<BYTE *>(thisValue + 0x5E84) = 0;
        *reinterpret_cast<BYTE *>(thisValue + 0x5E85) = 0;
        *reinterpret_cast<BYTE *>(thisValue + 0x5EE4) = 0;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }

    MountedDemonJumpLateLocalStateSnapshot afterState = {};
    const bool hasAfterState =
        TryReadMountedDemonJumpLateLocalStateSnapshot(
            thisPtr,
            &afterState) &&
        afterState.valid;

    static LONG s_mountedDemonJumpTailLocalGateClearLogBudget = 48;
    if (InterlockedDecrement(
            &s_mountedDemonJumpTailLocalGateClearLogBudget) >= 0)
    {
        WriteLogFmt(
            "[MountDemonJumpTailGate] clear reason=%s caller=0x%08X mount=%d localGate=%u/%u/%u->%u/%u/%u slot=0x%08X->0x%08X 5BB0=%d->%d",
            reasonTag ? reasonTag : "unknown",
            callerRet,
            mountItemId,
            static_cast<unsigned int>(beforeState.state5E84),
            static_cast<unsigned int>(beforeState.state5E85),
            static_cast<unsigned int>(beforeState.state5EE4),
            hasAfterState
                ? static_cast<unsigned int>(afterState.state5E84)
                : 0,
            hasAfterState
                ? static_cast<unsigned int>(afterState.state5E85)
                : 0,
            hasAfterState
                ? static_cast<unsigned int>(afterState.state5EE4)
                : 0,
            beforeState.state5E8C,
            hasAfterState ? afterState.state5E8C : 0,
            beforeState.state5BB0,
            hasAfterState ? afterState.state5BB0 : 0);
        WriteMountedDemonJumpLateLocalStateLog(
            "TailGate",
            "before-clear",
            thisPtr,
            beforeState);
        if (hasAfterState)
        {
            WriteMountedDemonJumpLateLocalStateLog(
                "TailGate",
                "after-clear",
                thisPtr,
                afterState);
        }
    }

    return true;
}

static bool TryForceClearMountedDemonJumpPostPacketTailSlotState(
    void *thisPtr,
    int mountItemId,
    int releasedSkillId,
    const char *reasonTag,
    DWORD callerRet,
    const MountedDemonJumpLateLocalStateSnapshot *beforeStateHint = nullptr)
{
    if (!thisPtr ||
        mountItemId <= 0 ||
        releasedSkillId != 30010183 ||
        ResolveMountedRuntimeSkillIdForKind(
            MountedRuntimeSkillKind_DemonJump,
            mountItemId) != 30010110)
    {
        return false;
    }

    MountedDemonJumpLateLocalStateSnapshot beforeState = {};
    if (beforeStateHint && beforeStateHint->valid)
    {
        beforeState = *beforeStateHint;
    }
    else if (!TryReadMountedDemonJumpLateLocalStateSnapshot(
                 thisPtr,
                 &beforeState) ||
             !beforeState.valid)
    {
        return false;
    }

    // Latest v22.29 logs show the remaining post-landing "can't move" tail on
    // mounted 30010183 after the packet phase is already finalized:
    // root/current is already logged as 0/0 by the caller, gate=0/0/0,
    // 5D7C=0, 5F2C=0, but slot(5E8C) still holds a native child handle until
    // the later 433FEC tail cleanup. Keep this helper local-state driven here:
    // if packet-finalize already reached the clean local window, do not re-block
    // on another context read that can lag one native frame behind the caller.
    if (beforeState.state5D7C != 0 ||
        beforeState.state5F2C != 0 ||
        beforeState.state5E84 != 0 ||
        beforeState.state5E85 != 0 ||
        beforeState.state5EE4 != 0 ||
        beforeState.state5E8C == 0)
    {
        static LONG s_mountedDemonJumpPostPacketTailSlotSkipLogBudget = 24;
        if (InterlockedDecrement(
                &s_mountedDemonJumpPostPacketTailSlotSkipLogBudget) >= 0)
        {
            WriteLogFmt(
                "[MountDemonJumpPostTail] skip reason=%s caller=0x%08X mount=%d released=%d slot=0x%08X gate=%u/%u/%u 5D7C=%d 5F2C=%d",
                reasonTag ? reasonTag : "unknown",
                callerRet,
                mountItemId,
                releasedSkillId,
                beforeState.state5E8C,
                static_cast<unsigned int>(beforeState.state5E84),
                static_cast<unsigned int>(beforeState.state5E85),
                static_cast<unsigned int>(beforeState.state5EE4),
                beforeState.state5D7C,
                beforeState.state5F2C);
        }
        return false;
    }

    const uintptr_t thisValue = reinterpret_cast<uintptr_t>(thisPtr);
    if (SafeIsBadWritePtr(
            reinterpret_cast<void *>(thisValue + 0x5E8C),
            sizeof(DWORD)))
    {
        static LONG s_mountedDemonJumpPostPacketTailSlotBadWriteLogBudget = 12;
        if (InterlockedDecrement(
                &s_mountedDemonJumpPostPacketTailSlotBadWriteLogBudget) >= 0)
        {
            WriteLogFmt(
                "[MountDemonJumpPostTail] bad-write reason=%s caller=0x%08X mount=%d released=%d addr=0x%08X slot=0x%08X",
                reasonTag ? reasonTag : "unknown",
                callerRet,
                mountItemId,
                releasedSkillId,
                static_cast<DWORD>(thisValue + 0x5E8C),
                beforeState.state5E8C);
        }
        return false;
    }

    __try
    {
        *reinterpret_cast<int *>(thisValue + 0x5E8C) = 0;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        static LONG s_mountedDemonJumpPostPacketTailSlotExceptionLogBudget = 12;
        if (InterlockedDecrement(
                &s_mountedDemonJumpPostPacketTailSlotExceptionLogBudget) >= 0)
        {
            WriteLogFmt(
                "[MountDemonJumpPostTail] exception reason=%s caller=0x%08X mount=%d released=%d addr=0x%08X code=0x%08X",
                reasonTag ? reasonTag : "unknown",
                callerRet,
                mountItemId,
                releasedSkillId,
                static_cast<DWORD>(thisValue + 0x5E8C),
                GetExceptionCode());
        }
        return false;
    }

    MountedDemonJumpLateLocalStateSnapshot afterState = {};
    const bool hasAfterState =
        TryReadMountedDemonJumpLateLocalStateSnapshot(
            thisPtr,
            &afterState) &&
        afterState.valid;

    static LONG s_mountedDemonJumpPostPacketTailSlotClearLogBudget = 32;
    if (InterlockedDecrement(
            &s_mountedDemonJumpPostPacketTailSlotClearLogBudget) >= 0)
    {
        WriteLogFmt(
            "[MountDemonJumpPostTail] clear reason=%s caller=0x%08X mount=%d released=%d slot=0x%08X->0x%08X gate=%u/%u/%u->%u/%u/%u",
            reasonTag ? reasonTag : "unknown",
            callerRet,
            mountItemId,
            releasedSkillId,
            beforeState.state5E8C,
            hasAfterState ? afterState.state5E8C : 0,
            static_cast<unsigned int>(beforeState.state5E84),
            static_cast<unsigned int>(beforeState.state5E85),
            static_cast<unsigned int>(beforeState.state5EE4),
            hasAfterState
                ? static_cast<unsigned int>(afterState.state5E84)
                : 0,
            hasAfterState
                ? static_cast<unsigned int>(afterState.state5E85)
                : 0,
            hasAfterState
                ? static_cast<unsigned int>(afterState.state5EE4)
                : 0);
        WriteMountedDemonJumpLateLocalStateLog(
            "PostTail",
            "before-clear",
            thisPtr,
            beforeState);
        if (hasAfterState)
        {
            WriteMountedDemonJumpLateLocalStateLog(
                "PostTail",
                "after-clear",
                thisPtr,
                afterState);
        }
    }

    return true;
}

static bool TryForceFinalizeMountedDemonJumpNativeActionTailState(
    void *thisPtr,
    int mountItemId,
    const char *reasonTag,
    DWORD callerRet)
{
    if (!thisPtr ||
        mountItemId <= 0 ||
        ResolveMountedRuntimeSkillIdForKind(
            MountedRuntimeSkillKind_DemonJump,
            mountItemId) != 30010110)
    {
        return false;
    }

    MountedDemonJumpLateLocalStateSnapshot beforeState = {};
    if (!TryReadMountedDemonJumpLateLocalStateSnapshot(
            thisPtr,
            &beforeState) ||
        !beforeState.valid)
    {
        static LONG s_mountedDemonJumpNativeTailSkipSnapshotLogBudget = 16;
        if (InterlockedDecrement(
                &s_mountedDemonJumpNativeTailSkipSnapshotLogBudget) >= 0)
        {
            WriteLogFmt(
                "[MountDemonJumpNativeTail] skip reason=%s caller=0x%08X mount=%d this=0x%08X stage=snapshot",
                reasonTag ? reasonTag : "unknown",
                callerRet,
                mountItemId,
                (DWORD)(uintptr_t)thisPtr);
        }
        return false;
    }

    if ((beforeState.state4A4 != 6 &&
         beforeState.state4A4 != 7) ||
        beforeState.state5D7C != 0 ||
        beforeState.state5F2C != 0 ||
        beforeState.state5E84 != 0 ||
        beforeState.state5E85 != 0 ||
        beforeState.state5EE4 != 0 ||
        beforeState.state5E8C != 0)
    {
        static LONG s_mountedDemonJumpNativeTailSkipStateLogBudget = 24;
        if (InterlockedDecrement(
                &s_mountedDemonJumpNativeTailSkipStateLogBudget) >= 0)
        {
            WriteLogFmt(
                "[MountDemonJumpNativeTail] skip reason=%s caller=0x%08X mount=%d this=0x%08X stage=state 4A4=0x%08X lock=%d/%d gate=%u/%u/%u slot=0x%08X",
                reasonTag ? reasonTag : "unknown",
                callerRet,
                mountItemId,
                (DWORD)(uintptr_t)thisPtr,
                static_cast<unsigned int>(beforeState.state4A4),
                beforeState.state5D7C,
                beforeState.state5F2C,
                static_cast<unsigned int>(beforeState.state5E84),
                static_cast<unsigned int>(beforeState.state5E85),
                static_cast<unsigned int>(beforeState.state5EE4),
                beforeState.state5E8C);
        }
        return false;
    }

    tMountedDemonJumpLateStateGetterFn afbStateGetter =
        oMountedDemonJumpAfbState42E170
            ? oMountedDemonJumpAfbState42E170
            : reinterpret_cast<tMountedDemonJumpLateStateGetterFn>(
                  ADDR_MountedDemonJumpAfbState42E170);
    tMountedDemonJumpNativeActionResetFn nativeActionResetFn =
        reinterpret_cast<tMountedDemonJumpNativeActionResetFn>(
            ADDR_MountedDemonJumpNativeActionReset47F1C0);
    if (!afbStateGetter || !nativeActionResetFn)
    {
        static LONG s_mountedDemonJumpNativeTailSkipEntryLogBudget = 12;
        if (InterlockedDecrement(
                &s_mountedDemonJumpNativeTailSkipEntryLogBudget) >= 0)
        {
            WriteLogFmt(
                "[MountDemonJumpNativeTail] skip reason=%s caller=0x%08X mount=%d this=0x%08X stage=entry getter=0x%08X reset=0x%08X",
                reasonTag ? reasonTag : "unknown",
                callerRet,
                mountItemId,
                (DWORD)(uintptr_t)thisPtr,
                (DWORD)(uintptr_t)afbStateGetter,
                (DWORD)(uintptr_t)nativeActionResetFn);
        }
        return false;
    }

    const uintptr_t thisValue = reinterpret_cast<uintptr_t>(thisPtr);
    const uintptr_t nativeActionThisValue = thisValue + 144;
    if (SafeIsBadReadPtr(
            reinterpret_cast<void *>(nativeActionThisValue),
            sizeof(DWORD)) ||
        SafeIsBadWritePtr(
            reinterpret_cast<void *>(thisValue + 0x494),
            sizeof(DWORD)) ||
        SafeIsBadWritePtr(
            reinterpret_cast<void *>(thisValue + 0x4A8),
            sizeof(DWORD)))
    {
        static LONG s_mountedDemonJumpNativeTailSkipPtrLogBudget = 12;
        if (InterlockedDecrement(
                &s_mountedDemonJumpNativeTailSkipPtrLogBudget) >= 0)
        {
            WriteLogFmt(
                "[MountDemonJumpNativeTail] skip reason=%s caller=0x%08X mount=%d this=0x%08X stage=ptrcheck native=0x%08X state1172=0x%08X state1192=0x%08X",
                reasonTag ? reasonTag : "unknown",
                callerRet,
                mountItemId,
                (DWORD)(uintptr_t)thisPtr,
                static_cast<DWORD>(nativeActionThisValue),
                static_cast<DWORD>(thisValue + 0x494),
                static_cast<DWORD>(thisValue + 0x4A8));
        }
        return false;
    }

    int nativeActionStateBefore = 0;
    int state1172Before = 0;
    int state1192Before = 0;
    __try
    {
        nativeActionStateBefore =
            afbStateGetter(reinterpret_cast<void *>(nativeActionThisValue));
        state1172Before =
            *reinterpret_cast<int *>(thisValue + 0x494);
        state1192Before =
            *reinterpret_cast<int *>(thisValue + 0x4A8);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }

    const bool allowExactMounted353TailReset =
        nativeActionStateBefore == 353 &&
        state1172Before == -1 &&
        state1192Before == 353;
    const bool allowExactMounted354TailReset =
        nativeActionStateBefore == 354 &&
        state1172Before == -1 &&
        state1192Before == 354;
    const bool allowExactMounted362TailReset =
        nativeActionStateBefore == 362 &&
        state1172Before == -1 &&
        state1192Before == 362;
    if (nativeActionStateBefore > -1 &&
        !allowExactMounted353TailReset &&
        !allowExactMounted354TailReset &&
        !allowExactMounted362TailReset)
    {
        static LONG s_mountedDemonJumpNativeTailSkipAfbLogBudget = 24;
        if (InterlockedDecrement(
                &s_mountedDemonJumpNativeTailSkipAfbLogBudget) >= 0)
        {
            WriteLogFmt(
                "[MountDemonJumpNativeTail] skip reason=%s caller=0x%08X mount=%d this=0x%08X stage=afb afb=%d state1172=%d state1192=%d 4A4=0x%08X exact353=%d exact354=%d exact362=%d",
                reasonTag ? reasonTag : "unknown",
                callerRet,
                mountItemId,
                (DWORD)(uintptr_t)thisPtr,
                nativeActionStateBefore,
                state1172Before,
                state1192Before,
                static_cast<unsigned int>(beforeState.state4A4),
                allowExactMounted353TailReset ? 1 : 0,
                allowExactMounted354TailReset ? 1 : 0,
                allowExactMounted362TailReset ? 1 : 0);
        }
        return false;
    }

    __try
    {
        *reinterpret_cast<int *>(thisValue + 0x494) = -1;
        *reinterpret_cast<int *>(thisValue + 0x4A8) = -1;
        nativeActionResetFn(
            reinterpret_cast<char *>(nativeActionThisValue),
            1);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        static LONG s_mountedDemonJumpNativeTailExceptionLogBudget = 12;
        if (InterlockedDecrement(
                &s_mountedDemonJumpNativeTailExceptionLogBudget) >= 0)
        {
            WriteLogFmt(
                "[MountDemonJumpNativeTail] exception reason=%s caller=0x%08X mount=%d this=0x%08X code=0x%08X",
                reasonTag ? reasonTag : "unknown",
                callerRet,
                mountItemId,
                static_cast<DWORD>(thisValue),
                GetExceptionCode());
        }
        return false;
    }

    MountedDemonJumpLateLocalStateSnapshot afterState = {};
    const bool hasAfterState =
        TryReadMountedDemonJumpLateLocalStateSnapshot(
            thisPtr,
            &afterState) &&
        afterState.valid;
    int nativeActionStateAfter = 0;
    int state1172After = 0;
    int state1192After = 0;
    bool hasAfterNativeState = false;
    __try
    {
        nativeActionStateAfter =
            afbStateGetter(reinterpret_cast<void *>(nativeActionThisValue));
        state1172After =
            *reinterpret_cast<int *>(thisValue + 0x494);
        state1192After =
            *reinterpret_cast<int *>(thisValue + 0x4A8);
        hasAfterNativeState = true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        hasAfterNativeState = false;
    }

    static LONG s_mountedDemonJumpNativeTailResetLogBudget = 24;
    if (InterlockedDecrement(
            &s_mountedDemonJumpNativeTailResetLogBudget) >= 0)
    {
        WriteLogFmt(
            "[MountDemonJumpNativeTail] reset reason=%s caller=0x%08X mount=%d this=0x%08X mode=%s afb=%d->%d state1172=%d->%d state1192=%d->%d 4A4=0x%08X->0x%08X",
            reasonTag ? reasonTag : "unknown",
            callerRet,
            mountItemId,
            static_cast<DWORD>(thisValue),
            allowExactMounted353TailReset
                ? "exact353"
                : (allowExactMounted354TailReset
                       ? "exact354"
                       : (allowExactMounted362TailReset
                              ? "exact362"
                              : "idle")),
            nativeActionStateBefore,
            hasAfterNativeState ? nativeActionStateAfter : 0,
            state1172Before,
            hasAfterNativeState ? state1172After : 0,
            state1192Before,
            hasAfterNativeState ? state1192After : 0,
            static_cast<unsigned int>(beforeState.state4A4),
            hasAfterState
                ? static_cast<unsigned int>(afterState.state4A4)
                : 0U);
        WriteMountedDemonJumpLateLocalStateLog(
            "NativeTail",
            "before-reset",
            thisPtr,
            beforeState);
        if (hasAfterState)
        {
            WriteMountedDemonJumpLateLocalStateLog(
                "NativeTail",
                "after-reset",
                thisPtr,
                afterState);
        }
    }

    return true;
}

static bool TryClearMountedDemonJump35121005CarrierTailState(
    void *thisPtr,
    int mountItemId,
    const char *reasonTag,
    DWORD callerRet)
{
    if (!thisPtr ||
        mountItemId <= 0 ||
        ResolveMountedRuntimeSkillIdForKind(
            MountedRuntimeSkillKind_DemonJump,
            mountItemId) != 30010110)
    {
        return false;
    }

    MountedDemonJumpLateLocalStateSnapshot beforeState = {};
    if (!TryReadMountedDemonJumpLateLocalStateSnapshot(
            thisPtr,
            &beforeState) ||
        !beforeState.valid)
    {
        return false;
    }

    if (beforeState.state5D7C != 0 ||
        beforeState.state5F2C != 0 ||
        beforeState.state5E84 != 0 ||
        beforeState.state5E85 != 0 ||
        beforeState.state5EE4 != 0)
    {
        return false;
    }

    const uintptr_t thisValue = reinterpret_cast<uintptr_t>(thisPtr);
    const uintptr_t carrierStateAddr = thisValue + 4989 * sizeof(DWORD);
    if (SafeIsBadReadPtr(
            reinterpret_cast<void *>(carrierStateAddr),
            7 * sizeof(DWORD)))
    {
        return false;
    }

    tMountedDemonJumpLateStateGetterFn afbStateGetter =
        oMountedDemonJumpAfbState42E170
            ? oMountedDemonJumpAfbState42E170
            : reinterpret_cast<tMountedDemonJumpLateStateGetterFn>(
                  ADDR_MountedDemonJumpAfbState42E170);
    tMountedDemonJumpCarrierResetFn carrierResetFn =
        reinterpret_cast<tMountedDemonJumpCarrierResetFn>(
            ADDR_MountedDemonJumpCarrierResetB22AE0);
    if (!afbStateGetter || !carrierResetFn)
    {
        return false;
    }

    int nativeActionStateBefore = 0;
    int state1192Before = 0;
    int carrier4989Before = 0;
    int carrier4990Before = 0;
    int carrier4991Before = 0;
    int carrier4992Before = 0;
    int carrier4993Before = 0;
    int carrier4994Before = 0;
    int carrier4995Before = 0;
    __try
    {
        nativeActionStateBefore =
            afbStateGetter(reinterpret_cast<void *>(thisValue + 144));
        state1192Before =
            *reinterpret_cast<int *>(thisValue + 0x4A8);
        carrier4989Before =
            *reinterpret_cast<int *>(thisValue + 4989 * sizeof(DWORD));
        carrier4990Before =
            *reinterpret_cast<int *>(thisValue + 4990 * sizeof(DWORD));
        carrier4991Before =
            *reinterpret_cast<int *>(thisValue + 4991 * sizeof(DWORD));
        carrier4992Before =
            *reinterpret_cast<int *>(thisValue + 4992 * sizeof(DWORD));
        carrier4993Before =
            *reinterpret_cast<int *>(thisValue + 4993 * sizeof(DWORD));
        carrier4994Before =
            *reinterpret_cast<int *>(thisValue + 4994 * sizeof(DWORD));
        carrier4995Before =
            *reinterpret_cast<int *>(thisValue + 4995 * sizeof(DWORD));
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }

    if (nativeActionStateBefore != 353 ||
        state1192Before != 353 ||
        carrier4989Before != 35121005 ||
        carrier4993Before != 0)
    {
        static LONG s_mountedDemonJumpCarrierTailSkipLogBudget = 24;
        if (InterlockedDecrement(
                &s_mountedDemonJumpCarrierTailSkipLogBudget) >= 0)
        {
            WriteLogFmt(
                "[MountDemonJumpCarrierTail] skip reason=%s caller=0x%08X mount=%d this=0x%08X afb=%d state1192=%d carrier=%d/%d/%d/%d/%d/%d/%d",
                reasonTag ? reasonTag : "unknown",
                callerRet,
                mountItemId,
                static_cast<DWORD>(thisValue),
                nativeActionStateBefore,
                state1192Before,
                carrier4989Before,
                carrier4990Before,
                carrier4991Before,
                carrier4992Before,
                carrier4993Before,
                carrier4994Before,
                carrier4995Before);
        }
        return false;
    }

    __try
    {
        carrierResetFn(reinterpret_cast<int *>(thisPtr));
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        static LONG s_mountedDemonJumpCarrierTailExceptionLogBudget = 12;
        if (InterlockedDecrement(
                &s_mountedDemonJumpCarrierTailExceptionLogBudget) >= 0)
        {
            WriteLogFmt(
                "[MountDemonJumpCarrierTail] exception reason=%s caller=0x%08X mount=%d this=0x%08X code=0x%08X",
                reasonTag ? reasonTag : "unknown",
                callerRet,
                mountItemId,
                static_cast<DWORD>(thisValue),
                GetExceptionCode());
        }
        return false;
    }

    int carrier4989After = 0;
    int carrier4990After = 0;
    int carrier4991After = 0;
    int carrier4992After = 0;
    int carrier4993After = 0;
    int carrier4994After = 0;
    int carrier4995After = 0;
    __try
    {
        carrier4989After =
            *reinterpret_cast<int *>(thisValue + 4989 * sizeof(DWORD));
        carrier4990After =
            *reinterpret_cast<int *>(thisValue + 4990 * sizeof(DWORD));
        carrier4991After =
            *reinterpret_cast<int *>(thisValue + 4991 * sizeof(DWORD));
        carrier4992After =
            *reinterpret_cast<int *>(thisValue + 4992 * sizeof(DWORD));
        carrier4993After =
            *reinterpret_cast<int *>(thisValue + 4993 * sizeof(DWORD));
        carrier4994After =
            *reinterpret_cast<int *>(thisValue + 4994 * sizeof(DWORD));
        carrier4995After =
            *reinterpret_cast<int *>(thisValue + 4995 * sizeof(DWORD));
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        carrier4989After = 0;
        carrier4990After = 0;
        carrier4991After = 0;
        carrier4992After = 0;
        carrier4993After = 0;
        carrier4994After = 0;
        carrier4995After = 0;
    }

    static LONG s_mountedDemonJumpCarrierTailClearLogBudget = 24;
    if (InterlockedDecrement(
            &s_mountedDemonJumpCarrierTailClearLogBudget) >= 0)
    {
        WriteLogFmt(
            "[MountDemonJumpCarrierTail] clear reason=%s caller=0x%08X mount=%d this=0x%08X afb=%d state1192=%d carrier=%d/%d/%d/%d/%d/%d/%d -> %d/%d/%d/%d/%d/%d/%d",
            reasonTag ? reasonTag : "unknown",
            callerRet,
            mountItemId,
            static_cast<DWORD>(thisValue),
            nativeActionStateBefore,
            state1192Before,
            carrier4989Before,
            carrier4990Before,
            carrier4991Before,
            carrier4992Before,
            carrier4993Before,
            carrier4994Before,
            carrier4995Before,
            carrier4989After,
            carrier4990After,
            carrier4991After,
            carrier4992After,
            carrier4993After,
            carrier4994After,
            carrier4995After);
        WriteMountedDemonJumpLateLocalStateLog(
            "CarrierTail",
            "before-clear",
            thisPtr,
            beforeState);
    }

    return true;
}

static bool TryForceClearMountedDemonJumpDescendingUpState(
    void *thisPtr,
    int mountItemId,
    int rootSkillId,
    int currentSkillId,
    const char *reasonTag,
    DWORD callerRet)
{
    if (!thisPtr ||
        mountItemId <= 0 ||
        ResolveMountedRuntimeSkillIdForKind(
            MountedRuntimeSkillKind_DemonJump,
            mountItemId) != 30010110)
    {
        return false;
    }

    const int effectiveChildSkillId =
        ResolveMountedDemonJumpLateEffectiveChildSkillId(
            mountItemId,
            rootSkillId,
            currentSkillId);
    if (effectiveChildSkillId != 30010183)
    {
        return false;
    }

    // v23.08 runtime evidence:
    // recent-child cache can still hold 30010183 while the live context for the
    // current trigger is only root/current=30010110/30010110. If we let the
    // descending-up cleanup consume that cached child, B1DB10 clears the gate
    // immediately after the first a2=18 step and the fresh up-jump never
    // reaches the later B22630 child-observed stage. Keep this cleanup narrow
    // to the real "stuck in up-child tail" case where the live context itself
    // is already the up child.
    if (currentSkillId != 30010183)
    {
        return false;
    }

    void *userLocal = nullptr;
    if (!TryReadCurrentUserLocalPtr(&userLocal) || !userLocal)
    {
        return false;
    }

    MountedDemonJumpUserGateSnapshot beforeGate = {};
    if (!TryReadMountedDemonJumpUserGateSnapshot(
            userLocal,
            &beforeGate) ||
        !beforeGate.valid)
    {
        return false;
    }

    // Evidence from v21.99 logs:
    // the "can't move" / "can't immediately retrigger" tail starts once the
    // local release latch flips to gate=3/down=0/up=1 while root/current still
    // remain pinned to 30010110/30010183. Only force-clear on that exact
    // descending tail, after the transient local lock bits are already down.
    if (beforeGate.upLatch == 0)
    {
        return false;
    }

    MountedDemonJumpLateLocalStateSnapshot beforeLocalState = {};
    if (!TryReadMountedDemonJumpLateLocalStateSnapshot(
            thisPtr,
            &beforeLocalState) ||
        !beforeLocalState.valid)
    {
        return false;
    }

    if (beforeLocalState.state5D7C != 0 ||
        beforeLocalState.state5F2C != 0)
    {
        return false;
    }

    MountedDemonJumpUserGateSnapshot afterGate = {};
    const bool clearedGateState =
        TryClearMountedDemonJumpUserGateState(
            userLocal,
            &beforeGate,
            &afterGate);

    int afterRootSkillId = 0;
    int afterCurrentSkillId = 0;
    TryReadMountedDemonJumpContextState(
        &afterRootSkillId,
        &afterCurrentSkillId,
        nullptr);

    static LONG s_mountedDemonJumpDescendingUpClearLogBudget = 48;
    if (InterlockedDecrement(
            &s_mountedDemonJumpDescendingUpClearLogBudget) >= 0)
    {
        WriteLogFmt(
            "[MountDemonJumpDescending] clear reason=%s caller=0x%08X mount=%d root=%d current=%d gate=%u/%u/%u->%u/%u/%u context=%d/%d->%d/%d preserveTransient=1",
            reasonTag ? reasonTag : "unknown",
            callerRet,
            mountItemId,
            rootSkillId,
            currentSkillId,
            static_cast<unsigned int>(beforeGate.gateMode),
            static_cast<unsigned int>(beforeGate.downLatch),
            static_cast<unsigned int>(beforeGate.upLatch),
            static_cast<unsigned int>(afterGate.gateMode),
            static_cast<unsigned int>(afterGate.downLatch),
            static_cast<unsigned int>(afterGate.upLatch),
            rootSkillId,
            currentSkillId,
            afterRootSkillId,
            afterCurrentSkillId);
        WriteMountedDemonJumpLateLocalStateLog(
            "DescendingUp",
            "before-clear",
            thisPtr,
            beforeLocalState);
    }

    if (clearedGateState)
    {
        TryForceClearMountedSoaringFlightActiveForMount(
            mountItemId,
            reasonTag ? reasonTag : "descending-up");
    }

    return clearedGateState;
}
static bool TryForceFinalizeMountedDemonJumpUpStateAfterPacket(
    void *thisPtr,
    int mountItemId,
    int releasedSkillId,
    const char *reasonTag,
    DWORD callerRet)
{
    if (!thisPtr ||
        mountItemId <= 0 ||
        (releasedSkillId != 30010183 &&
         releasedSkillId != 30010184 &&
         releasedSkillId != 30010186) ||
        ResolveMountedRuntimeSkillIdForKind(
            MountedRuntimeSkillKind_DemonJump,
            mountItemId) != 30010110)
    {
        return false;
    }

    void *userLocal = nullptr;
    if (!TryReadCurrentUserLocalPtr(&userLocal) || !userLocal)
    {
        return false;
    }

    MountedDemonJumpUserGateSnapshot beforeGate = {};
    if (!TryReadMountedDemonJumpUserGateSnapshot(
            userLocal,
            &beforeGate) ||
        !beforeGate.valid)
    {
        return false;
    }

    MountedDemonJumpLateLocalStateSnapshot beforeLocalState = {};
    if (!TryReadMountedDemonJumpLateLocalStateSnapshot(
            thisPtr,
            &beforeLocalState) ||
        !beforeLocalState.valid)
    {
        return false;
    }

    // Evidence from v22.03-v22.05 logs:
    // B28A00 succeeds for mounted child release only after the descending/helper
    // path has already dropped the local gate to 0/0/0 and the late local lock
    // bits are down. Clearing root/current any earlier breaks the packet path,
    // while keeping them after this point causes the residual "can't move /
    // can't retrigger" tail, especially for 30010184 side-child.
    if (beforeGate.gateMode != 0 ||
        beforeGate.downLatch != 0 ||
        beforeGate.upLatch != 0 ||
        beforeLocalState.state5D7C != 0 ||
        beforeLocalState.state5F2C != 0)
    {
        return false;
    }

    int beforeRootSkillId = 0;
    int beforeCurrentSkillId = 0;
    if (!TryReadMountedDemonJumpContextState(
            &beforeRootSkillId,
            &beforeCurrentSkillId,
            nullptr) ||
        beforeRootSkillId != 30010110 ||
        ResolveMountedDemonJumpLateEffectiveChildSkillId(
            mountItemId,
            beforeRootSkillId,
            beforeCurrentSkillId) != releasedSkillId)
    {
        return false;
    }

    const uintptr_t userLocalAddr = reinterpret_cast<uintptr_t>(userLocal);
    const uintptr_t contextAddr =
        userLocalAddr + kMountedDemonJumpContextOffset;
    const uintptr_t rootSkillAddr =
        userLocalAddr + kMountedDemonJumpContextRootSkillOffset;
    const uintptr_t readyFlagAddr =
        userLocalAddr + kMountedDemonJumpReadyFlagOffset;
    if (SafeIsBadWritePtr(
            reinterpret_cast<void *>(contextAddr),
            sizeof(DWORD)) ||
        SafeIsBadWritePtr(
            reinterpret_cast<void *>(rootSkillAddr),
            sizeof(DWORD)) ||
        SafeIsBadWritePtr(
            reinterpret_cast<void *>(readyFlagAddr),
            sizeof(BYTE)))
    {
        return false;
    }

    __try
    {
        *reinterpret_cast<DWORD *>(contextAddr) = 0;
        *reinterpret_cast<int *>(rootSkillAddr) = 0;
        *reinterpret_cast<BYTE *>(readyFlagAddr) = 0;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }

    ClearMountedDemonJumpTransientRuntimeState(
        mountItemId,
        reasonTag ? reasonTag : "B28A00-up-postpacket");
    RememberMountedDemonJumpPostPacketVisualChildSkill(
        mountItemId,
        releasedSkillId,
        reasonTag ? reasonTag : "B28A00-up-postpacket");

    int afterRootSkillId = 0;
    int afterCurrentSkillId = 0;
    const bool hasAfterContext =
        TryReadMountedDemonJumpContextState(
            &afterRootSkillId,
            &afterCurrentSkillId,
            nullptr);

    static LONG s_mountedDemonJumpUpPostPacketClearLogBudget = 48;
    if (InterlockedDecrement(
            &s_mountedDemonJumpUpPostPacketClearLogBudget) >= 0)
    {
        WriteLogFmt(
            "[MountDemonJumpPostPacket] clear reason=%s caller=0x%08X mount=%d released=%d gate=%u/%u/%u root=%d/%d->%d/%d",
            reasonTag ? reasonTag : "unknown",
            callerRet,
            mountItemId,
            releasedSkillId,
            static_cast<unsigned int>(beforeGate.gateMode),
            static_cast<unsigned int>(beforeGate.downLatch),
            static_cast<unsigned int>(beforeGate.upLatch),
            beforeRootSkillId,
            beforeCurrentSkillId,
            hasAfterContext ? afterRootSkillId : 0,
            hasAfterContext ? afterCurrentSkillId : 0);
        WriteMountedDemonJumpLateLocalStateLog(
            "PostPacket",
            "before-clear",
            thisPtr,
            beforeLocalState);
    }

    if (releasedSkillId == 30010183 ||
        releasedSkillId == 30010186)
    {
        TryForceClearMountedSoaringFlightActiveForMount(
            mountItemId,
            reasonTag ? reasonTag : "B28A00-up-postpacket");
    }

    const bool clearedPostPacketTailSlot =
        TryForceClearMountedDemonJumpPostPacketTailSlotState(
        thisPtr,
        mountItemId,
        releasedSkillId,
        reasonTag ? reasonTag : "B28A00-up-postpacket",
        callerRet,
        &beforeLocalState);

    if (clearedPostPacketTailSlot &&
        releasedSkillId == 30010183)
    {
        TryForceFinalizeMountedDemonJumpNativeActionTailState(
            thisPtr,
            mountItemId,
            "B28A00-up-postpacket-native-action",
            callerRet);
        TryClearMountedDemonJump35121005CarrierTailState(
            thisPtr,
            mountItemId,
            "B28A00-up-postpacket-carrier-tail",
            callerRet);
    }

    return true;
}

static bool TryReadMountedDemonJumpPrimePlayerObject(void **playerObjOut)
{
    if (playerObjOut)
    {
        *playerObjOut = nullptr;
    }

    const uintptr_t holderAddr = 0x00F5E5D0;
    if (SafeIsBadReadPtr(
            reinterpret_cast<void *>(holderAddr),
            sizeof(DWORD)))
    {
        return false;
    }

    __try
    {
        const uintptr_t holderValue =
            *reinterpret_cast<const DWORD *>(holderAddr);
        if (!holderValue ||
            SafeIsBadReadPtr(
                reinterpret_cast<void *>(holderValue + 8),
                sizeof(DWORD)))
        {
            return false;
        }

        void *playerObj =
            *reinterpret_cast<void **>(holderValue + 8);
        if (!playerObj ||
            SafeIsBadReadPtr(
                playerObj,
                0x5F30))
        {
            return false;
        }

        if (playerObjOut)
        {
            *playerObjOut = playerObj;
        }
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        if (playerObjOut)
        {
            *playerObjOut = nullptr;
        }
        return false;
    }
}

static const char *DescribeMountedDemonJumpKeyState7BECF0Caller(DWORD callerRet)
{
    switch (callerRet)
    {
    case 0x00B1DFC5:
        return "B1DB10-main";
    case 0x00B1E228:
        return "B1DB10-tail";
    case 0x00B1CB2A:
        return "B1C9E0-main";
    case 0x00B1CFC8:
        return "B1C9E0-tail";
    default:
        return nullptr;
    }
}

