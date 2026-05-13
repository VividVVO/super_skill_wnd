static bool ShouldMountedDoubleJumpBypassReturnZero(DWORD returnAddr)
{
    return returnAddr == 0x00B1BBAA ||
           returnAddr == 0x00B1C1C5;
}

static bool ShouldMountedDoubleJumpBypassReturnOne(DWORD returnAddr)
{
    return returnAddr == 0x00ADF02B ||
           returnAddr == 0x00B1BFA7 ||
           returnAddr == 0x00B1CCC7 ||
           returnAddr == 0x00B1D0B8;
}

static bool ShouldMountedDemonJumpForceReleaseFallthrough(DWORD returnAddr)
{
    // Mounted demon jump only survives the native late path when these
    // 42DE20 callers see zero and stay on their "continue" edge.
    return returnAddr == 0x00ADF02B ||
           returnAddr == 0x00B1BFA7 ||
           returnAddr == 0x00B1CCC7 ||
           returnAddr == 0x00B1D0B8;
}

static bool ShouldMountedDemonJumpNoIntentReleaseFallback(DWORD returnAddr)
{
    // Keep the no-intent fallback narrow to the two documented release callers.
    return returnAddr == 0x00B1BFA7 ||
           returnAddr == 0x00B1CCC7;
}

static int __fastcall hkMountedStateGate42DE20(void *thisPtr, void * /*edxUnused*/)
{
    int result = oMountedStateGate42DE20
                     ? oMountedStateGate42DE20(thisPtr)
                     : 0;
    if (!kEnableMountedDoubleJumpRuntimeHooks &&
        !kEnableMountedDemonJumpRuntimeHooks)
    {
        return result;
    }
    if (result <= 0)
    {
        return result;
    }

    const DWORD callerRet = (DWORD)(uintptr_t)_ReturnAddress();

    int mountItemId = 0;
    bool resolvedDoubleJumpMount = TryResolveMountedDoubleJumpMountItemIdWithFallback(
        thisPtr,
        &mountItemId,
        nullptr,
        1200);
    if (!resolvedDoubleJumpMount &&
        kEnableMountedDemonJumpRuntimeHooks)
    {
        resolvedDoubleJumpMount = TryResolveMountedDemonJumpMountItemIdWithFallback(
            thisPtr,
            &mountItemId,
            nullptr,
            1200);
    }
    if (!resolvedDoubleJumpMount)
    {
        return result;
    }

    if (ShouldMountedDoubleJumpBypassReturnZero(callerRet) ||
        ShouldMountedDoubleJumpBypassReturnOne(callerRet))
    {
        ObserveMountedDoubleJumpIntent(mountItemId);
    }

    const int mountedDoubleJumpSkillId =
        SkillOverlayBridgeResolveMountedDoubleJumpSkillId(mountItemId);
    const int mountedDemonJumpSkillId =
        kEnableMountedDemonJumpRuntimeHooks
            ? SkillOverlayBridgeResolveMountedDemonJumpSkillId(mountItemId)
            : 0;
    if (mountedDoubleJumpSkillId <= 0 && mountedDemonJumpSkillId <= 0)
    {
        return result;
    }

    const bool hasRecentMountedDoubleJumpIntent =
        mountedDoubleJumpSkillId > 0 &&
        HasRecentMountedDoubleJumpIntent(mountItemId, 250);
    const bool hasRecentMountedDemonJumpIntent =
        mountedDemonJumpSkillId > 0 &&
        HasRecentMountedDemonJumpIntent(mountItemId, 250);
    int demonContextSkillId = 0;
    int demonContextRootSkillId = 0;
    bool hasMountedDemonContext = false;
    if (mountedDemonJumpSkillId > 0 && hasRecentMountedDemonJumpIntent)
    {
        PrimeMountedDemonJumpContextIfNeeded(
            mountItemId,
            "42DE20",
            &demonContextSkillId);
        hasMountedDemonContext = HasMountedDemonJumpContextPrimedForMount(
            mountItemId,
            &demonContextSkillId,
            &demonContextRootSkillId);
    }
    static DWORD s_lastMountedStateGateObserveLogTick = 0;
    const DWORD nowTick = GetTickCount();
    if ((mountedDoubleJumpSkillId > 0 || mountedDemonJumpSkillId > 0) &&
        nowTick - s_lastMountedStateGateObserveLogTick > 250)
    {
        s_lastMountedStateGateObserveLogTick = nowTick;
        WriteLogFmt(
            "[MountStateGate] 42DE20 caller=0x%08X mount=%d native=%d forceOne=%d doubleSkill=%d demonSkill=%d doubleIntent=%d demonIntent=%d demonRoot=%d demonCurrent=%d",
            callerRet,
            mountItemId,
            result,
            ShouldMountedDoubleJumpBypassReturnOne(callerRet) ? 1 : 0,
            mountedDoubleJumpSkillId,
            mountedDemonJumpSkillId,
            hasRecentMountedDoubleJumpIntent ? 1 : 0,
            hasRecentMountedDemonJumpIntent ? 1 : 0,
            hasMountedDemonContext ? demonContextRootSkillId : 0,
            hasMountedDemonContext ? demonContextSkillId : 0);
    }

    if (mountedDemonJumpSkillId > 0 &&
        !hasRecentMountedDemonJumpIntent &&
        ShouldMountedDemonJumpForceReleaseFallthrough(callerRet))
    {
        ObserveMountedDemonJumpGateProbe(
            mountItemId,
            "42DE20",
            callerRet);
    }
    if (mountedDemonJumpSkillId > 0 &&
        !hasRecentMountedDoubleJumpIntent &&
        !hasRecentMountedDemonJumpIntent &&
        ShouldMountedDemonJumpNoIntentReleaseFallback(callerRet))
    {
        ObserveMountedDemonJumpGateProbe(
            mountItemId,
            "42DE20-no-intent",
            callerRet);
        ObserveMountedDemonJumpIntent(
            mountItemId,
            "42DE20-no-intent");
        PrimeMountedDemonJumpContextIfNeeded(
            mountItemId,
            "42DE20-no-intent",
            &demonContextSkillId);
        hasMountedDemonContext = HasMountedDemonJumpContextPrimedForMount(
            mountItemId,
            &demonContextSkillId,
            &demonContextRootSkillId);

        static LONG s_mountedStateGateDemonNoIntentFallbackLogBudget = 24;
        const LONG budgetAfterDecrement =
            InterlockedDecrement(
                &s_mountedStateGateDemonNoIntentFallbackLogBudget);
        if (budgetAfterDecrement >= 0)
        {
            WriteLogFmt(
                "[MountStateGate] 42DE20 demon no-intent fallback caller=0x%08X mount=%d native=%d root=%d current=%d context=%d -> forceZero",
                callerRet,
                mountItemId,
                result,
                hasMountedDemonContext ? demonContextRootSkillId : 0,
                hasMountedDemonContext ? demonContextSkillId : 0,
                hasMountedDemonContext ? 1 : 0);
        }
        return 0;
    }
    if (!hasRecentMountedDoubleJumpIntent && !hasRecentMountedDemonJumpIntent)
    {
        return result;
    }

    if (!hasRecentMountedDoubleJumpIntent &&
        hasRecentMountedDemonJumpIntent &&
        ShouldMountedDemonJumpForceReleaseFallthrough(callerRet))
    {
        static LONG s_mountedStateGateDemonFallthroughLogBudget = 24;
        const LONG budgetAfterDecrement =
            InterlockedDecrement(&s_mountedStateGateDemonFallthroughLogBudget);
        if (budgetAfterDecrement >= 0)
        {
            WriteLogFmt(
                "[MountStateGate] 42DE20 demon release fallthrough caller=0x%08X mount=%d native=%d root=%d current=%d context=%d -> forceZero",
                callerRet,
                mountItemId,
                result,
                hasMountedDemonContext ? demonContextRootSkillId : 0,
                hasMountedDemonContext ? demonContextSkillId : 0,
                hasMountedDemonContext ? 1 : 0);
        }
        return 0;
    }

    const int forcedResult =
        ShouldMountedDoubleJumpBypassReturnOne(callerRet) ? 1 : 0;

    return forcedResult;
}
