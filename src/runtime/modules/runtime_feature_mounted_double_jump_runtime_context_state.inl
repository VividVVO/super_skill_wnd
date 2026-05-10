static int __fastcall hkMountedDemonJumpKeyState7BECF0(
    void *thisPtr,
    void * /*edxUnused*/,
    unsigned int keyCode,
    unsigned int active)
{
    const DWORD callerRet = (DWORD)(uintptr_t)_ReturnAddress();
    const char *traceStage =
        DescribeMountedDemonJumpKeyState7BECF0Caller(callerRet);

    int mountItemId = 0;
    int rootSkillId = 0;
    int currentSkillId = 0;
    bool hasRecentIntent = false;
    const bool shouldTrace =
        traceStage &&
        ShouldObserveMountedDemonJumpLatePath(
            &mountItemId,
            &rootSkillId,
            &currentSkillId,
            &hasRecentIntent);

    void *userLocal = nullptr;
    BYTE beforeGateMode = 0;
    BYTE beforeDownLatch = 0;
    BYTE beforeUpLatch = 0;
    MountedDemonJumpLateLocalStateSnapshot beforeLocalState = {};
    bool hasBeforeLocalState = false;
    if (shouldTrace && TryReadCurrentUserLocalPtr(&userLocal) && userLocal)
    {
        const uintptr_t userLocalAddr =
            reinterpret_cast<uintptr_t>(userLocal);
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
        hasBeforeLocalState = TryReadMountedDemonJumpLateLocalStateSnapshot(
            userLocal,
            &beforeLocalState);
    }

    const int result = oMountedDemonJumpKeyState7BECF0
                           ? oMountedDemonJumpKeyState7BECF0(
                                 thisPtr,
                                 keyCode,
                                 active)
                           : 0;

    if (shouldTrace)
    {
        int afterRootSkillId = 0;
        int afterCurrentSkillId = 0;
        TryReadMountedDemonJumpContextState(
            &afterRootSkillId,
            &afterCurrentSkillId,
            nullptr);

        BYTE afterGateMode = 0;
        BYTE afterDownLatch = 0;
        BYTE afterUpLatch = 0;
        MountedDemonJumpLateLocalStateSnapshot afterLocalState = {};
        bool hasAfterLocalState = false;
        if (userLocal)
        {
            const uintptr_t userLocalAddr =
                reinterpret_cast<uintptr_t>(userLocal);
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
            hasAfterLocalState = TryReadMountedDemonJumpLateLocalStateSnapshot(
                userLocal,
                &afterLocalState);
        }

        static LONG s_mountedDemonJumpKeyState7BECF0LogBudget = 96;
        if (InterlockedDecrement(&s_mountedDemonJumpKeyState7BECF0LogBudget) >= 0)
        {
            WriteLogFmt(
                "[MountDemonJumpLateGate] 7BECF0 caller=0x%08X stage=%s key=%u active=%u result=%d mount=%d recent=%d root=%d->%d current=%d->%d gate=%u/%u/%u->%u/%u/%u",
                callerRet,
                traceStage,
                keyCode,
                active,
                result,
                mountItemId,
                hasRecentIntent ? 1 : 0,
                rootSkillId,
                afterRootSkillId,
                currentSkillId,
                afterCurrentSkillId,
                static_cast<unsigned int>(beforeGateMode),
                static_cast<unsigned int>(beforeDownLatch),
                static_cast<unsigned int>(beforeUpLatch),
                static_cast<unsigned int>(afterGateMode),
                static_cast<unsigned int>(afterDownLatch),
                static_cast<unsigned int>(afterUpLatch));
            if (hasBeforeLocalState && beforeLocalState.valid && userLocal)
            {
                WriteMountedDemonJumpLateLocalStateLog(
                    "7BECF0",
                    "enter",
                    userLocal,
                    beforeLocalState);
            }
            if (hasAfterLocalState && afterLocalState.valid && userLocal)
            {
                WriteMountedDemonJumpLateLocalStateLog(
                    "7BECF0",
                    "leave",
                    userLocal,
                    afterLocalState);
            }
        }
    }

    int finalResult = result;
    if (result != 0 &&
        active != 0 &&
        (callerRet == 0x00B1DFC5 || callerRet == 0x00B1CB2A) &&
        mountItemId > 0 &&
        rootSkillId == 30010110 &&
        hasRecentIntent)
    {
        int effectiveCurrentSkillId = currentSkillId;
        if (!IsMountedDemonJumpRuntimeChildSkillId(effectiveCurrentSkillId))
        {
            int recentChildSkillId = 0;
            if (TryGetRecentMountedDemonJumpNativeChildSkill(
                    mountItemId,
                    &recentChildSkillId,
                    nullptr,
                    1200))
            {
                effectiveCurrentSkillId = recentChildSkillId;
            }
        }

        if (IsMountedDemonJumpRuntimeChildSkillId(effectiveCurrentSkillId))
        {
            finalResult = 0;
            static LONG s_mountedDemonJumpKeyStateMainForceZeroLogBudget = 32;
            if (InterlockedDecrement(
                    &s_mountedDemonJumpKeyStateMainForceZeroLogBudget) >= 0)
            {
                WriteLogFmt(
                    "[MountDemonJumpLateGate] 7BECF0 main force zero caller=0x%08X stage=%s key=%u active=%u mount=%d root=%d current=%d result=%d->0",
                    callerRet,
                    traceStage ? traceStage : "unknown",
                    keyCode,
                    active,
                    mountItemId,
                    rootSkillId,
                    effectiveCurrentSkillId,
                    result);
            }
        }
    }

    return finalResult;
}

static int __fastcall hkMountedDemonJumpRequirementBF65C0(
    void *thisPtr,
    void * /*edxUnused*/,
    int skillId)
{
    const DWORD callerRet = (DWORD)(uintptr_t)_ReturnAddress();
    int mountItemId = 0;
    int rootSkillId = 0;
    int currentSkillId = 0;
    if (TryResolveMountedDemonJumpRequirementBypassContext(
            callerRet,
            skillId,
            &mountItemId,
            &rootSkillId,
            &currentSkillId))
    {
        static LONG s_mountedDemonJumpRequirementBypassLogBudget = 32;
        if (InterlockedDecrement(
                &s_mountedDemonJumpRequirementBypassLogBudget) >= 0)
        {
            WriteLogFmt(
                "[MountDemonJumpReq] BF65C0 bypass caller=0x%08X skill=%d mount=%d root=%d current=%d",
                callerRet,
                skillId,
                mountItemId,
                rootSkillId,
                currentSkillId);
        }
        return 0;
    }

    const int result = oMountedDemonJumpRequirementBF65C0
                           ? oMountedDemonJumpRequirementBF65C0(
                                 thisPtr,
                                 skillId)
                           : 0;

    if (callerRet == 0x00B29F1E &&
        IsMountedDemonJumpRuntimeChildSkillId(skillId) &&
        result != 0)
    {
        static LONG s_mountedDemonJumpRequirementObserveLogBudget = 24;
        if (InterlockedDecrement(
                &s_mountedDemonJumpRequirementObserveLogBudget) >= 0)
        {
            WriteLogFmt(
                "[MountDemonJumpReq] BF65C0 native fail caller=0x%08X skill=%d result=%d",
                callerRet,
                skillId,
                result);
        }
    }

    return result;
}

