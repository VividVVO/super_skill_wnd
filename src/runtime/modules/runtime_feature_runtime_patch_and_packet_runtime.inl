static bool PatchBytesIfExpected(DWORD address, const BYTE *expected, const BYTE *patch, size_t length, const char *label)
{
    if (!address || !expected || !patch || !length)
        return false;

    BYTE *target = (BYTE *)(uintptr_t)address;
    for (size_t i = 0; i < length; ++i)
    {
        if (target[i] != expected[i])
        {
            WriteLogFmt("[RuntimePatch] skip %s at 0x%08X: byte[%u]=0x%02X expected=0x%02X",
                        label ? label : "unknown",
                        address,
                        (unsigned int)i,
                        (unsigned int)target[i],
                        (unsigned int)expected[i]);
            return false;
        }
    }

    DWORD oldProtect = 0;
    if (!VirtualProtect(target, length, PAGE_EXECUTE_READWRITE, &oldProtect))
    {
        WriteLogFmt("[RuntimePatch] FAIL %s at 0x%08X: VirtualProtect", label ? label : "unknown", address);
        return false;
    }

    for (size_t i = 0; i < length; ++i)
        target[i] = patch[i];

    VirtualProtect(target, length, oldProtect, &oldProtect);
    FlushInstructionCache(GetCurrentProcess(), target, length);
    WriteLogFmt("[RuntimePatch] OK %s at 0x%08X len=%u",
                label ? label : "unknown",
                address,
                (unsigned int)length);
    return true;
}

static bool PatchNopsIfExpected(DWORD address, const BYTE *expected, size_t length, const char *label)
{
    BYTE nops[16] = {};
    if (length > sizeof(nops))
        return false;

    for (size_t i = 0; i < length; ++i)
        nops[i] = 0x90;

    return PatchBytesIfExpected(address, expected, nops, length, label);
}

