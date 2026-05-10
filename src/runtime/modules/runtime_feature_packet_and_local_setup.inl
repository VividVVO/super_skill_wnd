static bool SetupPacketHook()
{
    bool sendHookOk = false;
    bool recvHookOk = false;
    SkillOverlayBridgeSetResetPreviewReceiveHookReady(false);

    BYTE *pTarget = FollowJmpChain((void *)ADDR_43D94D);
    if (!pTarget)
    {
        WriteLog("[PacketHook] target missing");
    }
    else if (pTarget[0] != 0xE8)
    {
        WriteLogFmt("[PacketHook] unexpected prologue at 43D94D: opcode=0x%02X", pTarget[0]);
    }
    else
    {
        g_SendPacketOriginalCallTarget = (DWORD)(uintptr_t)(pTarget + 5 + *(int *)(pTarget + 1));
        oSendPacket = pTarget + 5;

        DWORD oldProtect = 0;
        if (!VirtualProtect(pTarget, 5, PAGE_EXECUTE_READWRITE, &oldProtect))
        {
            WriteLog("[PacketHook] VirtualProtect failed");
            g_SendPacketOriginalCallTarget = 0;
            oSendPacket = nullptr;
        }
        else
        {
            pTarget[0] = 0xE9;
            *(int *)(pTarget + 1) = (int)((uintptr_t)hkSendPacketNaked - (uintptr_t)pTarget - 5);
            VirtualProtect(pTarget, 5, oldProtect, &oldProtect);
            FlushInstructionCache(GetCurrentProcess(), pTarget, 5);

            sendHookOk = true;
            WriteLogFmt("[PacketHook] OK(43D94D): originalCall=0x%08X continue=0x%08X",
                        g_SendPacketOriginalCallTarget,
                        (DWORD)(uintptr_t)oSendPacket);
        }
    }

    BYTE *pRecvEntry = (BYTE *)ADDR_4D6A13;
    BYTE *pRecvTarget = FollowJmpChain((void *)ADDR_4D6A13);
    bool recvUsesExternalStubEntry = false;
    if (pRecvTarget == pRecvEntry)
    {
        BYTE *pRecvStubTarget = TryFollowAbsoluteRegisterJumpStub(pRecvEntry);
        if (pRecvStubTarget)
        {
            pRecvTarget = pRecvStubTarget;
            recvUsesExternalStubEntry = true;
            WriteLogFmt("[PacketHook] recv external stub detected entry=0x%08X -> target=0x%08X",
                        (DWORD)(uintptr_t)pRecvEntry,
                        (DWORD)(uintptr_t)pRecvTarget);
            const uintptr_t potentialIncreaseAddress = TryExtractPotentialIncreaseAddressFromRecvStub(pRecvStubTarget);
            if (potentialIncreaseAddress)
            {
                g_ExternalPotentialIncreaseAddressRuntime = potentialIncreaseAddress;
                SkillOverlayBridgeSetPotentialIncreaseAddress(potentialIncreaseAddress);
                WriteLogFmt("[PacketHook] recv external stub potentialIncrease=0x%08X",
                            (DWORD)potentialIncreaseAddress);
                PatchExternalPotentialIncreaseStub(pRecvStubTarget);
            }
        }
    }

    if (!pRecvTarget)
    {
        WriteLog("[PacketHook] recv target missing");
    }
    else if (!MatchesRecvPacketDirectPrologue(pRecvTarget))
    {
        BYTE *pRecvFallbackTarget = recvUsesExternalStubEntry ? pRecvEntry : pRecvTarget;
        if (recvUsesExternalStubEntry)
        {
            WriteLogFmt("[PacketHook] recv stub target keeps custom logic: %02X %02X %02X, fallback hook on entry=0x%08X",
                        pRecvTarget[0],
                        pRecvTarget[1],
                        pRecvTarget[2],
                        (DWORD)(uintptr_t)pRecvFallbackTarget);
        }
        else
        {
            WriteLogFmt("[PacketHook] unexpected recv prologue at 4D6A13: %02X %02X %02X",
                        pRecvTarget[0],
                        pRecvTarget[1],
                        pRecvTarget[2]);
        }
        const size_t recvCopyLen = CalculateRelocatedByteCount(pRecvFallbackTarget, 5);
        if (recvCopyLen == 0)
        {
            WriteLog("[PacketHook] recv fallback copy length resolve failed");
        }
        else
        {
            oRecvPacket = GenericInlineHook5(pRecvFallbackTarget, (void *)hkRecvPacketNakedFallback, (int)recvCopyLen);
            if (!oRecvPacket)
            {
                WriteLog("[PacketHook] recv fallback hook failed");
            }
            else
            {
                recvHookOk = true;
                WriteLogFmt("[PacketHook] fallback recv hook OK target=0x%08X tramp=0x%08X copyLen=%u",
                            (DWORD)(uintptr_t)pRecvFallbackTarget,
                            (DWORD)(uintptr_t)oRecvPacket,
                            (unsigned int)recvCopyLen);
            }
        }
    }
    else
    {
        oRecvPacket = pRecvTarget + 9;

        DWORD oldProtect = 0;
        if (!VirtualProtect(pRecvTarget, 9, PAGE_EXECUTE_READWRITE, &oldProtect))
        {
            WriteLog("[PacketHook] recv VirtualProtect failed");
            oRecvPacket = nullptr;
        }
        else
        {
            pRecvTarget[0] = 0xE9;
            *(int *)(pRecvTarget + 1) = (int)((uintptr_t)hkRecvPacketNaked - (uintptr_t)pRecvTarget - 5);
            for (int i = 5; i < 9; ++i)
                pRecvTarget[i] = 0x90;
            VirtualProtect(pRecvTarget, 9, oldProtect, &oldProtect);
            FlushInstructionCache(GetCurrentProcess(), pRecvTarget, 9);

            recvHookOk = true;
            WriteLogFmt("[PacketHook] OK(4D6A13 recv): continue=0x%08X",
                        (DWORD)(uintptr_t)oRecvPacket);
        }
    }

    bool localHookAnyOk = false;
    if (SetupLocalIndependentPotentialPrimaryFlatStatHook())
        localHookAnyOk = true;
    if (SetupLocalIndependentPotentialPrimaryPercentStatHook())
        localHookAnyOk = true;
    if (SetupLocalIndependentPotentialFlatStatHook())
        localHookAnyOk = true;
    if (SetupLocalIndependentPotentialDisplayFunctionHooks())
        localHookAnyOk = true;
    if (SetupAbilityRedHashContainerHooks())
        localHookAnyOk = true;
    if (SetupAbilityRedExtendedAggregateHook())
        localHookAnyOk = true;
    if (SetupMountMovementAbilityFeatureHooks())
        localHookAnyOk = true;
    if (SetupAbilityRedSiblingCalcHooks())
        localHookAnyOk = true;
    if (SetupAbilityRedDiff84C470PreSubHook())
        localHookAnyOk = true;
    if (SetupAbilityRedAdditionalDiffHooks())
        localHookAnyOk = true;
    if (SetupAbilityRedPositiveStyleHooks())
        localHookAnyOk = true;
    if (SetupAbilityRedBakeWriteHooks())
        localHookAnyOk = true;
    if (SetupAbilityRedBake198Hooks())
        localHookAnyOk = true;
    if (SetupAbilityRedFinalValueHooks())
        localHookAnyOk = true;
    if (SetupAbilityRedDisplayCandidateHook())
        localHookAnyOk = true;
    if (SetupAbilityRedDisplayCallsiteHook())
        localHookAnyOk = true;
    if (SetupAbilityRedLevelReadHook())
        localHookAnyOk = true;
    if (SetupAbilityRedSkillWriteHooks())
        localHookAnyOk = true;

    WriteLogFmt("[IndependentBuffLocal] local read-point hooks active=%d mode=recv_plus_readpoint",
        localHookAnyOk ? 1 : 0);
    if (!SetupPotentialTextDisplayHook())
        WriteLog("[PotentialTextHook] display text hook install failed");
    if (!SetupSurfaceDrawImageObservationHook())
        WriteLog("[ObservedSceneFade] surface draw observation hook install failed");
    if (!SetupNativeCursorStateHook())
        WriteLog("[ObservedCursorState] 5F3EC0 hook install failed");

    SkillOverlayBridgeSetResetPreviewReceiveHookReady(sendHookOk && recvHookOk);
    if (!SetupStatusBarBuffSlotHooks())
        WriteLog("[StatusBarBuffSlot] slot refresh hook install failed");
    return sendHookOk && recvHookOk;
}

