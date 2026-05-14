// AbilityRed / 本地潜能安装模块：负责 local potential、movement setter、output clamp 等安装。
static bool SetupLocalIndependentPotentialDisplayFunctionHooks()
{
    bool anyOk = false;

    if (!oLocalIndependentPotentialSkillLevelDisplayFn)
    {
        BYTE *pTarget = FollowJmpChain((void *)ADDR_AE0A70);
        if (pTarget)
        {
            int copyLen = CalcMinCopyLen(pTarget);
            if (copyLen < 5)
                copyLen = 5;
            oLocalIndependentPotentialSkillLevelDisplayFn =
                (tLocalIndependentPotentialSkillLevelDisplayFn)GenericInlineHook5(
                    pTarget,
                    (void *)hkLocalIndependentPotentialSkillLevelDisplayFunction,
                    copyLen);
            if (oLocalIndependentPotentialSkillLevelDisplayFn)
            {
                anyOk = true;
                WriteLogFmt("[IndependentBuffLocalDisplay] OK(AE0A70): entry=0x%08X tramp=0x%08X copyLen=%d",
                    (DWORD)(uintptr_t)pTarget,
                    (DWORD)(uintptr_t)oLocalIndependentPotentialSkillLevelDisplayFn,
                    copyLen);
            }
            else
            {
                WriteLog("[IndependentBuffLocalDisplay] AE0A70 hook failed");
            }
        }
        else
        {
            WriteLog("[IndependentBuffLocalDisplay] AE0A70 target missing");
        }
    }
    else
    {
        anyOk = true;
    }

    if (!oLocalIndependentPotentialPercentQuadDisplayFn)
    {
        BYTE *pTarget = FollowJmpChain((void *)ADDR_8538C0);
        if (pTarget)
        {
            int copyLen = CalcMinCopyLen(pTarget);
            if (copyLen < 5)
                copyLen = 5;
            oLocalIndependentPotentialPercentQuadDisplayFn =
                (tLocalIndependentPotentialPercentQuadDisplayFn)GenericInlineHook5(
                    pTarget,
                    (void *)hkLocalIndependentPotentialPercentQuadDisplayFunction,
                    copyLen);
            if (oLocalIndependentPotentialPercentQuadDisplayFn)
            {
                anyOk = true;
                WriteLogFmt("[IndependentBuffLocalDisplay] OK(8538C0): entry=0x%08X tramp=0x%08X copyLen=%d",
                    (DWORD)(uintptr_t)pTarget,
                    (DWORD)(uintptr_t)oLocalIndependentPotentialPercentQuadDisplayFn,
                    copyLen);
            }
            else
            {
                WriteLog("[IndependentBuffLocalDisplay] 8538C0 hook failed");
            }
        }
        else
        {
            WriteLog("[IndependentBuffLocalDisplay] 8538C0 target missing");
        }
    }
    else
    {
        anyOk = true;
    }

    if (!oLocalIndependentPotentialPercentFullDisplayFn)
    {
        BYTE *pTarget = FollowJmpChain((void *)ADDR_853E10);
        if (pTarget)
        {
            int copyLen = CalcMinCopyLen(pTarget);
            if (copyLen < 5)
                copyLen = 5;
            oLocalIndependentPotentialPercentFullDisplayFn =
                (tLocalIndependentPotentialPercentFullDisplayFn)GenericInlineHook5(
                    pTarget,
                    (void *)hkLocalIndependentPotentialPercentFullDisplayFunction,
                    copyLen);
            if (oLocalIndependentPotentialPercentFullDisplayFn)
            {
                anyOk = true;
                WriteLogFmt("[IndependentBuffLocalDisplay] OK(853E10): entry=0x%08X tramp=0x%08X copyLen=%d",
                    (DWORD)(uintptr_t)pTarget,
                    (DWORD)(uintptr_t)oLocalIndependentPotentialPercentFullDisplayFn,
                    copyLen);
            }
            else
            {
                WriteLog("[IndependentBuffLocalDisplay] 853E10 hook failed");
            }
        }
        else
        {
            WriteLog("[IndependentBuffLocalDisplay] 853E10 target missing");
        }
    }
    else
    {
        anyOk = true;
    }

    if (!oLocalIndependentPotentialFlatBasicDisplayFn)
    {
        BYTE *pTarget = FollowJmpChain((void *)ADDR_853B00);
        if (pTarget)
        {
            int copyLen = CalcMinCopyLen(pTarget);
            if (copyLen < 5)
                copyLen = 5;
            oLocalIndependentPotentialFlatBasicDisplayFn =
                (tLocalIndependentPotentialFlatBasicDisplayFn)GenericInlineHook5(
                    pTarget,
                    (void *)hkLocalIndependentPotentialFlatBasicDisplayFunction,
                    copyLen);
            if (oLocalIndependentPotentialFlatBasicDisplayFn)
            {
                anyOk = true;
                WriteLogFmt("[IndependentBuffLocalDisplay] OK(853B00): entry=0x%08X tramp=0x%08X copyLen=%d",
                    (DWORD)(uintptr_t)pTarget,
                    (DWORD)(uintptr_t)oLocalIndependentPotentialFlatBasicDisplayFn,
                    copyLen);
            }
            else
            {
                WriteLog("[IndependentBuffLocalDisplay] 853B00 hook failed");
            }
        }
        else
        {
            WriteLog("[IndependentBuffLocalDisplay] 853B00 target missing");
        }
    }
    else
    {
        anyOk = true;
    }

    if (!oLocalIndependentPotentialFlatExtendedDisplayFn)
    {
        BYTE *pTarget = FollowJmpChain((void *)ADDR_856830);
        if (pTarget)
        {
            int copyLen = CalcMinCopyLen(pTarget);
            if (copyLen < 5)
                copyLen = 5;
            oLocalIndependentPotentialFlatExtendedDisplayFn =
                (tLocalIndependentPotentialFlatExtendedDisplayFn)GenericInlineHook5(
                    pTarget,
                    (void *)hkLocalIndependentPotentialFlatExtendedDisplayFunction,
                    copyLen);
            if (oLocalIndependentPotentialFlatExtendedDisplayFn)
            {
                anyOk = true;
                WriteLogFmt("[IndependentBuffLocalDisplay] OK(856830): entry=0x%08X tramp=0x%08X copyLen=%d",
                    (DWORD)(uintptr_t)pTarget,
                    (DWORD)(uintptr_t)oLocalIndependentPotentialFlatExtendedDisplayFn,
                    copyLen);
            }
            else
            {
                WriteLog("[IndependentBuffLocalDisplay] 856830 hook failed");
            }
        }
        else
        {
            WriteLog("[IndependentBuffLocalDisplay] 856830 target missing");
        }
    }
    else
    {
        anyOk = true;
    }

    return anyOk;
}