static bool ApplyMountMovementCapPatches()
{
    bool ok = false;

    // Root cause for mounted 190/123: 888DF0 clamps the raw TamingMob data while
    // building the cached mount movement object. Keep the original 80 floor for
    // speed, but remove the upper caps so boosted data survives into later paths.
    static const BYTE kMountLoaderSpeedUpperClamp[] = {
        0x3D, 0xBE, 0x00, 0x00, 0x00,
        0x7C, 0x05,
        0xB8, 0xBE, 0x00, 0x00, 0x00};
    if (PatchNopsIfExpected(
            ADDR_889079,
            kMountLoaderSpeedUpperClamp,
            sizeof(kMountLoaderSpeedUpperClamp),
            "mount loader speed upper cap"))
    {
        ok = true;
    }

    static const BYTE kMountLoaderJumpUpperClamp[] = {
        0x83, 0xF8, 0x7B,
        0x7C, 0x05,
        0xB8, 0x7B, 0x00, 0x00, 0x00};
    if (PatchNopsIfExpected(
            ADDR_8890F7,
            kMountLoaderJumpUpperClamp,
            sizeof(kMountLoaderJumpUpperClamp),
            "mount loader jump upper cap"))
    {
        ok = true;
    }

    // B92D10 is the normal mounted data path. It writes the mount data img's raw
    // speed/jump first, then optionally overwrites both with legacy recompute
    // helpers (84CD10 / B92260). For custom mounts with boosted data this sends
    // the values straight back to the old 190/123 family. Keep the raw data.
    static const BYTE kMountedRawSpeedOverrideGate[] = {0x74, 0x13};
    static const BYTE kMountedRawSpeedOverrideGatePatch[] = {0xEB, 0x13};
    if (PatchBytesIfExpected(
            ADDR_B9301F,
            kMountedRawSpeedOverrideGate,
            kMountedRawSpeedOverrideGatePatch,
            sizeof(kMountedRawSpeedOverrideGate),
            "mounted raw speed override gate"))
    {
        ok = true;
    }

    static const BYTE kMountedRawJumpOverrideGate[] = {0x74, 0x0F};
    static const BYTE kMountedRawJumpOverrideGatePatch[] = {0xEB, 0x0F};
    if (PatchBytesIfExpected(
            ADDR_B93038,
            kMountedRawJumpOverrideGate,
            kMountedRawJumpOverrideGatePatch,
            sizeof(kMountedRawJumpOverrideGate),
            "mounted raw jump override gate"))
    {
        ok = true;
    }

    // AA0B90 is the mounted-only movement path.
    // Keep its original additive logic, but remove the two final upper-cap paths:
    // 1) special branch: [150,170] -> [150,+inf)
    // 2) ground branch: min(max(speed+30,130), mountBase+150) -> max(speed+30,130)
    static const BYTE kMountedSpecialUpperCapGate[] = {0x7C, 0x22};
    static const BYTE kMountedSpecialUpperCapGatePatch[] = {0xEB, 0x22};
    if (PatchBytesIfExpected(
            ADDR_AA0CFB,
            kMountedSpecialUpperCapGate,
            kMountedSpecialUpperCapGatePatch,
            sizeof(kMountedSpecialUpperCapGate),
            "mounted special speed upper cap gate"))
    {
        ok = true;
    }

    static const BYTE kMountedGroundUpperClamp[] = {
        0x8D, 0xB3, 0x96, 0x00, 0x00, 0x00,
        0x3B, 0xC6,
        0x7D, 0x02,
        0x8B, 0xF0
    };
    static const BYTE kMountedGroundUpperClampPatch[] = {
        0x8B, 0xF0,
        0x90, 0x90, 0x90, 0x90,
        0x90, 0x90,
        0x90, 0x90,
        0x90, 0x90
    };
    if (PatchBytesIfExpected(
            ADDR_AA0D13,
            kMountedGroundUpperClamp,
            kMountedGroundUpperClampPatch,
            sizeof(kMountedGroundUpperClamp),
            "mounted ground speed upper clamp"))
    {
        ok = true;
    }

    // B93B80 is the shared movement stat clamp path that feeds the mounted
    // speed/jump numbers seen in panel/output. Remove only the upper caps so
    // high mounted values can propagate without breaking the original floors.
    static const BYTE kMountedOutputSpeedUpperClamp[] = {0x3B, 0xC6, 0x7C, 0x02, 0x8B, 0xC6};
    if (PatchNopsIfExpected(
            ADDR_B93D0F,
            kMountedOutputSpeedUpperClamp,
            sizeof(kMountedOutputSpeedUpperClamp),
            "mounted output speed upper cap"))
    {
        ok = true;
    }

    static const BYTE kMountedOutputJumpUpperClamp[] = {0x3B, 0xFD, 0x7D, 0x02, 0x8B, 0xEF};
    static const BYTE kMountedOutputJumpUpperClampPatch[] = {0x8B, 0xEF, 0x90, 0x90, 0x90, 0x90};
    if (PatchBytesIfExpected(
            ADDR_B93D23,
            kMountedOutputJumpUpperClamp,
            kMountedOutputJumpUpperClampPatch,
            sizeof(kMountedOutputJumpUpperClamp),
            "mounted output jump upper cap"))
    {
        ok = true;
    }

    static const BYTE kMountedOutputMode2SpeedUpperClamp[] = {0x3B, 0xDE, 0x7D, 0x02, 0x8B, 0xF3};
    static const BYTE kMountedOutputMode2SpeedUpperClampPatch[] = {0x8B, 0xF3, 0x90, 0x90, 0x90, 0x90};
    if (PatchBytesIfExpected(
            ADDR_B93D4A,
            kMountedOutputMode2SpeedUpperClamp,
            kMountedOutputMode2SpeedUpperClampPatch,
            sizeof(kMountedOutputMode2SpeedUpperClamp),
            "mounted output mode2 speed upper cap"))
    {
        ok = true;
    }

    // Keep the lower bound (100) intact, but remove the final upper clamp.
    static const BYTE kSpeedUpperClamp[] = {0x3B, 0xD7, 0x7C, 0x02, 0x8B, 0xD7};
    if (PatchNopsIfExpected(ADDR_858D30, kSpeedUpperClamp, sizeof(kSpeedUpperClamp), "movement speed upper cap"))
        ok = true;

    static const BYTE kJumpUpperCompare[] = {0x83, 0xF8, 0x7B};
    if (PatchNopsIfExpected(ADDR_858D49, kJumpUpperCompare, sizeof(kJumpUpperCompare), "movement jump cap cmp"))
        ok = true;

    static const BYTE kJumpUpperClamp[] = {0x7C, 0x05, 0xBA, 0x7B, 0x00, 0x00, 0x00};
    if (PatchNopsIfExpected(ADDR_858D4E, kJumpUpperClamp, sizeof(kJumpUpperClamp), "movement jump upper cap"))
        ok = true;

    return ok;
}

