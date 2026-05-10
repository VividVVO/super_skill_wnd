static bool SetupNativeButtonMetricHooks()
{
    bool ok = false;

    oButtonMetric507DF0 = (tButtonMetricCurrent)InstallInlineHook(
        ADDR_507DF0, (void *)hkButtonMetric507DF0);
    if (oButtonMetric507DF0)
    {
        ok = true;
        WriteLogFmt("[BtnMetricHook] OK(507DF0): tramp=0x%08X", (DWORD)(uintptr_t)oButtonMetric507DF0);
    }
    else
    {
        WriteLog("[BtnMetricHook] hook failed: 507DF0");
    }

    oButtonMetric507ED0 = (tButtonMetricCurrent)InstallInlineHook(
        ADDR_507ED0, (void *)hkButtonMetric507ED0);
    if (oButtonMetric507ED0)
    {
        ok = true;
        WriteLogFmt("[BtnMetricHook] OK(507ED0): tramp=0x%08X", (DWORD)(uintptr_t)oButtonMetric507ED0);
    }
    else
    {
        WriteLog("[BtnMetricHook] hook failed: 507ED0");
    }

    // v16.1: hook 5095A0 to block state changes for SuperBtn in stableNormal mode
    oButtonRefreshState5095A0 = (tRefreshButtonState)InstallInlineHook(
        ADDR_5095A0, (void *)hkButtonRefreshState5095A0);
    if (oButtonRefreshState5095A0)
    {
        ok = true;
        WriteLogFmt("[BtnRefreshHook] OK(5095A0): tramp=0x%08X", (DWORD)(uintptr_t)oButtonRefreshState5095A0);
    }
    else
    {
        WriteLog("[BtnRefreshHook] hook failed: 5095A0");
    }

    // v17.7b: hook sub_529640 的矩形写入点 (0x52972E)
    // 覆盖 8 字节: 52972E(4) + 529732(4) → 5字节jmp + 3字节nop
    {
        BYTE *pPatch = (BYTE *)ADDR_52972E;
        DWORD oldProt = 0;
        if (VirtualProtect(pPatch, 8, PAGE_EXECUTE_READWRITE, &oldProt))
        {
            // 5 字节 jmp rel32
            pPatch[0] = 0xE9;
            *(DWORD *)(pPatch + 1) = (DWORD)((uintptr_t)hkRectWrite52972E - (uintptr_t)pPatch - 5);
            // 3 字节 nop
            pPatch[5] = 0x90;
            pPatch[6] = 0x90;
            pPatch[7] = 0x90;
            VirtualProtect(pPatch, 8, oldProt, &oldProt);
            FlushInstructionCache(GetCurrentProcess(), pPatch, 8);
            WriteLog("[RectWriteHook] OK: 0x52972E patched");
        }
        else
        {
            WriteLog("[RectWriteHook] FAIL: VirtualProtect failed");
        }
    }

    return ok;
}

static bool SetupMountMovementObservationHooks()
{
    if (oMountMovementDataLookup888B30)
    {
        return true;
    }

    oMountMovementDataLookup888B30 = (tMountMovementDataLookupFn)InstallInlineHook(
        ADDR_888B30, (void *)hkMountMovementDataLookup888B30);
    if (!oMountMovementDataLookup888B30)
    {
        WriteLog("[MountMoveObserve] hook failed: 888B30");
        return false;
    }

    WriteLogFmt("[MountMoveObserve] OK(888B30): tramp=0x%08X",
                (DWORD)(uintptr_t)oMountMovementDataLookup888B30);
    return true;
}