static bool SetupAbilityRedDisplayCallsiteHook()
{
    if (g_AbilityRedDisplayCallsiteOriginalTarget)
        return true;

    BYTE *pCallsite = (BYTE *)(uintptr_t)ADDR_AE6C21;
    if (!pCallsite || SafeIsBadReadPtr(pCallsite, 5) || pCallsite[0] != 0xE8)
    {
        WriteLog("[AbilityRedDisplay] AE6C21 callsite missing/unexpected");
        return false;
    }

    g_AbilityRedDisplayCallsiteOriginalTarget =
        (DWORD)(uintptr_t)(pCallsite + 5 + *(int *)(pCallsite + 1));

    DWORD oldProtect = 0;
    if (!VirtualProtect(pCallsite, 5, PAGE_EXECUTE_READWRITE, &oldProtect))
    {
        WriteLog("[AbilityRedDisplay] AE6C21 VirtualProtect failed");
        g_AbilityRedDisplayCallsiteOriginalTarget = 0;
        return false;
    }

    pCallsite[0] = 0xE8;
    *(int *)(pCallsite + 1) = (int)((uintptr_t)hkAbilityRedDisplayCallsiteNaked - (uintptr_t)pCallsite - 5);

    VirtualProtect(pCallsite, 5, oldProtect, &oldProtect);
    FlushInstructionCache(GetCurrentProcess(), pCallsite, 5);

    WriteLogFmt("[AbilityRedDisplay] OK(AE6C21): original=0x%08X patchedCall=0x%08X",
        g_AbilityRedDisplayCallsiteOriginalTarget,
        (DWORD)(uintptr_t)hkAbilityRedDisplayCallsiteNaked);
    return true;
}

static bool SetupAbilityRedLevelReadHook()
{
    if (oAbilityRedLevelReadHook)
        return true;

    BYTE *pTarget = FollowJmpChain((void *)ADDR_AE43D5);
    if (!pTarget)
    {
        WriteLog("[AbilityRedLevelRead] AE43D5 target missing");
        return false;
    }

    int copyLen = CalcMinCopyLen(pTarget);
    if (copyLen < 5)
        copyLen = 5;

    oAbilityRedLevelReadHook = GenericInlineHook5(
        pTarget,
        (void *)hkAbilityRedLevelReadNaked,
        copyLen);
    if (!oAbilityRedLevelReadHook)
    {
        WriteLog("[AbilityRedLevelRead] AE43D5 hook failed");
        return false;
    }

    WriteLogFmt("[AbilityRedLevelRead] OK(AE43D5): entry=0x%08X tramp=0x%08X copyLen=%d",
        (DWORD)(uintptr_t)pTarget,
        (DWORD)(uintptr_t)oAbilityRedLevelReadHook,
        copyLen);
    return true;
}

static bool SetupAbilityRedSkillWriteHooks()
{
    bool anyOk = false;

    if (!oAbilityRedSkillWrite52FE14Hook)
    {
        BYTE *pTarget = (BYTE *)(uintptr_t)ADDR_52FE14;
        oAbilityRedSkillWrite52FE14Hook = GenericInlineHook5(
            pTarget,
            (void *)hkAbilityRedSkillWrite52FE14Naked,
            5);
        if (oAbilityRedSkillWrite52FE14Hook)
        {
            anyOk = true;
            WriteLogFmt("[AbilityRedSkillWrite] OK(52FE14): tramp=0x%08X",
                (DWORD)(uintptr_t)oAbilityRedSkillWrite52FE14Hook);
        }
        else
        {
            WriteLog("[AbilityRedSkillWrite] 52FE14 hook failed");
        }
    }
    else
    {
        anyOk = true;
    }

    if (!oAbilityRedSkillWrite6226CEHook)
    {
        BYTE *pTarget = (BYTE *)(uintptr_t)ADDR_6226CE;
        oAbilityRedSkillWrite6226CEHook = GenericInlineHook5(
            pTarget,
            (void *)hkAbilityRedSkillWrite6226CENaked,
            5);
        if (oAbilityRedSkillWrite6226CEHook)
        {
            anyOk = true;
            WriteLogFmt("[AbilityRedSkillWrite] OK(6226CE): tramp=0x%08X",
                (DWORD)(uintptr_t)oAbilityRedSkillWrite6226CEHook);
        }
        else
        {
            WriteLog("[AbilityRedSkillWrite] 6226CE hook failed");
        }
    }
    else
    {
        anyOk = true;
    }

    if (!oAbilityRedSkillWrite49CA01Hook)
    {
        BYTE *pTarget = (BYTE *)(uintptr_t)ADDR_49CA01;
        oAbilityRedSkillWrite49CA01Hook = GenericInlineHook5(
            pTarget,
            (void *)hkAbilityRedSkillWrite49CA01Naked,
            5);
        if (oAbilityRedSkillWrite49CA01Hook)
        {
            anyOk = true;
            WriteLogFmt("[AbilityRedSkillWrite] OK(49CA01): tramp=0x%08X",
                (DWORD)(uintptr_t)oAbilityRedSkillWrite49CA01Hook);
        }
        else
        {
            WriteLog("[AbilityRedSkillWrite] 49CA01 hook failed");
        }
    }
    else
    {
        anyOk = true;
    }

    return anyOk;
}

static bool SetupAbilityRedHashContainerHooks()
{
    bool anyOk = false;

    if (!oAbilityRedHashLookupHook)
    {
        BYTE *pTarget = FollowJmpChain((void *)ADDR_49C9C0);
        if (pTarget)
        {
            int copyLen = CalcMinCopyLen(pTarget);
            if (copyLen < 5)
                copyLen = 5;

            oAbilityRedHashLookupHook = GenericInlineHook5(
                pTarget,
                (void *)hkAbilityRedHashLookupNaked,
                copyLen);
            if (oAbilityRedHashLookupHook)
            {
                anyOk = true;
                WriteLogFmt("[AbilityRedHashLookup] OK(49C9C0): entry=0x%08X tramp=0x%08X copyLen=%d",
                    (DWORD)(uintptr_t)pTarget,
                    (DWORD)(uintptr_t)oAbilityRedHashLookupHook,
                    copyLen);
            }
            else
            {
                WriteLog("[AbilityRedHashLookup] 49C9C0 hook failed");
            }
        }
        else
        {
            WriteLog("[AbilityRedHashLookup] 49C9C0 target missing");
        }
    }
    else
    {
        anyOk = true;
    }

    if (!oAbilityRedHashInsertHook)
    {
        BYTE *pTarget = FollowJmpChain((void *)ADDR_52FD80);
        if (pTarget)
        {
            int copyLen = CalcMinCopyLen(pTarget);
            if (copyLen < 5)
                copyLen = 5;

            oAbilityRedHashInsertHook = GenericInlineHook5(
                pTarget,
                (void *)hkAbilityRedHashInsertNaked,
                copyLen);
            if (oAbilityRedHashInsertHook)
            {
                anyOk = true;
                WriteLogFmt("[AbilityRedHashInsert] OK(52FD80): entry=0x%08X tramp=0x%08X copyLen=%d",
                    (DWORD)(uintptr_t)pTarget,
                    (DWORD)(uintptr_t)oAbilityRedHashInsertHook,
                    copyLen);
            }
            else
            {
                WriteLog("[AbilityRedHashInsert] 52FD80 hook failed");
            }
        }
        else
        {
            WriteLog("[AbilityRedHashInsert] 52FD80 target missing");
        }
    }
    else
    {
        anyOk = true;
    }

    return anyOk;
}