static void __cdecl hkSendPacketInspect(void **packetDataSlot, int *packetLenSlot, uintptr_t callerRetAddr)
{
    TryRewriteMountedDemonJumpOutgoingPacket(
        packetDataSlot,
        packetLenSlot,
        callerRetAddr);
    SkillOverlayBridgeInspectOutgoingPacketMutable(packetDataSlot, packetLenSlot, callerRetAddr);
}

__declspec(naked) static void hkSendPacketNaked()
{
    __asm {
        pushad
        mov edx, [esp + 32]
        push edx
        lea eax, [esp + 44]
        push eax
        lea ecx, [esp + 44]
        push ecx
        call hkSendPacketInspect
        add esp, 12
        popad
        call dword ptr [g_SendPacketOriginalCallTarget]
        jmp [oSendPacket]
    }
}

static void __cdecl hkRecvPacketInspect(void *inPacket, int opcode, uintptr_t callerRetAddr)
{
    SkillOverlayBridgeInspectIncomingPacket(inPacket, opcode, callerRetAddr);
}

__declspec(naked) static void hkRecvPacketNaked()
{
    __asm {
        movzx eax, ax
        lea ecx, dword ptr [eax - 0x10]
        pushad
        push 0
        push eax
        push esi
        call hkRecvPacketInspect
        add esp, 12
        popad
        cmp ecx, 0xA
        jmp [oRecvPacket]
    }
}

__declspec(naked) static void hkRecvPacketNakedFallback()
{
    __asm {
        pushfd
        pushad
        movzx eax, ax
        push 0
        push eax
        push esi
        call hkRecvPacketInspect
        add esp, 12
        popad
        popfd
        jmp [oRecvPacket]
    }
}

static bool MatchesRecvPacketDirectPrologue(const BYTE *code)
{
    if (!code)
        return false;

    return code[0] == 0x0F && code[1] == 0xB7 && code[2] == 0xC0 &&
           code[3] == 0x8D && code[4] == 0x48 && code[5] == 0xF0 &&
           code[6] == 0x83 && code[7] == 0xF9 && code[8] == 0x0A;
}

static void __cdecl hkLocalIndependentPotentialFlatStatsPrepare(uintptr_t sourcePtr)
{
    // 853B49 / 853E5A / 856879 feed the main ability display objects.
    // If we substitute a boosted buffer here, the client bakes the delta into
    // the primary shown value instead of keeping it as a red bonus delta.
    g_LocalIndependentPotentialPreparedPtr = sourcePtr;
}

__declspec(naked) static void hkLocalIndependentPotentialFlatStatsNaked()
{
    __asm {
        test eax, eax
        je no_source
        mov edi, [esp + 0x2C]
        imul edi, edi, 0xF0
        add edi, [eax + 0x18]
        jmp push_prepare
no_source:
        xor edi, edi
push_prepare:
        pushad
        push edi
        call hkLocalIndependentPotentialFlatStatsPrepare
        add esp, 4
        popad
        mov edi, dword ptr [g_LocalIndependentPotentialPreparedPtr]
        test edi, edi
        jne continue_nonzero
        jmp dword ptr [g_LocalIndependentPotentialContinueZero]
continue_nonzero:
        jmp dword ptr [g_LocalIndependentPotentialContinueNonZero]
    }
}