static bool SetupStatusBarBuffSlotHooks()
{
    if (g_StatusBarBuffSlotHooksInstalled)
        return true;

    bool ok = false;

    auto calcSafeCopyLen = [](BYTE* target) -> int
    {
        if (!target)
            return 0;
        int copyLen = CalcMinCopyLen(target);
        if (copyLen < 5)
            copyLen = 5;
        return copyLen;
    };

    BYTE* pPrimary = FollowJmpChain((void*)ADDR_StatusBarRefreshSlotsPrimary);
    if (pPrimary)
    {
        const int copyLen = calcSafeCopyLen(pPrimary);
        oStatusBarRefreshSlotsPrimary = (tStatusBarInternalRefreshFn)GenericInlineHook5(
            pPrimary,
            (void*)hkStatusBarRefreshSlotsPrimaryNaked,
            copyLen);
        if (oStatusBarRefreshSlotsPrimary)
        {
            WriteLogFmt("[StatusBarBuffSlot] primary hook OK entry=0x%08X tramp=0x%08X copyLen=%d",
                (DWORD)(uintptr_t)pPrimary,
                (DWORD)(uintptr_t)oStatusBarRefreshSlotsPrimary,
                copyLen);
            ok = true;
        }
        else
        {
            WriteLog("[StatusBarBuffSlot] primary hook failed");
        }
    }

    BYTE* pSecondary = FollowJmpChain((void*)ADDR_StatusBarRefreshSlotsSecondary);
    if (pSecondary)
    {
        const int copyLen = calcSafeCopyLen(pSecondary);
        oStatusBarRefreshSlotsSecondary = (tStatusBarInternalRefreshFn)GenericInlineHook5(
            pSecondary,
            (void*)hkStatusBarRefreshSlotsSecondaryNaked,
            copyLen);
        if (oStatusBarRefreshSlotsSecondary)
        {
            WriteLogFmt("[StatusBarBuffSlot] secondary hook OK entry=0x%08X tramp=0x%08X copyLen=%d",
                (DWORD)(uintptr_t)pSecondary,
                (DWORD)(uintptr_t)oStatusBarRefreshSlotsSecondary,
                copyLen);
            ok = true;
        }
        else
        {
            WriteLog("[StatusBarBuffSlot] secondary hook failed");
        }
    }

    // 9F5FE0 begins with a relative CALL within the first copied bytes:
    //   push esi / mov esi, ecx / call 9F4F00
    // Current GenericInlineHook5 trampolines are raw memcpy and do not relocate
    // rel32 call targets, so hooking 9F5FE0 directly corrupts the trampoline.
    // We already observe the real fixed-slot refresh through 9F4F00/9F4C30, so
    // disable this risky aggregate-entry hook instead of crashing on repeated use.
    WriteLog("[StatusBarBuffSlot] internal hook disabled: 9F5FE0 rel32 call trampoline unsafe");

    BYTE* pCleanup = FollowJmpChain((void*)ADDR_StatusBarCleanupTransient);
    if (pCleanup)
    {
        const int copyLen = calcSafeCopyLen(pCleanup);
        oStatusBarCleanupTransient = (tStatusBarInternalRefreshFn)GenericInlineHook5(
            pCleanup,
            (void*)hkStatusBarCleanupTransientNaked,
            copyLen);
        if (oStatusBarCleanupTransient)
        {
            WriteLogFmt("[StatusBarBuffSlot] cleanup hook OK entry=0x%08X tramp=0x%08X copyLen=%d",
                (DWORD)(uintptr_t)pCleanup,
                (DWORD)(uintptr_t)oStatusBarCleanupTransient,
                copyLen);
            ok = true;
        }
        else
        {
            WriteLog("[StatusBarBuffSlot] cleanup hook failed");
        }
    }

    BYTE* pTransientRefresh = FollowJmpChain((void*)0x009FC110);
    if (pTransientRefresh)
    {
        const int copyLen = calcSafeCopyLen(pTransientRefresh);
        oStatusBarTransientRefresh = (tStatusBarTransientRefreshFn)GenericInlineHook5(
            pTransientRefresh,
            (void*)hkStatusBarTransientRefreshNaked,
            copyLen);
        if (oStatusBarTransientRefresh)
        {
            WriteLogFmt("[StatusBarBuffSlot] transient refresh hook OK entry=0x%08X tramp=0x%08X copyLen=%d",
                (DWORD)(uintptr_t)pTransientRefresh,
                (DWORD)(uintptr_t)oStatusBarTransientRefresh,
                copyLen);
            ok = true;
        }
        else
        {
            WriteLog("[StatusBarBuffSlot] transient refresh hook failed");
        }
    }

    BYTE* pTransientDispatch = FollowJmpChain((void*)0x009FCC10);
    if (pTransientDispatch)
    {
        const int copyLen = calcSafeCopyLen(pTransientDispatch);
        oStatusBarTransientDispatch = (tStatusBarTransientDispatchFn)GenericInlineHook5(
            pTransientDispatch,
            (void*)hkStatusBarTransientDispatchNaked,
            copyLen);
        if (oStatusBarTransientDispatch)
        {
            WriteLogFmt("[StatusBarBuffSlot] transient dispatch hook OK entry=0x%08X tramp=0x%08X copyLen=%d",
                (DWORD)(uintptr_t)pTransientDispatch,
                (DWORD)(uintptr_t)oStatusBarTransientDispatch,
                copyLen);
            ok = true;
        }
        else
        {
            WriteLog("[StatusBarBuffSlot] transient dispatch hook failed");
        }
    }

    BYTE* pTransientToggle = FollowJmpChain((void*)0x009FCBD0);
    if (pTransientToggle)
    {
        const int copyLen = calcSafeCopyLen(pTransientToggle);
        oStatusBarTransientToggle = (tStatusBarTransientToggleFn)GenericInlineHook5(
            pTransientToggle,
            (void*)hkStatusBarTransientToggleNaked,
            copyLen);
        if (oStatusBarTransientToggle)
        {
            WriteLogFmt("[StatusBarBuffSlot] transient toggle hook OK entry=0x%08X tramp=0x%08X copyLen=%d",
                (DWORD)(uintptr_t)pTransientToggle,
                (DWORD)(uintptr_t)oStatusBarTransientToggle,
                copyLen);
            ok = true;
        }
        else
        {
            WriteLog("[StatusBarBuffSlot] transient toggle hook failed");
        }
    }

    g_StatusBarBuffSlotHooksInstalled = ok;
    return ok;
}

