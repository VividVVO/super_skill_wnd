static bool SetupMountedUseFailPromptSuppressHook()
{
    if (oMountedUseFailPromptAE6260)
    {
        return true;
    }

    oMountedUseFailPromptAE6260 = (tMountedUseFailPromptFn)InstallInlineHook(
        ADDR_AE6260, (void *)hkMountedUseFailPromptAE6260);
    if (!oMountedUseFailPromptAE6260)
    {
        WriteLog("[MountDoubleJump] hook failed: AE6260");
        return false;
    }

    WriteLogFmt("[MountDoubleJump] OK(AE6260): tramp=0x%08X",
                (DWORD)(uintptr_t)oMountedUseFailPromptAE6260);
    return true;
}

static bool SetupMountedDemonJumpRequirementBypassHook()
{
    if (oMountedDemonJumpRequirementBF65C0)
    {
        return true;
    }

    oMountedDemonJumpRequirementBF65C0 =
        (tMountedDemonJumpRequirementFn)InstallInlineHook(
            ADDR_MountedDemonJumpRequirementBF65C0,
            (void *)hkMountedDemonJumpRequirementBF65C0);
    if (!oMountedDemonJumpRequirementBF65C0)
    {
        WriteLog("[MountDemonJumpReq] hook failed: BF65C0");
        return false;
    }

    WriteLogFmt(
        "[MountDemonJumpReq] OK(BF65C0): tramp=0x%08X",
        (DWORD)(uintptr_t)oMountedDemonJumpRequirementBF65C0);
    return true;
}

static bool SetupMountedSkillContextGateCallsiteHook()
{
    if (g_MountedSkillContextGateCallsiteOriginalTarget)
    {
        return true;
    }

    BYTE *pCallsite = (BYTE *)(uintptr_t)ADDR_B3009F;
    if (!pCallsite || SafeIsBadReadPtr(pCallsite, 5) || pCallsite[0] != 0xE8)
    {
        WriteLog("[MountDoubleJump] B3009F callsite missing/unexpected");
        return false;
    }

    g_MountedSkillContextGateCallsiteOriginalTarget =
        (DWORD)(uintptr_t)(pCallsite + 5 + *(int *)(pCallsite + 1));
    if (g_MountedSkillContextGateCallsiteOriginalTarget != ADDR_A9BF40)
    {
        WriteLogFmt("[MountDoubleJump] B3009F callsite target unexpected: 0x%08X",
                    g_MountedSkillContextGateCallsiteOriginalTarget);
        g_MountedSkillContextGateCallsiteOriginalTarget = 0;
        return false;
    }

    DWORD oldProtect = 0;
    if (!VirtualProtect(pCallsite, 5, PAGE_EXECUTE_READWRITE, &oldProtect))
    {
        WriteLog("[MountDoubleJump] B3009F callsite VirtualProtect failed");
        g_MountedSkillContextGateCallsiteOriginalTarget = 0;
        return false;
    }

    pCallsite[0] = 0xE8;
    *(int *)(pCallsite + 1) =
        (int)((uintptr_t)hkMountedSkillContextGateCallsiteB3009F - (uintptr_t)pCallsite - 5);

    VirtualProtect(pCallsite, 5, oldProtect, &oldProtect);
    FlushInstructionCache(GetCurrentProcess(), pCallsite, 5);

    WriteLogFmt("[MountDoubleJump] OK(B3009F): original=0x%08X patchedCall=0x%08X",
                g_MountedSkillContextGateCallsiteOriginalTarget,
                (DWORD)(uintptr_t)hkMountedSkillContextGateCallsiteB3009F);
    return true;
}

static bool SetupMountedUnknownSkillReleaseBranchHook()
{
    if (g_MountedUnknownSkillReleaseBranchOriginalTarget)
    {
        return true;
    }

    BYTE *pBranch = (BYTE *)(uintptr_t)ADDR_B300AC;
    if (!pBranch || SafeIsBadReadPtr(pBranch, 5) || pBranch[0] != 0xE9)
    {
        WriteLog("[MountDoubleJump] B300AC branch missing/unexpected");
        return false;
    }

    g_MountedUnknownSkillReleaseBranchOriginalTarget =
        (DWORD)(uintptr_t)(pBranch + 5 + *(int *)(pBranch + 1));
    if (g_MountedUnknownSkillReleaseBranchOriginalTarget != ADDR_B30240)
    {
        WriteLogFmt("[MountDoubleJump] B300AC branch target unexpected: 0x%08X",
                    g_MountedUnknownSkillReleaseBranchOriginalTarget);
        g_MountedUnknownSkillReleaseBranchOriginalTarget = 0;
        return false;
    }

    DWORD oldProtect = 0;
    if (!VirtualProtect(pBranch, 5, PAGE_EXECUTE_READWRITE, &oldProtect))
    {
        WriteLog("[MountDoubleJump] B300AC branch VirtualProtect failed");
        g_MountedUnknownSkillReleaseBranchOriginalTarget = 0;
        return false;
    }

    pBranch[0] = 0xE9;
    *(int *)(pBranch + 1) =
        (int)((uintptr_t)hkMountedUnknownSkillReleaseBranchB300AC - (uintptr_t)pBranch - 5);

    VirtualProtect(pBranch, 5, oldProtect, &oldProtect);
    FlushInstructionCache(GetCurrentProcess(), pBranch, 5);

    WriteLogFmt("[MountDoubleJump] OK(B300AC): original=0x%08X patchedJmp=0x%08X",
                g_MountedUnknownSkillReleaseBranchOriginalTarget,
                (DWORD)(uintptr_t)hkMountedUnknownSkillReleaseBranchB300AC);
    return true;
}

