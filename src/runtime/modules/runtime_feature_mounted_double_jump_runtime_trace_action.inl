static bool TryResolveMountedDemonJumpActionAbilityGateContext(
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
    int rootSkillId = 0;
    int currentSkillId = 0;
    if (!TryResolveMountedDemonJumpActiveChildSkill(
            0,
            1200,
            &mountItemId,
            &rootSkillId,
            &currentSkillId,
            &runtimeSkillId))
    {
        int traceSkillId = 0;
        int traceMountItemId = 0;
        if (!IsMountedDemonJumpCrashTraceFresh(
                &traceSkillId,
                &traceMountItemId,
                1200) ||
            traceMountItemId <= 0 ||
            ResolveMountedRuntimeSkillIdForKind(
                MountedRuntimeSkillKind_DemonJump,
                traceMountItemId) != 30010110 ||
            !HasRecentMountedDemonJumpIntent(traceMountItemId, 1200) ||
            !SkillOverlayBridgeCanUseMountedDemonJumpRuntimeSkill(
                traceMountItemId,
                traceSkillId) ||
            !TryReadMountedDemonJumpContextState(
                &rootSkillId,
                &currentSkillId,
                nullptr) ||
            rootSkillId != 30010110)
        {
            return false;
        }

        runtimeSkillId = traceSkillId;
        mountItemId = traceMountItemId;
    }

    if (!IsMountedDemonJumpRuntimeChildSkillId(runtimeSkillId) ||
        mountItemId <= 0 ||
        rootSkillId != 30010110)
    {
        return false;
    }

    const bool hasRecentIntent = HasRecentMountedDemonJumpIntent(
        mountItemId,
        1200);
    if (!hasRecentIntent)
    {
        return false;
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

static bool ShouldBypassMountedDemonJumpActionAbilityGateAE5870(
    DWORD callerRet,
    int *runtimeSkillIdOut,
    int *mountItemIdOut,
    int *rootSkillIdOut,
    int *currentSkillIdOut,
    bool *recentIntentOut)
{
    if (!TryResolveMountedDemonJumpActionAbilityGateContext(
            runtimeSkillIdOut,
            mountItemIdOut,
            rootSkillIdOut,
            currentSkillIdOut,
            recentIntentOut))
    {
        return false;
    }

    switch (callerRet)
    {
    case 0x00B24038:
    case 0x00B2403D:
    case 0x00B242D9:
    case 0x00B242DE:
    case 0x00B24909:
    case 0x00B2490E:
    case 0x00B275A8:
    case 0x00B275AD:
    case 0x00B27A18:
    case 0x00B27A1D:
    case 0x00B29AD9:
    case 0x00B29ADE:
    case 0x00B2D0A9:
    case 0x00B2D0AE:
    case 0x00B2D34F:
    case 0x00B2D354:
    case 0x00B2D3E3:
    case 0x00B2D3E8:
        return true;
    default:
        return false;
    }
}

static int __fastcall hkMountedDemonJumpActionAbilityGateAE5870(
    void *thisPtr,
    void * /*edxUnused*/)
{
    const DWORD callerRet = (DWORD)(uintptr_t)_ReturnAddress();
    int runtimeSkillId = 0;
    int mountItemId = 0;
    int rootSkillId = 0;
    int currentSkillId = 0;
    bool hasRecentIntent = false;
    const bool bypass =
        ShouldBypassMountedDemonJumpActionAbilityGateAE5870(
            callerRet,
            &runtimeSkillId,
            &mountItemId,
            &rootSkillId,
            &currentSkillId,
            &hasRecentIntent);

    DWORD cooldownTick = 0;
    DWORD cooldownAge = 0;
    if (thisPtr &&
        !SafeIsBadReadPtr(
            reinterpret_cast<void *>(
                reinterpret_cast<uintptr_t>(thisPtr) + 0x5B00),
            sizeof(DWORD)))
    {
        __try
        {
            cooldownTick = *reinterpret_cast<DWORD *>(
                reinterpret_cast<uintptr_t>(thisPtr) + 0x5B00);
            typedef DWORD(__cdecl *tGameTickFn)();
            tGameTickFn gameTickFn = reinterpret_cast<tGameTickFn>(ADDR_B4C450);
            const DWORD nowTick = gameTickFn ? gameTickFn() : GetTickCount();
            cooldownAge = nowTick - cooldownTick;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            cooldownTick = 0;
            cooldownAge = 0;
        }
    }

    if (bypass)
    {
        static LONG s_mountedDemonJumpActionAbilityGateAE5870BypassLogBudget = 48;
        if (InterlockedDecrement(
                &s_mountedDemonJumpActionAbilityGateAE5870BypassLogBudget) >= 0)
        {
            WriteLogFmt(
                "[MountDemonJumpAction] AE5870 bypass caller=0x%08X runtime=%d mount=%d root=%d current=%d recent=%d cooldown=[tick=0x%08X age=%u]",
                callerRet,
                runtimeSkillId,
                mountItemId,
                rootSkillId,
                currentSkillId,
                hasRecentIntent ? 1 : 0,
                cooldownTick,
                cooldownAge);
        }
        return 0;
    }

    const int result = oMountedDemonJumpActionAbilityGateAE5870
                           ? oMountedDemonJumpActionAbilityGateAE5870(thisPtr)
                           : 0;

    int traceRuntimeSkillId = 0;
    int traceMountItemId = 0;
    const bool trace =
        IsMountedDemonJumpCrashTraceFresh(
            &traceRuntimeSkillId,
            &traceMountItemId,
            1200) ||
        TryResolveMountedDemonJumpActionAbilityGateContext(
            &traceRuntimeSkillId,
            &traceMountItemId,
            &rootSkillId,
            &currentSkillId,
            &hasRecentIntent);
    if (trace)
    {
        static LONG s_mountedDemonJumpActionAbilityGateAE5870TraceLogBudget = 48;
        if (InterlockedDecrement(
                &s_mountedDemonJumpActionAbilityGateAE5870TraceLogBudget) >= 0)
        {
            WriteLogFmt(
                "[MountDemonJumpAction] AE5870 leave caller=0x%08X result=%d runtime=%d mount=%d root=%d current=%d recent=%d cooldown=[tick=0x%08X age=%u]",
                callerRet,
                result,
                traceRuntimeSkillId,
                traceMountItemId,
                rootSkillId,
                currentSkillId,
                hasRecentIntent ? 1 : 0,
                cooldownTick,
                cooldownAge);
        }
    }

    return result;
}

static bool ShouldBypassMountedDemonJumpActionLocalGate(
    DWORD callerRet,
    int runtimeSkillId,
    int mountItemId)
{
    if (!IsMountedDemonJumpRuntimeChildSkillId(runtimeSkillId) ||
        mountItemId <= 0 ||
        !HasRecentMountedDemonJumpIntent(mountItemId, 1200) ||
        !SkillOverlayBridgeCanUseMountedDemonJumpRuntimeSkill(
            mountItemId,
            runtimeSkillId))
    {
        return false;
    }

    switch (callerRet)
    {
    case 0x00B27577:
    case 0x00B2757C:
    case 0x00B2F42A:
    case 0x00B277C2:
    case 0x00B327C3:
        return true;
    default:
        return false;
    }
}

static bool ShouldBypassMountedDemonJumpActionBattlegroundSkillGate(
    DWORD callerRet,
    int skillId,
    int runtimeSkillId,
    int mountItemId)
{
    (void)callerRet;
    if (!IsMountedDemonJumpRuntimeChildSkillId(runtimeSkillId) ||
        mountItemId <= 0 ||
        !HasRecentMountedDemonJumpIntent(mountItemId, 1200) ||
        ResolveMountedRuntimeSkillIdForKind(
            MountedRuntimeSkillKind_DemonJump,
            mountItemId) != 30010110 ||
        !SkillOverlayBridgeCanUseMountedDemonJumpRuntimeSkill(
            mountItemId,
            runtimeSkillId))
    {
        return false;
    }

    return skillId == runtimeSkillId ||
           skillId == 30010110 ||
           IsMountedDemonJumpRuntimeProxySkillId(skillId);
}

static int __cdecl hkMountedDemonJumpActionBattlegroundSkillGate4E1D30(
    int skillId)
{
    const DWORD callerRet = (DWORD)(uintptr_t)_ReturnAddress();
    int runtimeSkillId = 0;
    int mountItemId = 0;
    const bool trace = IsMountedDemonJumpCrashTraceFresh(
        &runtimeSkillId,
        &mountItemId);
    const int result = oMountedDemonJumpActionBattlegroundSkillGate4E1D30
                           ? oMountedDemonJumpActionBattlegroundSkillGate4E1D30(
                                 skillId)
                           : 0;
    if (!trace)
    {
        return result;
    }

    const bool bypass =
        ShouldBypassMountedDemonJumpActionBattlegroundSkillGate(
            callerRet,
            skillId,
            runtimeSkillId,
            mountItemId);
    const bool shouldLog =
        bypass ||
        skillId == runtimeSkillId ||
        skillId == 30010110 ||
        IsMountedDemonJumpRuntimeProxySkillId(skillId);
    if (shouldLog)
    {
        static LONG s_mountedDemonJumpActionBattlegroundSkillGate4E1D30LogBudget = 32;
        if (InterlockedDecrement(
                &s_mountedDemonJumpActionBattlegroundSkillGate4E1D30LogBudget) >= 0)
        {
            WriteLogFmt(
                "[MountDemonJumpAction] 4E1D30 caller=0x%08X runtime=%d argSkill=%d mount=%d result=%d%s",
                callerRet,
                runtimeSkillId,
                skillId,
                mountItemId,
                result,
                bypass && result != 0 ? " -> force0" : "");
        }
    }

    return bypass ? 0 : result;
}

static int __cdecl hkMountedDemonJumpActionUsable7DAAF0(int skillId)
{
    const DWORD callerRet = (DWORD)(uintptr_t)_ReturnAddress();
    int runtimeSkillId = 0;
    int mountItemId = 0;
    const bool trace = IsMountedDemonJumpCrashTraceFresh(
        &runtimeSkillId,
        &mountItemId);

    const int result = oMountedDemonJumpActionUsable7DAAF0
                           ? oMountedDemonJumpActionUsable7DAAF0(skillId)
                           : 0;
    const bool bypass =
        trace &&
        ShouldBypassMountedDemonJumpActionLocalGate(
            callerRet,
            runtimeSkillId,
            mountItemId);

    if (trace)
    {
        static LONG s_mountedDemonJumpActionUsable7DAAF0LogBudget = 48;
        if (InterlockedDecrement(&s_mountedDemonJumpActionUsable7DAAF0LogBudget) >= 0)
        {
            WriteLogFmt(
                "[MountDemonJumpAction] 7DAAF0 caller=0x%08X runtime=%d argSkill=%d mount=%d result=%d bypass=%d%s",
                callerRet,
                runtimeSkillId,
                skillId,
                mountItemId,
                result,
                bypass ? 1 : 0,
                bypass && result == 0 ? " -> force1" : "");
        }
    }

    return bypass && result == 0 ? 1 : result;
}

static int __fastcall hkMountedDemonJumpActionJobGate7D7D20(
    void *thisPtr,
    void * /*edxUnused*/,
    DWORD arg1,
    DWORD arg2)
{
    const DWORD callerRet = (DWORD)(uintptr_t)_ReturnAddress();
    int runtimeSkillId = 0;
    int mountItemId = 0;
    const bool trace = IsMountedDemonJumpCrashTraceFresh(
        &runtimeSkillId,
        &mountItemId);

    int rowSkillId = 0;
    if (thisPtr &&
        !SafeIsBadReadPtr(thisPtr, sizeof(DWORD)))
    {
        __try
        {
            rowSkillId = *reinterpret_cast<int *>(thisPtr);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            rowSkillId = 0;
        }
    }

    const int result = oMountedDemonJumpActionJobGate7D7D20
                           ? oMountedDemonJumpActionJobGate7D7D20(
                                 thisPtr,
                                 arg1,
                                 arg2)
                           : 0;
    const bool bypass =
        trace &&
        ShouldBypassMountedDemonJumpActionLocalGate(
            callerRet,
            runtimeSkillId,
            mountItemId);

    if (trace)
    {
        static LONG s_mountedDemonJumpActionJobGate7D7D20LogBudget = 48;
        if (InterlockedDecrement(&s_mountedDemonJumpActionJobGate7D7D20LogBudget) >= 0)
        {
            WriteLogFmt(
                "[MountDemonJumpAction] 7D7D20 caller=0x%08X runtime=%d rowSkill=%d args=[0x%08X,0x%08X] mount=%d result=%d bypass=%d%s",
                callerRet,
                runtimeSkillId,
                rowSkillId,
                arg1,
                arg2,
                mountItemId,
                result,
                bypass ? 1 : 0,
                bypass && result == 0 ? " -> force1" : "");
        }
    }

    return bypass && result == 0 ? 1 : result;
}

static int __stdcall hkMountedDemonJumpActionPromptReason42DAF0(
    int arg1,
    int arg2)
{
    const DWORD callerRet = (DWORD)(uintptr_t)_ReturnAddress();
    const int result = oMountedDemonJumpActionPromptReason42DAF0
                           ? oMountedDemonJumpActionPromptReason42DAF0(
                                 arg1,
                                 arg2)
                           : 0;

    if (InterlockedCompareExchange(
            &g_MountedDemonJumpActionPrepareTraceDepth,
            0,
            0) > 0)
    {
        int runtimeSkillId = 0;
        int mountItemId = 0;
        if (IsMountedDemonJumpCrashTraceFresh(
                &runtimeSkillId,
                &mountItemId))
        {
            int slotSkillId = 0;
            if (arg1 != 0 &&
                !SafeIsBadReadPtr(
                    reinterpret_cast<void *>(arg1),
                    sizeof(int)))
            {
                __try
                {
                    slotSkillId = *reinterpret_cast<int *>(arg1);
                }
                __except (EXCEPTION_EXECUTE_HANDLER)
                {
                    slotSkillId = 0;
                }
            }

            static LONG s_mountedDemonJumpActionPromptReason42DAF0LogBudget = 32;
            if (InterlockedDecrement(
                    &s_mountedDemonJumpActionPromptReason42DAF0LogBudget) >= 0)
            {
                WriteLogFmt(
                    "[MountDemonJumpAction] 42DAF0 caller=0x%08X runtime=%d slotSkill=%d mount=%d message=%d result=0x%08X",
                    callerRet,
                    runtimeSkillId,
                    slotSkillId,
                    mountItemId,
                    arg2,
                    result);
            }
        }
    }

    return result;
}

static int __fastcall hkMountedDemonJumpActionPrepareB273B0(
    void *thisPtr,
    void * /*edxUnused*/,
    DWORD arg1,
    DWORD arg2,
    DWORD arg3,
    DWORD arg4,
    DWORD arg5)
{
    const DWORD callerRet = (DWORD)(uintptr_t)_ReturnAddress();
    int runtimeSkillId = 0;
    int mountItemId = 0;
    const bool trace =
        callerRet == 0x00B31199 &&
        IsMountedDemonJumpCrashTraceFresh(&runtimeSkillId, &mountItemId);
    DWORD effectiveSkillId = arg1;
    bool remappedSkillId = false;
    if (trace &&
        IsMountedDemonJumpRuntimeChildSkillId(runtimeSkillId) &&
        mountItemId > 0 &&
        HasRecentMountedDemonJumpIntent(mountItemId, 1200) &&
        ResolveMountedRuntimeSkillIdForKind(
            MountedRuntimeSkillKind_DemonJump,
            mountItemId) == 30010110 &&
        (static_cast<int>(arg1) == 30010110 ||
         IsMountedDemonJumpRuntimeProxySkillId(static_cast<int>(arg1))) &&
        SkillOverlayBridgeCanUseMountedDemonJumpRuntimeSkill(
            mountItemId,
            runtimeSkillId))
    {
        effectiveSkillId = static_cast<DWORD>(runtimeSkillId);
        remappedSkillId = effectiveSkillId != arg1;
    }
    if (trace)
    {
        WriteLogFmt(
            "[MountDemonJumpAction] B273B0 enter caller=0x%08X runtime=%d mount=%d skill=0x%08X->0x%08X remap=%d this=0x%08X args=[0x%08X,0x%08X,0x%08X,0x%08X,0x%08X]",
            callerRet,
            runtimeSkillId,
            mountItemId,
            arg1,
            effectiveSkillId,
            remappedSkillId ? 1 : 0,
            (DWORD)(uintptr_t)thisPtr,
            arg1,
            arg2,
            arg3,
            arg4,
            arg5);
    }
    if (trace)
    {
        InterlockedIncrement(&g_MountedDemonJumpActionPrepareTraceDepth);
    }

    const int result = oMountedDemonJumpActionPrepareB273B0
                           ? oMountedDemonJumpActionPrepareB273B0(
                                 thisPtr,
                                 effectiveSkillId,
                                 arg2,
                                 arg3,
                                 arg4,
                                 arg5)
                           : 0;
    if (trace)
    {
        InterlockedDecrement(&g_MountedDemonJumpActionPrepareTraceDepth);
    }
    if (trace)
    {
        WriteLogFmt(
            "[MountDemonJumpAction] B273B0 leave caller=0x%08X result=%d mount=%d skill=0x%08X->0x%08X remap=%d",
            callerRet,
            result,
            mountItemId,
            arg1,
            effectiveSkillId,
            remappedSkillId ? 1 : 0);
    }
    return result;
}

static bool TryResolveMountedDemonJumpActionGateA9B710Context(
    int runtimeSkillId,
    int mountItemId,
    int *rootSkillIdOut,
    int *currentSkillIdOut,
    bool *recentIntentOut)
{
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

    if (mountItemId <= 0 ||
        !IsMountedDemonJumpRuntimeChildSkillId(runtimeSkillId) ||
        ResolveMountedRuntimeSkillIdForKind(
            MountedRuntimeSkillKind_DemonJump,
            mountItemId) != 30010110)
    {
        return false;
    }

    int rootSkillId = 0;
    int currentSkillId = 0;
    const bool hasContext = TryReadMountedDemonJumpContextState(
        &rootSkillId,
        &currentSkillId,
        nullptr);
    const bool hasRecentIntent = HasRecentMountedDemonJumpIntent(
        mountItemId,
        1200);

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

    if (hasContext &&
        rootSkillId == 30010110 &&
        (currentSkillId == 30010110 ||
         currentSkillId == runtimeSkillId ||
         IsMountedDemonJumpRuntimeChildSkillId(currentSkillId)))
    {
        return true;
    }

    return hasRecentIntent;
}

static bool ShouldForceMountedDemonJumpActionGateA9B710Zero(
    DWORD callerRet,
    int runtimeSkillId,
    int mountItemId,
    int *rootSkillIdOut,
    int *currentSkillIdOut,
    bool *recentIntentOut)
{
    if (callerRet != 0x00B3109D)
    {
        return false;
    }

    // Normal successful demon jump traces reach B31220 from the B3109D
    // callsite only when A9B710 returns 0. Returning 1 here skips that native
    // continuation and leaves mounted demon jump with effect/packet side
    // effects but no actual local movement.
    return TryResolveMountedDemonJumpActionGateA9B710Context(
        runtimeSkillId,
        mountItemId,
        rootSkillIdOut,
        currentSkillIdOut,
        recentIntentOut);
}

static bool ShouldBypassMountedDemonJumpActionGateA9B710(
    DWORD callerRet,
    int runtimeSkillId,
    int mountItemId,
    int *rootSkillIdOut,
    int *currentSkillIdOut,
    bool *recentIntentOut)
{
    if (callerRet != 0x00B311FB)
    {
        return false;
    }

    return TryResolveMountedDemonJumpActionGateA9B710Context(
        runtimeSkillId,
        mountItemId,
        rootSkillIdOut,
        currentSkillIdOut,
        recentIntentOut);
}

static int __fastcall hkMountedDemonJumpActionGateA9B710(
    void *thisPtr,
    void * /*edxUnused*/)
{
    const DWORD callerRet = (DWORD)(uintptr_t)_ReturnAddress();
    int runtimeSkillId = 0;
    int mountItemId = 0;
    int rootSkillId = 0;
    int currentSkillId = 0;
    bool hasRecentIntent = false;
    bool trace =
        (callerRet == 0x00B3109D || callerRet == 0x00B311FB) &&
        IsMountedDemonJumpCrashTraceFresh(&runtimeSkillId, &mountItemId);
    if (!trace &&
        (callerRet == 0x00B3109D || callerRet == 0x00B311FB) &&
        TryResolveMountedDemonJumpMountItemIdWithFallback(
            nullptr,
            &mountItemId,
            nullptr,
            1200) &&
        mountItemId > 0 &&
        ResolveMountedRuntimeSkillIdForKind(
            MountedRuntimeSkillKind_DemonJump,
            mountItemId) == 30010110 &&
        TryReadMountedDemonJumpEffectiveContextState(
            mountItemId,
            &rootSkillId,
            &currentSkillId) &&
        rootSkillId == 30010110)
    {
        if (!IsMountedDemonJumpRuntimeChildSkillId(currentSkillId))
        {
            int recentChildSkillId = 0;
            if (TryGetRecentMountedDemonJumpNativeChildSkill(
                    mountItemId,
                    &recentChildSkillId,
                    nullptr,
                    1200))
            {
                currentSkillId = recentChildSkillId;
            }
        }
        if (IsMountedDemonJumpRuntimeChildSkillId(currentSkillId))
        {
            runtimeSkillId = currentSkillId;
            trace = true;
        }
    }
    if (trace)
    {
        WriteLogFmt(
            "[MountDemonJumpAction] A9B710 enter caller=0x%08X runtime=%d mount=%d this=0x%08X",
            callerRet,
            runtimeSkillId,
            mountItemId,
            (DWORD)(uintptr_t)thisPtr);
    }

    if (trace &&
        ShouldForceMountedDemonJumpActionGateA9B710Zero(
            callerRet,
            runtimeSkillId,
            mountItemId,
            &rootSkillId,
            &currentSkillId,
            &hasRecentIntent))
    {
        WriteLogFmt(
            "[MountDemonJumpAction] A9B710 force zero caller=0x%08X runtime=%d mount=%d root=%d current=%d recent=%d",
            callerRet,
            runtimeSkillId,
            mountItemId,
            rootSkillId,
            currentSkillId,
            hasRecentIntent ? 1 : 0);
        return 0;
    }

    if (trace &&
        ShouldBypassMountedDemonJumpActionGateA9B710(
            callerRet,
            runtimeSkillId,
            mountItemId,
            &rootSkillId,
            &currentSkillId,
            &hasRecentIntent))
    {
        WriteLogFmt(
            "[MountDemonJumpAction] A9B710 bypass caller=0x%08X runtime=%d mount=%d root=%d current=%d recent=%d",
            callerRet,
            runtimeSkillId,
            mountItemId,
            rootSkillId,
            currentSkillId,
            hasRecentIntent ? 1 : 0);
        return 1;
    }

    const int result = oMountedDemonJumpActionGateA9B710
                           ? oMountedDemonJumpActionGateA9B710(thisPtr)
                           : 0;
    if (trace)
    {
        WriteLogFmt(
            "[MountDemonJumpAction] A9B710 leave caller=0x%08X result=%d mount=%d",
            callerRet,
            result,
            mountItemId);
    }
    return result;
}

static int __cdecl hkMountedDemonJumpActionKind52BAD0(int skillId)
{
    const DWORD callerRet = (DWORD)(uintptr_t)_ReturnAddress();
    int runtimeSkillId = 0;
    int mountItemId = 0;
    const bool trace =
        callerRet == 0x00B312DF &&
        IsMountedDemonJumpCrashTraceFresh(&runtimeSkillId, &mountItemId);
    if (trace)
    {
        WriteLogFmt(
            "[MountDemonJumpAction] 52BAD0 enter caller=0x%08X runtime=%d argSkill=%d mount=%d",
            callerRet,
            runtimeSkillId,
            skillId,
            mountItemId);
    }

    const int result = oMountedDemonJumpActionKind52BAD0
                           ? oMountedDemonJumpActionKind52BAD0(skillId)
                           : 0;
    if (trace)
    {
        WriteLogFmt(
            "[MountDemonJumpAction] 52BAD0 leave caller=0x%08X result=%d(0x%08X) mount=%d",
            callerRet,
            result,
            result,
            mountItemId);
    }
    return result;
}