static bool SetupSurfaceDrawImageObservationHook()
{
    if (oSurfaceDrawImageFn)
        return true;

    BYTE *pTarget = FollowJmpChain((void *)ADDR_401C90);
    if (!pTarget)
    {
        WriteLog("[ObservedSceneFade] 401C90 target missing");
        return false;
    }

    int copyLen = CalcMinCopyLen(pTarget);
    if (copyLen < 5)
        copyLen = 5;

    oSurfaceDrawImageFn = (tSurfaceDrawImageFn)GenericInlineHook5(
        pTarget,
        (void *)hkSurfaceDrawImage,
        copyLen);
    if (!oSurfaceDrawImageFn)
    {
        WriteLog("[ObservedSceneFade] 401C90 hook failed");
        return false;
    }

    WriteLogFmt("[ObservedSceneFade] 401C90 hook OK entry=0x%08X tramp=0x%08X copyLen=%d",
        (DWORD)(uintptr_t)pTarget,
        (DWORD)(uintptr_t)oSurfaceDrawImageFn,
        copyLen);
    return true;
}

static bool SetupNativeCursorStateHook()
{
    if (oNativeCursorStateSetFn)
        return true;

    BYTE* pTarget = FollowJmpChain((void*)ADDR_5F3EC0);
    if (!pTarget)
    {
        WriteLog("[ObservedCursorState] 5F3EC0 target missing");
        return false;
    }

    int copyLen = CalcMinCopyLen(pTarget);
    if (copyLen < 5)
        copyLen = 5;

    oNativeCursorStateSetFn = (tNativeCursorStateSetFn)GenericInlineHook5(
        pTarget,
        (void*)hkNativeCursorStateSetNaked,
        copyLen);
    if (!oNativeCursorStateSetFn)
    {
        WriteLog("[ObservedCursorState] 5F3EC0 hook failed");
        return false;
    }

    WriteLogFmt("[ObservedCursorState] 5F3EC0 hook OK entry=0x%08X tramp=0x%08X copyLen=%d",
        (DWORD)(uintptr_t)pTarget,
        (DWORD)(uintptr_t)oNativeCursorStateSetFn,
        copyLen);
    return true;
}