static bool SetupMountedFlightPhysicsSpeedHooks()
{
    bool ok = false;

    if (!oMountedFlightPhysicsDispatchB87E60)
    {
        BYTE *pTarget = FollowJmpChain((void *)ADDR_B87E60);
        if (!pTarget)
        {
            WriteLog("[MountFlightSpeed] B87E60 target missing");
        }
        else
        {
            int copyLen = CalcMinCopyLen(pTarget);
            if (copyLen < 5)
                copyLen = 5;

            oMountedFlightPhysicsDispatchB87E60 =
                (tMountedFlightPhysicsDispatchFn)GenericInlineHook5(
                    pTarget,
                    (void *)hkMountedFlightPhysicsDispatchB87E60,
                    copyLen);
            if (oMountedFlightPhysicsDispatchB87E60)
            {
                ok = true;
                WriteLogFmt("[MountFlightSpeed] OK(B87E60): entry=0x%08X tramp=0x%08X copyLen=%d",
                    (DWORD)(uintptr_t)pTarget,
                    (DWORD)(uintptr_t)oMountedFlightPhysicsDispatchB87E60,
                    copyLen);
            }
            else
            {
                WriteLog("[MountFlightSpeed] B87E60 hook failed");
            }
        }
    }
    else
    {
        ok = true;
    }

    if (!oMountedFlightPhysicsStepB83C90)
    {
        BYTE *pTarget = FollowJmpChain((void *)ADDR_B83C90);
        if (!pTarget)
        {
            WriteLog("[MountFlightSpeed] B83C90 target missing");
        }
        else
        {
            int copyLen = CalcMinCopyLen(pTarget);
            if (copyLen < 5)
                copyLen = 5;

            oMountedFlightPhysicsStepB83C90 =
                (tMountedFlightPhysicsStepFn)GenericInlineHook5(
                    pTarget,
                    (void *)hkMountedFlightPhysicsStepB83C90,
                    copyLen);
            if (oMountedFlightPhysicsStepB83C90)
            {
                ok = true;
                WriteLogFmt("[MountFlightSpeed] OK(B83C90): entry=0x%08X tramp=0x%08X copyLen=%d",
                    (DWORD)(uintptr_t)pTarget,
                    (DWORD)(uintptr_t)oMountedFlightPhysicsStepB83C90,
                    copyLen);
            }
            else
            {
                WriteLog("[MountFlightSpeed] B83C90 hook failed");
            }
        }
    }
    else
    {
        ok = true;
    }

    if (!oMountedFlightPhysicsStepB844D0)
    {
        BYTE *pTarget = FollowJmpChain((void *)ADDR_B844D0);
        if (!pTarget)
        {
            WriteLog("[MountFlightSpeed] B844D0 target missing");
        }
        else
        {
            int copyLen = CalcMinCopyLen(pTarget);
            if (copyLen < 5)
                copyLen = 5;

            oMountedFlightPhysicsStepB844D0 =
                (tMountedFlightPhysicsStepFn)GenericInlineHook5(
                    pTarget,
                    (void *)hkMountedFlightPhysicsStepB844D0,
                    copyLen);
            if (oMountedFlightPhysicsStepB844D0)
            {
                ok = true;
                WriteLogFmt("[MountFlightSpeed] OK(B844D0): entry=0x%08X tramp=0x%08X copyLen=%d",
                    (DWORD)(uintptr_t)pTarget,
                    (DWORD)(uintptr_t)oMountedFlightPhysicsStepB844D0,
                    copyLen);
            }
            else
            {
                WriteLog("[MountFlightSpeed] B844D0 hook failed");
            }
        }
    }
    else
    {
        ok = true;
    }

    if (!oMountedFlightPhysicsStateB84D70)
    {
        BYTE *pTarget = FollowJmpChain((void *)ADDR_B84D70);
        if (!pTarget)
        {
            WriteLog("[MountFlightSpeed] B84D70 target missing");
        }
        else
        {
            int copyLen = CalcMinCopyLen(pTarget);
            if (copyLen < 5)
                copyLen = 5;

            oMountedFlightPhysicsStateB84D70 =
                (tMountedFlightPhysicsStateFn)GenericInlineHook5(
                    pTarget,
                    (void *)hkMountedFlightPhysicsStateB84D70,
                    copyLen);
            if (oMountedFlightPhysicsStateB84D70)
            {
                ok = true;
                WriteLogFmt("[MountFlightSpeed] OK(B84D70): entry=0x%08X tramp=0x%08X copyLen=%d",
                    (DWORD)(uintptr_t)pTarget,
                    (DWORD)(uintptr_t)oMountedFlightPhysicsStateB84D70,
                    copyLen);
            }
            else
            {
                WriteLog("[MountFlightSpeed] B84D70 hook failed");
            }
        }
    }
    else
    {
        ok = true;
    }

    if (!oMountedFlightPhysicsFinalizeB851F0)
    {
        BYTE *pTarget = FollowJmpChain((void *)ADDR_B851F0);
        if (!pTarget)
        {
            WriteLog("[MountFlightSpeed] B851F0 target missing");
        }
        else
        {
            int copyLen = CalcMinCopyLen(pTarget);
            if (copyLen < 5)
                copyLen = 5;

            oMountedFlightPhysicsFinalizeB851F0 =
                (tMountedFlightPhysicsFinalizeFn)GenericInlineHook5(
                    pTarget,
                    (void *)hkMountedFlightPhysicsFinalizeB851F0,
                    copyLen);
            if (oMountedFlightPhysicsFinalizeB851F0)
            {
                ok = true;
                WriteLogFmt("[MountFlightSpeed] OK(B851F0): entry=0x%08X tramp=0x%08X copyLen=%d",
                    (DWORD)(uintptr_t)pTarget,
                    (DWORD)(uintptr_t)oMountedFlightPhysicsFinalizeB851F0,
                    copyLen);
            }
            else
            {
                WriteLog("[MountFlightSpeed] B851F0 hook failed");
            }
        }
    }
    else
    {
        ok = true;
    }

    if (!oMountedFlightPhysicsStepB88090)
    {
        BYTE *pTarget = FollowJmpChain((void *)ADDR_B88090);
        if (!pTarget)
        {
            WriteLog("[MountFlightSpeed] B88090 target missing");
        }
        else
        {
            int copyLen = CalcMinCopyLen(pTarget);
            if (copyLen < 5)
                copyLen = 5;

            oMountedFlightPhysicsStepB88090 =
                (tMountedFlightPhysicsStepFn)GenericInlineHook5(
                    pTarget,
                    (void *)hkMountedFlightPhysicsStepB88090,
                    copyLen);
            if (oMountedFlightPhysicsStepB88090)
            {
                ok = true;
                WriteLogFmt("[MountFlightSpeed] OK(B88090): entry=0x%08X tramp=0x%08X copyLen=%d",
                    (DWORD)(uintptr_t)pTarget,
                    (DWORD)(uintptr_t)oMountedFlightPhysicsStepB88090,
                    copyLen);
            }
            else
            {
                WriteLog("[MountFlightSpeed] B88090 hook failed");
            }
        }
    }
    else
    {
        ok = true;
    }

    if (!oMountedFlightPhysicsVerticalB8FE30)
    {
        BYTE *pTarget = FollowJmpChain((void *)ADDR_B8FE30);
        if (!pTarget)
        {
            WriteLog("[MountFlightSpeed] B8FE30 target missing");
        }
        else
        {
            int copyLen = CalcMinCopyLen(pTarget);
            if (copyLen < 5)
                copyLen = 5;

            oMountedFlightPhysicsVerticalB8FE30 =
                (tMountedFlightPhysicsVerticalFn)GenericInlineHook5(
                    pTarget,
                    (void *)hkMountedFlightPhysicsVerticalB8FE30,
                    copyLen);
            if (oMountedFlightPhysicsVerticalB8FE30)
            {
                ok = true;
                WriteLogFmt("[MountFlightSpeed] OK(B8FE30): entry=0x%08X tramp=0x%08X copyLen=%d",
                    (DWORD)(uintptr_t)pTarget,
                    (DWORD)(uintptr_t)oMountedFlightPhysicsVerticalB8FE30,
                    copyLen);
            }
            else
            {
                WriteLog("[MountFlightSpeed] B8FE30 hook failed");
            }
        }
    }
    else
    {
        ok = true;
    }

    return ok;
}