static bool SetupAbilityRedExtendedAggregateHook()
{
    if (oAbilityRedExtendedAggregateFn)
        return true;

    BYTE *pTarget = FollowJmpChain((void *)ADDR_856BA0);
    if (!pTarget)
    {
        WriteLog("[AbilityRedAggregate] 856BA0 target missing");
        return false;
    }

    int copyLen = CalcMinCopyLen(pTarget);
    if (copyLen < 5)
        copyLen = 5;

    oAbilityRedExtendedAggregateFn =
        (tAbilityRedExtendedAggregateFn)GenericInlineHook5(
            pTarget,
            (void *)hkAbilityRedExtendedAggregateFunction,
            copyLen);
    if (!oAbilityRedExtendedAggregateFn)
    {
        WriteLog("[AbilityRedAggregate] 856BA0 hook failed");
        return false;
    }

    WriteLogFmt("[AbilityRedAggregate] OK(856BA0): entry=0x%08X tramp=0x%08X copyLen=%d",
        (DWORD)(uintptr_t)pTarget,
        (DWORD)(uintptr_t)oAbilityRedExtendedAggregateFn,
        copyLen);
    return true;
}

static bool SetupAbilityRedMasterAggregateHook()
{
    if (oAbilityRedMasterAggregateFn)
        return true;

    BYTE *pTarget = FollowJmpChain((void *)ADDR_856C60);
    if (!pTarget)
    {
        WriteLog("[AbilityRedMaster] 856C60 target missing");
        return false;
    }

    int copyLen = CalcMinCopyLen(pTarget);
    if (copyLen < 5)
        copyLen = 5;

    oAbilityRedMasterAggregateFn =
        (tAbilityRedMasterAggregateFn)GenericInlineHook5(
            pTarget,
            (void *)hkAbilityRedMasterAggregateFunction,
            copyLen);
    if (!oAbilityRedMasterAggregateFn)
    {
        WriteLog("[AbilityRedMaster] 856C60 hook failed");
        return false;
    }

    WriteLogFmt("[AbilityRedMaster] OK(856C60): entry=0x%08X tramp=0x%08X copyLen=%d",
        (DWORD)(uintptr_t)pTarget,
        (DWORD)(uintptr_t)oAbilityRedMasterAggregateFn,
        copyLen);
    return true;
}

static bool SetupAbilityRedMovementSetterHooks()
{
    bool anyOk = false;

    if (!oAbilityRedMovementSpeedSetter831F00Fn)
    {
        BYTE *pTarget = FollowJmpChain((void *)ADDR_831F00);
        if (!pTarget)
        {
            WriteLog("[MoveSetter] 831F00 target missing");
        }
        else
        {
            int copyLen = CalcMinCopyLen(pTarget);
            if (copyLen < 5)
                copyLen = 5;

            oAbilityRedMovementSpeedSetter831F00Fn =
                (tAbilityRedMovementSetterFn)GenericInlineHook5(
                    pTarget,
                    (void *)hkAbilityRedMovementSpeedSetter831F00,
                    copyLen);
            if (!oAbilityRedMovementSpeedSetter831F00Fn)
            {
                WriteLog("[MoveSetter] 831F00 hook failed");
            }
            else
            {
                anyOk = true;
                WriteLogFmt("[MoveSetter] OK(831F00): entry=0x%08X tramp=0x%08X copyLen=%d",
                    (DWORD)(uintptr_t)pTarget,
                    (DWORD)(uintptr_t)oAbilityRedMovementSpeedSetter831F00Fn,
                    copyLen);
            }
        }
    }
    else
    {
        anyOk = true;
    }

    if (!oAbilityRedMovementJumpSetter832000Fn)
    {
        BYTE *pTarget = FollowJmpChain((void *)ADDR_832000);
        if (!pTarget)
        {
            WriteLog("[MoveSetter] 832000 target missing");
        }
        else
        {
            int copyLen = CalcMinCopyLen(pTarget);
            if (copyLen < 5)
                copyLen = 5;

            oAbilityRedMovementJumpSetter832000Fn =
                (tAbilityRedMovementSetterFn)GenericInlineHook5(
                    pTarget,
                    (void *)hkAbilityRedMovementJumpSetter832000,
                    copyLen);
            if (!oAbilityRedMovementJumpSetter832000Fn)
            {
                WriteLog("[MoveSetter] 832000 hook failed");
            }
            else
            {
                anyOk = true;
                WriteLogFmt("[MoveSetter] OK(832000): entry=0x%08X tramp=0x%08X copyLen=%d",
                    (DWORD)(uintptr_t)pTarget,
                    (DWORD)(uintptr_t)oAbilityRedMovementJumpSetter832000Fn,
                    copyLen);
            }
        }
    }
    else
    {
        anyOk = true;
    }

    return anyOk;
}

static bool SetupMovementOutputClampHook()
{
    if (oMovementOutputClampComputeB93B80Fn)
        return true;

    BYTE *pTarget = FollowJmpChain((void *)ADDR_B93B80);
    if (!pTarget)
    {
        WriteLog("[MoveClamp] B93B80 target missing");
        return false;
    }

    int copyLen = CalcMinCopyLen(pTarget);
    if (copyLen < 5)
        copyLen = 5;

    oMovementOutputClampComputeB93B80Fn =
        (tMovementOutputClampComputeFn)GenericInlineHook5(
            pTarget,
            (void *)hkMovementOutputClampComputeB93B80,
            copyLen);
    if (!oMovementOutputClampComputeB93B80Fn)
    {
        WriteLog("[MoveClamp] B93B80 hook failed");
        return false;
    }

    WriteLogFmt("[MoveClamp] OK(B93B80): entry=0x%08X tramp=0x%08X copyLen=%d",
        (DWORD)(uintptr_t)pTarget,
        (DWORD)(uintptr_t)oMovementOutputClampComputeB93B80Fn,
        copyLen);
    return true;
}