static bool InstallMountedDemonJumpCrashTraceHook(
    const char *tag,
    DWORD address,
    void *hook,
    void **originalOut)
{
    if (originalOut && *originalOut)
    {
        return true;
    }

    void *trampoline = InstallInlineHook(address, hook);
    if (!trampoline)
    {
        WriteLogFmt("[MountDemonJumpTrace] hook failed: %s (0x%08X)",
                    tag ? tag : "unknown",
                    address);
        return false;
    }

    if (originalOut)
    {
        *originalOut = trampoline;
    }

    WriteLogFmt("[MountDemonJumpTrace] OK(%s): addr=0x%08X tramp=0x%08X",
                tag ? tag : "unknown",
                address,
                (DWORD)(uintptr_t)trampoline);
    return true;
}

static bool SetupMountedDemonJumpCrashTraceHooks()
{
    bool ok = false;

    if (InstallMountedDemonJumpCrashTraceHook(
            "8057F0",
            0x008057F0,
            (void *)hkMountedDemonJumpTrace8057F0,
            reinterpret_cast<void **>(&oMountedDemonJumpTrace8057F0)))
    {
        ok = true;
    }
    if (InstallMountedDemonJumpCrashTraceHook(
            "550FF0",
            0x00550FF0,
            (void *)hkMountedDemonJumpTrace550FF0,
            reinterpret_cast<void **>(&oMountedDemonJumpTrace550FF0)))
    {
        ok = true;
    }
    if (InstallMountedDemonJumpCrashTraceHook(
            "829EC0",
            0x00829EC0,
            (void *)hkMountedDemonJumpTrace829EC0,
            reinterpret_cast<void **>(&oMountedDemonJumpTrace829EC0)))
    {
        ok = true;
    }
    if (InstallMountedDemonJumpCrashTraceHook(
            "829F10",
            0x00829F10,
            (void *)hkMountedDemonJumpTrace829F10,
            reinterpret_cast<void **>(&oMountedDemonJumpTrace829F10)))
    {
        ok = true;
    }
    if (InstallMountedDemonJumpCrashTraceHook(
            "551170",
            0x00551170,
            (void *)hkMountedDemonJumpTrace551170,
            reinterpret_cast<void **>(&oMountedDemonJumpTrace551170)))
    {
        ok = true;
    }
    if (InstallMountedDemonJumpCrashTraceHook(
            "A01BF0",
            0x00A01BF0,
            (void *)hkMountedDemonJumpTraceA01BF0,
            reinterpret_cast<void **>(&oMountedDemonJumpTraceA01BF0)))
    {
        ok = true;
    }
    if (InstallMountedDemonJumpCrashTraceHook(
            "4C1720",
            0x004C1720,
            (void *)hkMountedDemonJumpTrace4C1720,
            reinterpret_cast<void **>(&oMountedDemonJumpTrace4C1720)))
    {
        ok = true;
    }
    if (InstallMountedDemonJumpCrashTraceHook(
            "52BCB0",
            0x0052BCB0,
            (void *)hkMountedDemonJumpTrace52BCB0,
            reinterpret_cast<void **>(&oMountedDemonJumpTrace52BCB0)))
    {
        ok = true;
    }
    if (InstallMountedDemonJumpCrashTraceHook(
            "805850",
            0x00805850,
            (void *)hkMountedDemonJumpTrace805850,
            reinterpret_cast<void **>(&oMountedDemonJumpTrace805850)))
    {
        ok = true;
    }

    return ok;
}