static bool SetupSkillNativeIdGateHooks()
{
    bool ok = false;

    BYTE *pGate7CE790 = FollowJmpChain((void *)ADDR_7CE790);
    if (pGate7CE790)
    {
        oSkillNativeIdGate7CE790 = (tSkillNativeIdGateFn)GenericInlineHook5(
            pGate7CE790, (void *)hkSkillNativeIdGate7CE790, 9);
    }
    else
    {
        oSkillNativeIdGate7CE790 = nullptr;
    }
    if (oSkillNativeIdGate7CE790)
    {
        ok = true;
        WriteLogFmt("[SkillGate] OK(7CE790): tramp=0x%08X", (DWORD)(uintptr_t)oSkillNativeIdGate7CE790);
    }
    else
    {
        WriteLog("[SkillGate] hook failed: 7CE790");
    }

    BYTE *pGate7D0000 = FollowJmpChain((void *)ADDR_7D0000);
    if (pGate7D0000)
    {
        oSkillNativeIdGate7D0000 = (tSkillNativeIdGateFn)GenericInlineHook5(
            pGate7D0000, (void *)hkSkillNativeIdGate7D0000, 9);
    }
    else
    {
        oSkillNativeIdGate7D0000 = nullptr;
    }
    if (oSkillNativeIdGate7D0000)
    {
        ok = true;
        WriteLogFmt("[SkillGate] OK(7D0000): tramp=0x%08X", (DWORD)(uintptr_t)oSkillNativeIdGate7D0000);
    }
    else
    {
        WriteLog("[SkillGate] hook failed: 7D0000");
    }

    if (SetupMountedDoubleJumpRuntimeFeatureHooks())
        ok = true;

    // 坐骑攀爬/绳索动作在 0042C300 case 51/52 前会先过 4069E0 白名单。
    // 这里把扩展骑宠补进去，避免只改服务端后出现“能飞但不能爬”的半支持状态。
    if (SetupMountClimbGateFeatureHooks())
        ok = true;

    if (SetupMountFlightMappingFeatureHooks())
        ok = true;
    if (SetupMountMovementRuntimeFeatureHooks())
        ok = true;

    return ok;
}