static bool SetupAbilityRedSiblingCalcHooks()
{
    bool anyOk = false;

    if (!oAbilityRedSiblingCalc82F780Fn)
    {
        BYTE *pTarget = FollowJmpChain((void *)ADDR_82F780);
        if (!pTarget)
        {
            WriteLog("[AbilityRedSibling] 82F780 target missing");
        }
        else
        {
            int copyLen = CalcMinCopyLen(pTarget);
            if (copyLen < 5)
                copyLen = 5;

            oAbilityRedSiblingCalc82F780Fn =
                (tAbilityRedSiblingCalcFn)GenericInlineHook5(
                    pTarget,
                    (void *)hkAbilityRedSiblingCalc82F780,
                    copyLen);
            if (!oAbilityRedSiblingCalc82F780Fn)
            {
                WriteLog("[AbilityRedSibling] 82F780 hook failed");
            }
            else
            {
                WriteLogFmt("[AbilityRedSibling] OK(82F780): entry=0x%08X tramp=0x%08X copyLen=%d",
                    (DWORD)(uintptr_t)pTarget,
                    (DWORD)(uintptr_t)oAbilityRedSiblingCalc82F780Fn,
                    copyLen);
            }
        }
    }
    if (oAbilityRedSiblingCalc82F780Fn)
        anyOk = true;

    if (!oAbilityRedSiblingCalc82F870Fn)
    {
        BYTE *pTarget = FollowJmpChain((void *)ADDR_82F870);
        if (!pTarget)
        {
            WriteLog("[AbilityRedSibling] 82F870 target missing");
        }
        else
        {
            int copyLen = CalcMinCopyLen(pTarget);
            if (copyLen < 5)
                copyLen = 5;

            oAbilityRedSiblingCalc82F870Fn =
                (tAbilityRedSiblingCalcFn)GenericInlineHook5(
                    pTarget,
                    (void *)hkAbilityRedSiblingCalc82F870,
                    copyLen);
            if (!oAbilityRedSiblingCalc82F870Fn)
            {
                WriteLog("[AbilityRedSibling] 82F870 hook failed");
            }
            else
            {
                WriteLogFmt("[AbilityRedSibling] OK(82F870): entry=0x%08X tramp=0x%08X copyLen=%d",
                    (DWORD)(uintptr_t)pTarget,
                    (DWORD)(uintptr_t)oAbilityRedSiblingCalc82F870Fn,
                    copyLen);
            }
        }
    }
    if (oAbilityRedSiblingCalc82F870Fn)
        anyOk = true;

    if (!oAbilityRedSiblingCalc82F960Fn)
    {
        BYTE *pTarget = FollowJmpChain((void *)ADDR_82F960);
        if (!pTarget)
        {
            WriteLog("[AbilityRedSibling] 82F960 target missing");
        }
        else
        {
            int copyLen = CalcMinCopyLen(pTarget);
            if (copyLen < 5)
                copyLen = 5;

            oAbilityRedSiblingCalc82F960Fn =
                (tAbilityRedSiblingCalcFn)GenericInlineHook5(
                    pTarget,
                    (void *)hkAbilityRedSiblingCalc82F960,
                    copyLen);
            if (!oAbilityRedSiblingCalc82F960Fn)
            {
                WriteLog("[AbilityRedSibling] 82F960 hook failed");
            }
            else
            {
                WriteLogFmt("[AbilityRedSibling] OK(82F960): entry=0x%08X tramp=0x%08X copyLen=%d",
                    (DWORD)(uintptr_t)pTarget,
                    (DWORD)(uintptr_t)oAbilityRedSiblingCalc82F960Fn,
                    copyLen);
            }
        }
    }
    if (oAbilityRedSiblingCalc82F960Fn)
        anyOk = true;

    if (!oAbilityRedSiblingCalc82FA50Fn)
    {
        BYTE *pTarget = FollowJmpChain((void *)ADDR_82FA50);
        if (!pTarget)
        {
            WriteLog("[AbilityRedSibling] 82FA50 target missing");
        }
        else
        {
            int copyLen = CalcMinCopyLen(pTarget);
            if (copyLen < 5)
                copyLen = 5;

            oAbilityRedSiblingCalc82FA50Fn =
                (tAbilityRedSiblingCalcFn)GenericInlineHook5(
                    pTarget,
                    (void *)hkAbilityRedSiblingCalc82FA50,
                    copyLen);
            if (!oAbilityRedSiblingCalc82FA50Fn)
            {
                WriteLog("[AbilityRedSibling] 82FA50 hook failed");
            }
            else
            {
                WriteLogFmt("[AbilityRedSibling] OK(82FA50): entry=0x%08X tramp=0x%08X copyLen=%d",
                    (DWORD)(uintptr_t)pTarget,
                    (DWORD)(uintptr_t)oAbilityRedSiblingCalc82FA50Fn,
                    copyLen);
            }
        }
    }
    if (oAbilityRedSiblingCalc82FA50Fn)
        anyOk = true;

    return anyOk;
}

static bool SetupAbilityRedDiff84C470PreSubHook()
{
    if (g_AbilityRedDiff84C470PreSubContinue)
        return true;

    BYTE *pSite = (BYTE *)(uintptr_t)ADDR_9F7546;
    if (!pSite ||
        SafeIsBadReadPtr(pSite, 5) ||
        pSite[0] != 0x03 ||
        pSite[1] != 0xF3 ||
        pSite[2] != 0x03 ||
        pSite[3] != 0x75 ||
        pSite[4] != 0x48)
    {
        WriteLog("[AbilityRedDiff] 9F7546 site missing/unexpected");
        return false;
    }

    DWORD oldProtect = 0;
    if (!VirtualProtect(pSite, 5, PAGE_EXECUTE_READWRITE, &oldProtect))
    {
        WriteLog("[AbilityRedDiff] 9F7546 VirtualProtect failed");
        return false;
    }

    g_AbilityRedDiff84C470PreSubContinue = ADDR_9F754B;
    pSite[0] = 0xE9;
    *(int *)(pSite + 1) = (int)((uintptr_t)hkAbilityRedDiff84C470PreSubNaked - (uintptr_t)pSite - 5);

    VirtualProtect(pSite, 5, oldProtect, &oldProtect);
    FlushInstructionCache(GetCurrentProcess(), pSite, 5);

    WriteLogFmt("[AbilityRedDiff] OK(9F7546): continue=0x%08X hook=0x%08X",
        g_AbilityRedDiff84C470PreSubContinue,
        (DWORD)(uintptr_t)hkAbilityRedDiff84C470PreSubNaked);
    return true;
}

static bool InstallAbilityRedMidHook(
    DWORD siteAddress,
    const BYTE* expectedBytes,
    size_t expectedLength,
    void* hookProc,
    DWORD continueAddress,
    DWORD* continueSlot,
    const char* tag)
{
    if (continueSlot && *continueSlot)
        return true;
    if (!expectedBytes || expectedLength < 5 || !hookProc || !continueSlot)
        return false;

    BYTE* pSite = reinterpret_cast<BYTE*>(static_cast<uintptr_t>(siteAddress));
    if (!pSite || SafeIsBadReadPtr(pSite, expectedLength) || memcmp(pSite, expectedBytes, expectedLength) != 0)
    {
        WriteLogFmt("[AbilityRed] %s site missing/unexpected", tag ? tag : "midhook");
        return false;
    }

    DWORD oldProtect = 0;
    if (!VirtualProtect(pSite, expectedLength, PAGE_EXECUTE_READWRITE, &oldProtect))
    {
        WriteLogFmt("[AbilityRed] %s VirtualProtect failed", tag ? tag : "midhook");
        return false;
    }

    *continueSlot = continueAddress;
    pSite[0] = 0xE9;
    *reinterpret_cast<int*>(pSite + 1) =
        static_cast<int>(reinterpret_cast<uintptr_t>(hookProc) - reinterpret_cast<uintptr_t>(pSite) - 5);
    for (size_t i = 5; i < expectedLength; ++i)
        pSite[i] = 0x90;

    VirtualProtect(pSite, expectedLength, oldProtect, &oldProtect);
    FlushInstructionCache(GetCurrentProcess(), pSite, expectedLength);

    WriteLogFmt(
        "[AbilityRed] OK(%s): site=0x%08X continue=0x%08X hook=0x%08X",
        tag ? tag : "midhook",
        siteAddress,
        continueAddress,
        static_cast<DWORD>(reinterpret_cast<uintptr_t>(hookProc)));
    return true;
}