__declspec(naked) static void hkLocalIndependentPotentialPrimaryFlatStatsNaked()
{
    __asm {
        test eax, eax
        je no_source
        mov edi, [esp + 0x2C]
        mov edx, [esi + 0x28]
        imul edi, edi, 0xF0
        add edi, [eax + 0x18]
        jmp push_prepare
no_source:
        xor edi, edi
push_prepare:
        pushad
        push edi
        call hkLocalIndependentPotentialFlatStatsPrepare
        add esp, 4
        popad
        mov edi, dword ptr [g_LocalIndependentPotentialPreparedPtr]
        test edi, edi
        jne prepared_nonzero
        test eax, eax
        jne original_nonzero
        jmp dword ptr [g_LocalIndependentPotentialPrimaryContinueZero]
original_nonzero:
        mov edi, [esp + 0x2C]
        mov edx, [esi + 0x28]
        imul edi, edi, 0xF0
        add edi, [eax + 0x18]
        jmp dword ptr [g_LocalIndependentPotentialPrimaryContinueNonZero]
prepared_nonzero:
        mov edx, [esi + 0x28]
        jmp dword ptr [g_LocalIndependentPotentialPrimaryContinueNonZero]
    }
}

__declspec(naked) static void hkLocalIndependentPotentialPrimaryPercentStatsNaked()
{
    __asm {
        test ebp, ebp
        je no_source
        mov esi, [esp + 0x30]
        mov ecx, [esp + 0x34]
        imul esi, esi, 0xF0
        add esi, [ebp + 0x18]
        jmp push_prepare
no_source:
        xor esi, esi
push_prepare:
        pushad
        push esi
        call hkLocalIndependentPotentialFlatStatsPrepare
        add esp, 4
        popad
        mov esi, dword ptr [g_LocalIndependentPotentialPreparedPtr]
        test esi, esi
        jne prepared_nonzero
        test ebp, ebp
        jne original_nonzero
        jmp dword ptr [g_LocalIndependentPotentialPrimaryPercentContinueZero]
original_nonzero:
        mov esi, [esp + 0x30]
        mov ecx, [esp + 0x34]
        imul esi, esi, 0xF0
        add esi, [ebp + 0x18]
        jmp dword ptr [g_LocalIndependentPotentialPrimaryPercentContinueNonZero]
prepared_nonzero:
        mov ecx, [esp + 0x34]
        jmp dword ptr [g_LocalIndependentPotentialPrimaryPercentContinueNonZero]
    }
}

static BYTE *TryFollowAbsoluteRegisterJumpStub(BYTE *code)
{
    if (!code)
        return nullptr;

    const BYTE movOpcode = code[0];
    if (movOpcode < 0xB8 || movOpcode > 0xBF)
        return nullptr;

    const BYTE regIndex = (BYTE)(movOpcode - 0xB8);
    if (code[5] != 0xFF || code[6] != (BYTE)(0xE0 + regIndex))
        return nullptr;

    const DWORD target = *(DWORD *)(code + 1);
    if (target == 0 || target == (DWORD)(uintptr_t)code)
        return nullptr;

    return FollowJmpChain((void *)(uintptr_t)target);
}

static uintptr_t TryExtractPotentialIncreaseAddressFromRecvStub(BYTE* stubTarget)
{
    if (!stubTarget)
        return 0;

    for (size_t i = 0; i + 10 < 0x200; ++i)
    {
        BYTE* p = stubTarget + i;
        if (p[0] == 0x6B && p[1] == 0xC0 && p[2] == 0x04 &&
            p[3] == 0x05 && p[8] == 0x89 && p[9] == 0x08)
        {
            return *(DWORD*)(p + 4);
        }
    }

    return 0;
}