static bool SetupSkillLevelLookupHooks()
{
    bool ok = false;

    oSkillLevelBase = (tSkillLevelBaseFn)InstallInlineHook(
        ADDR_7DA7D0, (void *)hkSkillLevelBase);
    if (oSkillLevelBase)
    {
        ok = true;
        WriteLogFmt("[SkillLevelHook] OK(7DA7D0): tramp=0x%08X", (DWORD)(uintptr_t)oSkillLevelBase);
    }
    else
    {
        WriteLog("[SkillLevelHook] hook failed: 7DA7D0");
    }

    oSkillLevelCurrent = (tSkillLevelCurrentFn)InstallInlineHook(
        ADDR_7DBC50, (void *)hkSkillLevelCurrent);
    if (oSkillLevelCurrent)
    {
        ok = true;
        WriteLogFmt("[SkillLevelHook] OK(7DBC50): tramp=0x%08X", (DWORD)(uintptr_t)oSkillLevelCurrent);
    }
    else
    {
        WriteLog("[SkillLevelHook] hook failed: 7DBC50");
    }

    if (SetupSkillEffectPassiveBonusHooks())
        ok = true;
    else
        WriteLog("[SuperPassiveEffectHook] effect hooks failed (non-fatal)");

    return ok;
}

static bool SetupSkillEffectPassiveBonusHooks()
{
    bool ok = false;

    if (!oSkillEffect800260)
    {
        oSkillEffect800260 = (tSkillEffectFn)InstallInlineHook(
            ADDR_800260, (void *)hkSkillEffect800260);
        if (oSkillEffect800260)
        {
            ok = true;
            WriteLogFmt("[SuperPassiveEffectHook] OK(800260): tramp=0x%08X", (DWORD)(uintptr_t)oSkillEffect800260);
        }
        else
        {
            WriteLog("[SuperPassiveEffectHook] hook failed: 800260");
        }
    }
    else
    {
        ok = true;
    }

    if (!oSkillEffect800580)
    {
        oSkillEffect800580 = (tSkillEffectFn)InstallInlineHook(
            ADDR_800580, (void *)hkSkillEffect800580);
        if (oSkillEffect800580)
        {
            ok = true;
            WriteLogFmt("[SuperPassiveEffectHook] OK(800580): tramp=0x%08X", (DWORD)(uintptr_t)oSkillEffect800580);
        }
        else
        {
            WriteLog("[SuperPassiveEffectHook] hook failed: 800580");
        }
    }
    else
    {
        ok = true;
    }

    if (!oPassiveEffectDamage43DE00)
    {
        oPassiveEffectDamage43DE00 = (tPassiveEffectGetterFn)InstallInlineHook(
            ADDR_43DE00, (void *)hkPassiveEffectDamage43DE00);
        if (oPassiveEffectDamage43DE00)
        {
            ok = true;
            WriteLogFmt("[SuperPassiveGetterHook] OK(43DE00 damage): tramp=0x%08X", (DWORD)(uintptr_t)oPassiveEffectDamage43DE00);
        }
        else
        {
            WriteLog("[SuperPassiveGetterHook] hook failed: 43DE00 damage");
        }
    }
    else
    {
        ok = true;
    }

    if (!oPassiveEffectDamage43DE50)
    {
        oPassiveEffectDamage43DE50 = (tPassiveEffectGetterFn)InstallInlineHook(
            ADDR_43DE50, (void *)hkPassiveEffectDamage43DE50);
        if (oPassiveEffectDamage43DE50)
        {
            ok = true;
            WriteLogFmt("[SuperPassiveGetterHook] OK(43DE50 damageAlt): tramp=0x%08X", (DWORD)(uintptr_t)oPassiveEffectDamage43DE50);
        }
        else
        {
            WriteLog("[SuperPassiveGetterHook] hook failed: 43DE50 damageAlt");
        }
    }
    else
    {
        ok = true;
    }

    if (!oPassiveEffectAttackCount5E9EE0)
    {
        oPassiveEffectAttackCount5E9EE0 = (tPassiveEffectGetterFn)InstallInlineHook(
            ADDR_5E9EE0, (void *)hkPassiveEffectAttackCount5E9EE0);
        if (oPassiveEffectAttackCount5E9EE0)
        {
            ok = true;
            WriteLogFmt("[SuperPassiveGetterHook] OK(5E9EE0 attackCount): tramp=0x%08X", (DWORD)(uintptr_t)oPassiveEffectAttackCount5E9EE0);
        }
        else
        {
            WriteLog("[SuperPassiveGetterHook] hook failed: 5E9EE0 attackCount");
        }
    }
    else
    {
        ok = true;
    }

    if (!oPassiveEffectMobCount7D1990)
    {
        oPassiveEffectMobCount7D1990 = (tPassiveEffectGetterFn)InstallInlineHook(
            ADDR_7D1990, (void *)hkPassiveEffectMobCount7D1990);
        if (oPassiveEffectMobCount7D1990)
        {
            ok = true;
            WriteLogFmt("[SuperPassiveGetterHook] OK(7D1990 mobCount): tramp=0x%08X", (DWORD)(uintptr_t)oPassiveEffectMobCount7D1990);
        }
        else
        {
            WriteLog("[SuperPassiveGetterHook] hook failed: 7D1990 mobCount");
        }
    }
    else
    {
        ok = true;
    }

    if (!oPassiveEffectAttackCount7D19E0)
    {
        oPassiveEffectAttackCount7D19E0 = (tPassiveEffectGetterFn)InstallInlineHook(
            ADDR_7D19E0, (void *)hkPassiveEffectAttackCount7D19E0);
        if (oPassiveEffectAttackCount7D19E0)
        {
            ok = true;
            WriteLogFmt("[SuperPassiveGetterHook] OK(7D19E0 attackCount): tramp=0x%08X", (DWORD)(uintptr_t)oPassiveEffectAttackCount7D19E0);
        }
        else
        {
            WriteLog("[SuperPassiveGetterHook] hook failed: 7D19E0 attackCount");
        }
    }
    else
    {
        ok = true;
    }

    if (!oPassiveEffectIgnore7D28E0)
    {
        oPassiveEffectIgnore7D28E0 = (tPassiveEffectGetterFn)InstallInlineHook(
            ADDR_7D28E0, (void *)hkPassiveEffectIgnore7D28E0);
        if (oPassiveEffectIgnore7D28E0)
        {
            ok = true;
            WriteLogFmt("[SuperPassiveGetterHook] OK(7D28E0 ignoreMobpdpR): tramp=0x%08X", (DWORD)(uintptr_t)oPassiveEffectIgnore7D28E0);
        }
        else
        {
            WriteLog("[SuperPassiveGetterHook] hook failed: 7D28E0 ignoreMobpdpR");
        }
    }
    else
    {
        ok = true;
    }

    return ok;
}

