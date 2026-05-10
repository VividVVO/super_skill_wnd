static bool TryCollectMountedDemonJumpPrelocalGateTraceContext(
    int *runtimeSkillIdOut,
    int *mountItemIdOut,
    int *rootSkillIdOut,
    int *currentSkillIdOut,
    bool *recentIntentOut)
{
    if (runtimeSkillIdOut)
    {
        *runtimeSkillIdOut = 0;
    }
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

    int runtimeSkillId = 0;
    int mountItemId = 0;
    const bool hasFreshTrace = IsMountedDemonJumpCrashTraceFresh(
        &runtimeSkillId,
        &mountItemId,
        2500);

    int rootSkillId = 0;
    int currentSkillId = 0;
    bool hasRecentIntent = false;
    const bool shouldObserve = ShouldObserveMountedDemonJumpLatePath(
        &mountItemId,
        &rootSkillId,
        &currentSkillId,
        &hasRecentIntent);

    if (!hasFreshTrace && !shouldObserve)
    {
        return false;
    }

    if (!IsMountedDemonJumpRuntimeChildSkillId(runtimeSkillId))
    {
        if (IsMountedDemonJumpRuntimeChildSkillId(currentSkillId))
        {
            runtimeSkillId = currentSkillId;
        }
        else if (mountItemId > 0)
        {
            int recentChildSkillId = 0;
            if (TryGetRecentMountedDemonJumpNativeChildSkill(
                    mountItemId,
                    &recentChildSkillId,
                    nullptr,
                    1200))
            {
                runtimeSkillId = recentChildSkillId;
            }
        }
    }

    if (runtimeSkillIdOut)
    {
        *runtimeSkillIdOut = runtimeSkillId;
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

static bool ShouldForceMountedDemonJumpPrelocalGateZero(
    const char *hookTag,
    int runtimeSkillId,
    int mountItemId,
    int rootSkillId,
    int currentSkillId,
    bool hasRecentIntent,
    int originalResult)
{
    if (!hookTag ||
        strcmp(hookTag, "7DC710") != 0 ||
        originalResult == 0 ||
        mountItemId <= 0 ||
        !IsMountedDemonJumpRuntimeChildSkillId(runtimeSkillId) ||
        ResolveMountedRuntimeSkillIdForKind(
            MountedRuntimeSkillKind_DemonJump,
            mountItemId) != 30010110)
    {
        return false;
    }

    // v21.74 logs prove that forcing 7DC710 from 1 -> 0 no longer unlocks the
    // real local leap path: B273B0/B29C70/B28A00 still run, but the rider only
    // gets effect/packet side effects and ends up stuck. Keep the native result
    // here and let the local action gate finish on its own.
    (void)runtimeSkillId;
    (void)rootSkillId;
    (void)currentSkillId;
    (void)hasRecentIntent;
    return false;
}

static int TraceMountedDemonJumpPrelocalGateCdecl1(
    const char *hookTag,
    DWORD expectedCallerRet,
    tMountedCrashTraceCdecl1ArgFn originalFn,
    int skillId)
{
    const DWORD callerRet = (DWORD)(uintptr_t)_ReturnAddress();
    int runtimeSkillId = 0;
    int mountItemId = 0;
    int rootSkillId = 0;
    int currentSkillId = 0;
    bool hasRecentIntent = false;
    const bool trace = TryCollectMountedDemonJumpPrelocalGateTraceContext(
        &runtimeSkillId,
        &mountItemId,
        &rootSkillId,
        &currentSkillId,
        &hasRecentIntent);
    const bool callerMatch = callerRet == expectedCallerRet;
    static LONG s_mountedDemonJumpPrelocalGateLogBudget = 192;
    const bool shouldLog =
        trace &&
        InterlockedDecrement(&s_mountedDemonJumpPrelocalGateLogBudget) >= 0;
    if (shouldLog)
    {
        WriteLogFmt(
            "[MountDemonJumpAction] %s enter caller=0x%08X expected=0x%08X match=%d runtime=%d argSkill=%d mount=%d root=%d current=%d recent=%d",
            hookTag ? hookTag : "gate",
            callerRet,
            expectedCallerRet,
            callerMatch ? 1 : 0,
            runtimeSkillId,
            skillId,
            mountItemId,
            rootSkillId,
            currentSkillId,
            hasRecentIntent ? 1 : 0);
    }

    const int result = originalFn ? originalFn(skillId) : 0;
    int finalResult = result;
    if (shouldLog)
    {
        int afterRootSkillId = 0;
        int afterCurrentSkillId = 0;
        TryReadMountedDemonJumpContextState(
            &afterRootSkillId,
            &afterCurrentSkillId,
            nullptr);
        WriteLogFmt(
            "[MountDemonJumpAction] %s leave caller=0x%08X expected=0x%08X match=%d result=%d mount=%d root=%d current=%d recent=%d",
            hookTag ? hookTag : "gate",
            callerRet,
            expectedCallerRet,
            callerMatch ? 1 : 0,
            result,
            mountItemId,
            afterRootSkillId,
            afterCurrentSkillId,
            hasRecentIntent ? 1 : 0);
    }
    if (ShouldForceMountedDemonJumpPrelocalGateZero(
            hookTag,
            runtimeSkillId,
            mountItemId,
            rootSkillId,
            currentSkillId,
            hasRecentIntent,
            result))
    {
        finalResult = 0;
        WriteLogFmt(
            "[MountDemonJumpAction] %s force zero runtime=%d mount=%d root=%d current=%d recent=%d original=%d -> 0",
            hookTag ? hookTag : "gate",
            runtimeSkillId,
            mountItemId,
            rootSkillId,
            currentSkillId,
            hasRecentIntent ? 1 : 0,
            result);
    }
    return finalResult;
}

static int __cdecl hkMountedDemonJumpActionGate7DC870(int skillId)
{
    return TraceMountedDemonJumpPrelocalGateCdecl1(
        "7DC870",
        0x00B310AB,
        oMountedDemonJumpActionGate7DC870,
        skillId);
}

static int __cdecl hkMountedDemonJumpActionGate7DC810(int skillId)
{
    return TraceMountedDemonJumpPrelocalGateCdecl1(
        "7DC810",
        0x00B310BC,
        oMountedDemonJumpActionGate7DC810,
        skillId);
}

static int __cdecl hkMountedDemonJumpActionGate7DC7B0(int skillId)
{
    return TraceMountedDemonJumpPrelocalGateCdecl1(
        "7DC7B0",
        0x00B310DD,
        oMountedDemonJumpActionGate7DC7B0,
        skillId);
}

static int __cdecl hkMountedDemonJumpActionGate7DC8D0(int skillId)
{
    return TraceMountedDemonJumpPrelocalGateCdecl1(
        "7DC8D0",
        0x00B310F6,
        oMountedDemonJumpActionGate7DC8D0,
        skillId);
}

static int __cdecl hkMountedDemonJumpActionGate7DC710(int skillId)
{
    return TraceMountedDemonJumpPrelocalGateCdecl1(
        "7DC710",
        0x00B31107,
        oMountedDemonJumpActionGate7DC710,
        skillId);
}

static int __cdecl hkMountedDemonJumpActionGate7DC900(int skillId)
{
    return TraceMountedDemonJumpPrelocalGateCdecl1(
        "7DC900",
        0x00B31118,
        oMountedDemonJumpActionGate7DC900,
        skillId);
}

static int __cdecl hkMountedDemonJumpActionGate7CF840(int skillId)
{
    return TraceMountedDemonJumpPrelocalGateCdecl1(
        "7CF840",
        0x00B31131,
        oMountedDemonJumpActionGate7CF840,
        skillId);
}

static int __cdecl hkMountedDemonJumpActionGate7DC750(int skillId)
{
    return TraceMountedDemonJumpPrelocalGateCdecl1(
        "7DC750",
        0x00B31142,
        oMountedDemonJumpActionGate7DC750,
        skillId);
}

static int __cdecl hkMountedDemonJumpActionGate7DC8A0(int skillId)
{
    return TraceMountedDemonJumpPrelocalGateCdecl1(
        "7DC8A0",
        0x00B31164,
        oMountedDemonJumpActionGate7DC8A0,
        skillId);
}

static int __cdecl hkMountedDemonJumpActionKind7CE210(int skillId)
{
    const DWORD callerRet = (DWORD)(uintptr_t)_ReturnAddress();
    int runtimeSkillId = 0;
    int mountItemId = 0;
    const bool trace =
        (callerRet == 0x00B31153 || callerRet == 0x00B312B6) &&
        IsMountedDemonJumpCrashTraceFresh(&runtimeSkillId, &mountItemId);
    if (trace)
    {
        WriteLogFmt(
            "[MountDemonJumpAction] 7CE210 enter caller=0x%08X runtime=%d argSkill=%d mount=%d",
            callerRet,
            runtimeSkillId,
            skillId,
            mountItemId);
    }

    const int result = oMountedDemonJumpActionKind7CE210
                           ? oMountedDemonJumpActionKind7CE210(skillId)
                           : 0;
    if (trace)
    {
        int rootSkillId = 0;
        int currentSkillId = 0;
        TryReadMountedDemonJumpContextState(
            &rootSkillId,
            &currentSkillId,
            nullptr);
        WriteLogFmt(
            "[MountDemonJumpAction] 7CE210 leave caller=0x%08X result=%d mount=%d root=%d current=%d",
            callerRet,
            result,
            mountItemId,
            rootSkillId,
            currentSkillId);
    }
    return result;
}