static bool SetupAbilityRedAdditionalDiffHooks()
{
    bool anyOk = false;

    static const BYTE kWdefPreSub[] = { 0x03, 0xF3, 0x03, 0x75, 0x48 };
    if (InstallAbilityRedMidHook(
            ADDR_9F7241,
            kWdefPreSub,
            sizeof(kWdefPreSub),
            reinterpret_cast<void*>(hkAbilityRedDiff84BE40PreSubNaked),
            ADDR_9F7246,
            &g_AbilityRedDiff84BE40PreSubContinue,
            "9F7241 wdef diff"))
    {
        anyOk = true;
    }

    static const BYTE kAccPreSub[] = { 0x03, 0xF7, 0x03, 0x75, 0x48 };
    if (InstallAbilityRedMidHook(
            ADDR_9F7893,
            kAccPreSub,
            sizeof(kAccPreSub),
            reinterpret_cast<void*>(hkAbilityRedDiff84CA90AccPreSubNaked),
            ADDR_9F7898,
            &g_AbilityRedDiff84CA90AccPreSubContinue,
            "9F7893 acc diff"))
    {
        anyOk = true;
    }

    static const BYTE kMagicAccPreSub[] = { 0x03, 0xF3, 0x03, 0x75, 0x48 };
    if (InstallAbilityRedMidHook(
            ADDR_9F7C7F,
            kMagicAccPreSub,
            sizeof(kMagicAccPreSub),
            reinterpret_cast<void*>(hkAbilityRedDiff84CA90MagicAccPreSubNaked),
            ADDR_9F7C84,
            &g_AbilityRedDiff84CA90MagicAccPreSubContinue,
            "9F7C7F magic-acc diff"))
    {
        anyOk = true;
    }

    static const BYTE kAvoidPreSub[] = { 0x03, 0xF3, 0x03, 0x75, 0x48 };
    if (InstallAbilityRedMidHook(
            ADDR_9F8048,
            kAvoidPreSub,
            sizeof(kAvoidPreSub),
            reinterpret_cast<void*>(hkAbilityRedDiff84CBD0AvoidPreSubNaked),
            ADDR_9F804D,
            &g_AbilityRedDiff84CBD0AvoidPreSubContinue,
            "9F8048 avoid diff"))
    {
        anyOk = true;
    }

    static const BYTE kMagicAvoidPreSub[] = { 0x03, 0x45, 0x48, 0x03, 0xF0 };
    if (InstallAbilityRedMidHook(
            ADDR_9F82A8,
            kMagicAvoidPreSub,
            sizeof(kMagicAvoidPreSub),
            reinterpret_cast<void*>(hkAbilityRedDiff84CBD0MagicAvoidPreSubNaked),
            ADDR_9F82AD,
            &g_AbilityRedDiff84CBD0MagicAvoidPreSubContinue,
            "9F82A8 magic-avoid diff"))
    {
        anyOk = true;
    }

    return anyOk;
}

static bool SetupAbilityRedPositiveStyleHooks()
{
    static const BYTE kAttackStyle[] = { 0x8B, 0x45, 0x68, 0x8D, 0x4D, 0x34, 0x51 };
    return InstallAbilityRedMidHook(
            ADDR_9F6E6F,
            kAttackStyle,
            sizeof(kAttackStyle),
            reinterpret_cast<void*>(hkAbilityRedAttackRangeStyleNaked),
            ADDR_9F6E76,
            &g_AbilityRedAttackRangeStyleContinue,
            "9F6E6F attack style");
}

static bool SetupAbilityRedBakeWriteHooks()
{
    bool anyOk = false;

    if (!oAbilityRedBake857BB6Hook)
    {
        BYTE *pTarget = (BYTE *)(uintptr_t)ADDR_857BB6;
        oAbilityRedBake857BB6Hook = GenericInlineHook5(
            pTarget,
            (void *)hkAbilityRedBake857BB6Naked,
            6);
        if (oAbilityRedBake857BB6Hook)
        {
            anyOk = true;
            WriteLogFmt("[AbilityRedBake] OK(857BB6): tramp=0x%08X",
                (DWORD)(uintptr_t)oAbilityRedBake857BB6Hook);
        }
        else
        {
            WriteLog("[AbilityRedBake] 857BB6 hook failed");
        }
    }
    else
    {
        anyOk = true;
    }

    if (!oAbilityRedBake857C29Hook)
    {
        BYTE *pTarget = (BYTE *)(uintptr_t)ADDR_857C29;
        oAbilityRedBake857C29Hook = GenericInlineHook5(
            pTarget,
            (void *)hkAbilityRedBake857C29Naked,
            6);
        if (oAbilityRedBake857C29Hook)
        {
            anyOk = true;
            WriteLogFmt("[AbilityRedBake] OK(857C29): tramp=0x%08X",
                (DWORD)(uintptr_t)oAbilityRedBake857C29Hook);
        }
        else
        {
            WriteLog("[AbilityRedBake] 857C29 hook failed");
        }
    }
    else
    {
        anyOk = true;
    }

    if (!oAbilityRedBake857C9CHook)
    {
        BYTE *pTarget = (BYTE *)(uintptr_t)ADDR_857C9C;
        oAbilityRedBake857C9CHook = GenericInlineHook5(
            pTarget,
            (void *)hkAbilityRedBake857C9CNaked,
            6);
        if (oAbilityRedBake857C9CHook)
        {
            anyOk = true;
            WriteLogFmt("[AbilityRedBake] OK(857C9C): tramp=0x%08X",
                (DWORD)(uintptr_t)oAbilityRedBake857C9CHook);
        }
        else
        {
            WriteLog("[AbilityRedBake] 857C9C hook failed");
        }
    }
    else
    {
        anyOk = true;
    }

    if (!oAbilityRedBake857D0FHook)
    {
        BYTE *pTarget = (BYTE *)(uintptr_t)ADDR_857D0F;
        oAbilityRedBake857D0FHook = GenericInlineHook5(
            pTarget,
            (void *)hkAbilityRedBake857D0FNaked,
            6);
        if (oAbilityRedBake857D0FHook)
        {
            anyOk = true;
            WriteLogFmt("[AbilityRedBake] OK(857D0F): tramp=0x%08X",
                (DWORD)(uintptr_t)oAbilityRedBake857D0FHook);
        }
        else
        {
            WriteLog("[AbilityRedBake] 857D0F hook failed");
        }
    }
    else
    {
        anyOk = true;
    }

    return anyOk;
}