static bool SetupNativeTextGlyphHook()
{
    if (SafeIsBadReadPtr((void *)ADDR_5000E520, 8))
    {
        WriteLogFmt("[NativeText] glyph target unreadable: 0x%08X", ADDR_5000E520);
        return false;
    }

    oNativeGlyphLookup = (tNativeGlyphLookupFn)InstallInlineHook(
        ADDR_5000E520, (void *)hkNativeGlyphLookup);
    if (!oNativeGlyphLookup)
    {
        WriteLog("[NativeText] glyph hook failed: 5000E520");
        return false;
    }

    RetroSkillDWriteRegisterNativeGlyphLookup((void *)oNativeGlyphLookup);
    WriteLogFmt("[NativeText] OK(5000E520): tramp=0x%08X", (DWORD)(uintptr_t)oNativeGlyphLookup);
    return true;
}

// ============================================================================
// route-B child draw接入确认
// 说明：
//   v10.1 起不再 hook 52AA90 函数体；draw 由自定义 VT1 槽位接到 SuperCWndDraw
// ============================================================================
static bool SetupSuperChildDrawHook()
{
    WriteLog("[SuperChildDrawHook] VT1-draw route enabled");
    return true;
}

// ============================================================================
// SkillWnd 移动 hook安装
// ============================================================================
static bool SetupSkillWndMoveHook()
{
    oSkillWndMove = (tSkillWndMove)InstallInlineHook(
        ADDR_9D95A0, (void *)hkSkillWndMoveNaked);
    if (!oSkillWndMove)
    {
        WriteLog("[MoveHook] Hook failed");
        return false;
    }
    WriteLogFmt("[MoveHook] OK: tramp=0x%08X", (DWORD)oSkillWndMove);
    return true;
}