static bool SetupLocalIndependentPotentialPrimaryFlatStatHook()
{
    BYTE* pEntry = (BYTE*)ADDR_853B49;
    BYTE* pTarget = FollowJmpChain((void*)ADDR_853B49);
    if (!pTarget && pEntry)
        pTarget = TryFollowAbsoluteRegisterJumpStub(pEntry);
    if (!pTarget)
    {
        WriteLog("[IndependentBuffLocal] target missing primary");
        return false;
    }

    static const BYTE kExpected[] = {
        0x85, 0xC0,
        0x0F, 0x84, 0x9C, 0x02, 0x00, 0x00,
        0x8B, 0x7C, 0x24, 0x2C,
        0x8B, 0x56, 0x28,
        0x69, 0xFF, 0xF0, 0x00, 0x00, 0x00,
        0x03, 0x78, 0x18
    };

    if (memcmp(pTarget, kExpected, sizeof(kExpected)) != 0)
    {
        char hexDump[256] = {};
        size_t cursor = 0;
        for (size_t dumpIndex = 0; dumpIndex < 16; ++dumpIndex)
        {
            cursor += sprintf_s(hexDump + cursor, sizeof(hexDump) - cursor, "%02X%s",
                (unsigned int)pTarget[dumpIndex],
                dumpIndex + 1 < 16 ? " " : "");
            if (cursor + 4 >= sizeof(hexDump))
                break;
        }
        WriteLog("[IndependentBuffLocal] skip 853B49 unexpected prologue");
        WriteLogFmt("[IndependentBuffLocal] 853B49 bytes=%s", hexDump);
        return false;
    }

    const DWORD continueNonZero = (DWORD)(uintptr_t)(pTarget + sizeof(kExpected));
    const DWORD continueZero = *(DWORD*)(pTarget + 4) + (DWORD)(uintptr_t)(pTarget + 8);

    DWORD oldProtect = 0;
    if (!VirtualProtect(pTarget, sizeof(kExpected), PAGE_EXECUTE_READWRITE, &oldProtect))
    {
        WriteLog("[IndependentBuffLocal] VirtualProtect failed primary");
        return false;
    }

    pTarget[0] = 0xE9;
    *(int*)(pTarget + 1) = (int)((uintptr_t)hkLocalIndependentPotentialPrimaryFlatStatsNaked - (uintptr_t)pTarget - 5);
    for (size_t i = 5; i < sizeof(kExpected); ++i)
        pTarget[i] = 0x90;

    VirtualProtect(pTarget, sizeof(kExpected), oldProtect, &oldProtect);
    FlushInstructionCache(GetCurrentProcess(), pTarget, sizeof(kExpected));
    oLocalIndependentPotentialPrimaryFlatStats = pTarget + sizeof(kExpected);
    g_LocalIndependentPotentialPrimaryContinueNonZero = continueNonZero;
    g_LocalIndependentPotentialPrimaryContinueZero = continueZero;
    WriteLogFmt("[IndependentBuffLocal] OK(853B49): continueNonZero=0x%08X continueZero=0x%08X",
        g_LocalIndependentPotentialPrimaryContinueNonZero,
        g_LocalIndependentPotentialPrimaryContinueZero);
    return true;
}