static bool SetupAbilityRedBake198Hooks()
{
    bool anyOk = false;

    if (!oAbilityRedBake1988569C3Hook)
    {
        BYTE *pTarget = (BYTE *)(uintptr_t)ADDR_8569C3;
        oAbilityRedBake1988569C3Hook = GenericInlineHook5(
            pTarget,
            (void *)hkAbilityRedBake1988569C3Naked,
            6);
        if (oAbilityRedBake1988569C3Hook)
        {
            anyOk = true;
            WriteLogFmt("[AbilityRedBake198] OK(8569C3): tramp=0x%08X",
                (DWORD)(uintptr_t)oAbilityRedBake1988569C3Hook);
        }
        else
        {
            WriteLog("[AbilityRedBake198] 8569C3 hook failed");
        }
    }
    else
    {
        anyOk = true;
    }

    if (!oAbilityRedBake198856D57Hook)
    {
        BYTE *pTarget = (BYTE *)(uintptr_t)ADDR_856D57;
        oAbilityRedBake198856D57Hook = GenericInlineHook5(
            pTarget,
            (void *)hkAbilityRedBake198856D57Naked,
            6);
        if (oAbilityRedBake198856D57Hook)
        {
            anyOk = true;
            WriteLogFmt("[AbilityRedBake198] OK(856D57): tramp=0x%08X",
                (DWORD)(uintptr_t)oAbilityRedBake198856D57Hook);
        }
        else
        {
            WriteLog("[AbilityRedBake198] 856D57 hook failed");
        }
    }
    else
    {
        anyOk = true;
    }

    if (!oAbilityRedBake19885725FHook)
    {
        BYTE *pTarget = (BYTE *)(uintptr_t)ADDR_85725F;
        oAbilityRedBake19885725FHook = GenericInlineHook5(
            pTarget,
            (void *)hkAbilityRedBake19885725FNaked,
            6);
        if (oAbilityRedBake19885725FHook)
        {
            anyOk = true;
            WriteLogFmt("[AbilityRedBake198] OK(85725F): tramp=0x%08X",
                (DWORD)(uintptr_t)oAbilityRedBake19885725FHook);
        }
        else
        {
            WriteLog("[AbilityRedBake198] 85725F hook failed");
        }
    }
    else
    {
        anyOk = true;
    }

    if (!oAbilityRedBake198857C3BHook)
    {
        BYTE *pTarget = (BYTE *)(uintptr_t)ADDR_857C3B;
        oAbilityRedBake198857C3BHook = GenericInlineHook5(
            pTarget,
            (void *)hkAbilityRedBake198857C3BNaked,
            6);
        if (oAbilityRedBake198857C3BHook)
        {
            anyOk = true;
            WriteLogFmt("[AbilityRedBake198] OK(857C3B): tramp=0x%08X",
                (DWORD)(uintptr_t)oAbilityRedBake198857C3BHook);
        }
        else
        {
            WriteLog("[AbilityRedBake198] 857C3B hook failed");
        }
    }
    else
    {
        anyOk = true;
    }

    if (!oAbilityRedBake198858AEDHook)
    {
        BYTE *pTarget = (BYTE *)(uintptr_t)ADDR_858AED;
        oAbilityRedBake198858AEDHook = GenericInlineHook5(
            pTarget,
            (void *)hkAbilityRedBake198858AEDNaked,
            6);
        if (oAbilityRedBake198858AEDHook)
        {
            anyOk = true;
            WriteLogFmt("[AbilityRedBake198] OK(858AED): tramp=0x%08X",
                (DWORD)(uintptr_t)oAbilityRedBake198858AEDHook);
        }
        else
        {
            WriteLog("[AbilityRedBake198] 858AED hook failed");
        }
    }
    else
    {
        anyOk = true;
    }

    if (!oAbilityRedBake198831A50Hook)
    {
        BYTE *pTarget = (BYTE *)(uintptr_t)ADDR_831A50;
        oAbilityRedBake198831A50Hook = GenericInlineHook5(
            pTarget,
            (void *)hkAbilityRedBake198831A50Naked,
            6);
        if (oAbilityRedBake198831A50Hook)
        {
            anyOk = true;
            WriteLogFmt("[AbilityRedBake198] OK(831A50): tramp=0x%08X",
                (DWORD)(uintptr_t)oAbilityRedBake198831A50Hook);
        }
        else
        {
            WriteLog("[AbilityRedBake198] 831A50 hook failed");
        }
    }
    else
    {
        anyOk = true;
    }

    if (!oAbilityRedBake19883AF02Hook)
    {
        BYTE *pTarget = (BYTE *)(uintptr_t)ADDR_83AF02;
        oAbilityRedBake19883AF02Hook = GenericInlineHook5(
            pTarget,
            (void *)hkAbilityRedBake19883AF02Naked,
            6);
        if (oAbilityRedBake19883AF02Hook)
        {
            anyOk = true;
            WriteLogFmt("[AbilityRedBake198] OK(83AF02): tramp=0x%08X",
                (DWORD)(uintptr_t)oAbilityRedBake19883AF02Hook);
        }
        else
        {
            WriteLog("[AbilityRedBake198] 83AF02 hook failed");
        }
    }
    else
    {
        anyOk = true;
    }

    return anyOk;
}

static bool SetupAbilityRedFinalValueHooks()
{
    bool anyOk = false;

    if (!oAbilityRedFinalCalc84BE40Fn)
    {
        BYTE *pTarget = FollowJmpChain((void *)ADDR_84BE40);
        if (!pTarget)
        {
            WriteLog("[AbilityRedFinal] 84BE40 target missing");
        }
        else
        {
            int copyLen = CalcMinCopyLen(pTarget);
            if (copyLen < 5)
                copyLen = 5;

            oAbilityRedFinalCalc84BE40Fn =
                (tAbilityRedFinalCalc7Fn)GenericInlineHook5(
                    pTarget,
                    (void *)hkAbilityRedFinalCalc84BE40,
                    copyLen);
            if (!oAbilityRedFinalCalc84BE40Fn)
            {
                WriteLog("[AbilityRedFinal] 84BE40 hook failed");
            }
            else
            {
                WriteLogFmt("[AbilityRedFinal] OK(84BE40): entry=0x%08X tramp=0x%08X copyLen=%d",
                    (DWORD)(uintptr_t)pTarget,
                    (DWORD)(uintptr_t)oAbilityRedFinalCalc84BE40Fn,
                    copyLen);
            }
        }
    }
    if (oAbilityRedFinalCalc84BE40Fn)
        anyOk = true;

    if (!oAbilityRedFinalCalc84C470Fn)
    {
        BYTE *pTarget = FollowJmpChain((void *)ADDR_84C470);
        if (!pTarget)
        {
            WriteLog("[AbilityRedFinal] 84C470 target missing");
        }
        else
        {
            int copyLen = CalcMinCopyLen(pTarget);
            if (copyLen < 5)
                copyLen = 5;

            oAbilityRedFinalCalc84C470Fn =
                (tAbilityRedFinalCalc6Fn)GenericInlineHook5(
                    pTarget,
                    (void *)hkAbilityRedFinalCalc84C470,
                    copyLen);
            if (!oAbilityRedFinalCalc84C470Fn)
            {
                WriteLog("[AbilityRedFinal] 84C470 hook failed");
            }
            else
            {
                WriteLogFmt("[AbilityRedFinal] OK(84C470): entry=0x%08X tramp=0x%08X copyLen=%d",
                    (DWORD)(uintptr_t)pTarget,
                    (DWORD)(uintptr_t)oAbilityRedFinalCalc84C470Fn,
                    copyLen);
            }
        }
    }
    if (oAbilityRedFinalCalc84C470Fn)
        anyOk = true;

    if (!oAbilityRedFinalCalc84CA90Fn)
    {
        BYTE *pTarget = FollowJmpChain((void *)ADDR_84CA90);
        if (!pTarget)
        {
            WriteLog("[AbilityRedFinal] 84CA90 target missing");
        }
        else
        {
            int copyLen = CalcMinCopyLen(pTarget);
            if (copyLen < 5)
                copyLen = 5;

            oAbilityRedFinalCalc84CA90Fn =
                (tAbilityRedFinalCalc5Fn)GenericInlineHook5(
                    pTarget,
                    (void *)hkAbilityRedFinalCalc84CA90,
                    copyLen);
            if (!oAbilityRedFinalCalc84CA90Fn)
            {
                WriteLog("[AbilityRedFinal] 84CA90 hook failed");
            }
            else
            {
                WriteLogFmt("[AbilityRedFinal] OK(84CA90): entry=0x%08X tramp=0x%08X copyLen=%d",
                    (DWORD)(uintptr_t)pTarget,
                    (DWORD)(uintptr_t)oAbilityRedFinalCalc84CA90Fn,
                    copyLen);
            }
        }
    }
    if (oAbilityRedFinalCalc84CA90Fn)
        anyOk = true;

    if (!oAbilityRedFinalCalc84CBD0Fn)
    {
        BYTE *pTarget = FollowJmpChain((void *)ADDR_84CBD0);
        if (!pTarget)
        {
            WriteLog("[AbilityRedFinal] 84CBD0 target missing");
        }
        else
        {
            int copyLen = CalcMinCopyLen(pTarget);
            if (copyLen < 5)
                copyLen = 5;

            oAbilityRedFinalCalc84CBD0Fn =
                (tAbilityRedFinalCalc5Fn)GenericInlineHook5(
                    pTarget,
                    (void *)hkAbilityRedFinalCalc84CBD0,
                    copyLen);
            if (!oAbilityRedFinalCalc84CBD0Fn)
            {
                WriteLog("[AbilityRedFinal] 84CBD0 hook failed");
            }
            else
            {
                WriteLogFmt("[AbilityRedFinal] OK(84CBD0): entry=0x%08X tramp=0x%08X copyLen=%d",
                    (DWORD)(uintptr_t)pTarget,
                    (DWORD)(uintptr_t)oAbilityRedFinalCalc84CBD0Fn,
                    copyLen);
            }
        }
    }
    if (oAbilityRedFinalCalc84CBD0Fn)
        anyOk = true;

    return anyOk;
}