// ============================================================================
// SkillWnd refresh hook安装
// ============================================================================
static bool SetupSkillWndRefreshHook()
{
    oSkillWndRefresh = (tSkillWndRefresh)InstallInlineHook(
        ADDR_9E1770, (void *)hkSkillWndRefreshNaked);
    if (!oSkillWndRefresh)
    {
        WriteLog("[RefreshHook] Hook failed");
        return false;
    }
    WriteLogFmt("[RefreshHook] OK: tramp=0x%08X", (DWORD)oSkillWndRefresh);
    return true;
}

// ============================================================================
// SkillWnd 绘制 Hook安装
// ============================================================================
static bool SetupSkillWndDrawHook()
{
    oSkillWndDraw = (tSkillWndDraw)InstallInlineHook(
        ADDR_9DEE30, (void *)hkSkillWndDrawNaked);
    if (!oSkillWndDraw)
    {
        WriteLog("[DrawHook] Hook failed");
        return false;
    }
    WriteLogFmt("[DrawHook] OK: tramp=0x%08X", (DWORD)oSkillWndDraw);
    return true;
}

static bool SetupPostB9F6E0TimingTestHook()
{
    if (!ENABLE_POST_B9F6E0_NATIVE_TIMING_TEST)
        return true;

    oPostB9F6E0DrawContinue = GenericInlineHook5(
        (BYTE *)ADDR_BBC965,
        (void *)hkPostB9F6E0DrawNaked,
        9);
    if (!oPostB9F6E0DrawContinue)
    {
        WriteLog("[PostUiTimingTest] hook failed at BBC965");
        return false;
    }

    WriteLogFmt("[PostUiTimingTest] OK(BBC965): tramp=0x%08X", (DWORD)(uintptr_t)oPostB9F6E0DrawContinue);
    return true;
}

// ============================================================================
// SkillWnd 析构 Hook安装
// ============================================================================
static bool SetupSkillWndDtorHook()
{
    oSkillWndDtor = (tSkillWndDtor)InstallInlineHook(
        ADDR_9E14D0, (void *)hkSkillWndDtorNaked);
    if (!oSkillWndDtor)
    {
        WriteLog("[DtorHook] Hook failed");
        return false;
    }
    WriteLogFmt("[DtorHook] OK: tramp=0x%08X", (DWORD)oSkillWndDtor);
    return true;
}

// ============================================================================
// WndProc Hook安装
// ============================================================================