static bool SetupLocalIndependentPotentialPrimaryPercentStatHook()
{
    BYTE* pEntry = (BYTE*)ADDR_853E5A;
    BYTE* pTarget = FollowJmpChain((void*)ADDR_853E5A);
    if (!pTarget && pEntry)
        pTarget = TryFollowAbsoluteRegisterJumpStub(pEntry);
    if (!pTarget)
    {
        WriteLog("[IndependentBuffLocal] target missing primary percent");
        return false;
    }

    static const BYTE kExpected[] = {
        0x85, 0xED,
        0x0F, 0x84, 0xB5, 0x00, 0x00, 0x00,
        0x8B, 0x74, 0x24, 0x30,
        0x8B, 0x4C, 0x24, 0x34,
        0x69, 0xF6, 0xF0, 0x00, 0x00, 0x00,
        0x03, 0x75, 0x18
    };

    if (memcmp(pTarget, kExpected, sizeof(kExpected)) != 0)
    {
        char hexDump[256] = {};
        size_t cursor = 0;
        for (size_t dumpIndex = 0; dumpIndex < 16; ++dumpIndex)
        {
            cursor += sprintf_s(hexDump + cursor, sizeof(hexDump) - cursor, "%02X%s",
                (unsigned int)pTarget[dumpIndex],
                dumpIndex + 1 < 16 ? " " : "");
            if (cursor + 4 >= sizeof(hexDump))
                break;
        }
        WriteLog("[IndependentBuffLocal] skip 853E5A unexpected prologue");
        WriteLogFmt("[IndependentBuffLocal] 853E5A bytes=%s", hexDump);
        return false;
    }

    const DWORD continueNonZero = (DWORD)(uintptr_t)(pTarget + sizeof(kExpected));
    const DWORD continueZero = *(DWORD*)(pTarget + 4) + (DWORD)(uintptr_t)(pTarget + 8);

    DWORD oldProtect = 0;
    if (!VirtualProtect(pTarget, sizeof(kExpected), PAGE_EXECUTE_READWRITE, &oldProtect))
    {
        WriteLog("[IndependentBuffLocal] VirtualProtect failed primary percent");
        return false;
    }

    pTarget[0] = 0xE9;
    *(int*)(pTarget + 1) = (int)((uintptr_t)hkLocalIndependentPotentialPrimaryPercentStatsNaked - (uintptr_t)pTarget - 5);
    for (size_t i = 5; i < sizeof(kExpected); ++i)
        pTarget[i] = 0x90;

    VirtualProtect(pTarget, sizeof(kExpected), oldProtect, &oldProtect);
    FlushInstructionCache(GetCurrentProcess(), pTarget, sizeof(kExpected));
    oLocalIndependentPotentialPrimaryPercentStats = pTarget + sizeof(kExpected);
    g_LocalIndependentPotentialPrimaryPercentContinueNonZero = continueNonZero;
    g_LocalIndependentPotentialPrimaryPercentContinueZero = continueZero;
    WriteLogFmt("[IndependentBuffLocal] OK(853E5A): continueNonZero=0x%08X continueZero=0x%08X",
        g_LocalIndependentPotentialPrimaryPercentContinueNonZero,
        g_LocalIndependentPotentialPrimaryPercentContinueZero);
    return true;
}