static bool SetupAbilityRedDisplayCandidateHook()
{
    if (oAbilityRedDisplayCandidateFn)
        return true;

    BYTE *pTarget = FollowJmpChain((void *)ADDR_AE0E60);
    if (!pTarget)
    {
        WriteLog("[AbilityRedDisplay] AE0E60 target missing");
        return false;
    }

    int copyLen = CalcMinCopyLen(pTarget);
    if (copyLen < 5)
        copyLen = 5;

    oAbilityRedDisplayCandidateFn =
        (tAbilityRedDisplayCandidateFn)GenericInlineHook5(
            pTarget,
            (void *)hkAbilityRedDisplayCandidateFunction,
            copyLen);
    if (!oAbilityRedDisplayCandidateFn)
    {
        WriteLog("[AbilityRedDisplay] AE0E60 hook failed");
        return false;
    }

    WriteLogFmt("[AbilityRedDisplay] OK(AE0E60): entry=0x%08X tramp=0x%08X copyLen=%d",
        (DWORD)(uintptr_t)pTarget,
        (DWORD)(uintptr_t)oAbilityRedDisplayCandidateFn,
        copyLen);
    return true;
}

static bool SetupPotentialTextDisplayHook()
{
    if (oPotentialTextFormat)
        return true;

    BYTE* pTarget = FollowJmpChain((void*)ADDR_A4CA60);
    if (!pTarget)
    {
        WriteLog("[PotentialTextHook] A4CA60 target missing");
        return false;
    }

    const int copyLen = CalcMinCopyLen(pTarget);
    if (copyLen < 5)
    {
        WriteLogFmt("[PotentialTextHook] A4CA60 invalid copyLen=%d", copyLen);
        return false;
    }

    oPotentialTextFormat = (tPotentialTextFormatFn)GenericInlineHook5(
        pTarget,
        (void*)hkPotentialTextFormat,
        copyLen);
    if (!oPotentialTextFormat)
    {
        WriteLog("[PotentialTextHook] A4CA60 hook failed");
        return false;
    }

    WriteLogFmt("[PotentialTextHook] OK(A4CA60): entry=0x%08X tramp=0x%08X copyLen=%d",
        (DWORD)(uintptr_t)pTarget,
        (DWORD)(uintptr_t)oPotentialTextFormat,
        copyLen);
    return true;
}

static bool SetupLocalIndependentPotentialSkillLevelDisplayHook()
{
    if (oLocalIndependentPotentialSkillLevelDisplay)
        return true;

    BYTE* pTarget = FollowJmpChain((void*)ADDR_AE0B23);
    if (!pTarget)
        pTarget = TryFollowAbsoluteRegisterJumpStub((BYTE*)ADDR_AE0B23);
    if (pTarget && pTarget[0] >= 0xB8 && pTarget[0] <= 0xBF)
    {
        BYTE* pResolvedStub = TryFollowAbsoluteRegisterJumpStub(pTarget);
        if (pResolvedStub)
            pTarget = pResolvedStub;
    }
    if (!pTarget)
    {
        WriteLog("[IndependentBuffLocalDisplay] AE0B23 target missing");
        return false;
    }

    const int copyLen = CalcMinCopyLen(pTarget);
    if (copyLen < 5)
    {
        WriteLogFmt("[IndependentBuffLocalDisplay] AE0B23 invalid copyLen=%d", copyLen);
        return false;
    }

    oLocalIndependentPotentialSkillLevelDisplay = GenericInlineHook5(
        pTarget,
        (void*)hkLocalIndependentPotentialSkillLevelDisplayNaked,
        copyLen);
    if (!oLocalIndependentPotentialSkillLevelDisplay)
    {
        WriteLog("[IndependentBuffLocalDisplay] AE0B23 hook failed");
        return false;
    }

    WriteLogFmt("[IndependentBuffLocalDisplay] OK(AE0B23): entry=0x%08X tramp=0x%08X copyLen=%d",
        (DWORD)(uintptr_t)pTarget,
        (DWORD)(uintptr_t)oLocalIndependentPotentialSkillLevelDisplay,
        copyLen);
    return true;
}

static bool SetupLocalIndependentPotentialDamageDisplayHook()
{
    if (oLocalIndependentPotentialDamageDisplay)
        return true;

    BYTE* pTarget = FollowJmpChain((void*)ADDR_AE0FDC);
    if (!pTarget)
        pTarget = TryFollowAbsoluteRegisterJumpStub((BYTE*)ADDR_AE0FDC);
    if (pTarget && pTarget[0] >= 0xB8 && pTarget[0] <= 0xBF)
    {
        BYTE* pResolvedStub = TryFollowAbsoluteRegisterJumpStub(pTarget);
        if (pResolvedStub)
            pTarget = pResolvedStub;
    }
    if (!pTarget)
    {
        WriteLog("[IndependentBuffLocalDisplay] AE0FDC target missing");
        return false;
    }

    const int copyLen = CalcMinCopyLen(pTarget);
    if (copyLen < 5)
    {
        WriteLogFmt("[IndependentBuffLocalDisplay] AE0FDC invalid copyLen=%d", copyLen);
        return false;
    }

    oLocalIndependentPotentialDamageDisplay = GenericInlineHook5(
        pTarget,
        (void*)hkLocalIndependentPotentialDamageDisplayNaked,
        copyLen);
    if (!oLocalIndependentPotentialDamageDisplay)
    {
        WriteLog("[IndependentBuffLocalDisplay] AE0FDC hook failed");
        return false;
    }

    WriteLogFmt("[IndependentBuffLocalDisplay] OK(AE0FDC): entry=0x%08X tramp=0x%08X copyLen=%d",
        (DWORD)(uintptr_t)pTarget,
        (DWORD)(uintptr_t)oLocalIndependentPotentialDamageDisplay,
        copyLen);
    return true;
}