static bool SetupMountedDemonJumpPacketObserveHooks()
{
    bool ok = false;

    if (!oMountedSkillPacketDispatchB26760)
    {
        oMountedSkillPacketDispatchB26760 =
            (tMountedSkillPacketDispatchFn)InstallInlineHook(
                ADDR_MountedSkillPacketDispatchB26760,
                (void *)hkMountedSkillPacketDispatchB26760);
        if (oMountedSkillPacketDispatchB26760)
        {
            ok = true;
            WriteLogFmt(
                "[MountDemonJumpPacket] OK(B26760): tramp=0x%08X",
                (DWORD)(uintptr_t)oMountedSkillPacketDispatchB26760);
        }
        else
        {
            WriteLog("[MountDemonJumpPacket] hook failed: B26760");
        }
    }
    else
    {
        ok = true;
    }

    if (!oMountedSkillAttackPacketB28A00)
    {
        oMountedSkillAttackPacketB28A00 =
            (tMountedSkillAttackPacketFn)InstallInlineHook(
                ADDR_MountedSkillAttackPacketB28A00,
                (void *)hkMountedSkillAttackPacketB28A00);
        if (oMountedSkillAttackPacketB28A00)
        {
            ok = true;
            WriteLogFmt(
                "[MountDemonJumpPacket] OK(B28A00): tramp=0x%08X",
                (DWORD)(uintptr_t)oMountedSkillAttackPacketB28A00);
        }
        else
        {
            WriteLog("[MountDemonJumpPacket] hook failed: B28A00");
        }
    }
    else
    {
        ok = true;
    }

    return ok;
}