static bool PatchExternalPotentialIncreaseStub(BYTE* stubTarget)
{
    if (!stubTarget || !g_ExternalPotentialIncreaseAddressRuntime)
        return false;

    BYTE* writeSite = nullptr;
    BYTE* clearSite = nullptr;

    for (size_t i = 0; i + 5 < 0x200; ++i)
    {
        BYTE* p = stubTarget + i;
        if (!writeSite &&
            p[0] == 0x89 && p[1] == 0x08 &&
            p[2] == 0x90 &&
            p[3] == 0xEB)
        {
            writeSite = p;
        }

        if (!clearSite &&
            p[0] == 0xBF &&
            *(DWORD*)(p + 1) == (DWORD)g_ExternalPotentialIncreaseAddressRuntime &&
            p[5] == 0x31 && p[6] == 0xC0 &&
            p[7] == 0xB9 && *(DWORD*)(p + 8) == 0x128 &&
            p[12] == 0xC1 && p[13] == 0xE9 && p[14] == 0x02 &&
            p[15] == 0xF3 && p[16] == 0xAB)
        {
            clearSite = p;
        }

        if (writeSite && clearSite)
            break;
    }

    if (!writeSite)
    {
        WriteLog("[PacketHook] external potential write site not found");
        return false;
    }

    if (!clearSite)
    {
        WriteLog("[PacketHook] external potential clear site not found");
        return false;
    }

    g_ExternalPotentialWriteContinue = (DWORD)(uintptr_t)(writeSite + 5);
    g_ExternalPotentialWriteLoopTarget = (DWORD)(uintptr_t)(writeSite + 5 + (signed char)writeSite[4]);
    g_ExternalPotentialClearContinue = (DWORD)(uintptr_t)(clearSite + 17);

    DWORD oldProtect = 0;
    if (!VirtualProtect(writeSite, 5, PAGE_EXECUTE_READWRITE, &oldProtect))
    {
        WriteLog("[PacketHook] external potential write patch protect failed");
        return false;
    }
    writeSite[0] = 0xE9;
    *(int*)(writeSite + 1) = (int)((uintptr_t)hkExternalPotentialWriteNaked - (uintptr_t)writeSite - 5);
    VirtualProtect(writeSite, 5, oldProtect, &oldProtect);
    FlushInstructionCache(GetCurrentProcess(), writeSite, 5);
    oExternalPotentialWritePatch = writeSite;

    if (!VirtualProtect(clearSite, 17, PAGE_EXECUTE_READWRITE, &oldProtect))
    {
        WriteLog("[PacketHook] external potential clear patch protect failed");
        return false;
    }
    clearSite[0] = 0xE9;
    *(int*)(clearSite + 1) = (int)((uintptr_t)hkExternalPotentialClearNaked - (uintptr_t)clearSite - 5);
    for (int i = 5; i < 17; ++i)
        clearSite[i] = 0x90;
    VirtualProtect(clearSite, 17, oldProtect, &oldProtect);
    FlushInstructionCache(GetCurrentProcess(), clearSite, 17);
    oExternalPotentialClearPatch = clearSite;

    WriteLogFmt("[PacketHook] external potential stub patched write=0x%08X loop=0x%08X clear=0x%08X continue=0x%08X",
        (DWORD)(uintptr_t)writeSite,
        g_ExternalPotentialWriteLoopTarget,
        (DWORD)(uintptr_t)clearSite,
        g_ExternalPotentialClearContinue);
    return true;
}

static void __cdecl hkExternalPotentialWriteApplied(uintptr_t writeAddress, int baseValue)
{
    SkillOverlayBridgeApplyPotentialBaseValue(writeAddress, baseValue);
}

static void __cdecl hkExternalPotentialClearApplied()
{
    SkillOverlayBridgeClearPotentialBaseValues();
}

__declspec(naked) static void hkExternalPotentialWriteNaked()
{
    __asm {
        mov dword ptr [eax], ecx
        pushad
        push ecx
        push eax
        call hkExternalPotentialWriteApplied
        add esp, 8
        popad
        jmp dword ptr [g_ExternalPotentialWriteLoopTarget]
    }
}

__declspec(naked) static void hkExternalPotentialClearNaked()
{
    __asm {
        mov edi, dword ptr [g_ExternalPotentialIncreaseAddressRuntime]
        xor eax, eax
        mov ecx, 0x128
        shr ecx, 2
        rep stosd
        pushad
        call hkExternalPotentialClearApplied
        add esp, 0
        popad
        jmp dword ptr [g_ExternalPotentialClearContinue]
    }
}