// ============================================================================
// 技能释放分类 hook安装
// 说明：
//   00B31349 不是安全 hook 入口：多个原生分支会直接跳到 00B3134D，
//   5-byte inline patch 会覆盖共享跳转目标，Echo(1005) 会落入补丁中间字节。
//   00B3144D 是释放高层分类块，不是函数入口；首条指令长度 6 字节。
//   00B2F370 是独立函数分支入口（部分技能路径会直接走这里），需要在入口改 arg0(skillId)。
//   这里使用 GenericInlineHook5(copyLen=6)，保证整条 cmp 指令被完整搬到 trampoline。
// ============================================================================
static bool SetupSkillReleaseClassifierHook()
{
    if (!ssw::runtime::IsFeatureEnabled(ssw::runtime::FeatureSwitchId::CoreSkillReleaseClassifierHooks))
    {
        WriteLog("[SkillReleaseHook] disabled by feature switch");
        return true;
    }

    WriteLog("[SkillReleaseHook] skip(B31349 root): unsafe split-label, B3134D is shared jump target");

    bool branchOk = false;
    BYTE *pTarget = FollowJmpChain((void *)ADDR_B3144D);
    if (!pTarget)
    {
        WriteLog("[SkillReleaseHook] classifier branch target missing");
    }
    else
    {
        oSkillReleaseClassifier = GenericInlineHook5(pTarget, (void *)hkSkillReleaseClassifierNaked, 6);
        if (!oSkillReleaseClassifier)
        {
            WriteLog("[SkillReleaseHook] classifier branch hook failed");
        }
        else
        {
            branchOk = true;
            WriteLogFmt("[SkillReleaseHook] OK(B3144D): tramp=0x%08X", (DWORD)(uintptr_t)oSkillReleaseClassifier);
        }
    }

    bool b2f370Ok = false;
    BYTE *pB2F370 = FollowJmpChain((void *)ADDR_B2F370);
    if (!pB2F370)
    {
        WriteLog("[SkillReleaseHook] B2F370 target missing");
    }
    else
    {
        // 00B2F370 前三条指令长度 = 2 + 5 + 6 = 13
        oSkillReleaseClassifierB2F370 = GenericInlineHook5(
            pB2F370,
            (void *)hkSkillReleaseClassifierB2F370Naked,
            13);
        if (!oSkillReleaseClassifierB2F370)
        {
            WriteLog("[SkillReleaseHook] B2F370 hook failed");
        }
        else
        {
            b2f370Ok = true;
            WriteLogFmt("[SkillReleaseHook] OK(B2F370): tramp=0x%08X", (DWORD)(uintptr_t)oSkillReleaseClassifierB2F370);
        }
    }

    bool echoPostGuardOk = false;
    if (!oEchoOfHeroPostReleaseContextGuardB3355C)
    {
        oEchoOfHeroPostReleaseContextGuardB3355C = GenericInlineHook5(
            (BYTE *)ADDR_B3355C,
            (void *)hkEchoOfHeroPostReleaseContextGuardB3355CNaked,
            7);
        if (!oEchoOfHeroPostReleaseContextGuardB3355C)
        {
            WriteLog("[SkillReleaseHook] B3355C echo post-release guard hook failed");
        }
        else
        {
            echoPostGuardOk = true;
            WriteLogFmt(
                "[SkillReleaseHook] OK(B3355C echo-post): tramp=0x%08X",
                (DWORD)(uintptr_t)oEchoOfHeroPostReleaseContextGuardB3355C);
        }
    }
    else
    {
        echoPostGuardOk = true;
    }

    if (!branchOk && !b2f370Ok && !echoPostGuardOk)
    {
        return false;
    }
    return true;
}

static bool SetupSkillPresentationHook()
{
    if (!ssw::runtime::IsFeatureEnabled(ssw::runtime::FeatureSwitchId::CoreSkillPresentationHooks))
    {
        WriteLog("[SkillVisualHook] disabled by feature switch");
        return true;
    }

    oSkillPresentationDispatch = (tSkillPresentationDispatch)InstallInlineHook(
        ADDR_ABAF70, (void *)hkSkillPresentationDispatch);
    if (!oSkillPresentationDispatch)
    {
        WriteLog("[SkillVisualHook] hook failed");
        return false;
    }

    WriteLogFmt("[SkillVisualHook] OK(ABAF70): tramp=0x%08X", (DWORD)(uintptr_t)oSkillPresentationDispatch);
    return true;
}

static bool SetupNativeButtonAssetPathHook()
{
    if (ShouldUseVirtualSuperButtonRuntime())
    {
        WriteLog("[BtnSkinHook] retired: virtual self-render button mode");
        return true;
    }

    if (!ssw::runtime::IsFeatureEnabled(ssw::runtime::FeatureSwitchId::UiNativeButtonHooks))
    {
        WriteLog("[BtnSkinHook] disabled by feature switch");
        return true;
    }

    if (!ENABLE_NATIVE_BUTTON_SKIN_REMAP)
    {
        WriteLog("[BtnSkinHook] disabled");
        return true;
    }

    oMakeGameWStringHooked = (tMakeGameWString)InstallInlineHook(
        ADDR_402F60, (void *)hkMakeGameWString);
    if (!oMakeGameWStringHooked)
    {
        WriteLog("[BtnSkinHook] hook failed: 402F60");
        return false;
    }

    WriteLogFmt("[BtnSkinHook] OK(402F60): tramp=0x%08X", (DWORD)(uintptr_t)oMakeGameWStringHooked);
    return true;
}

static bool SetupNativeButtonResolveHook()
{
    if (ShouldUseVirtualSuperButtonRuntime())
    {
        WriteLog("[BtnResolveHook] retired: virtual self-render button mode");
        return true;
    }

    if (!ssw::runtime::IsFeatureEnabled(ssw::runtime::FeatureSwitchId::UiNativeButtonHooks))
    {
        WriteLog("[BtnResolveHook] disabled by feature switch");
        return true;
    }

    oButtonResolveCurrentDrawObj = (tButtonResolveCurrentDrawObj)InstallInlineHook(
        ADDR_506EE0, (void *)hkButtonResolveCurrentDrawObj);
    if (!oButtonResolveCurrentDrawObj)
    {
        WriteLog("[BtnResolveHook] hook failed: 506EE0");
        return false;
    }

    WriteLogFmt("[BtnResolveHook] OK(506EE0): tramp=0x%08X", (DWORD)(uintptr_t)oButtonResolveCurrentDrawObj);
    return true;
}

static bool SetupNativeButtonDrawHook()
{
    if (ShouldUseVirtualSuperButtonRuntime())
    {
        WriteLog("[BtnDrawHook] retired: virtual self-render button mode");
        return true;
    }

    if (!ssw::runtime::IsFeatureEnabled(ssw::runtime::FeatureSwitchId::UiNativeButtonHooks))
    {
        WriteLog("[BtnDrawHook] disabled by feature switch");
        return true;
    }

    oButtonDrawCurrentState = (tButtonDrawCurrentState)InstallInlineHook(
        ADDR_507020, (void *)hkButtonDrawCurrentState);
    if (!oButtonDrawCurrentState)
    {
        WriteLog("[BtnDrawHook] hook failed: 507020");
        return false;
    }

    WriteLogFmt("[BtnDrawHook] OK(507020): tramp=0x%08X", (DWORD)(uintptr_t)oButtonDrawCurrentState);
    return true;
}