static bool SetupMountedDemonJumpLatePathHooks()
{
    bool ok = false;

    if (!oMountedDemonJumpLateRoute575D60)
    {
        oMountedDemonJumpLateRoute575D60 =
            (tMountedDemonJumpLateRouteFn)InstallInlineHook(
                ADDR_MountedDemonJumpLateRoute575D60,
                (void *)hkMountedDemonJumpLateRoute575D60);
        if (oMountedDemonJumpLateRoute575D60)
        {
            ok = true;
            WriteLogFmt(
                "[MountDemonJumpLate] OK(575D60): tramp=0x%08X",
                (DWORD)(uintptr_t)oMountedDemonJumpLateRoute575D60);
        }
        else
        {
            WriteLog("[MountDemonJumpLate] hook failed: 575D60");
        }
    }
    else
    {
        ok = true;
    }

    if (!oMountedDemonJumpLateTick576020)
    {
        oMountedDemonJumpLateTick576020 =
            (tMountedDemonJumpLateTickFn)InstallInlineHook(
                ADDR_MountedDemonJumpLateTick576020,
                (void *)hkMountedDemonJumpLateTick576020);
        if (oMountedDemonJumpLateTick576020)
        {
            ok = true;
            WriteLogFmt(
                "[MountDemonJumpLate] OK(576020): tramp=0x%08X",
                (DWORD)(uintptr_t)oMountedDemonJumpLateTick576020);
        }
        else
        {
            WriteLog("[MountDemonJumpLate] hook failed: 576020");
        }
    }
    else
    {
        ok = true;
    }

    if (!oMountedDemonJumpContextInputB22630)
    {
        oMountedDemonJumpContextInputB22630 =
            (tMountedDemonJumpContextInputFn)InstallInlineHook(
                ADDR_MountedDemonJumpContextInputB22630,
                (void *)hkMountedDemonJumpContextInputB22630);
        if (oMountedDemonJumpContextInputB22630)
        {
            ok = true;
            WriteLogFmt(
                "[MountDemonJumpLate] OK(B22630): tramp=0x%08X",
                (DWORD)(uintptr_t)oMountedDemonJumpContextInputB22630);
        }
        else
        {
            WriteLog("[MountDemonJumpLate] hook failed: B22630");
        }
    }
    else
    {
        ok = true;
    }

    if (!oMountedDemonJumpMoveB1DB10)
    {
        oMountedDemonJumpMoveB1DB10 =
            (tMountedDemonJumpLateVoidRouteFn)InstallInlineHook(
                ADDR_MountedDemonJumpMoveB1DB10,
                (void *)hkMountedDemonJumpMoveB1DB10);
        if (oMountedDemonJumpMoveB1DB10)
        {
            ok = true;
            WriteLogFmt(
                "[MountDemonJumpLate] OK(B1DB10): tramp=0x%08X",
                (DWORD)(uintptr_t)oMountedDemonJumpMoveB1DB10);
        }
        else
        {
            WriteLog("[MountDemonJumpLate] hook failed: B1DB10");
        }
    }
    else
    {
        ok = true;
    }

    if (!oMountedDemonJumpMoveB1C9E0)
    {
        oMountedDemonJumpMoveB1C9E0 =
            (tMountedDemonJumpLateVoidRouteFn)InstallInlineHook(
                ADDR_MountedDemonJumpMoveB1C9E0,
                (void *)hkMountedDemonJumpMoveB1C9E0);
        if (oMountedDemonJumpMoveB1C9E0)
        {
            ok = true;
            WriteLogFmt(
                "[MountDemonJumpLate] OK(B1C9E0): tramp=0x%08X",
                (DWORD)(uintptr_t)oMountedDemonJumpMoveB1C9E0);
        }
        else
        {
            WriteLog("[MountDemonJumpLate] hook failed: B1C9E0");
        }
    }
    else
    {
        ok = true;
    }

    if (!oMountedDemonJumpUpActionAFB710)
    {
        oMountedDemonJumpUpActionAFB710 =
            (tMountedDemonJumpLateNoArgFn)InstallInlineHook(
                ADDR_MountedDemonJumpUpActionAFB710,
                (void *)hkMountedDemonJumpUpActionAFB710);
        if (oMountedDemonJumpUpActionAFB710)
        {
            ok = true;
            WriteLogFmt(
                "[MountDemonJumpLate] OK(AFB710): tramp=0x%08X",
                (DWORD)(uintptr_t)oMountedDemonJumpUpActionAFB710);
        }
        else
        {
            WriteLog("[MountDemonJumpLate] hook failed: AFB710");
        }
    }
    else
    {
        ok = true;
    }

    if (!oMountedDemonJumpPrimeAE8F70)
    {
        oMountedDemonJumpPrimeAE8F70 =
            (tMountedDemonJumpLatePrimeFn)InstallInlineHook(
                ADDR_MountedDemonJumpPrimeAE8F70,
                (void *)hkMountedDemonJumpPrimeAE8F70);
        if (oMountedDemonJumpPrimeAE8F70)
        {
            ok = true;
            WriteLogFmt(
                "[MountDemonJumpLate] OK(AE8F70): tramp=0x%08X",
                (DWORD)(uintptr_t)oMountedDemonJumpPrimeAE8F70);
        }
        else
        {
            WriteLog("[MountDemonJumpLate] hook failed: AE8F70");
        }
    }
    else
    {
        ok = true;
    }

    if (!oMountedDemonJumpAfbState42E170)
    {
        oMountedDemonJumpAfbState42E170 =
            (tMountedDemonJumpLateStateGetterFn)InstallInlineHook(
                ADDR_MountedDemonJumpAfbState42E170,
                (void *)hkMountedDemonJumpAfbState42E170);
        if (oMountedDemonJumpAfbState42E170)
        {
            ok = true;
            WriteLogFmt(
                "[MountDemonJumpLate] OK(42E170-AFB): tramp=0x%08X",
                (DWORD)(uintptr_t)oMountedDemonJumpAfbState42E170);
        }
        else
        {
            WriteLog("[MountDemonJumpLate] hook failed: 42E170-AFB");
        }
    }
    else
    {
        ok = true;
    }

    if (!oMountedDemonJumpAfbGateADB240)
    {
        oMountedDemonJumpAfbGateADB240 =
            (tMountedDemonJumpLateStateGetterFn)InstallInlineHook(
                ADDR_MountedDemonJumpAfbGateADB240,
                (void *)hkMountedDemonJumpAfbGateADB240);
        if (oMountedDemonJumpAfbGateADB240)
        {
            ok = true;
            WriteLogFmt(
                "[MountDemonJumpLate] OK(ADB240-AFB): tramp=0x%08X",
                (DWORD)(uintptr_t)oMountedDemonJumpAfbGateADB240);
        }
        else
        {
            WriteLog("[MountDemonJumpLate] hook failed: ADB240-AFB");
        }
    }
    else
    {
        ok = true;
    }

    if (!oMountedDemonJumpAfbGateAD9500)
    {
        oMountedDemonJumpAfbGateAD9500 =
            (tMountedDemonJumpLateStateGetterFn)InstallInlineHook(
                ADDR_MountedDemonJumpAfbGateAD9500,
                (void *)hkMountedDemonJumpAfbGateAD9500);
        if (oMountedDemonJumpAfbGateAD9500)
        {
            ok = true;
            WriteLogFmt(
                "[MountDemonJumpLate] OK(AD9500-AFB): tramp=0x%08X",
                (DWORD)(uintptr_t)oMountedDemonJumpAfbGateAD9500);
        }
        else
        {
            WriteLog("[MountDemonJumpLate] hook failed: AD9500-AFB");
        }
    }
    else
    {
        ok = true;
    }

    if (!oMountedDemonJumpAfbLookup773500)
    {
        oMountedDemonJumpAfbLookup773500 =
            (tMountedDemonJumpLateLookupFn)InstallInlineHook(
                ADDR_MountedDemonJumpAfbLookup773500,
                (void *)hkMountedDemonJumpAfbLookup773500);
        if (oMountedDemonJumpAfbLookup773500)
        {
            ok = true;
            WriteLogFmt(
                "[MountDemonJumpLate] OK(773500-AFB): tramp=0x%08X",
                (DWORD)(uintptr_t)oMountedDemonJumpAfbLookup773500);
        }
        else
        {
            WriteLog("[MountDemonJumpLate] hook failed: 773500-AFB");
        }
    }
    else
    {
        ok = true;
    }

    if (!oMountedDemonJumpBranchADEDA0)
    {
        oMountedDemonJumpBranchADEDA0 =
            (tMountedDemonJumpLateBranchFn)InstallInlineHook(
                ADDR_MountedDemonJumpBranchADEDA0,
                (void *)hkMountedDemonJumpBranchADEDA0);
        if (oMountedDemonJumpBranchADEDA0)
        {
            ok = true;
            WriteLogFmt(
                "[MountDemonJumpLate] OK(ADEDA0): tramp=0x%08X",
                (DWORD)(uintptr_t)oMountedDemonJumpBranchADEDA0);
        }
        else
        {
            WriteLog("[MountDemonJumpLate] hook failed: ADEDA0");
        }
    }
    else
    {
        ok = true;
    }

    if (!oMountedDemonJumpFilterBDBFD0)
    {
        oMountedDemonJumpFilterBDBFD0 =
            (tMountedDemonJumpLateFilterFn)InstallInlineHook(
                ADDR_MountedDemonJumpFilterBDBFD0,
                (void *)hkMountedDemonJumpFilterBDBFD0);
        if (oMountedDemonJumpFilterBDBFD0)
        {
            ok = true;
            WriteLogFmt(
                "[MountDemonJumpLate] OK(BDBFD0): tramp=0x%08X",
                (DWORD)(uintptr_t)oMountedDemonJumpFilterBDBFD0);
        }
        else
        {
            WriteLog("[MountDemonJumpLate] hook failed: BDBFD0");
        }
    }
    else
    {
        ok = true;
    }

    if (!oMountedDemonJumpKeyState7BECF0)
    {
        oMountedDemonJumpKeyState7BECF0 =
            (tMountedDemonJumpLateKeyStateFn)InstallInlineHook(
                ADDR_MountedDemonJumpKeyState7BECF0,
                (void *)hkMountedDemonJumpKeyState7BECF0);
        if (oMountedDemonJumpKeyState7BECF0)
        {
            ok = true;
            WriteLogFmt(
                "[MountDemonJumpLate] OK(7BECF0): tramp=0x%08X",
                (DWORD)(uintptr_t)oMountedDemonJumpKeyState7BECF0);
        }
        else
        {
            WriteLog("[MountDemonJumpLate] hook failed: 7BECF0");
        }
    }
    else
    {
        ok = true;
    }

    return ok;
}