static bool SetupLocalIndependentPotentialFlatStatHook()
{
    BYTE* pEntry = (BYTE*)ADDR_856879;
    BYTE* pTarget = FollowJmpChain((void*)ADDR_856879);
    if (!pTarget && pEntry)
        pTarget = TryFollowAbsoluteRegisterJumpStub(pEntry);
    if (!pTarget)
    {
        WriteLog("[IndependentBuffLocal] target missing");
        return false;
    }

    if (pTarget[0] >= 0xB8 && pTarget[0] <= 0xBF)
    {
        BYTE* pResolvedStub = TryFollowAbsoluteRegisterJumpStub(pTarget);
        if (pResolvedStub)
        {
            WriteLogFmt("[IndependentBuffLocal] resolved absolute jump stub 0x%08X -> 0x%08X",
                (DWORD)(uintptr_t)pTarget,
                (DWORD)(uintptr_t)pResolvedStub);
            pTarget = pResolvedStub;
        }
    }

    static const BYTE kExpectedLong[] = {
        0x85, 0xC0,
        0x0F, 0x84, 0xFB, 0x02, 0x00, 0x00,
        0x8B, 0x7C, 0x24, 0x2C,
        0x69, 0xFF, 0xF0, 0x00, 0x00, 0x00,
        0x03, 0x78, 0x18
    };
    static const BYTE kExpectedShort[] = {
        0x85, 0xC0,
        0x74, 0x0D,
        0x8B, 0x7C, 0x24, 0x2C,
        0x69, 0xFF, 0xF0, 0x00, 0x00, 0x00,
        0x03, 0x78, 0x18
    };

    const BYTE* expected = nullptr;
    size_t expectedLen = 0;
    DWORD continueNonZero = 0;
    DWORD continueZero = 0;

    if (memcmp(pTarget, kExpectedLong, sizeof(kExpectedLong)) == 0)
    {
        expected = kExpectedLong;
        expectedLen = sizeof(kExpectedLong);
        continueNonZero = (DWORD)(uintptr_t)(pTarget + expectedLen);
        continueZero = *(DWORD*)(pTarget + 4) + (DWORD)(uintptr_t)(pTarget + 8);
    }
    else if (memcmp(pTarget, kExpectedShort, sizeof(kExpectedShort)) == 0)
    {
        expected = kExpectedShort;
        expectedLen = sizeof(kExpectedShort);
        continueNonZero = (DWORD)(uintptr_t)(pTarget + expectedLen);
        continueZero = (DWORD)(uintptr_t)(pTarget + expectedLen);
        WriteLogFmt("[IndependentBuffLocal] using short-branch variant target=0x%08X continue=0x%08X",
            (DWORD)(uintptr_t)pTarget,
            continueNonZero);
    }
    else
    {
        char hexDump[256] = {};
        size_t cursor = 0;
        for (size_t dumpIndex = 0; dumpIndex < 16; ++dumpIndex)
        {
            cursor += sprintf_s(hexDump + cursor, sizeof(hexDump) - cursor, "%02X%s",
                (unsigned int)pTarget[dumpIndex],
                dumpIndex + 1 < 16 ? " " : "");
            if (cursor + 4 >= sizeof(hexDump))
                break;
        }
        WriteLogFmt("[IndependentBuffLocal] skip 856879 unexpected prologue");
        WriteLogFmt("[IndependentBuffLocal] 856879 bytes=%s", hexDump);
        return false;
    }

    DWORD oldProtect = 0;
    if (!VirtualProtect(pTarget, expectedLen, PAGE_EXECUTE_READWRITE, &oldProtect))
    {
        WriteLog("[IndependentBuffLocal] VirtualProtect failed");
        return false;
    }

    pTarget[0] = 0xE9;
    *(int*)(pTarget + 1) = (int)((uintptr_t)hkLocalIndependentPotentialFlatStatsNaked - (uintptr_t)pTarget - 5);
    for (size_t i = 5; i < expectedLen; ++i)
        pTarget[i] = 0x90;

    VirtualProtect(pTarget, expectedLen, oldProtect, &oldProtect);
    FlushInstructionCache(GetCurrentProcess(), pTarget, expectedLen);
    oLocalIndependentPotentialFlatStats = pTarget + expectedLen;
    g_LocalIndependentPotentialContinueNonZero = continueNonZero;
    g_LocalIndependentPotentialContinueZero = continueZero;
    WriteLogFmt("[IndependentBuffLocal] OK(856879): continueNonZero=0x%08X continueZero=0x%08X",
        g_LocalIndependentPotentialContinueNonZero,
        g_LocalIndependentPotentialContinueZero);
    return true;
}