static bool SetupMountedDemonJumpActionTraceHooks()
{
    bool ok = false;

    if (!oMountedDemonJumpActionGate7DC870)
    {
        oMountedDemonJumpActionGate7DC870 =
            (tMountedCrashTraceCdecl1ArgFn)InstallInlineHook(
                ADDR_MountedDemonJumpActionGate7DC870,
                (void *)hkMountedDemonJumpActionGate7DC870);
        if (oMountedDemonJumpActionGate7DC870)
        {
            ok = true;
            WriteLogFmt(
                "[MountDemonJumpAction] OK(7DC870): tramp=0x%08X",
                (DWORD)(uintptr_t)oMountedDemonJumpActionGate7DC870);
        }
        else
        {
            WriteLog("[MountDemonJumpAction] hook failed: 7DC870");
        }
    }
    else
    {
        ok = true;
    }

    if (!oMountedDemonJumpActionGate7DC810)
    {
        oMountedDemonJumpActionGate7DC810 =
            (tMountedCrashTraceCdecl1ArgFn)InstallInlineHook(
                ADDR_MountedDemonJumpActionGate7DC810,
                (void *)hkMountedDemonJumpActionGate7DC810);
        if (oMountedDemonJumpActionGate7DC810)
        {
            ok = true;
            WriteLogFmt(
                "[MountDemonJumpAction] OK(7DC810): tramp=0x%08X",
                (DWORD)(uintptr_t)oMountedDemonJumpActionGate7DC810);
        }
        else
        {
            WriteLog("[MountDemonJumpAction] hook failed: 7DC810");
        }
    }
    else
    {
        ok = true;
    }

    if (!oMountedDemonJumpActionGate7DC7B0)
    {
        oMountedDemonJumpActionGate7DC7B0 =
            (tMountedCrashTraceCdecl1ArgFn)InstallInlineHook(
                ADDR_MountedDemonJumpActionGate7DC7B0,
                (void *)hkMountedDemonJumpActionGate7DC7B0);
        if (oMountedDemonJumpActionGate7DC7B0)
        {
            ok = true;
            WriteLogFmt(
                "[MountDemonJumpAction] OK(7DC7B0): tramp=0x%08X",
                (DWORD)(uintptr_t)oMountedDemonJumpActionGate7DC7B0);
        }
        else
        {
            WriteLog("[MountDemonJumpAction] hook failed: 7DC7B0");
        }
    }
    else
    {
        ok = true;
    }

    if (!oMountedDemonJumpActionGate7DC8D0)
    {
        oMountedDemonJumpActionGate7DC8D0 =
            (tMountedCrashTraceCdecl1ArgFn)InstallInlineHook(
                ADDR_MountedDemonJumpActionGate7DC8D0,
                (void *)hkMountedDemonJumpActionGate7DC8D0);
        if (oMountedDemonJumpActionGate7DC8D0)
        {
            ok = true;
            WriteLogFmt(
                "[MountDemonJumpAction] OK(7DC8D0): tramp=0x%08X",
                (DWORD)(uintptr_t)oMountedDemonJumpActionGate7DC8D0);
        }
        else
        {
            WriteLog("[MountDemonJumpAction] hook failed: 7DC8D0");
        }
    }
    else
    {
        ok = true;
    }

    if (!oMountedDemonJumpActionGate7DC710)
    {
        oMountedDemonJumpActionGate7DC710 =
            (tMountedCrashTraceCdecl1ArgFn)InstallInlineHook(
                ADDR_MountedDemonJumpActionGate7DC710,
                (void *)hkMountedDemonJumpActionGate7DC710);
        if (oMountedDemonJumpActionGate7DC710)
        {
            ok = true;
            WriteLogFmt(
                "[MountDemonJumpAction] OK(7DC710): tramp=0x%08X",
                (DWORD)(uintptr_t)oMountedDemonJumpActionGate7DC710);
        }
        else
        {
            WriteLog("[MountDemonJumpAction] hook failed: 7DC710");
        }
    }
    else
    {
        ok = true;
    }

    if (!oMountedDemonJumpActionGate7DC900)
    {
        oMountedDemonJumpActionGate7DC900 =
            (tMountedCrashTraceCdecl1ArgFn)InstallInlineHook(
                ADDR_MountedDemonJumpActionGate7DC900,
                (void *)hkMountedDemonJumpActionGate7DC900);
        if (oMountedDemonJumpActionGate7DC900)
        {
            ok = true;
            WriteLogFmt(
                "[MountDemonJumpAction] OK(7DC900): tramp=0x%08X",
                (DWORD)(uintptr_t)oMountedDemonJumpActionGate7DC900);
        }
        else
        {
            WriteLog("[MountDemonJumpAction] hook failed: 7DC900");
        }
    }
    else
    {
        ok = true;
    }

    if (!oMountedDemonJumpActionGate7CF840)
    {
        oMountedDemonJumpActionGate7CF840 =
            (tMountedCrashTraceCdecl1ArgFn)InstallInlineHook(
                ADDR_MountedDemonJumpActionGate7CF840,
                (void *)hkMountedDemonJumpActionGate7CF840);
        if (oMountedDemonJumpActionGate7CF840)
        {
            ok = true;
            WriteLogFmt(
                "[MountDemonJumpAction] OK(7CF840): tramp=0x%08X",
                (DWORD)(uintptr_t)oMountedDemonJumpActionGate7CF840);
        }
        else
        {
            WriteLog("[MountDemonJumpAction] hook failed: 7CF840");
        }
    }
    else
    {
        ok = true;
    }

    if (!oMountedDemonJumpActionGate7DC750)
    {
        oMountedDemonJumpActionGate7DC750 =
            (tMountedCrashTraceCdecl1ArgFn)InstallInlineHook(
                ADDR_MountedDemonJumpActionGate7DC750,
                (void *)hkMountedDemonJumpActionGate7DC750);
        if (oMountedDemonJumpActionGate7DC750)
        {
            ok = true;
            WriteLogFmt(
                "[MountDemonJumpAction] OK(7DC750): tramp=0x%08X",
                (DWORD)(uintptr_t)oMountedDemonJumpActionGate7DC750);
        }
        else
        {
            WriteLog("[MountDemonJumpAction] hook failed: 7DC750");
        }
    }
    else
    {
        ok = true;
    }

    if (!oMountedDemonJumpActionGate7DC8A0)
    {
        oMountedDemonJumpActionGate7DC8A0 =
            (tMountedCrashTraceCdecl1ArgFn)InstallInlineHook(
                ADDR_MountedDemonJumpActionGate7DC8A0,
                (void *)hkMountedDemonJumpActionGate7DC8A0);
        if (oMountedDemonJumpActionGate7DC8A0)
        {
            ok = true;
            WriteLogFmt(
                "[MountDemonJumpAction] OK(7DC8A0): tramp=0x%08X",
                (DWORD)(uintptr_t)oMountedDemonJumpActionGate7DC8A0);
        }
        else
        {
            WriteLog("[MountDemonJumpAction] hook failed: 7DC8A0");
        }
    }
    else
    {
        ok = true;
    }

    if (!oMountedDemonJumpActionKind7CE210)
    {
        oMountedDemonJumpActionKind7CE210 =
            (tMountedCrashTraceCdecl1ArgFn)InstallInlineHook(
                ADDR_MountedDemonJumpActionKind7CE210,
                (void *)hkMountedDemonJumpActionKind7CE210);
        if (oMountedDemonJumpActionKind7CE210)
        {
            ok = true;
            WriteLogFmt(
                "[MountDemonJumpAction] OK(7CE210): tramp=0x%08X",
                (DWORD)(uintptr_t)oMountedDemonJumpActionKind7CE210);
        }
        else
        {
            WriteLog("[MountDemonJumpAction] hook failed: 7CE210");
        }
    }
    else
    {
        ok = true;
    }

    if (!oMountedDemonJumpActionUsable7DAAF0)
    {
        oMountedDemonJumpActionUsable7DAAF0 =
            (tMountedCrashTraceCdecl1ArgFn)InstallInlineHook(
                ADDR_MountedDemonJumpActionUsable7DAAF0,
                (void *)hkMountedDemonJumpActionUsable7DAAF0);
        if (oMountedDemonJumpActionUsable7DAAF0)
        {
            ok = true;
            WriteLogFmt(
                "[MountDemonJumpAction] OK(7DAAF0): tramp=0x%08X",
                (DWORD)(uintptr_t)oMountedDemonJumpActionUsable7DAAF0);
        }
        else
        {
            WriteLog("[MountDemonJumpAction] hook failed: 7DAAF0");
        }
    }
    else
    {
        ok = true;
    }

    if (!oMountedDemonJumpActionJobGate7D7D20)
    {
        oMountedDemonJumpActionJobGate7D7D20 =
            (tMountedCrashTrace2ArgFn)InstallInlineHook(
                ADDR_MountedDemonJumpActionJobGate7D7D20,
                (void *)hkMountedDemonJumpActionJobGate7D7D20);
        if (oMountedDemonJumpActionJobGate7D7D20)
        {
            ok = true;
            WriteLogFmt(
                "[MountDemonJumpAction] OK(7D7D20): tramp=0x%08X",
                (DWORD)(uintptr_t)oMountedDemonJumpActionJobGate7D7D20);
        }
        else
        {
            WriteLog("[MountDemonJumpAction] hook failed: 7D7D20");
        }
    }
    else
    {
        ok = true;
    }

    if (!oMountedDemonJumpActionBattlegroundSkillGate4E1D30)
    {
        oMountedDemonJumpActionBattlegroundSkillGate4E1D30 =
            (tMountedCrashTraceCdecl1ArgFn)InstallInlineHook(
                ADDR_MountedDemonJumpActionBattlegroundSkillGate4E1D30,
                (void *)hkMountedDemonJumpActionBattlegroundSkillGate4E1D30);
        if (oMountedDemonJumpActionBattlegroundSkillGate4E1D30)
        {
            ok = true;
            WriteLogFmt(
                "[MountDemonJumpAction] OK(4E1D30): tramp=0x%08X",
                (DWORD)(uintptr_t)oMountedDemonJumpActionBattlegroundSkillGate4E1D30);
        }
        else
        {
            WriteLog("[MountDemonJumpAction] hook failed: 4E1D30");
        }
    }
    else
    {
        ok = true;
    }

    if (!oMountedDemonJumpActionAbilityGateAE5870)
    {
        oMountedDemonJumpActionAbilityGateAE5870 =
            (tMountedCrashTraceNoArgFn)InstallInlineHook(
                ADDR_MountedDemonJumpActionAbilityGateAE5870,
                (void *)hkMountedDemonJumpActionAbilityGateAE5870);
        if (oMountedDemonJumpActionAbilityGateAE5870)
        {
            ok = true;
            WriteLogFmt(
                "[MountDemonJumpAction] OK(AE5870): tramp=0x%08X",
                (DWORD)(uintptr_t)oMountedDemonJumpActionAbilityGateAE5870);
        }
        else
        {
            WriteLog("[MountDemonJumpAction] hook failed: AE5870");
        }
    }
    else
    {
        ok = true;
    }

    // 42DAF0 是全局提示文本构造入口。把 mounted demon jump 诊断直接挂在这里
    // 会把启动期所有提示路径都纳入 hook 面，风险高于收益，先停用这条诊断。
    if (oMountedDemonJumpActionPromptReason42DAF0)
    {
        ok = true;
    }

    if (!oMountedDemonJumpActionPrepareB273B0)
    {
        oMountedDemonJumpActionPrepareB273B0 =
            (tMountedCrashTrace5ArgFn)InstallInlineHook(
                ADDR_MountedDemonJumpActionPrepareB273B0,
                (void *)hkMountedDemonJumpActionPrepareB273B0);
        if (oMountedDemonJumpActionPrepareB273B0)
        {
            ok = true;
            WriteLogFmt(
                "[MountDemonJumpAction] OK(B273B0): tramp=0x%08X",
                (DWORD)(uintptr_t)oMountedDemonJumpActionPrepareB273B0);
        }
        else
        {
            WriteLog("[MountDemonJumpAction] hook failed: B273B0");
        }
    }
    else
    {
        ok = true;
    }

    if (!oMountedDemonJumpActionGateA9B710)
    {
        oMountedDemonJumpActionGateA9B710 =
            (tMountedCrashTraceNoArgFn)InstallInlineHook(
                ADDR_MountedDemonJumpActionGateA9B710,
                (void *)hkMountedDemonJumpActionGateA9B710);
        if (oMountedDemonJumpActionGateA9B710)
        {
            ok = true;
            WriteLogFmt(
                "[MountDemonJumpAction] OK(A9B710): tramp=0x%08X",
                (DWORD)(uintptr_t)oMountedDemonJumpActionGateA9B710);
        }
        else
        {
            WriteLog("[MountDemonJumpAction] hook failed: A9B710");
        }
    }
    else
    {
        ok = true;
    }

    if (!oMountedDemonJumpActionKind52BAD0)
    {
        oMountedDemonJumpActionKind52BAD0 =
            (tMountedCrashTraceCdecl1ArgFn)InstallInlineHook(
                ADDR_MountedDemonJumpActionKind52BAD0,
                (void *)hkMountedDemonJumpActionKind52BAD0);
        if (oMountedDemonJumpActionKind52BAD0)
        {
            ok = true;
            WriteLogFmt(
                "[MountDemonJumpAction] OK(52BAD0): tramp=0x%08X",
                (DWORD)(uintptr_t)oMountedDemonJumpActionKind52BAD0);
        }
        else
        {
            WriteLog("[MountDemonJumpAction] hook failed: 52BAD0");
        }
    }
    else
    {
        ok = true;
    }

    if (!oMountedDemonJumpActionRouteB29C70)
    {
        oMountedDemonJumpActionRouteB29C70 =
            (tMountedCrashTrace3ArgFn)InstallInlineHook(
                ADDR_MountedDemonJumpActionRouteB29C70,
                (void *)hkMountedDemonJumpActionRouteB29C70);
        if (oMountedDemonJumpActionRouteB29C70)
        {
            ok = true;
            WriteLogFmt(
                "[MountDemonJumpAction] OK(B29C70): tramp=0x%08X",
                (DWORD)(uintptr_t)oMountedDemonJumpActionRouteB29C70);
        }
        else
        {
            WriteLog("[MountDemonJumpAction] hook failed: B29C70");
        }
    }
    else
    {
        ok = true;
    }

    if (!oMountedDemonJumpActionRouteB24010)
    {
        oMountedDemonJumpActionRouteB24010 =
            (tMountedCrashTrace2ArgFn)InstallInlineHook(
                ADDR_MountedDemonJumpActionRouteB24010,
                (void *)hkMountedDemonJumpActionRouteB24010);
        if (oMountedDemonJumpActionRouteB24010)
        {
            ok = true;
            WriteLogFmt(
                "[MountDemonJumpAction] OK(B24010): tramp=0x%08X",
                (DWORD)(uintptr_t)oMountedDemonJumpActionRouteB24010);
        }
        else
        {
            WriteLog("[MountDemonJumpAction] hook failed: B24010");
        }
    }
    else
    {
        ok = true;
    }

    if (!oMountedDemonJumpActionRouteB24EA0)
    {
        oMountedDemonJumpActionRouteB24EA0 =
            (tMountedCrashTrace2ArgFn)InstallInlineHook(
                ADDR_MountedDemonJumpActionRouteB24EA0,
                (void *)hkMountedDemonJumpActionRouteB24EA0);
        if (oMountedDemonJumpActionRouteB24EA0)
        {
            ok = true;
            WriteLogFmt(
                "[MountDemonJumpAction] OK(B24EA0): tramp=0x%08X",
                (DWORD)(uintptr_t)oMountedDemonJumpActionRouteB24EA0);
        }
        else
        {
            WriteLog("[MountDemonJumpAction] hook failed: B24EA0");
        }
    }
    else
    {
        ok = true;
    }

    if (!oMountedDemonJumpActionRouteB26550)
    {
        oMountedDemonJumpActionRouteB26550 =
            (tMountedCrashTrace1ArgFn)InstallInlineHook(
                ADDR_MountedDemonJumpActionRouteB26550,
                (void *)hkMountedDemonJumpActionRouteB26550);
        if (oMountedDemonJumpActionRouteB26550)
        {
            ok = true;
            WriteLogFmt(
                "[MountDemonJumpAction] OK(B26550): tramp=0x%08X",
                (DWORD)(uintptr_t)oMountedDemonJumpActionRouteB26550);
        }
        else
        {
            WriteLog("[MountDemonJumpAction] hook failed: B26550");
        }
    }
    else
    {
        ok = true;
    }

    if (!oMountedDemonJumpActionRouteB26050)
    {
        oMountedDemonJumpActionRouteB26050 =
            (tMountedCrashTrace1ArgFn)InstallInlineHook(
                ADDR_MountedDemonJumpActionRouteB26050,
                (void *)hkMountedDemonJumpActionRouteB26050);
        if (oMountedDemonJumpActionRouteB26050)
        {
            ok = true;
            WriteLogFmt(
                "[MountDemonJumpAction] OK(B26050): tramp=0x%08X",
                (DWORD)(uintptr_t)oMountedDemonJumpActionRouteB26050);
        }
        else
        {
            WriteLog("[MountDemonJumpAction] hook failed: B26050");
        }
    }
    else
    {
        ok = true;
    }

    return ok;
}

