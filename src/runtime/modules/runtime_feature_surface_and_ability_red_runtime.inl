// 观测与 AbilityRed 运行时模块：负责 scene fade/cursor 观测和本地属性诊断注入。
static void ObserveSurfaceDrawImageCall(void *surface, int x, int y, int imageObj, DWORD *variantLikeAlpha)
{
    UNREFERENCED_PARAMETER(surface);
    if (!EnableSceneFadeObservationHooks())
        return;

    HWND hwnd = g_GameHwnd ? g_GameHwnd : g_D3D8GameHwnd;
    RECT clientRect = {};
    if (!hwnd || !::GetClientRect(hwnd, &clientRect))
        return;

    int w = 0;
    int h = 0;
    if (!TryGetObservedDrawObjectSize(imageObj, &w, &h))
        return;

    const int clientW = clientRect.right - clientRect.left;
    const int clientH = clientRect.bottom - clientRect.top;
    WORD variantType = VT_EMPTY;
    const int alpha = ExtractObservedDrawAlpha(variantLikeAlpha, &variantType);
    const bool coversViewport =
        x <= 16 &&
        y <= 16 &&
        x + w >= clientW - 16 &&
        y + h >= clientH - 16 &&
        w >= clientW - 32 &&
        h >= clientH - 32;
    const bool nearFullscreen =
        x <= 64 &&
        y <= 64 &&
        x + w >= clientW - 64 &&
        y + h >= clientH - 64 &&
        w >= ((clientW * 3) / 4) &&
        h >= ((clientH * 3) / 4);
    if (nearFullscreen)
    {
        static DWORD s_lastNearFullscreenDrawLogTick = 0;
        const DWORD nowTick = GetTickCount();
        if (nowTick - s_lastNearFullscreenDrawLogTick > 250)
        {
            s_lastNearFullscreenDrawLogTick = nowTick;
            WriteLogFmt(
                "[ObservedSceneFadeNearFullscreen] imageObj=0x%08X rect=(%d,%d,%d,%d) size=%dx%d alpha=%d vt=0x%04X raw=[0x%08X,0x%08X,0x%08X,0x%08X] client=%dx%d",
                imageObj,
                x,
                y,
                x + w,
                y + h,
                w,
                h,
                alpha,
                static_cast<unsigned int>(variantType),
                variantLikeAlpha ? variantLikeAlpha[0] : 0u,
                variantLikeAlpha ? variantLikeAlpha[1] : 0u,
                variantLikeAlpha ? variantLikeAlpha[2] : 0u,
                variantLikeAlpha ? variantLikeAlpha[3] : 0u,
                clientW,
                clientH);
        }
    }
    if (coversViewport)
    {
        static DWORD s_lastFullscreenDrawLogTick = 0;
        const DWORD nowTick = GetTickCount();
        if (nowTick - s_lastFullscreenDrawLogTick > 250)
        {
            s_lastFullscreenDrawLogTick = nowTick;
            WriteLogFmt("[ObservedSceneFadeCandidate] imageObj=0x%08X rect=(%d,%d,%d,%d) size=%dx%d alpha=%d vt=0x%04X client=%dx%d",
                imageObj,
                x,
                y,
                x + w,
                y + h,
                w,
                h,
                alpha,
                static_cast<unsigned int>(variantType),
                clientW,
                clientH);
        }
    }

    SkillOverlayBridgeObserveSceneFadeCandidate(imageObj, x, y, w, h, alpha, clientW, clientH);

    POINT mousePt = {};
    if (::GetCursorPos(&mousePt) && ::ScreenToClient(hwnd, &mousePt))
    {
        const bool nearMouse =
            w <= 96 &&
            h <= 96 &&
            abs(x - mousePt.x) <= 48 &&
            abs(y - mousePt.y) <= 48;
        if (nearMouse)
        {
            static DWORD s_lastMouseDrawLogTick = 0;
            const DWORD nowTick = GetTickCount();
            if (nowTick - s_lastMouseDrawLogTick > 100)
            {
                s_lastMouseDrawLogTick = nowTick;
                WriteLogFmt("[ObservedCursorDraw] imageObj=0x%08X pos=(%d,%d) size=%dx%d alpha=%d mouse=(%d,%d)",
                    imageObj,
                    x,
                    y,
                    w,
                    h,
                    alpha,
                    mousePt.x,
                    mousePt.y);
            }
        }
    }
}

static int __fastcall hkSurfaceDrawImage(void *surface, void * /*edxUnused*/, int x, int y, int imageObj, DWORD *variantLikeAlpha)
{
    ObserveSurfaceDrawImageCall(surface, x, y, imageObj, variantLikeAlpha);
    return oSurfaceDrawImageFn
        ? oSurfaceDrawImageFn(surface, x, y, imageObj, variantLikeAlpha)
        : 0;
}

static char __cdecl hkNativeCursorStateSetHandler(uintptr_t thisPtr, unsigned int requestedState)
{
    const char result = oNativeCursorStateSetFn
        ? oNativeCursorStateSetFn(thisPtr, requestedState)
        : 0;

    int currentState = -1;
    uintptr_t currentHandle = 0;
    if (thisPtr && !SafeIsBadReadPtr((void*)(thisPtr + 0x9C8), 4))
    {
        currentState = *(int*)(thisPtr + 0x9C4);
        currentHandle = *(uintptr_t*)(thisPtr + 0x978);
    }

    SkillOverlayBridgeSetObservedNativeCursorState(currentState);

    if (EnableUiObservationDiagnosticLogs())
    {
        static int s_lastLoggedState = -9999;
        static uintptr_t s_lastLoggedHandle = 0;
        if (currentState != s_lastLoggedState ||
            currentHandle != s_lastLoggedHandle)
        {
            s_lastLoggedState = currentState;
            s_lastLoggedHandle = currentHandle;
            WriteLogFmt("[ObservedCursorState] req=%u current=%d manager=0x%08X handle=0x%08X result=%d",
                requestedState,
                currentState,
                (DWORD)thisPtr,
                (DWORD)currentHandle,
                (int)result);
        }
    }

    return result;
}

__declspec(naked) static void hkNativeCursorStateSetNaked()
{
    __asm {
        mov eax, [esp + 4]
        push eax
        push ecx
        call hkNativeCursorStateSetHandler
        add esp, 8
        ret 4
    }
}

static bool IsAbilityRedHashReturnAddressOfInterest(DWORD returnAddr)
{
    return (returnAddr >= 0x009F5000 && returnAddr < 0x009F5600) ||
           (returnAddr >= 0x00AE4000 && returnAddr < 0x00AE7800);
}

static void ReadAbilityRedHashContainerMeta(
    uintptr_t thisPtr,
    DWORD *bucketBase,
    DWORD *bucketCount,
    DWORD *entryCount)
{
    if (bucketBase)
        *bucketBase = 0;
    if (bucketCount)
        *bucketCount = 0;
    if (entryCount)
        *entryCount = 0;
    if (!thisPtr || SafeIsBadReadPtr(reinterpret_cast<void*>(thisPtr + 0x04), 0x0C))
        return;

    if (bucketBase)
        *bucketBase = *reinterpret_cast<DWORD*>(thisPtr + 0x04);
    if (bucketCount)
        *bucketCount = *reinterpret_cast<DWORD*>(thisPtr + 0x08);
    if (entryCount)
        *entryCount = *reinterpret_cast<DWORD*>(thisPtr + 0x0C);
}

static bool ShouldLogAbilityRedHashLookup(DWORD returnAddr, uintptr_t thisPtr, DWORD key)
{
    if (!EnableAbilityRedDiagnosticLogs())
        return false;
    if (!IsAbilityRedHashReturnAddressOfInterest(returnAddr))
        return false;

    const DWORD now = GetTickCount();
    if (g_AbilityRedHashLookupLastCaller == returnAddr &&
        g_AbilityRedHashLookupLastThis == thisPtr &&
        g_AbilityRedHashLookupLastKey == key &&
        now - g_AbilityRedHashLookupLastTick <= 1000)
    {
        return false;
    }

    g_AbilityRedHashLookupLastCaller = returnAddr;
    g_AbilityRedHashLookupLastThis = thisPtr;
    g_AbilityRedHashLookupLastKey = key;
    g_AbilityRedHashLookupLastTick = now;
    return true;
}

static bool ShouldLogAbilityRedHashInsert(DWORD returnAddr, uintptr_t thisPtr, DWORD key, DWORD value)
{
    if (!EnableAbilityRedDiagnosticLogs())
        return false;
    if (!IsAbilityRedHashReturnAddressOfInterest(returnAddr))
        return false;

    const DWORD now = GetTickCount();
    if (g_AbilityRedHashInsertLastCaller == returnAddr &&
        g_AbilityRedHashInsertLastThis == thisPtr &&
        g_AbilityRedHashInsertLastKey == key &&
        g_AbilityRedHashInsertLastValue == value &&
        now - g_AbilityRedHashInsertLastTick <= 1000)
    {
        return false;
    }

    g_AbilityRedHashInsertLastCaller = returnAddr;
    g_AbilityRedHashInsertLastThis = thisPtr;
    g_AbilityRedHashInsertLastKey = key;
    g_AbilityRedHashInsertLastValue = value;
    g_AbilityRedHashInsertLastTick = now;
    return true;
}

static bool ShouldLogAbilityRedExtendedAggregate(DWORD returnAddr)
{
    if (!EnableAbilityRedDiagnosticLogs())
        return false;
    const DWORD now = GetTickCount();
    if (g_AbilityRedExtendedAggregateLastCaller == returnAddr &&
        now - g_AbilityRedExtendedAggregateLastTick <= 1000)
    {
        return false;
    }

    g_AbilityRedExtendedAggregateLastCaller = returnAddr;
    g_AbilityRedExtendedAggregateLastTick = now;
    return true;
}

static bool ShouldLogAbilityRedMasterAggregate(DWORD returnAddr)
{
    if (!EnableAbilityRedDiagnosticLogs())
        return false;
    const DWORD now = GetTickCount();
    if (g_AbilityRedMasterAggregateLastCaller == returnAddr &&
        now - g_AbilityRedMasterAggregateLastTick <= 1000)
    {
        return false;
    }

    g_AbilityRedMasterAggregateLastCaller = returnAddr;
    g_AbilityRedMasterAggregateLastTick = now;
    return true;
}

static unsigned int RotL32(unsigned int value, unsigned int count)
{
    count &= 31u;
    return (value << count) | (value >> ((32u - count) & 31u));
}

static unsigned int RotR32(unsigned int value, unsigned int count)
{
    count &= 31u;
    return (value >> count) | (value << ((32u - count) & 31u));
}

static int GenerateLocalIndependentPotentialCipherKey()
{
    typedef int (__fastcall *tGenerateCipherKeyFn)(void *seedPtr, void *edxUnused);
    tGenerateCipherKeyFn generateCipherKey = reinterpret_cast<tGenerateCipherKeyFn>(ADDR_4098C0);
    if (!generateCipherKey)
        return 0;
    return generateCipherKey(reinterpret_cast<void*>(ADDR_F631B8), nullptr);
}

static bool ReadEncryptedTripletValue(DWORD *base, size_t keyIndex, int *outValue)
{
    if (!base || !outValue)
        return false;

    const size_t maxIndex = keyIndex + 2;
    if (SafeIsBadReadPtr(base, (maxIndex + 1) * sizeof(DWORD)))
        return false;

    const unsigned int key = static_cast<unsigned int>(base[keyIndex]);
    const unsigned int enc = static_cast<unsigned int>(base[keyIndex + 1]);
    const unsigned int check = static_cast<unsigned int>(base[keyIndex + 2]);
    if (enc + RotR32(key ^ 0xBAADF00Du, 5) != check)
        return false;

    *outValue = static_cast<int>(key ^ RotL32(enc, 5));
    return true;
}

static bool TryReadUnalignedDword(const void *ptr, DWORD *outValue)
{
    if (outValue)
    {
        *outValue = 0;
    }
    if (!ptr || !outValue || SafeIsBadReadPtr(const_cast<void *>(ptr), sizeof(DWORD)))
    {
        return false;
    }

    __try
    {
        *outValue = *reinterpret_cast<const DWORD *>(ptr);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        *outValue = 0;
        return false;
    }

    return true;
}

static bool ReadEncryptedTripletValueByByteOffset(
    const void *base,
    size_t keyOffset,
    DWORD *keyOut,
    DWORD *encOut,
    DWORD *checkOut,
    int *decodedOut,
    bool *checkOkOut)
{
    if (keyOut)
    {
        *keyOut = 0;
    }
    if (encOut)
    {
        *encOut = 0;
    }
    if (checkOut)
    {
        *checkOut = 0;
    }
    if (decodedOut)
    {
        *decodedOut = 0;
    }
    if (checkOkOut)
    {
        *checkOkOut = false;
    }
    if (!base)
    {
        return false;
    }

    const BYTE *bytes = reinterpret_cast<const BYTE *>(base);
    DWORD key = 0;
    DWORD enc = 0;
    DWORD check = 0;
    if (!TryReadUnalignedDword(bytes + keyOffset, &key) ||
        !TryReadUnalignedDword(bytes + keyOffset + 4, &enc) ||
        !TryReadUnalignedDword(bytes + keyOffset + 8, &check))
    {
        return false;
    }

    const bool checkOk =
        static_cast<unsigned int>(enc) +
            RotR32(static_cast<unsigned int>(key) ^ 0xBAADF00Du, 5) ==
        static_cast<unsigned int>(check);
    if (keyOut)
    {
        *keyOut = key;
    }
    if (encOut)
    {
        *encOut = enc;
    }
    if (checkOut)
    {
        *checkOut = check;
    }
    if (checkOkOut)
    {
        *checkOkOut = checkOk;
    }
    if (decodedOut && checkOk)
    {
        *decodedOut = static_cast<int>(
            static_cast<unsigned int>(key) ^
            RotL32(static_cast<unsigned int>(enc), 5));
    }

    return true;
}

static void LogMountedDemonJumpTraceRowTriplets(
    const char *hookTag,
    void *thisPtr)
{
    if (!hookTag || !thisPtr)
    {
        return;
    }

    DWORD rowHead = 0;
    const bool rowHeadOk = TryReadUnalignedDword(thisPtr, &rowHead);

    DWORD key73 = 0;
    DWORD enc73 = 0;
    DWORD check73 = 0;
    int decoded73 = 0;
    bool check73Ok = false;
    const bool triplet73Ok = ReadEncryptedTripletValueByByteOffset(
        thisPtr,
        73,
        &key73,
        &enc73,
        &check73,
        &decoded73,
        &check73Ok);

    DWORD key97 = 0;
    DWORD enc97 = 0;
    DWORD check97 = 0;
    int decoded97 = 0;
    bool check97Ok = false;
    const bool triplet97Ok = ReadEncryptedTripletValueByByteOffset(
        thisPtr,
        97,
        &key97,
        &enc97,
        &check97,
        &decoded97,
        &check97Ok);

    static LONG s_mountedDemonJumpTraceRowTripletsLogBudget = 48;
    if (InterlockedDecrement(&s_mountedDemonJumpTraceRowTripletsLogBudget) < 0)
    {
        return;
    }

    WriteLogFmt(
        "[MountDemonJumpTrace] %s row=0x%08X head=%d headOk=%d t73ok=%d t73check=%d t73=[0x%08X,0x%08X,0x%08X] t73dec=%d t97ok=%d t97check=%d t97=[0x%08X,0x%08X,0x%08X] t97dec=%d",
        hookTag,
        (DWORD)(uintptr_t)thisPtr,
        rowHeadOk ? static_cast<int>(rowHead) : 0,
        rowHeadOk ? 1 : 0,
        triplet73Ok ? 1 : 0,
        check73Ok ? 1 : 0,
        key73,
        enc73,
        check73,
        decoded73,
        triplet97Ok ? 1 : 0,
        check97Ok ? 1 : 0,
        key97,
        enc97,
        check97,
        decoded97);
}

static bool WriteEncryptedTripletValue(DWORD *base, size_t keyIndex, int plainValue)
{
    if (!base)
        return false;

    const size_t maxIndex = keyIndex + 2;
    if (SafeIsBadWritePtr(base, (maxIndex + 1) * sizeof(DWORD)))
        return false;

    const int generatedKey = GenerateLocalIndependentPotentialCipherKey();
    if (generatedKey == 0)
        return false;

    const unsigned int key = static_cast<unsigned int>(generatedKey);
    const unsigned int enc = RotR32(static_cast<unsigned int>(plainValue) ^ key, 5);
    const unsigned int check = enc + RotR32(key ^ 0xBAADF00Du, 5);
    base[keyIndex] = key;
    base[keyIndex + 1] = enc;
    base[keyIndex + 2] = check;
    return true;
}

static void LogAbilityRedDecodedSnapshot(const char *tag)
{
    if (!EnableAbilityRedDiagnosticLogs())
        return;
    const DWORD now = GetTickCount();
    if (now - g_AbilityRedSnapshotLastTick <= 1000)
        return;
    g_AbilityRedSnapshotLastTick = now;

    DWORD *basePrimary = reinterpret_cast<DWORD*>(0x00F6D134);
    DWORD *baseExtended = reinterpret_cast<DWORD*>(0x00F6D200);

    int strVal = 0, dexVal = 0, intVal = 0, lukVal = 0, hpVal = 0, mpVal = 0;
    int watkVal = 0, matkVal = 0, wdefVal = 0, mdefVal = 0, accVal = 0, avoidVal = 0, speedVal = 0, jumpVal = 0;

    ReadEncryptedTripletValue(basePrimary, 9, &strVal);
    ReadEncryptedTripletValue(basePrimary, 12, &dexVal);
    ReadEncryptedTripletValue(basePrimary, 15, &intVal);
    ReadEncryptedTripletValue(basePrimary, 18, &lukVal);
    ReadEncryptedTripletValue(basePrimary, 24, &hpVal);
    ReadEncryptedTripletValue(basePrimary, 27, &mpVal);

    ReadEncryptedTripletValue(baseExtended, 57, &watkVal);
    ReadEncryptedTripletValue(baseExtended, 87, &matkVal);
    ReadEncryptedTripletValue(baseExtended, 72, &wdefVal);
    ReadEncryptedTripletValue(baseExtended, 102, &mdefVal);
    ReadEncryptedTripletValue(baseExtended, 117, &accVal);
    ReadEncryptedTripletValue(baseExtended, 132, &avoidVal);
    ReadEncryptedTripletValue(baseExtended, 159, &speedVal);
    ReadEncryptedTripletValue(baseExtended, 171, &jumpVal);

    WriteLogFmt(
        "[AbilityRedSnapshot] %s primary[str=%d dex=%d int=%d luk=%d hp=%d mp=%d] extended[watk=%d matk=%d wdef=%d mdef=%d acc=%d avoid=%d speed=%d jump=%d] active=%d",
        tag ? tag : "unknown",
        strVal,
        dexVal,
        intVal,
        lukVal,
        hpVal,
        mpVal,
        watkVal,
        matkVal,
        wdefVal,
        mdefVal,
        accVal,
        avoidVal,
        speedVal,
        jumpVal,
        SkillOverlayBridgeHasLocalIndependentPotentialDisplayBonuses() ? 1 : 0);
}

static void __cdecl hkApplyLocalIndependentPotentialSkillLevelDisplay(uintptr_t targetPtr)
{
    if (!targetPtr || SafeIsBadWritePtr(reinterpret_cast<void*>(targetPtr), sizeof(int)))
        return;
    if (!SkillOverlayBridgeHasLocalIndependentPotentialDisplayDeltaValue(0x88))
        return;
    if (!ShouldApplyLocalIndependentPotentialBurst(targetPtr, &g_LocalIndependentPotentialSkillLevelLastTarget, &g_LocalIndependentPotentialSkillLevelLastTick))
        return;

    const int delta = SkillOverlayBridgeGetLocalIndependentPotentialDisplayDeltaValue(0x88);
    if (delta == 0)
        return;

    if (kLocalIndependentPotentialDisplayObserveOnly)
    {
        WriteLogFmt("[IndependentBuffLocalDisplay] observe AE0A70 target=0x%08X delta=%d", (DWORD)targetPtr, delta);
        return;
    }

    *reinterpret_cast<int*>(targetPtr) += delta;
    WriteLogFmt("[IndependentBuffLocalDisplay] AE0B23 target=0x%08X delta=%d", (DWORD)targetPtr, delta);
}

static void __cdecl hkApplyLocalIndependentPotentialDamageDisplay(
    uintptr_t critRatePtr,
    uintptr_t option31Ptr,
    uintptr_t damagePtr,
    uintptr_t bossDamagePtr,
    uintptr_t ignoreDefensePtr)
{
    (void)option31Ptr;

    const int relevantOffsets[] = { 0x78, 0xAC, 0xC4, 0xA0 };
    if (!SkillOverlayBridgeHasAnyLocalIndependentPotentialDisplayDeltaValue(relevantOffsets, ARRAYSIZE(relevantOffsets)))
        return;

    const uintptr_t key = critRatePtr ^ (option31Ptr << 1) ^ (damagePtr << 2) ^ (bossDamagePtr << 3) ^ (ignoreDefensePtr << 4);
    if (!ShouldApplyLocalIndependentPotentialBurst(key, &g_LocalIndependentPotentialDamageLastKey, &g_LocalIndependentPotentialDamageLastTick))
        return;

    const struct
    {
        uintptr_t targetPtr;
        int offset;
    } targets[] = {
        { critRatePtr, 0x78 },
        { damagePtr, 0xAC },
        { bossDamagePtr, 0xC4 },
        { ignoreDefensePtr, 0xA0 },
    };

    int appliedValues[4] = {};
    for (int i = 0; i < 4; ++i)
    {
        const uintptr_t targetPtr = targets[i].targetPtr;
        if (!targetPtr || SafeIsBadWritePtr(reinterpret_cast<void*>(targetPtr), sizeof(int)))
            continue;

        const int delta = SkillOverlayBridgeGetLocalIndependentPotentialDisplayDeltaValue(targets[i].offset);
        if (delta == 0)
            continue;

        if (kLocalIndependentPotentialDisplayObserveOnly)
        {
            appliedValues[i] = delta;
            continue;
        }

        *reinterpret_cast<int*>(targetPtr) += delta;
        appliedValues[i] = delta;
    }

    if (appliedValues[0] || appliedValues[1] || appliedValues[2] || appliedValues[3])
    {
        WriteLogFmt(kLocalIndependentPotentialDisplayObserveOnly
                ? "[IndependentBuffLocalDisplay] observe AE0FDC crit=%d damage=%d boss=%d ignore=%d"
                : "[IndependentBuffLocalDisplay] AE0FDC crit=%d damage=%d boss=%d ignore=%d",
            appliedValues[0],
            appliedValues[1],
            appliedValues[2],
            appliedValues[3]);
    }
}

static void __cdecl hkApplyLocalIndependentPotentialPercentQuadDisplay(
    uintptr_t strPtr,
    uintptr_t intPtr,
    uintptr_t dexPtr,
    uintptr_t lukPtr)
{
    const int relevantOffsets[] = { 0x48, 0x50, 0x4C, 0x54 };
    if (!SkillOverlayBridgeHasAnyLocalIndependentPotentialDisplayDeltaValue(relevantOffsets, ARRAYSIZE(relevantOffsets)))
        return;

    const uintptr_t key = strPtr ^ (intPtr << 1) ^ (dexPtr << 2) ^ (lukPtr << 3);
    if (!ShouldApplyLocalIndependentPotentialBurst(key, &g_LocalIndependentPotentialPercentQuadLastKey, &g_LocalIndependentPotentialPercentQuadLastTick))
        return;

    const struct
    {
        uintptr_t targetPtr;
        int offset;
    } targets[] = {
        { strPtr, 0x48 },
        { intPtr, 0x50 },
        { dexPtr, 0x4C },
        { lukPtr, 0x54 },
    };

    int appliedValues[4] = {};
    for (int i = 0; i < 4; ++i)
    {
        if (!targets[i].targetPtr || SafeIsBadWritePtr(reinterpret_cast<void*>(targets[i].targetPtr), sizeof(DWORD)))
            continue;

        const int delta = SkillOverlayBridgeGetLocalIndependentPotentialDisplayDeltaValue(targets[i].offset);
        if (delta == 0)
            continue;

        if (kLocalIndependentPotentialDisplayObserveOnly)
        {
            appliedValues[i] = delta;
            continue;
        }

        *reinterpret_cast<DWORD*>(targets[i].targetPtr) += delta;
        appliedValues[i] = delta;
    }

    if (appliedValues[0] || appliedValues[1] || appliedValues[2] || appliedValues[3])
    {
        WriteLogFmt(kLocalIndependentPotentialDisplayObserveOnly
                ? "[IndependentBuffLocalDisplay] observe 8538C0 str=%d int=%d dex=%d luk=%d"
                : "[IndependentBuffLocalDisplay] 8538C0 str=%d int=%d dex=%d luk=%d",
            appliedValues[0],
            appliedValues[1],
            appliedValues[2],
            appliedValues[3]);
    }
}

static void __cdecl hkApplyLocalIndependentPotentialPercentFullDisplay(uintptr_t valuesPtr)
{
    if (!valuesPtr || SafeIsBadWritePtr(reinterpret_cast<void*>(valuesPtr), 6 * sizeof(DWORD)))
        return;
    const int offsets[] = { 0x48, 0x4C, 0x50, 0x54, 0x58, 0x5C };
    if (!SkillOverlayBridgeHasAnyLocalIndependentPotentialDisplayDeltaValue(offsets, ARRAYSIZE(offsets)))
        return;
    if (!ShouldApplyLocalIndependentPotentialBurst(valuesPtr, &g_LocalIndependentPotentialPercentFullLastKey, &g_LocalIndependentPotentialPercentFullLastTick))
        return;

    DWORD *values = reinterpret_cast<DWORD*>(valuesPtr);
    int appliedValues[6] = {};
    for (int i = 0; i < 6; ++i)
    {
        const int delta = SkillOverlayBridgeGetLocalIndependentPotentialDisplayDeltaValue(offsets[i]);
        if (delta == 0)
            continue;
        if (kLocalIndependentPotentialDisplayObserveOnly)
        {
            appliedValues[i] = delta;
            continue;
        }
        values[i] += delta;
        appliedValues[i] = delta;
    }

    if (appliedValues[0] || appliedValues[1] || appliedValues[2] || appliedValues[3] || appliedValues[4] || appliedValues[5])
    {
        WriteLogFmt(kLocalIndependentPotentialDisplayObserveOnly
                ? "[IndependentBuffLocalDisplay] observe 853E10 str=%d dex=%d int=%d luk=%d hp=%d mp=%d"
                : "[IndependentBuffLocalDisplay] 853E10 str=%d dex=%d int=%d luk=%d hp=%d mp=%d",
            appliedValues[0],
            appliedValues[1],
            appliedValues[2],
            appliedValues[3],
            appliedValues[4],
            appliedValues[5]);
    }
}

static void __cdecl hkApplyLocalIndependentPotentialFlatBasicDisplay(uintptr_t thisPtr)
{
    if (!thisPtr)
        return;
    const struct
    {
        size_t keyIndex;
        int offset;
    } targets[] = {
        { 9,  0x08 }, // STR
        { 12, 0x0C }, // DEX
        { 15, 0x10 }, // INT
        { 18, 0x14 }, // LUK
        { 24, 0x20 }, // MAXHP
        { 27, 0x24 }, // MAXMP
    };
    const int relevantOffsets[] = { 0x08, 0x0C, 0x10, 0x14, 0x20, 0x24 };
    if (!SkillOverlayBridgeHasAnyLocalIndependentPotentialDisplayDeltaValue(relevantOffsets, ARRAYSIZE(relevantOffsets)))
        return;
    if (!ShouldApplyLocalIndependentPotentialBurst(thisPtr, &g_LocalIndependentPotentialFlatBasicLastKey, &g_LocalIndependentPotentialFlatBasicLastTick))
        return;

    DWORD *values = reinterpret_cast<DWORD*>(thisPtr);

    int appliedCount = 0;
    for (int i = 0; i < (int)ARRAYSIZE(targets); ++i)
    {
        const int delta = SkillOverlayBridgeGetLocalIndependentPotentialDisplayDeltaValue(targets[i].offset);
        if (delta == 0)
            continue;

        if (kLocalIndependentPotentialDisplayObserveOnly)
        {
            ++appliedCount;
            continue;
        }

        int currentValue = 0;
        if (!ReadEncryptedTripletValue(values, targets[i].keyIndex, &currentValue))
            continue;
        if (!WriteEncryptedTripletValue(values, targets[i].keyIndex, currentValue + delta))
            continue;
        ++appliedCount;
    }

    if (appliedCount > 0)
        WriteLogFmt(kLocalIndependentPotentialDisplayObserveOnly
                ? "[IndependentBuffLocalDisplay] observe 853B00 applied=%d this=0x%08X"
                : "[IndependentBuffLocalDisplay] 853B00 applied=%d this=0x%08X",
            appliedCount,
            (DWORD)thisPtr);
}

static void __cdecl hkApplyLocalIndependentPotentialFlatExtendedDisplay(uintptr_t thisPtr)
{
    if (!thisPtr)
        return;
    const struct
    {
        size_t keyIndex;
        int offset;
    } targets[] = {
        { 117, 0x28 }, // ACC
        { 132, 0x2C }, // AVOID
        { 159, 0x30 }, // SPEED
        { 171, 0x34 }, // JUMP
        { 57,  0x38 }, // WATK
        { 87,  0x3C }, // MATK
        { 72,  0x40 }, // WDEF
        { 102, 0x44 }, // MDEF
        { 1712,0xC8 }, // CRIT MIN
        { 1724,0xCC }, // CRIT MAX
        { 1736,0xD0 }, // TER
        { 1748,0xD4 }, // ASR
    };
    const int relevantOffsets[] = { 0x28, 0x2C, 0x30, 0x34, 0x38, 0x3C, 0x40, 0x44, 0xC8, 0xCC, 0xD0, 0xD4 };
    if (!SkillOverlayBridgeHasAnyLocalIndependentPotentialDisplayDeltaValue(relevantOffsets, ARRAYSIZE(relevantOffsets)))
        return;
    if (!ShouldApplyLocalIndependentPotentialBurst(thisPtr, &g_LocalIndependentPotentialFlatExtendedLastKey, &g_LocalIndependentPotentialFlatExtendedLastTick))
        return;

    DWORD *values = reinterpret_cast<DWORD*>(thisPtr);

    int appliedCount = 0;
    for (int i = 0; i < (int)ARRAYSIZE(targets); ++i)
    {
        const int delta = SkillOverlayBridgeGetLocalIndependentPotentialDisplayDeltaValue(targets[i].offset);
        if (delta == 0)
            continue;

        if (kLocalIndependentPotentialDisplayObserveOnly)
        {
            ++appliedCount;
            continue;
        }

        int currentValue = 0;
        if (!ReadEncryptedTripletValue(values, targets[i].keyIndex, &currentValue))
            continue;
        if (!WriteEncryptedTripletValue(values, targets[i].keyIndex, currentValue + delta))
            continue;
        ++appliedCount;
    }

    if (appliedCount > 0)
        WriteLogFmt(kLocalIndependentPotentialDisplayObserveOnly
                ? "[IndependentBuffLocalDisplay] observe 856830 applied=%d this=0x%08X"
                : "[IndependentBuffLocalDisplay] 856830 applied=%d this=0x%08X",
            appliedCount,
            (DWORD)thisPtr);
}

static void __cdecl hkObserveAbilityRedDisplayCandidate(
    uintptr_t thisPtr,
    DWORD callerRet,
    int resultValue,
    DWORD arg1,
    DWORD arg2,
    DWORD arg3,
    DWORD arg4,
    DWORD arg5,
    DWORD arg6,
    DWORD arg7,
    DWORD ptrMaskBefore,
    const DWORD *ptrValuesBefore,
    DWORD ptrMaskAfter,
    const DWORD *ptrValuesAfter)
{
    if (!EnableAbilityRedDiagnosticLogs())
        return;
    const DWORD now = GetTickCount();
    if (g_AbilityRedDisplayCandidateLastThis == thisPtr &&
        now - g_AbilityRedDisplayCandidateLastTick <= 1000)
    {
        return;
    }

    g_AbilityRedDisplayCandidateLastThis = thisPtr;
    g_AbilityRedDisplayCandidateLastTick = now;

    DWORD vtable = 0;
    DWORD vtD8 = 0;
    DWORD field04 = 0;
    DWORD field08 = 0;
    DWORD field0C = 0;
    DWORD field10 = 0;
    DWORD field14 = 0;
    DWORD field18 = 0;

    if (thisPtr && !SafeIsBadReadPtr(reinterpret_cast<void*>(thisPtr), 0x1C))
    {
        vtable = *reinterpret_cast<DWORD*>(thisPtr + 0x00);
        field04 = *reinterpret_cast<DWORD*>(thisPtr + 0x04);
        field08 = *reinterpret_cast<DWORD*>(thisPtr + 0x08);
        field0C = *reinterpret_cast<DWORD*>(thisPtr + 0x0C);
        field10 = *reinterpret_cast<DWORD*>(thisPtr + 0x10);
        field14 = *reinterpret_cast<DWORD*>(thisPtr + 0x14);
        field18 = *reinterpret_cast<DWORD*>(thisPtr + 0x18);
        if (vtable && !SafeIsBadReadPtr(reinterpret_cast<void*>(vtable + 0xD8), 4))
            vtD8 = *reinterpret_cast<DWORD*>(vtable + 0xD8);
    }

    WriteLogFmt(
        "[AbilityRedDisplay] AE0E60 caller=0x%08X this=0x%08X vt=0x%08X vtD8=0x%08X result=%d args=[0x%08X,0x%08X,0x%08X,0x%08X,0x%08X,0x%08X,0x%08X] ptrBefore(mask=0x%02X)=[0x%08X,0x%08X,0x%08X,0x%08X,0x%08X,0x%08X,0x%08X] ptrAfter(mask=0x%02X)=[0x%08X,0x%08X,0x%08X,0x%08X,0x%08X,0x%08X,0x%08X] fields=[0x%08X,0x%08X,0x%08X,0x%08X,0x%08X,0x%08X]",
        callerRet,
        (DWORD)thisPtr,
        vtable,
        vtD8,
        resultValue,
        arg1,
        arg2,
        arg3,
        arg4,
        arg5,
        arg6,
        arg7,
        ptrMaskBefore,
        ptrValuesBefore ? ptrValuesBefore[0] : 0,
        ptrValuesBefore ? ptrValuesBefore[1] : 0,
        ptrValuesBefore ? ptrValuesBefore[2] : 0,
        ptrValuesBefore ? ptrValuesBefore[3] : 0,
        ptrValuesBefore ? ptrValuesBefore[4] : 0,
        ptrValuesBefore ? ptrValuesBefore[5] : 0,
        ptrValuesBefore ? ptrValuesBefore[6] : 0,
        ptrMaskAfter,
        ptrValuesAfter ? ptrValuesAfter[0] : 0,
        ptrValuesAfter ? ptrValuesAfter[1] : 0,
        ptrValuesAfter ? ptrValuesAfter[2] : 0,
        ptrValuesAfter ? ptrValuesAfter[3] : 0,
        ptrValuesAfter ? ptrValuesAfter[4] : 0,
        ptrValuesAfter ? ptrValuesAfter[5] : 0,
        ptrValuesAfter ? ptrValuesAfter[6] : 0,
        field04,
        field08,
        field0C,
        field10,
        field14,
        field18);
}

static DWORD ReadAbilityRedDisplayPointerValue(DWORD candidatePtr, DWORD bitMask, DWORD *maskOut)
{
    if (maskOut && candidatePtr && !SafeIsBadReadPtr(reinterpret_cast<void*>(candidatePtr), sizeof(DWORD)))
    {
        *maskOut |= bitMask;
        return *reinterpret_cast<DWORD*>(candidatePtr);
    }
    return 0;
}

static bool ShouldLogAbilityRedFinalCalculator(
    DWORD *lastCaller,
    uintptr_t *lastThis,
    DWORD *lastTick,
    int *lastActive,
    DWORD returnAddr,
    uintptr_t thisPtr,
    int activeState)
{
    if (!EnableAbilityRedDiagnosticLogs())
        return false;
    if (!lastCaller || !lastThis || !lastTick || !lastActive)
        return true;

    const DWORD now = GetTickCount();
    if (*lastCaller == returnAddr &&
        *lastThis == thisPtr &&
        *lastActive == activeState &&
        now - *lastTick <= 1000)
    {
        return false;
    }

    *lastCaller = returnAddr;
    *lastThis = thisPtr;
    *lastActive = activeState;
    *lastTick = now;
    return true;
}

static int __fastcall hkAbilityRedDisplayCandidateFunction(
    void *thisPtr,
    void *edxUnused,
    DWORD arg1,
    DWORD arg2,
    DWORD arg3,
    DWORD arg4,
    DWORD arg5,
    DWORD arg6,
    DWORD arg7)
{
    const DWORD callerRet = (DWORD)(uintptr_t)_ReturnAddress();
    const DWORD args[7] = { arg1, arg2, arg3, arg4, arg5, arg6, arg7 };
    DWORD ptrMaskBefore = 0;
    DWORD ptrValuesBefore[7] = {};
    for (int i = 0; i < 7; ++i)
        ptrValuesBefore[i] = ReadAbilityRedDisplayPointerValue(args[i], (1u << i), &ptrMaskBefore);

    const int resultValue = oAbilityRedDisplayCandidateFn
        ? oAbilityRedDisplayCandidateFn(thisPtr, edxUnused, arg1, arg2, arg3, arg4, arg5, arg6, arg7)
        : 0;

    DWORD ptrMaskAfter = 0;
    DWORD ptrValuesAfter[7] = {};
    for (int i = 0; i < 7; ++i)
        ptrValuesAfter[i] = ReadAbilityRedDisplayPointerValue(args[i], (1u << i), &ptrMaskAfter);

    hkObserveAbilityRedDisplayCandidate(
        reinterpret_cast<uintptr_t>(thisPtr),
        callerRet,
        resultValue,
        arg1,
        arg2,
        arg3,
        arg4,
        arg5,
        arg6,
        arg7,
        ptrMaskBefore,
        ptrValuesBefore,
        ptrMaskAfter,
        ptrValuesAfter);
    return resultValue;
}

static int __fastcall hkAbilityRedExtendedAggregateFunction(
    void *thisPtr,
    void *edxUnused,
    DWORD arg1,
    DWORD arg2,
    DWORD arg3)
{
    (void)edxUnused;
    const bool wantDiagnosticLog = EnableAbilityRedDiagnosticLogs();
    if (!wantDiagnosticLog)
    {
        return oAbilityRedExtendedAggregateFn
            ? oAbilityRedExtendedAggregateFn(thisPtr, edxUnused, arg1, arg2, arg3)
            : 0;
    }

    const DWORD callerRet = (DWORD)(uintptr_t)_ReturnAddress();
    DWORD before[6] = {};
    DWORD after[6] = {};
    if (arg3 && !SafeIsBadReadPtr(reinterpret_cast<void*>(arg3), sizeof(before)))
        memcpy(before, reinterpret_cast<void*>(arg3), sizeof(before));

    const int resultValue = oAbilityRedExtendedAggregateFn
        ? oAbilityRedExtendedAggregateFn(thisPtr, edxUnused, arg1, arg2, arg3)
        : 0;

    if (arg3 && !SafeIsBadReadPtr(reinterpret_cast<void*>(arg3), sizeof(after)))
        memcpy(after, reinterpret_cast<void*>(arg3), sizeof(after));

    if (ShouldLogAbilityRedExtendedAggregate(callerRet))
    {
        WriteLogFmt(
            "[AbilityRedAggregate] 856BA0 caller=0x%08X this=0x%08X arg1=0x%08X arg2=0x%08X out=0x%08X result=%d before=[%u,%u,%u,%u,%u,%u] after=[%u,%u,%u,%u,%u,%u] active=%d",
            callerRet,
            (DWORD)(uintptr_t)thisPtr,
            arg1,
            arg2,
            arg3,
            resultValue,
            before[0], before[1], before[2], before[3], before[4], before[5],
            after[0], after[1], after[2], after[3], after[4], after[5],
            SkillOverlayBridgeHasLocalIndependentPotentialDisplayBonuses() ? 1 : 0);
    }

    return resultValue;
}

static void ReadAbilityRedMasterAggregateBuffer(DWORD ptr, DWORD *outValues, size_t count)
{
    if (!outValues || count == 0)
        return;
    for (size_t i = 0; i < count; ++i)
        outValues[i] = 0;
    if (!ptr || SafeIsBadReadPtr(reinterpret_cast<void*>(ptr), count * sizeof(DWORD)))
        return;
    memcpy(outValues, reinterpret_cast<void*>(ptr), count * sizeof(DWORD));
}

static void DecodeAbilityRedMasterTripletPair(
    DWORD ptr,
    DWORD *rawValues,
    int *decodedA,
    int *decodedB,
    bool *okA,
    bool *okB)
{
    if (decodedA)
        *decodedA = 0;
    if (decodedB)
        *decodedB = 0;
    if (okA)
        *okA = false;
    if (okB)
        *okB = false;

    DWORD localRaw[6] = {};
    if (rawValues)
        memcpy(localRaw, rawValues, sizeof(localRaw));
    else if (ptr && !SafeIsBadReadPtr(reinterpret_cast<void*>(ptr), sizeof(localRaw)))
        memcpy(localRaw, reinterpret_cast<void*>(ptr), sizeof(localRaw));
    else
        return;

    int valueA = 0;
    int valueB = 0;
    const bool localOkA = ReadEncryptedTripletValue(localRaw, 0, &valueA);
    const bool localOkB = ReadEncryptedTripletValue(localRaw, 3, &valueB);

    if (decodedA)
        *decodedA = valueA;
    if (decodedB)
        *decodedB = valueB;
    if (okA)
        *okA = localOkA;
    if (okB)
        *okB = localOkB;
}

static void DecodeAbilityRedMasterDefenseValues(
    DWORD ptr,
    int *wdefValue,
    int *mdefValue,
    bool *wdefOk,
    bool *mdefOk)
{
    if (wdefValue)
        *wdefValue = 0;
    if (mdefValue)
        *mdefValue = 0;
    if (wdefOk)
        *wdefOk = false;
    if (mdefOk)
        *mdefOk = false;
    if (!ptr)
        return;

    int localWdef = 0;
    int localMdef = 0;
    const bool localWdefOk = ReadEncryptedTripletValue(reinterpret_cast<DWORD*>(ptr), 72, &localWdef);
    const bool localMdefOk = ReadEncryptedTripletValue(reinterpret_cast<DWORD*>(ptr), 102, &localMdef);

    if (wdefValue)
        *wdefValue = localWdef;
    if (mdefValue)
        *mdefValue = localMdef;
    if (wdefOk)
        *wdefOk = localWdefOk;
    if (mdefOk)
        *mdefOk = localMdefOk;
}

static void DecodeAbilityRedTripletAtOffset(
    uintptr_t thisPtr,
    size_t byteOffset,
    int *outValue,
    bool *outOk)
{
    if (outValue)
        *outValue = 0;
    if (outOk)
        *outOk = false;
    if (!thisPtr)
        return;

    DWORD *base = reinterpret_cast<DWORD*>(thisPtr + byteOffset);
    int localValue = 0;
    const bool localOk = ReadEncryptedTripletValue(base, 0, &localValue);
    if (outValue)
        *outValue = localValue;
    if (outOk)
        *outOk = localOk;
}

struct AbilityRedMovementDiagnosis
{
    int mountItemIdFromA4 = 0;
    int mountItemIdFromUser = 0;
    int speedSourceAdd = 0;
    int speedCapBase = 0;
    int speedCapOverride = 0;
    int currentSpeed = 0;
    int currentJump = 0;
    bool mountItemIdFromA4Ok = false;
    bool mountItemIdFromUserOk = false;
    bool speedSourceAddOk = false;
    bool speedCapBaseOk = false;
    bool speedCapOverrideOk = false;
    bool currentSpeedOk = false;
    bool currentJumpOk = false;
};

static int ComputeAbilityRedMovementFinalCap(
    const AbilityRedMovementDiagnosis *diag,
    bool *outOk)
{
    if (outOk)
        *outOk = false;
    if (!diag)
        return 0;

    if (diag->speedCapOverrideOk && diag->speedCapOverride != 0)
    {
        if (outOk)
            *outOk = true;
        return diag->speedCapOverride;
    }

    if (diag->speedCapBaseOk)
    {
        if (outOk)
            *outOk = true;
        return diag->speedCapBase + 140;
    }

    return 0;
}

static void CollectAbilityRedMovementDiagnosis(
    void *thisPtr,
    DWORD playerObjArg,
    DWORD capArg,
    AbilityRedMovementDiagnosis *outDiag)
{
    if (!outDiag)
        return;
    *outDiag = AbilityRedMovementDiagnosis();

    int mountItemId = 0;
    if (TryReadMountItemIdFromPlayerObject(
            reinterpret_cast<void *>(static_cast<uintptr_t>(playerObjArg)),
            &mountItemId))
    {
        outDiag->mountItemIdFromA4 = mountItemId;
        outDiag->mountItemIdFromA4Ok = true;
    }

    mountItemId = 0;
    if (TryReadCurrentUserMountItemId(&mountItemId))
    {
        outDiag->mountItemIdFromUser = mountItemId;
        outDiag->mountItemIdFromUserOk = true;
    }

    const tAbilityRedMovementSpeedSourceFn speedSourceFn =
        reinterpret_cast<tAbilityRedMovementSpeedSourceFn>(ADDR_804550);
    const tAbilityRedMovementSpeedCapBaseFn speedCapBaseFn =
        reinterpret_cast<tAbilityRedMovementSpeedCapBaseFn>(ADDR_82C700);
    const tAbilityRedMovementCapOverrideFn speedCapOverrideFn =
        reinterpret_cast<tAbilityRedMovementCapOverrideFn>(ADDR_8213D0);
    const tAbilityRedMovementValueFn currentSpeedFn =
        reinterpret_cast<tAbilityRedMovementValueFn>(ADDR_8222B0);
    const tAbilityRedMovementValueFn currentJumpFn =
        reinterpret_cast<tAbilityRedMovementValueFn>(ADDR_8223F0);

    __try
    {
        if (playerObjArg && speedSourceFn)
        {
            outDiag->speedSourceAdd = speedSourceFn(static_cast<int>(playerObjArg));
            outDiag->speedSourceAddOk = true;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }

    __try
    {
        if (playerObjArg && speedCapBaseFn)
        {
            outDiag->speedCapBase = speedCapBaseFn(static_cast<int>(playerObjArg));
            outDiag->speedCapBaseOk = true;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }

    __try
    {
        if (capArg && speedCapOverrideFn)
        {
            outDiag->speedCapOverride =
                speedCapOverrideFn(reinterpret_cast<void *>(static_cast<uintptr_t>(capArg)));
            outDiag->speedCapOverrideOk = true;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }

    __try
    {
        if (thisPtr && currentSpeedFn)
        {
            outDiag->currentSpeed = currentSpeedFn(thisPtr);
            outDiag->currentSpeedOk = true;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }

    __try
    {
        if (thisPtr && currentJumpFn)
        {
            outDiag->currentJump = currentJumpFn(thisPtr);
            outDiag->currentJumpOk = true;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }
}

static const DWORD kMovementSetterProtectWindowMs = 1500;
static const int kMovementSpeedProtectHighValueThreshold = 160;
static const int kMovementJumpProtectHighValueThreshold = 123;
static const int kMovementSetterPreserveMinValue = 100;
static const int kMovementSpeedProtectLegacyRewriteThreshold = 220;
static const int kMovementJumpProtectLegacyRewriteThreshold = 140;
#define kEnableGlobalMovementOutputClampHook (ssw::runtime::IsFeatureEnabled(ssw::runtime::FeatureSwitchId::GlobalMovementOutputClampHook))
static const DWORD kMountedFlightCruiseMinActiveMs = 120;
static const DWORD kMountedSoaringFlightActiveRefreshGapMs = 3000;
static const DWORD kMountedSoaringFlightActiveTimeoutMs = 10000;
static volatile LONG g_MovementSetterLastHighSpeedValue = 0;
static volatile LONG g_MovementSetterLastHighJumpValue = 0;
static volatile LONG g_MovementSetterLastHighSpeedTick = 0;
static volatile LONG g_MovementSetterLastHighJumpTick = 0;
static volatile LONG g_MovementSetterLastHighSpeedThisPtr = 0;
static volatile LONG g_MovementSetterLastHighJumpThisPtr = 0;
static volatile LONG g_MovementSetterLastHighSpeedCaller = 0;
static volatile LONG g_MovementSetterLastHighJumpCaller = 0;
static volatile LONG g_MovementSetterSpeedLogBudget = 24;
static volatile LONG g_MovementSetterJumpLogBudget = 24;
static volatile LONG g_MovementOutputClampLogBudget = 24;

static bool ShouldUseMountedFlightMovementSetterProtection()
{
    int mountItemId = 0;
    if (!TryResolveCurrentUserMountItemIdWithFallback(&mountItemId, nullptr) ||
        !IsExtendedMountSoaringContextMount(mountItemId))
    {
        return false;
    }

    const MountedFlightPhysicsScaleSample sample = g_MountedFlightPhysicsScaleSample;
    if (sample.mountItemId <= 0 ||
        sample.mountItemId != mountItemId ||
        sample.tick == 0)
    {
        return false;
    }

    return (sample.overrideSwim > sample.nativeSwim && sample.nativeSwim > 0.0) ||
           (sample.overrideFs > sample.nativeFs && sample.nativeFs > 0.0);
}

static int MaybePreserveMovementSetterHighValue(
    const char *tag,
    void *thisPtr,
    int value,
    int highValueThreshold,
    int legacyRewriteThreshold,
    volatile LONG *lastHighValue,
    volatile LONG *lastHighTick,
    volatile LONG *lastHighThisPtr,
    volatile LONG *lastHighCaller,
    volatile LONG *logBudget)
{
    if (!ssw::runtime::IsFeatureEnabled(ssw::runtime::FeatureSwitchId::FeatureMountMovementEnabled))
    {
        return value;
    }

    const DWORD callerRet = (DWORD)(uintptr_t)_ReturnAddress();
    const DWORD nowTick = GetTickCount();
    const DWORD thisAddr = (DWORD)(uintptr_t)thisPtr;
    if (!ShouldUseMountedFlightMovementSetterProtection())
    {
        return value;
    }

    if (value > highValueThreshold)
    {
        InterlockedExchange(lastHighValue, value);
        InterlockedExchange(lastHighTick, static_cast<LONG>(nowTick));
        InterlockedExchange(lastHighThisPtr, static_cast<LONG>(thisAddr));
        InterlockedExchange(lastHighCaller, static_cast<LONG>(callerRet));
        if (logBudget && InterlockedDecrement(logBudget) >= 0)
        {
            WriteLogFmt(
                "[MoveSetter] capture %s this=0x%08X high=%d caller=0x%08X",
                tag ? tag : "?",
                thisAddr,
                value,
                callerRet);
        }
        return value;
    }

    const int lastHigh = InterlockedCompareExchange(lastHighValue, 0, 0);
    const DWORD lastTick = static_cast<DWORD>(InterlockedCompareExchange(lastHighTick, 0, 0));
    const DWORD lastThis = static_cast<DWORD>(InterlockedCompareExchange(lastHighThisPtr, 0, 0));
    const DWORD lastCaller = static_cast<DWORD>(InterlockedCompareExchange(lastHighCaller, 0, 0));
    if (lastHigh <= 0 || lastTick == 0 || lastThis == 0)
    {
        return value;
    }

    if (nowTick - lastTick > kMovementSetterProtectWindowMs || lastThis != thisAddr)
    {
        return value;
    }

    if (value < kMovementSetterPreserveMinValue || value > legacyRewriteThreshold || lastHigh <= value)
    {
        return value;
    }

    if (logBudget && InterlockedDecrement(logBudget) >= 0)
    {
        WriteLogFmt(
            "[MoveSetter] preserve %s this=0x%08X low=%d caller=0x%08X recentHigh=%d highCaller=0x%08X age=%u",
            tag ? tag : "?",
            thisAddr,
            value,
            callerRet,
            lastHigh,
            lastCaller,
            nowTick - lastTick);
    }
    return lastHigh;
}

static int __fastcall hkAbilityRedMovementSpeedSetter831F00(void *thisPtr, int value)
{
    const int patchedValue = MaybePreserveMovementSetterHighValue(
        "speed",
        thisPtr,
        value,
        kMovementSpeedProtectHighValueThreshold,
        kMovementSpeedProtectLegacyRewriteThreshold,
        &g_MovementSetterLastHighSpeedValue,
        &g_MovementSetterLastHighSpeedTick,
        &g_MovementSetterLastHighSpeedThisPtr,
        &g_MovementSetterLastHighSpeedCaller,
        &g_MovementSetterSpeedLogBudget);
    return oAbilityRedMovementSpeedSetter831F00Fn
        ? oAbilityRedMovementSpeedSetter831F00Fn(thisPtr, patchedValue)
        : patchedValue;
}

static int __fastcall hkAbilityRedMovementJumpSetter832000(void *thisPtr, int value)
{
    const int patchedValue = MaybePreserveMovementSetterHighValue(
        "jump",
        thisPtr,
        value,
        kMovementJumpProtectHighValueThreshold,
        kMovementJumpProtectLegacyRewriteThreshold,
        &g_MovementSetterLastHighJumpValue,
        &g_MovementSetterLastHighJumpTick,
        &g_MovementSetterLastHighJumpThisPtr,
        &g_MovementSetterLastHighJumpCaller,
        &g_MovementSetterJumpLogBudget);
    return oAbilityRedMovementJumpSetter832000Fn
        ? oAbilityRedMovementJumpSetter832000Fn(thisPtr, patchedValue)
        : patchedValue;
}

static LONG __cdecl hkMovementOutputClampComputeB93B80(
    DWORD *a1,
    int a2,
    int a3,
    int a4,
    int a5,
    int a6,
    int a7,
    int *a8,
    int *a9,
    double *a10,
    DWORD *a11)
{
    const LONG result = oMovementOutputClampComputeB93B80Fn
        ? oMovementOutputClampComputeB93B80Fn(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11)
        : 0;
    if (!a8 || SafeIsBadWritePtr(a8, sizeof(int)))
        return result;

    const bool enablePlayerMovement =
        ssw::runtime::IsFeatureEnabled(ssw::runtime::FeatureSwitchId::FeaturePlayerMovementEnabled);
    const bool enableMountMovement =
        ssw::runtime::IsFeatureEnabled(ssw::runtime::FeatureSwitchId::FeatureMountMovementEnabled);
    if (!enablePlayerMovement && !enableMountMovement)
        return result;

    int contextualPlayerMountItemId = 0;
    const bool contextualPlayerMountReadable =
        TryReadMountItemIdFromPlayerObjectRaw(
            reinterpret_cast<void *>(static_cast<uintptr_t>(a4)),
            &contextualPlayerMountItemId);

    int mountedRawMountItemId = 0;
    int mountedRawDataKey = 0;
    int mountedRawSpeed = 0;
    int mountedRawJump = 0;
    bool hasMountedRawSample =
        TryGetRecentMountedMovementRawSample(
            &mountedRawMountItemId,
            &mountedRawDataKey,
            &mountedRawSpeed,
            &mountedRawJump,
            1200);
    if (contextualPlayerMountReadable &&
        (contextualPlayerMountItemId <= 0 ||
         (hasMountedRawSample && contextualPlayerMountItemId != mountedRawMountItemId)))
    {
        // B93B80 is still hit for a few frames after dismount. Use the current
        // player-object mount field as the earliest "we are on foot now"
        // signal and drop the cached mounted raw sample immediately so the
        // clamp hook stops reapplying mounted speed/jump on foot.
        ClearRecentMountedMovementRawSample();
        hasMountedRawSample = false;
        mountedRawMountItemId = 0;
        mountedRawDataKey = 0;
        mountedRawSpeed = 0;
        mountedRawJump = 0;
    }

    int currentUserMountItemId = 0;
    const bool currentUserMountReadable =
        TryReadCurrentUserMountItemId(&currentUserMountItemId);
    const bool isOnFootMovementContext =
        (contextualPlayerMountReadable && contextualPlayerMountItemId <= 0) ||
        (!contextualPlayerMountReadable && currentUserMountReadable && currentUserMountItemId <= 0) ||
        (!contextualPlayerMountReadable && !currentUserMountReadable && !hasMountedRawSample);
    const int movementDisplayOffsets[] = { 0x30, 0x34 };
    const bool shouldClampPlayerMovementOutput =
        !enablePlayerMovement &&
        isOnFootMovementContext &&
        (enableMountMovement ||
         SkillOverlayBridgeHasAnyLocalIndependentPotentialDisplayDeltaValue(movementDisplayOffsets, ARRAYSIZE(movementDisplayOffsets)));

    MountedMovementOverride mountedRawOverride = {};
    const bool shouldRaiseFromMountedRaw =
        enableMountMovement &&
        hasMountedRawSample &&
        SkillOverlayBridgeResolveMountedMovementOverride(
            mountedRawMountItemId,
            mountedRawDataKey,
            mountedRawOverride) &&
        mountedRawOverride.matched;

    DWORD *baseExtended = reinterpret_cast<DWORD *>(0x00F6D200);
    int decodedSpeed = 0;
    int decodedJump = 0;
    const bool hasSpeed =
        enablePlayerMovement &&
        ReadEncryptedTripletValue(baseExtended, 159, &decodedSpeed);
    const bool hasJump =
        enablePlayerMovement &&
        ReadEncryptedTripletValue(baseExtended, 171, &decodedJump);

    const int originalSpeedOut = *a8;
    int originalJumpOut = 0;
    if (a9 && !SafeIsBadReadPtr(a9, sizeof(int)))
        originalJumpOut = *a9;

    int metricExtra = 0;
    if (a11 && !SafeIsBadReadPtr(a11, sizeof(DWORD)))
    {
        const int originalMetricOut = static_cast<int>(*a11);
        if (originalMetricOut > originalSpeedOut)
            metricExtra = originalMetricOut - originalSpeedOut;
    }

    if (shouldClampPlayerMovementOutput)
    {
        int suppressedSpeedOut = originalSpeedOut;
        int suppressedJumpOut = originalJumpOut;
        if (suppressedSpeedOut > kMovementSpeedProtectHighValueThreshold)
            suppressedSpeedOut = kMovementSpeedProtectHighValueThreshold;
        if (a9 && !SafeIsBadWritePtr(a9, sizeof(int)) &&
            suppressedJumpOut > kMovementJumpProtectHighValueThreshold)
        {
            suppressedJumpOut = kMovementJumpProtectHighValueThreshold;
        }

        if (suppressedSpeedOut != originalSpeedOut)
        {
            *a8 = suppressedSpeedOut;
            if (a11 && !SafeIsBadWritePtr(a11, sizeof(DWORD)))
                *a11 = static_cast<DWORD>(suppressedSpeedOut + metricExtra);
        }
        if (a9 && !SafeIsBadWritePtr(a9, sizeof(int)) &&
            suppressedJumpOut != originalJumpOut)
        {
            *a9 = suppressedJumpOut;
        }

        if ((suppressedSpeedOut != originalSpeedOut || suppressedJumpOut != originalJumpOut) &&
            InterlockedDecrement(&g_MovementOutputClampLogBudget) >= 0)
        {
            WriteLogFmt(
                "[MoveClamp] B93B80 clamp player speed=%d->%d jump=%d->%d mountCtx=%d/%d userMount=%d/%d mountFeature=%d active=%d metricExtra=%d",
                originalSpeedOut,
                suppressedSpeedOut,
                originalJumpOut,
                suppressedJumpOut,
                contextualPlayerMountItemId,
                contextualPlayerMountReadable ? 1 : 0,
                currentUserMountItemId,
                currentUserMountReadable ? 1 : 0,
                enableMountMovement ? 1 : 0,
                SkillOverlayBridgeHasLocalIndependentPotentialDisplayBonuses() ? 1 : 0,
                metricExtra);
        }
        return result;
    }

    if ((!hasSpeed || decodedSpeed <= kMovementSpeedProtectHighValueThreshold) &&
        (!hasJump || decodedJump <= kMovementJumpProtectHighValueThreshold) &&
        (!shouldRaiseFromMountedRaw ||
         (mountedRawSpeed <= kMovementSpeedProtectHighValueThreshold &&
          mountedRawJump <= kMovementJumpProtectHighValueThreshold)))
    {
        return result;
    }

    int raisedSpeedOut = originalSpeedOut;
    if (hasSpeed && decodedSpeed > kMovementSpeedProtectHighValueThreshold)
    {
        int transformedSpeed = decodedSpeed;
        __try
        {
            tMovementSpeedTransformFn transformFn =
                reinterpret_cast<tMovementSpeedTransformFn>(ADDR_82C810);
            if (transformFn && a3)
                transformedSpeed = transformFn(reinterpret_cast<void *>(static_cast<uintptr_t>(a3)), decodedSpeed);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            transformedSpeed = decodedSpeed;
        }

        if (transformedSpeed > 0)
        {
            const int finalSpeedOut = transformedSpeed + 20;
            if (finalSpeedOut > raisedSpeedOut)
                raisedSpeedOut = finalSpeedOut;
        }
    }

    int raisedJumpOut = originalJumpOut;
    if (a9 && !SafeIsBadWritePtr(a9, sizeof(int)) &&
        hasJump && decodedJump > kMovementJumpProtectHighValueThreshold &&
        decodedJump > raisedJumpOut)
    {
        raisedJumpOut = decodedJump;
    }

    if (shouldRaiseFromMountedRaw)
    {
        if (mountedRawSpeed > raisedSpeedOut)
        {
            raisedSpeedOut = mountedRawSpeed;
        }
        if (a9 &&
            !SafeIsBadWritePtr(a9, sizeof(int)) &&
            mountedRawJump > raisedJumpOut)
        {
            raisedJumpOut = mountedRawJump;
        }
    }

    if (raisedSpeedOut != originalSpeedOut)
    {
        *a8 = raisedSpeedOut;
        if (a11 && !SafeIsBadWritePtr(a11, sizeof(DWORD)))
            *a11 = static_cast<DWORD>(raisedSpeedOut + metricExtra);
    }

    if (a9 && !SafeIsBadWritePtr(a9, sizeof(int)) && raisedJumpOut != originalJumpOut)
        *a9 = raisedJumpOut;

    if ((raisedSpeedOut != originalSpeedOut || raisedJumpOut != originalJumpOut) &&
        InterlockedDecrement(&g_MovementOutputClampLogBudget) >= 0)
    {
        WriteLogFmt(
            "[MoveClamp] B93B80 raise speed=%d->%d jump=%d->%d decoded=(%d,%d) mountedRaw=[mount=%d key=%d speed=%d jump=%d use=%d] a3=0x%08X a4=0x%08X metricExtra=%d",
            originalSpeedOut,
            raisedSpeedOut,
            originalJumpOut,
            raisedJumpOut,
            decodedSpeed,
            decodedJump,
            mountedRawMountItemId,
            mountedRawDataKey,
            mountedRawSpeed,
            mountedRawJump,
            shouldRaiseFromMountedRaw ? 1 : 0,
            a3,
            a4,
            metricExtra);
    }

    return result;
}

static BYTE ReadAbilityRedSiblingByte(uintptr_t thisPtr, size_t byteOffset, bool *outOk)
{
    if (outOk)
        *outOk = false;
    if (!thisPtr)
        return 0;

    BYTE *ptr = reinterpret_cast<BYTE*>(thisPtr + byteOffset);
    if (SafeIsBadReadPtr(ptr, sizeof(BYTE)))
        return 0;

    if (outOk)
        *outOk = true;
    return *ptr;
}

static DWORD ReadAbilityRedSiblingDword(uintptr_t thisPtr, size_t byteOffset, bool *outOk)
{
    if (outOk)
        *outOk = false;
    if (!thisPtr)
        return 0;

    DWORD *ptr = reinterpret_cast<DWORD*>(thisPtr + byteOffset);
    if (SafeIsBadReadPtr(ptr, sizeof(DWORD)))
        return 0;

    if (outOk)
        *outOk = true;
    return *ptr;
}

static void ObserveAbilityRedSiblingCalculator(
    const char *label,
    DWORD *lastCaller,
    uintptr_t *lastThis,
    DWORD *lastTick,
    int *lastActive,
    DWORD callerRet,
    uintptr_t thisValue,
    int resultValue)
{
    if (!EnableAbilityRedDiagnosticLogs())
        return;

    const int activeState = SkillOverlayBridgeHasLocalIndependentPotentialDisplayBonuses() ? 1 : 0;
    if (!ShouldLogAbilityRedFinalCalculator(
            lastCaller,
            lastThis,
            lastTick,
            lastActive,
            callerRet,
            thisValue,
            activeState))
    {
        return;
    }

    int slot24 = 0, slot30 = 0, slot3C = 0, slot48 = 0, slotB4 = 0;
    bool slot24Ok = false, slot30Ok = false, slot3COk = false, slot48Ok = false, slotB4Ok = false;
    DecodeAbilityRedTripletAtOffset(thisValue, 0x24, &slot24, &slot24Ok);
    DecodeAbilityRedTripletAtOffset(thisValue, 0x30, &slot30, &slot30Ok);
    DecodeAbilityRedTripletAtOffset(thisValue, 0x3C, &slot3C, &slot3COk);
    DecodeAbilityRedTripletAtOffset(thisValue, 0x48, &slot48, &slot48Ok);
    DecodeAbilityRedTripletAtOffset(thisValue, 0xB4, &slotB4, &slotB4Ok);

    bool meta8COk = false, meta8DOk = false, meta90Ok = false;
    bool meta94Ok = false, meta95Ok = false, meta98Ok = false;
    const BYTE meta8C = ReadAbilityRedSiblingByte(thisValue, 0x8C, &meta8COk);
    const BYTE meta8D = ReadAbilityRedSiblingByte(thisValue, 0x8D, &meta8DOk);
    const DWORD meta90 = ReadAbilityRedSiblingDword(thisValue, 0x90, &meta90Ok);
    const BYTE meta94 = ReadAbilityRedSiblingByte(thisValue, 0x94, &meta94Ok);
    const BYTE meta95 = ReadAbilityRedSiblingByte(thisValue, 0x95, &meta95Ok);
    const DWORD meta98 = ReadAbilityRedSiblingDword(thisValue, 0x98, &meta98Ok);

    WriteLogFmt(
        "[AbilityRedSibling] %s caller=0x%08X this=0x%08X result=%d active=%d metaA=[8C=0x%02X/%d 8D=0x%02X/%d 90=0x%08X/%d] metaB=[94=0x%02X/%d 95=0x%02X/%d 98=0x%08X/%d]",
        label ? label : "unknown",
        callerRet,
        (DWORD)thisValue,
        resultValue,
        activeState,
        meta8C, meta8COk ? 1 : 0,
        meta8D, meta8DOk ? 1 : 0,
        meta90, meta90Ok ? 1 : 0,
        meta94, meta94Ok ? 1 : 0,
        meta95, meta95Ok ? 1 : 0,
        meta98, meta98Ok ? 1 : 0);
    WriteLogFmt(
        "[AbilityRedSibling] %s slots 24=%d/%d 30=%d/%d 3C=%d/%d 48=%d/%d B4=%d/%d",
        label ? label : "unknown",
        slot24, slot24Ok ? 1 : 0,
        slot30, slot30Ok ? 1 : 0,
        slot3C, slot3COk ? 1 : 0,
        slot48, slot48Ok ? 1 : 0,
        slotB4, slotB4Ok ? 1 : 0);
}

static int __fastcall hkAbilityRedSiblingCalc82F780(void *thisPtr, void *edxUnused)
{
    (void)edxUnused;
    const DWORD callerRet = (DWORD)(uintptr_t)_ReturnAddress();
    const int resultValue = oAbilityRedSiblingCalc82F780Fn
        ? oAbilityRedSiblingCalc82F780Fn(thisPtr, edxUnused)
        : 0;
    ObserveAbilityRedSiblingCalculator(
        "82F780",
        &g_AbilityRedSibling82F780LastCaller,
        &g_AbilityRedSibling82F780LastThis,
        &g_AbilityRedSibling82F780LastTick,
        &g_AbilityRedSibling82F780LastActive,
        callerRet,
        reinterpret_cast<uintptr_t>(thisPtr),
        resultValue);
    return resultValue;
}

static int __fastcall hkAbilityRedSiblingCalc82F870(void *thisPtr, void *edxUnused)
{
    (void)edxUnused;
    const DWORD callerRet = (DWORD)(uintptr_t)_ReturnAddress();
    const int resultValue = oAbilityRedSiblingCalc82F870Fn
        ? oAbilityRedSiblingCalc82F870Fn(thisPtr, edxUnused)
        : 0;
    ObserveAbilityRedSiblingCalculator(
        "82F870",
        &g_AbilityRedSibling82F870LastCaller,
        &g_AbilityRedSibling82F870LastThis,
        &g_AbilityRedSibling82F870LastTick,
        &g_AbilityRedSibling82F870LastActive,
        callerRet,
        reinterpret_cast<uintptr_t>(thisPtr),
        resultValue);
    return resultValue;
}

static int __fastcall hkAbilityRedSiblingCalc82F960(void *thisPtr, void *edxUnused)
{
    (void)edxUnused;
    const DWORD callerRet = (DWORD)(uintptr_t)_ReturnAddress();
    const int resultValue = oAbilityRedSiblingCalc82F960Fn
        ? oAbilityRedSiblingCalc82F960Fn(thisPtr, edxUnused)
        : 0;
    ObserveAbilityRedSiblingCalculator(
        "82F960",
        &g_AbilityRedSibling82F960LastCaller,
        &g_AbilityRedSibling82F960LastThis,
        &g_AbilityRedSibling82F960LastTick,
        &g_AbilityRedSibling82F960LastActive,
        callerRet,
        reinterpret_cast<uintptr_t>(thisPtr),
        resultValue);
    return resultValue;
}

static int __fastcall hkAbilityRedSiblingCalc82FA50(void *thisPtr, void *edxUnused)
{
    (void)edxUnused;
    const DWORD callerRet = (DWORD)(uintptr_t)_ReturnAddress();
    const int resultValue = oAbilityRedSiblingCalc82FA50Fn
        ? oAbilityRedSiblingCalc82FA50Fn(thisPtr, edxUnused)
        : 0;
    ObserveAbilityRedSiblingCalculator(
        "82FA50",
        &g_AbilityRedSibling82FA50LastCaller,
        &g_AbilityRedSibling82FA50LastThis,
        &g_AbilityRedSibling82FA50LastTick,
        &g_AbilityRedSibling82FA50LastActive,
        callerRet,
        reinterpret_cast<uintptr_t>(thisPtr),
        resultValue);
    return resultValue;
}

static DWORD __cdecl hkAdjustAbilityRedDiff84C470PreSub(
    DWORD finalThisValue,
    DWORD frameEbp,
    DWORD mainSlotValue,
    DWORD sumAfterAdds)
{
    DWORD adjustedSum = sumAfterAdds;
    const int activeState = SkillOverlayBridgeHasLocalIndependentPotentialDisplayDeltaValue(0x44) ? 1 : 0;
    if (!activeState)
        g_AbilityRedBaseSumInactive9F7546 = sumAfterAdds;

    DWORD inactiveBaseline = g_AbilityRedBaseSumInactive9F7546;
    if (activeState && inactiveBaseline == 0)
    {
        const DWORD seededBaseline = SeedAbilityRedInactiveBaselineFromPrimary(ADDR_9F7546, sumAfterAdds);
        if (seededBaseline > 0)
        {
            g_AbilityRedBaseSumInactive9F7546 = seededBaseline;
            inactiveBaseline = seededBaseline;
            WriteLogFmt("[AbilityRedDiff] seed baseline site=0x%08X current=%u seeded=%u",
                ADDR_9F7546,
                sumAfterAdds,
                seededBaseline);
        }
    }
    if (!ShouldLogAbilityRedFinalCalculator(
            &g_AbilityRedDiff84C470LastCaller,
            &g_AbilityRedDiff84C470LastThis,
            &g_AbilityRedDiff84C470LastTick,
            &g_AbilityRedDiff84C470LastActive,
            ADDR_9F7546,
            finalThisValue,
            activeState))
    {
        const int localDeltaQuiet = SkillOverlayBridgeGetLocalIndependentPotentialDisplayDeltaValue(0x44);
        if (activeState && inactiveBaseline > 0 && sumAfterAdds > inactiveBaseline)
            adjustedSum = inactiveBaseline;
        else if (activeState && localDeltaQuiet > 0 && (int)mainSlotValue >= localDeltaQuiet)
            adjustedSum = (DWORD)((int)sumAfterAdds - localDeltaQuiet);
        return adjustedSum;
    }

    DWORD local48 = 0;
    DWORD local50 = 0;
    DWORD local60 = 0;
    if (frameEbp && !SafeIsBadReadPtr(reinterpret_cast<void*>(frameEbp + 0x48), sizeof(DWORD)))
        local48 = *reinterpret_cast<DWORD*>(frameEbp + 0x48);
    if (frameEbp && !SafeIsBadReadPtr(reinterpret_cast<void*>(frameEbp + 0x50), sizeof(DWORD)))
        local50 = *reinterpret_cast<DWORD*>(frameEbp + 0x50);
    if (frameEbp && !SafeIsBadReadPtr(reinterpret_cast<void*>(frameEbp + 0x60), sizeof(DWORD)))
        local60 = *reinterpret_cast<DWORD*>(frameEbp + 0x60);

    int decoded198 = 0;
    bool decoded198Ok = false;
    DecodeAbilityRedTripletAtOffset(finalThisValue, 0x198, &decoded198, &decoded198Ok);

    const int helperValue = static_cast<int>(sumAfterAdds) - static_cast<int>(mainSlotValue) - static_cast<int>(local48);
    const int localDelta = SkillOverlayBridgeGetLocalIndependentPotentialDisplayDeltaValue(0x44);
    if (activeState && inactiveBaseline > 0 && sumAfterAdds > inactiveBaseline)
        adjustedSum = inactiveBaseline;
    else if (activeState && localDelta > 0 && (int)mainSlotValue >= localDelta)
        adjustedSum = (DWORD)((int)sumAfterAdds - localDelta);
    const int predictedDiffRaw = static_cast<int>(local60) - static_cast<int>(sumAfterAdds);
    const int predictedDiffAdjusted = static_cast<int>(local60) - static_cast<int>(adjustedSum);

    WriteLogFmt(
        "[AbilityRedDiff] 84C470 this=0x%08X ebp=0x%08X final=%u sumRaw=%u sumAdj=%u helper=%d mainReg=%u slot198=%d/%d local48=%u local50=0x%08X localDelta=%d inactiveBase=%u diffRaw=%d diffAdj=%d active=%d",
        finalThisValue,
        frameEbp,
        local60,
        sumAfterAdds,
        adjustedSum,
        helperValue,
        mainSlotValue,
        decoded198,
        decoded198Ok ? 1 : 0,
        local48,
        local50,
        localDelta,
        inactiveBaseline,
        predictedDiffRaw,
        predictedDiffAdjusted,
        activeState);
    return adjustedSum;
}

__declspec(naked) static void hkAbilityRedDiff84C470PreSubNaked()
{
    __asm {
        add esi, ebx
        add esi, dword ptr [ebp + 0x48]
        pushfd
        pushad
        push esi
        push ebx
        push ebp
        push edi
        call hkAdjustAbilityRedDiff84C470PreSub
        add esp, 16
        mov dword ptr [esp + 4], eax
        popad
        popfd
        jmp dword ptr [g_AbilityRedDiff84C470PreSubContinue]
    }
}

static DWORD __cdecl hkAdjustAbilityRedBaseSumByLocalDelta(
    DWORD frameEbp,
    DWORD sumAfterAdds,
    int deltaOffset)
{
    (void)frameEbp;

    if (!SkillOverlayBridgeHasLocalIndependentPotentialDisplayDeltaValue(deltaOffset))
        return sumAfterAdds;

    const int localDelta = SkillOverlayBridgeGetLocalIndependentPotentialDisplayDeltaValue(deltaOffset);
    if (localDelta <= 0)
        return sumAfterAdds;

    const int rawSum = static_cast<int>(sumAfterAdds);
    if (rawSum < localDelta)
        return sumAfterAdds;

    return static_cast<DWORD>(rawSum - localDelta);
}

static DWORD* GetAbilityRedInactiveBaselineSlot(DWORD siteId)
{
    switch (siteId)
    {
    case ADDR_9F7241: return &g_AbilityRedBaseSumInactive9F7241;
    case ADDR_9F7546: return &g_AbilityRedBaseSumInactive9F7546;
    case ADDR_9F7893: return &g_AbilityRedBaseSumInactive9F7893;
    case ADDR_9F7C7F: return &g_AbilityRedBaseSumInactive9F7C7F;
    case ADDR_9F8048: return &g_AbilityRedBaseSumInactive9F8048;
    case ADDR_9F82A8: return &g_AbilityRedBaseSumInactive9F82A8;
    default:
        return nullptr;
    }
}

static bool HasPositiveLocalIndependentPotentialDeltaAny(const int* offsets, size_t count);

static bool HasPositiveLocalIndependentPotentialPrimaryDelta()
{
    const int offsets[] = { 0x08, 0x0C, 0x10, 0x14 };
    if (!SkillOverlayBridgeHasAnyLocalIndependentPotentialDisplayDeltaValue(offsets, ARRAYSIZE(offsets)))
        return false;
    return HasPositiveLocalIndependentPotentialDeltaAny(offsets, ARRAYSIZE(offsets));
}

struct AbilityRedPrimaryDeltaBackup
{
    int originalValues[4];
    bool valid[4];
    bool applied;
};

static LONG g_AbilityRedBaselineSeedGuard = 0;

static bool BeginTemporaryAbilityRedPrimaryBaseline(AbilityRedPrimaryDeltaBackup* backup)
{
    if (!backup)
        return false;

    ZeroMemory(backup, sizeof(*backup));

    DWORD* const basePrimary = reinterpret_cast<DWORD*>(ADDR_AbilityPrimaryCache);
    if (!basePrimary)
        return false;

    const struct
    {
        size_t keyIndex;
        int offset;
    } targets[] = {
        { 9,  0x08 },
        { 12, 0x0C },
        { 15, 0x10 },
        { 18, 0x14 },
    };

    for (int i = 0; i < (int)ARRAYSIZE(targets); ++i)
    {
        const int delta = SkillOverlayBridgeGetLocalIndependentPotentialDisplayDeltaValue(targets[i].offset);
        if (delta <= 0)
            continue;

        int currentValue = 0;
        if (!ReadEncryptedTripletValue(basePrimary, targets[i].keyIndex, &currentValue))
            return false;

        backup->originalValues[i] = currentValue;
        backup->valid[i] = true;
        backup->applied = true;

        if (!WriteEncryptedTripletValue(basePrimary, targets[i].keyIndex, currentValue - delta))
            return false;
    }

    return backup->applied;
}

static void EndTemporaryAbilityRedPrimaryBaseline(const AbilityRedPrimaryDeltaBackup* backup)
{
    if (!backup)
        return;

    DWORD* const basePrimary = reinterpret_cast<DWORD*>(ADDR_AbilityPrimaryCache);
    if (!basePrimary)
        return;

    const size_t keyIndices[] = { 9, 12, 15, 18 };
    for (int i = 0; i < (int)ARRAYSIZE(keyIndices); ++i)
    {
        if (!backup->valid[i])
            continue;
        WriteEncryptedTripletValue(basePrimary, keyIndices[i], backup->originalValues[i]);
    }
}

static tAbilityRedSiblingCalcFn SelectAbilityRedSiblingBaselineFn(DWORD siteId)
{
    switch (siteId)
    {
    case ADDR_9F7241:
    case ADDR_9F7546:
        return oAbilityRedSiblingCalc82F780Fn;
    case ADDR_9F7893:
        return oAbilityRedSiblingCalc82F870Fn;
    case ADDR_9F7C7F:
        return oAbilityRedSiblingCalc82F960Fn;
    case ADDR_9F8048:
    case ADDR_9F82A8:
        return oAbilityRedSiblingCalc82FA50Fn;
    default:
        return nullptr;
    }
}

static DWORD SeedAbilityRedInactiveBaselineFromPrimary(DWORD siteId, DWORD currentSum)
{
    if (!HasPositiveLocalIndependentPotentialPrimaryDelta())
        return 0;

    tAbilityRedSiblingCalcFn siblingFn = SelectAbilityRedSiblingBaselineFn(siteId);
    if (!siblingFn)
        return 0;

    void* const primaryThis = reinterpret_cast<void*>(ADDR_AbilityPrimaryCache);
    if (!primaryThis || SafeIsBadReadPtr(primaryThis, 0x40))
        return 0;

    if (InterlockedExchange(&g_AbilityRedBaselineSeedGuard, 1) != 0)
        return 0;

    DWORD seededBaseline = 0;
    AbilityRedPrimaryDeltaBackup backup = {};
    const int activeComparable = siblingFn(primaryThis, nullptr);
    if (BeginTemporaryAbilityRedPrimaryBaseline(&backup))
    {
        const int inactiveComparable = siblingFn(primaryThis, nullptr);
        const int effectDelta = activeComparable - inactiveComparable;
        if (effectDelta > 0 && static_cast<int>(currentSum) >= effectDelta)
            seededBaseline = static_cast<DWORD>(static_cast<int>(currentSum) - effectDelta);
    }
    EndTemporaryAbilityRedPrimaryBaseline(&backup);

    InterlockedExchange(&g_AbilityRedBaselineSeedGuard, 0);
    return seededBaseline;
}

static DWORD __cdecl hkAdjustAbilityRedBaseSumBySiteBaseline(
    DWORD frameEbp,
    DWORD sumAfterAdds,
    int deltaOffset,
    DWORD siteId)
{
    (void)frameEbp;

    const int primaryOffsets[] = { 0x08, 0x0C, 0x10, 0x14 };
    const bool hasDirectDelta = SkillOverlayBridgeHasLocalIndependentPotentialDisplayDeltaValue(deltaOffset);
    const bool hasPrimaryDelta = SkillOverlayBridgeHasAnyLocalIndependentPotentialDisplayDeltaValue(primaryOffsets, ARRAYSIZE(primaryOffsets));
    const bool active = hasDirectDelta || hasPrimaryDelta;
    DWORD* inactiveBaselineSlot = GetAbilityRedInactiveBaselineSlot(siteId);
    if (!active)
    {
        if (inactiveBaselineSlot)
            *inactiveBaselineSlot = sumAfterAdds;
        return sumAfterAdds;
    }

    if (inactiveBaselineSlot && *inactiveBaselineSlot == 0)
    {
        const DWORD seededBaseline = SeedAbilityRedInactiveBaselineFromPrimary(siteId, sumAfterAdds);
        if (seededBaseline > 0)
        {
            *inactiveBaselineSlot = seededBaseline;
            WriteLogFmt("[AbilityRedDiff] seed baseline site=0x%08X current=%u seeded=%u",
                siteId,
                sumAfterAdds,
                seededBaseline);
            return seededBaseline;
        }
    }

    if (inactiveBaselineSlot && *inactiveBaselineSlot > 0 && sumAfterAdds > *inactiveBaselineSlot)
    {
        const int directDelta = SkillOverlayBridgeGetLocalIndependentPotentialDisplayDeltaValue(deltaOffset);
        if (directDelta > 0 || HasPositiveLocalIndependentPotentialPrimaryDelta())
            return *inactiveBaselineSlot;
    }

    return hkAdjustAbilityRedBaseSumByLocalDelta(frameEbp, sumAfterAdds, deltaOffset);
}

static bool HasPositiveLocalIndependentPotentialDeltaAny(const int* offsets, size_t count)
{
    if (!offsets || count == 0)
        return false;
    if (count > 256)
        return false;
    if (!SkillOverlayBridgeHasAnyLocalIndependentPotentialDisplayDeltaValue(offsets, static_cast<int>(count)))
        return false;

    for (size_t i = 0; i < count; ++i)
    {
        if (SkillOverlayBridgeGetLocalIndependentPotentialDisplayDeltaValue(offsets[i]) > 0)
            return true;
    }
    return false;
}

enum AbilityRedPositiveStyleMode
{
    AbilityRedPositiveStyle_AttackRange = 1,
    AbilityRedPositiveStyle_CriticalRate = 2,
    AbilityRedPositiveStyle_Speed = 3,
    AbilityRedPositiveStyle_Jump = 4,
};

static DWORD __cdecl hkSelectAbilityRedPositiveStyle(
    DWORD frameEbp,
    DWORD currentStyle,
    int mode)
{
    const int attackOffsets[] = { 0x38, 0x3C };
    const int criticalOffsets[] = { 0x78 };
    const int speedOffsets[] = { 0x30 };
    const int jumpOffsets[] = { 0x34 };

    bool shouldUseRedStyle = false;
    switch (mode)
    {
    case AbilityRedPositiveStyle_AttackRange:
        shouldUseRedStyle = HasPositiveLocalIndependentPotentialDeltaAny(attackOffsets, ARRAYSIZE(attackOffsets));
        if (!shouldUseRedStyle)
            shouldUseRedStyle = HasPositiveLocalIndependentPotentialPrimaryDelta();
        break;
    case AbilityRedPositiveStyle_CriticalRate:
        shouldUseRedStyle = HasPositiveLocalIndependentPotentialDeltaAny(criticalOffsets, ARRAYSIZE(criticalOffsets));
        break;
    case AbilityRedPositiveStyle_Speed:
        shouldUseRedStyle = HasPositiveLocalIndependentPotentialDeltaAny(speedOffsets, ARRAYSIZE(speedOffsets));
        break;
    case AbilityRedPositiveStyle_Jump:
        shouldUseRedStyle = HasPositiveLocalIndependentPotentialDeltaAny(jumpOffsets, ARRAYSIZE(jumpOffsets));
        break;
    default:
        break;
    }

    if (!shouldUseRedStyle)
        return currentStyle;

    if (!frameEbp || SafeIsBadReadPtr(reinterpret_cast<void*>(frameEbp + 0x24), sizeof(DWORD)))
        return currentStyle;

    const DWORD redStyle = *reinterpret_cast<DWORD*>(frameEbp + 0x24);
    return redStyle ? redStyle : currentStyle;
}

__declspec(naked) static void hkAbilityRedDiff84BE40PreSubNaked()
{
    __asm {
        add esi, ebx
        add esi, dword ptr [ebp + 0x48]
        pushfd
        pushad
        push ADDR_9F7241
        push 0x40
        push esi
        push ebp
        call hkAdjustAbilityRedBaseSumBySiteBaseline
        add esp, 16
        mov dword ptr [esp + 4], eax
        popad
        popfd
        jmp dword ptr [g_AbilityRedDiff84BE40PreSubContinue]
    }
}

__declspec(naked) static void hkAbilityRedDiff84CA90AccPreSubNaked()
{
    __asm {
        add esi, edi
        add esi, dword ptr [ebp + 0x48]
        pushfd
        pushad
        push ADDR_9F7893
        push 0x28
        push esi
        push ebp
        call hkAdjustAbilityRedBaseSumBySiteBaseline
        add esp, 16
        mov dword ptr [esp + 4], eax
        popad
        popfd
        jmp dword ptr [g_AbilityRedDiff84CA90AccPreSubContinue]
    }
}

__declspec(naked) static void hkAbilityRedDiff84CA90MagicAccPreSubNaked()
{
    __asm {
        add esi, ebx
        add esi, dword ptr [ebp + 0x48]
        pushfd
        pushad
        push ADDR_9F7C7F
        push 0x28
        push esi
        push ebp
        call hkAdjustAbilityRedBaseSumBySiteBaseline
        add esp, 16
        mov dword ptr [esp + 4], eax
        popad
        popfd
        jmp dword ptr [g_AbilityRedDiff84CA90MagicAccPreSubContinue]
    }
}

__declspec(naked) static void hkAbilityRedDiff84CBD0AvoidPreSubNaked()
{
    __asm {
        add esi, ebx
        add esi, dword ptr [ebp + 0x48]
        pushfd
        pushad
        push ADDR_9F8048
        push 0x2C
        push esi
        push ebp
        call hkAdjustAbilityRedBaseSumBySiteBaseline
        add esp, 16
        mov dword ptr [esp + 4], eax
        popad
        popfd
        jmp dword ptr [g_AbilityRedDiff84CBD0AvoidPreSubContinue]
    }
}

__declspec(naked) static void hkAbilityRedDiff84CBD0MagicAvoidPreSubNaked()
{
    __asm {
        add eax, dword ptr [ebp + 0x48]
        add esi, eax
        pushfd
        pushad
        push ADDR_9F82A8
        push 0x2C
        push esi
        push ebp
        call hkAdjustAbilityRedBaseSumBySiteBaseline
        add esp, 16
        mov dword ptr [esp + 4], eax
        popad
        popfd
        jmp dword ptr [g_AbilityRedDiff84CBD0MagicAvoidPreSubContinue]
    }
}

__declspec(naked) static void hkAbilityRedAttackRangeStyleNaked()
{
    __asm {
        pushfd
        pushad
        push AbilityRedPositiveStyle_AttackRange
        push dword ptr [ebp + 0x68]
        push ebp
        call hkSelectAbilityRedPositiveStyle
        add esp, 12
        mov dword ptr [esp + 28], eax
        popad
        popfd
        lea ecx, [ebp + 0x34]
        push ecx
        jmp dword ptr [g_AbilityRedAttackRangeStyleContinue]
    }
}

__declspec(naked) static void hkAbilityRedCriticalRateStyleNaked()
{
    __asm {
        pushfd
        pushad
        push AbilityRedPositiveStyle_CriticalRate
        push dword ptr [ebp + 0x68]
        push ebp
        call hkSelectAbilityRedPositiveStyle
        add esp, 12
        mov dword ptr [esp + 28], eax
        popad
        popfd
        lea ecx, [ebp + 0x34]
        push ecx
        jmp dword ptr [g_AbilityRedCriticalRateStyleContinue]
    }
}

__declspec(naked) static void hkAbilityRedSpeedStyleNaked()
{
    __asm {
        pushfd
        pushad
        push AbilityRedPositiveStyle_Speed
        push dword ptr [ebp + 0x30]
        push ebp
        call hkSelectAbilityRedPositiveStyle
        add esp, 12
        mov dword ptr [esp + 28], eax
        popad
        popfd
        lea ecx, [ebp + 0x94]
        push ecx
        jmp dword ptr [g_AbilityRedSpeedStyleContinue]
    }
}

__declspec(naked) static void hkAbilityRedJumpStyleNaked()
{
    __asm {
        pushfd
        pushad
        push AbilityRedPositiveStyle_Jump
        push dword ptr [ebp + 0x30]
        push ebp
        call hkSelectAbilityRedPositiveStyle
        add esp, 12
        mov dword ptr [esp + 24], eax
        popad
        popfd
        mov esi, dword ptr [ebp + 0x64]
        jmp dword ptr [g_AbilityRedJumpStyleContinue]
    }
}

static bool ShouldLogAbilityRedBakeWrite(
    DWORD *lastSig,
    DWORD *lastTick,
    uintptr_t thisValue,
    uintptr_t srcValue,
    int rawDelta,
    int priorValue,
    int activeState)
{
    if (!EnableAbilityRedDiagnosticLogs())
        return false;
    if (!lastSig || !lastTick)
        return true;

    const DWORD signature =
        (DWORD)thisValue ^
        ((DWORD)srcValue << 1) ^
        (DWORD)(rawDelta * 131) ^
        (DWORD)(priorValue * 17) ^
        (DWORD)(activeState << 30);

    const DWORD now = GetTickCount();
    if (*lastSig == signature && now - *lastTick <= 1000)
        return false;

    *lastSig = signature;
    *lastTick = now;
    return true;
}

static void ObserveAbilityRedBakeWrite(
    const char *label,
    DWORD *lastSig,
    DWORD *lastTick,
    DWORD thisValue,
    DWORD srcValue,
    DWORD rawEax,
    DWORD oldValue,
    size_t destOffset,
    size_t srcStartOffset,
    size_t srcEndOffset)
{
    if (!EnableAbilityRedDiagnosticLogs())
        return;

    const int activeState = SkillOverlayBridgeHasLocalIndependentPotentialDisplayBonuses() ? 1 : 0;
    const short rawShort = (short)(rawEax & 0xFFFF);
    const int rawDelta = (int)rawShort;
    const int priorValue = (int)oldValue;

    if (!(activeState || rawDelta != 0 || destOffset == 0x198))
        return;

    if (!ShouldLogAbilityRedBakeWrite(
            lastSig,
            lastTick,
            thisValue,
            srcValue,
            rawDelta,
            priorValue,
            activeState))
    {
        return;
    }

    int slotBefore = 0;
    bool slotBeforeOk = false;
    DecodeAbilityRedTripletAtOffset((uintptr_t)thisValue, destOffset, &slotBefore, &slotBeforeOk);

    DWORD srcTail = 0;
    bool srcTailOk = false;
    if (srcValue && !SafeIsBadReadPtr(reinterpret_cast<void*>(srcValue + srcEndOffset), sizeof(DWORD)))
    {
        srcTail = *reinterpret_cast<DWORD*>(srcValue + srcEndOffset);
        srcTailOk = true;
    }

    WriteLogFmt(
        "[AbilityRedBake] %s this=0x%08X src=0x%08X dest=0x%03X srcField=[0x%02X..0x%02X] raw=%d prior=%d slotBefore=%d/%d predicted=%d tail=0x%08X/%d active=%d",
        label ? label : "unknown",
        thisValue,
        srcValue,
        (unsigned int)destOffset,
        (unsigned int)srcStartOffset,
        (unsigned int)srcEndOffset,
        rawDelta,
        priorValue,
        slotBefore,
        slotBeforeOk ? 1 : 0,
        priorValue + rawDelta,
        srcTail,
        srcTailOk ? 1 : 0,
        activeState);
}

static void __cdecl hkObserveAbilityRedBake857BB6(DWORD thisValue, DWORD srcValue, DWORD rawEax, DWORD oldValue)
{
    ObserveAbilityRedBakeWrite("857BB6", &g_AbilityRedBake857BB6LastSig, &g_AbilityRedBake857BB6LastTick,
        thisValue, srcValue, rawEax, oldValue, 0x15C, 0x91, 0x95);
}

static void __cdecl hkObserveAbilityRedBake857C29(DWORD thisValue, DWORD srcValue, DWORD rawEax, DWORD oldValue)
{
    ObserveAbilityRedBakeWrite("857C29", &g_AbilityRedBake857C29LastSig, &g_AbilityRedBake857C29LastTick,
        thisValue, srcValue, rawEax, oldValue, 0x198, 0xA1, 0xA5);
}

static void __cdecl hkObserveAbilityRedBake857C9C(DWORD thisValue, DWORD srcValue, DWORD rawEax, DWORD oldValue)
{
    ObserveAbilityRedBakeWrite("857C9C", &g_AbilityRedBake857C9CLastSig, &g_AbilityRedBake857C9CLastTick,
        thisValue, srcValue, rawEax, oldValue, 0x1D4, 0xA9, 0xAD);
}

static void __cdecl hkObserveAbilityRedBake857D0F(DWORD thisValue, DWORD srcValue, DWORD rawEax, DWORD oldValue)
{
    ObserveAbilityRedBakeWrite("857D0F", &g_AbilityRedBake857D0FLastSig, &g_AbilityRedBake857D0FLastTick,
        thisValue, srcValue, rawEax, oldValue, 0x210, 0xB1, 0xB5);
}

static void ObserveAbilityRedBake198Site(
    const char *label,
    DWORD *lastSig,
    DWORD *lastTick,
    DWORD thisValue,
    DWORD auxPtr,
    int auxValue,
    int sumValue)
{
    if (!EnableAbilityRedDiagnosticLogs())
        return;

    const int activeState = SkillOverlayBridgeHasLocalIndependentPotentialDisplayBonuses() ? 1 : 0;

    int prior198 = 0;
    bool prior198Ok = false;
    DecodeAbilityRedTripletAtOffset((uintptr_t)thisValue, 0x198, &prior198, &prior198Ok);

    if (!(activeState || sumValue != prior198))
        return;

    if (!ShouldLogAbilityRedBakeWrite(
            lastSig,
            lastTick,
            thisValue,
            auxPtr,
            sumValue - prior198,
            prior198,
            activeState))
    {
        return;
    }

    WriteLogFmt(
        "[AbilityRedBake198] %s this=0x%08X prior198=%d/%d sum=%d delta=%d auxPtr=0x%08X auxValue=%d active=%d",
        label ? label : "unknown",
        thisValue,
        prior198,
        prior198Ok ? 1 : 0,
        sumValue,
        sumValue - prior198,
        auxPtr,
        auxValue,
        activeState);
}

static void __cdecl hkObserveAbilityRedBake1988569C3(DWORD thisValue, DWORD ediValue, DWORD ebxValue)
{
    int auxValue = 0;
    if (ediValue && !SafeIsBadReadPtr(reinterpret_cast<void*>(ediValue + 0x44), sizeof(DWORD)))
        auxValue = (int)(*reinterpret_cast<DWORD*>(ediValue + 0x44));

    ObserveAbilityRedBake198Site(
        "8569C3",
        &g_AbilityRedBake1988569C3LastSig,
        &g_AbilityRedBake1988569C3LastTick,
        thisValue,
        ediValue ? (ediValue + 0x44) : 0,
        auxValue,
        (int)ebxValue);
}

static void __cdecl hkObserveAbilityRedBake198856D57(DWORD thisValue)
{
    ObserveAbilityRedBake198Site(
        "856D57",
        &g_AbilityRedBake198856D57LastSig,
        &g_AbilityRedBake198856D57LastTick,
        thisValue,
        0,
        0,
        0);
}

static void __cdecl hkObserveAbilityRedBake19885725F(DWORD thisValue, DWORD ediValue, DWORD ebxValue)
{
    DWORD sourceNode = 0;
    int auxValue = 0;
    if (ediValue && !SafeIsBadReadPtr(reinterpret_cast<void*>(ediValue), sizeof(DWORD)))
    {
        sourceNode = *reinterpret_cast<DWORD*>(ediValue);
        if (sourceNode && !SafeIsBadReadPtr(reinterpret_cast<void*>(sourceNode + 0x18), sizeof(short)))
            auxValue = (int)(*(short*)(sourceNode + 0x18));
    }

    ObserveAbilityRedBake198Site(
        "85725F",
        &g_AbilityRedBake19885725FLastSig,
        &g_AbilityRedBake19885725FLastTick,
        thisValue,
        sourceNode ? (sourceNode + 0x18) : ediValue,
        auxValue,
        (int)ebxValue);
}

static void __cdecl hkObserveAbilityRedBake198857C3B(DWORD thisValue, DWORD ediValue, DWORD ebxValue)
{
    int auxValue = 0;
    if (ediValue && !SafeIsBadReadPtr(reinterpret_cast<void*>(ediValue + 0xA5), sizeof(DWORD)))
        auxValue = (int)(*(DWORD*)(ediValue + 0xA5));

    ObserveAbilityRedBake198Site(
        "857C3B",
        &g_AbilityRedBake198857C3BLastSig,
        &g_AbilityRedBake198857C3BLastTick,
        thisValue,
        ediValue ? (ediValue + 0xA1) : 0,
        auxValue,
        (int)ebxValue);
}

static void __cdecl hkObserveAbilityRedBake198858AED(DWORD thisValue, DWORD ediValue)
{
    ObserveAbilityRedBake198Site(
        "858AED",
        &g_AbilityRedBake198858AEDLastSig,
        &g_AbilityRedBake198858AEDLastTick,
        thisValue,
        0,
        0,
        (int)ediValue);
}

static void __cdecl hkObserveAbilityRedBake198831A50(DWORD thisValue, DWORD ediValue)
{
    ObserveAbilityRedBake198Site(
        "831A50",
        &g_AbilityRedBake198831A50LastSig,
        &g_AbilityRedBake198831A50LastTick,
        thisValue,
        0,
        0,
        (int)ediValue);
}

static void __cdecl hkObserveAbilityRedBake19883AF02(DWORD thisValue, DWORD ediValue, DWORD ebxValue)
{
    int auxValue = 0;
    if (ediValue && !SafeIsBadReadPtr(reinterpret_cast<void*>(ediValue + 0x7C5), sizeof(DWORD)))
        auxValue = (int)(*(DWORD*)(ediValue + 0x7C5));

    ObserveAbilityRedBake198Site(
        "83AF02",
        &g_AbilityRedBake19883AF02LastSig,
        &g_AbilityRedBake19883AF02LastTick,
        thisValue,
        ediValue ? (ediValue + 0x7C5) : 0,
        auxValue,
        (int)ebxValue);
}

__declspec(naked) static void hkAbilityRedBake857BB6Naked()
{
    __asm {
        pushfd
        pushad
        push ebp
        push eax
        push edi
        push esi
        call hkObserveAbilityRedBake857BB6
        add esp, 16
        popad
        popfd
        jmp [oAbilityRedBake857BB6Hook]
    }
}

__declspec(naked) static void hkAbilityRedBake857C29Naked()
{
    __asm {
        pushfd
        pushad
        push ebp
        push eax
        push edi
        push esi
        call hkObserveAbilityRedBake857C29
        add esp, 16
        popad
        popfd
        jmp [oAbilityRedBake857C29Hook]
    }
}

__declspec(naked) static void hkAbilityRedBake857C9CNaked()
{
    __asm {
        pushfd
        pushad
        push ebp
        push eax
        push edi
        push esi
        call hkObserveAbilityRedBake857C9C
        add esp, 16
        popad
        popfd
        jmp [oAbilityRedBake857C9CHook]
    }
}

__declspec(naked) static void hkAbilityRedBake857D0FNaked()
{
    __asm {
        pushfd
        pushad
        push ebp
        push eax
        push edi
        push esi
        call hkObserveAbilityRedBake857D0F
        add esp, 16
        popad
        popfd
        jmp [oAbilityRedBake857D0FHook]
    }
}

__declspec(naked) static void hkAbilityRedBake1988569C3Naked()
{
    __asm {
        pushfd
        pushad
        push ebx
        push edi
        push esi
        call hkObserveAbilityRedBake1988569C3
        add esp, 12
        popad
        popfd
        jmp [oAbilityRedBake1988569C3Hook]
    }
}

__declspec(naked) static void hkAbilityRedBake198856D57Naked()
{
    __asm {
        pushfd
        pushad
        push esi
        call hkObserveAbilityRedBake198856D57
        add esp, 4
        popad
        popfd
        jmp [oAbilityRedBake198856D57Hook]
    }
}

__declspec(naked) static void hkAbilityRedBake19885725FNaked()
{
    __asm {
        pushfd
        pushad
        push ebx
        push edi
        push esi
        call hkObserveAbilityRedBake19885725F
        add esp, 12
        popad
        popfd
        jmp [oAbilityRedBake19885725FHook]
    }
}

__declspec(naked) static void hkAbilityRedBake198857C3BNaked()
{
    __asm {
        pushfd
        pushad
        push ebx
        push edi
        push esi
        call hkObserveAbilityRedBake198857C3B
        add esp, 12
        popad
        popfd
        jmp [oAbilityRedBake198857C3BHook]
    }
}

__declspec(naked) static void hkAbilityRedBake198858AEDNaked()
{
    __asm {
        pushfd
        pushad
        push edi
        push esi
        call hkObserveAbilityRedBake198858AED
        add esp, 8
        popad
        popfd
        jmp [oAbilityRedBake198858AEDHook]
    }
}

__declspec(naked) static void hkAbilityRedBake198831A50Naked()
{
    __asm {
        pushfd
        pushad
        push edi
        push esi
        call hkObserveAbilityRedBake198831A50
        add esp, 8
        popad
        popfd
        jmp [oAbilityRedBake198831A50Hook]
    }
}

__declspec(naked) static void hkAbilityRedBake19883AF02Naked()
{
    __asm {
        pushfd
        pushad
        push ebx
        push edi
        push esi
        call hkObserveAbilityRedBake19883AF02
        add esp, 12
        popad
        popfd
        jmp [oAbilityRedBake19883AF02Hook]
    }
}

static int __fastcall hkAbilityRedMasterAggregateFunction(
    void *thisPtr,
    void *edxUnused,
    DWORD arg1,
    DWORD arg2,
    DWORD arg3,
    DWORD arg4,
    DWORD arg5,
    DWORD arg6,
    DWORD arg7)
{
    (void)edxUnused;
    const bool wantDiagnosticLog = EnableAbilityRedDiagnosticLogs();
    const int aggregatePatchOffsets[] = { 0x30, 0x34, 0x38, 0x3C };
    const bool hasAggregatePatchDelta = SkillOverlayBridgeHasAnyLocalIndependentPotentialDisplayDeltaValue(
        aggregatePatchOffsets,
        ARRAYSIZE(aggregatePatchOffsets));

    if (!wantDiagnosticLog)
    {
        const int resultValue = oAbilityRedMasterAggregateFn
            ? oAbilityRedMasterAggregateFn(thisPtr, edxUnused, arg1, arg2, arg3, arg4, arg5, arg6, arg7)
            : 0;

        if (thisPtr && hasAggregatePatchDelta)
        {
            const struct
            {
                size_t keyIndex;
                int deltaOffset;
            } targets[] = {
                { 159, 0x30 },
                { 171, 0x34 },
                { 57, 0x38 },
                { 87, 0x3C },
            };

            DWORD *tripletBase = reinterpret_cast<DWORD*>(thisPtr);
            for (int i = 0; i < (int)ARRAYSIZE(targets); ++i)
            {
                const int delta = SkillOverlayBridgeGetLocalIndependentPotentialDisplayDeltaValue(targets[i].deltaOffset);
                if (delta == 0)
                    continue;

                int currentValue = 0;
                if (!ReadEncryptedTripletValue(tripletBase, targets[i].keyIndex, &currentValue))
                    continue;

                int targetValue = currentValue + delta;
                if (targets[i].deltaOffset == 0x30 || targets[i].deltaOffset == 0x34)
                {
                    if (targetValue < 0)
                        targetValue = 0;
                    if (targetValue > 9999)
                        targetValue = 9999;
                }
                if (currentValue != targetValue)
                    WriteEncryptedTripletValue(tripletBase, targets[i].keyIndex, targetValue);
            }
        }

        return resultValue;
    }

    const DWORD callerRet = (DWORD)(uintptr_t)_ReturnAddress();
    DWORD before3[6] = {};
    DWORD before4[6] = {};
    DWORD before5[6] = {};
    DWORD before6[6] = {};
    DWORD before7[6] = {};
    int before3A = 0, before3B = 0, before4A = 0, before4B = 0, before5A = 0, before5B = 0, before6A = 0, before6B = 0, before7A = 0, before7B = 0;
    int after3A = 0, after3B = 0, after4A = 0, after4B = 0, after5A = 0, after5B = 0, after6A = 0, after6B = 0, after7A = 0, after7B = 0;
    bool before3OkA = false, before3OkB = false, before4OkA = false, before4OkB = false, before5OkA = false, before5OkB = false, before6OkA = false, before6OkB = false, before7OkA = false, before7OkB = false;
    bool after3OkA = false, after3OkB = false, after4OkA = false, after4OkB = false, after5OkA = false, after5OkB = false, after6OkA = false, after6OkB = false, after7OkA = false, after7OkB = false;
    int before3Wdef = 0, before3Mdef = 0, before4Wdef = 0, before4Mdef = 0, before5Wdef = 0, before5Mdef = 0, before6Wdef = 0, before6Mdef = 0, before7Wdef = 0, before7Mdef = 0;
    int after3Wdef = 0, after3Mdef = 0, after4Wdef = 0, after4Mdef = 0, after5Wdef = 0, after5Mdef = 0, after6Wdef = 0, after6Mdef = 0, after7Wdef = 0, after7Mdef = 0;
    bool before3WdefOk = false, before3MdefOk = false, before4WdefOk = false, before4MdefOk = false, before5WdefOk = false, before5MdefOk = false, before6WdefOk = false, before6MdefOk = false, before7WdefOk = false, before7MdefOk = false;
    bool after3WdefOk = false, after3MdefOk = false, after4WdefOk = false, after4MdefOk = false, after5WdefOk = false, after5MdefOk = false, after6WdefOk = false, after6MdefOk = false, after7WdefOk = false, after7MdefOk = false;
    const size_t tripletOffsets[] = { 0x90, 0xE4, 0x114, 0x120, 0x150, 0x15C, 0x18C, 0x198, 0x1C8, 0x1D4, 0x204, 0x210, 0x240 };
    int tripletBefore[(sizeof(tripletOffsets) / sizeof(tripletOffsets[0]))] = {};
    int tripletAfter[(sizeof(tripletOffsets) / sizeof(tripletOffsets[0]))] = {};
    bool tripletBeforeOk[(sizeof(tripletOffsets) / sizeof(tripletOffsets[0]))] = {};
    bool tripletAfterOk[(sizeof(tripletOffsets) / sizeof(tripletOffsets[0]))] = {};
    AbilityRedMovementDiagnosis movementBefore = {};
    AbilityRedMovementDiagnosis movementAfter = {};
    ReadAbilityRedMasterAggregateBuffer(arg3, before3, ARRAYSIZE(before3));
    ReadAbilityRedMasterAggregateBuffer(arg4, before4, ARRAYSIZE(before4));
    ReadAbilityRedMasterAggregateBuffer(arg5, before5, ARRAYSIZE(before5));
    ReadAbilityRedMasterAggregateBuffer(arg6, before6, ARRAYSIZE(before6));
    ReadAbilityRedMasterAggregateBuffer(arg7, before7, ARRAYSIZE(before7));
    DecodeAbilityRedMasterTripletPair(arg3, before3, &before3A, &before3B, &before3OkA, &before3OkB);
    DecodeAbilityRedMasterTripletPair(arg4, before4, &before4A, &before4B, &before4OkA, &before4OkB);
    DecodeAbilityRedMasterTripletPair(arg5, before5, &before5A, &before5B, &before5OkA, &before5OkB);
    DecodeAbilityRedMasterTripletPair(arg6, before6, &before6A, &before6B, &before6OkA, &before6OkB);
    DecodeAbilityRedMasterTripletPair(arg7, before7, &before7A, &before7B, &before7OkA, &before7OkB);
    DecodeAbilityRedMasterDefenseValues(arg3, &before3Wdef, &before3Mdef, &before3WdefOk, &before3MdefOk);
    DecodeAbilityRedMasterDefenseValues(arg4, &before4Wdef, &before4Mdef, &before4WdefOk, &before4MdefOk);
    DecodeAbilityRedMasterDefenseValues(arg5, &before5Wdef, &before5Mdef, &before5WdefOk, &before5MdefOk);
    DecodeAbilityRedMasterDefenseValues(arg6, &before6Wdef, &before6Mdef, &before6WdefOk, &before6MdefOk);
    DecodeAbilityRedMasterDefenseValues(arg7, &before7Wdef, &before7Mdef, &before7WdefOk, &before7MdefOk);
    for (size_t i = 0; i < ARRAYSIZE(tripletOffsets); ++i)
        DecodeAbilityRedTripletAtOffset(reinterpret_cast<uintptr_t>(thisPtr), tripletOffsets[i], &tripletBefore[i], &tripletBeforeOk[i]);
    CollectAbilityRedMovementDiagnosis(thisPtr, arg2, arg4, &movementBefore);

    const int resultValue = oAbilityRedMasterAggregateFn
        ? oAbilityRedMasterAggregateFn(thisPtr, edxUnused, arg1, arg2, arg3, arg4, arg5, arg6, arg7)
        : 0;

    int patchedWatkBefore = 0;
    int patchedWatkAfter = 0;
    int patchedMatkBefore = 0;
    int patchedMatkAfter = 0;
    int patchedSpeedBefore = 0;
    int patchedSpeedAfter = 0;
    int patchedJumpBefore = 0;
    int patchedJumpAfter = 0;
    bool patchedWatk = false;
    bool patchedMatk = false;
    bool patchedSpeed = false;
    bool patchedJump = false;
    if (thisPtr && hasAggregatePatchDelta)
    {
        // Keep upstream display-prep observe-only, but patch the final aggregate object
        // so attack-range and movement consumers read local display-only deltas.
        const struct
        {
            size_t keyIndex;
            int deltaOffset;
            int *beforeValue;
            int *afterValue;
            bool *applied;
        } targets[] = {
            { 159, 0x30, &patchedSpeedBefore, &patchedSpeedAfter, &patchedSpeed },
            { 171, 0x34, &patchedJumpBefore, &patchedJumpAfter, &patchedJump },
            { 57, 0x38, &patchedWatkBefore, &patchedWatkAfter, &patchedWatk },
            { 87, 0x3C, &patchedMatkBefore, &patchedMatkAfter, &patchedMatk },
        };

        DWORD *tripletBase = reinterpret_cast<DWORD*>(thisPtr);
        for (int i = 0; i < (int)ARRAYSIZE(targets); ++i)
        {
            const int delta = SkillOverlayBridgeGetLocalIndependentPotentialDisplayDeltaValue(targets[i].deltaOffset);
            if (delta == 0)
                continue;

            int currentValue = 0;
            if (!ReadEncryptedTripletValue(tripletBase, targets[i].keyIndex, &currentValue))
                continue;

            int targetValue = currentValue + delta;
            if (targets[i].deltaOffset == 0x30 || targets[i].deltaOffset == 0x34)
            {
                if (targetValue < 0)
                    targetValue = 0;
                if (targetValue > 9999)
                    targetValue = 9999;
            }
            if (currentValue == targetValue)
                continue;
            if (!WriteEncryptedTripletValue(tripletBase, targets[i].keyIndex, targetValue))
                continue;

            *targets[i].beforeValue = currentValue;
            *targets[i].afterValue = targetValue;
            *targets[i].applied = true;
        }
    }

    DWORD after3[6] = {};
    DWORD after4[6] = {};
    DWORD after5[6] = {};
    DWORD after6[6] = {};
    DWORD after7[6] = {};
    ReadAbilityRedMasterAggregateBuffer(arg3, after3, ARRAYSIZE(after3));
    ReadAbilityRedMasterAggregateBuffer(arg4, after4, ARRAYSIZE(after4));
    ReadAbilityRedMasterAggregateBuffer(arg5, after5, ARRAYSIZE(after5));
    ReadAbilityRedMasterAggregateBuffer(arg6, after6, ARRAYSIZE(after6));
    ReadAbilityRedMasterAggregateBuffer(arg7, after7, ARRAYSIZE(after7));
    DecodeAbilityRedMasterTripletPair(arg3, after3, &after3A, &after3B, &after3OkA, &after3OkB);
    DecodeAbilityRedMasterTripletPair(arg4, after4, &after4A, &after4B, &after4OkA, &after4OkB);
    DecodeAbilityRedMasterTripletPair(arg5, after5, &after5A, &after5B, &after5OkA, &after5OkB);
    DecodeAbilityRedMasterTripletPair(arg6, after6, &after6A, &after6B, &after6OkA, &after6OkB);
    DecodeAbilityRedMasterTripletPair(arg7, after7, &after7A, &after7B, &after7OkA, &after7OkB);
    DecodeAbilityRedMasterDefenseValues(arg3, &after3Wdef, &after3Mdef, &after3WdefOk, &after3MdefOk);
    DecodeAbilityRedMasterDefenseValues(arg4, &after4Wdef, &after4Mdef, &after4WdefOk, &after4MdefOk);
    DecodeAbilityRedMasterDefenseValues(arg5, &after5Wdef, &after5Mdef, &after5WdefOk, &after5MdefOk);
    DecodeAbilityRedMasterDefenseValues(arg6, &after6Wdef, &after6Mdef, &after6WdefOk, &after6MdefOk);
    DecodeAbilityRedMasterDefenseValues(arg7, &after7Wdef, &after7Mdef, &after7WdefOk, &after7MdefOk);
    for (size_t i = 0; i < ARRAYSIZE(tripletOffsets); ++i)
        DecodeAbilityRedTripletAtOffset(reinterpret_cast<uintptr_t>(thisPtr), tripletOffsets[i], &tripletAfter[i], &tripletAfterOk[i]);
    CollectAbilityRedMovementDiagnosis(thisPtr, arg2, arg4, &movementAfter);

    if (ShouldLogAbilityRedMasterAggregate(callerRet))
    {
        bool movementFinalCapOk = false;
        const int movementFinalCap = ComputeAbilityRedMovementFinalCap(&movementAfter, &movementFinalCapOk);

        WriteLogFmt(
            "[AbilityRedMaster] 856C60 caller=0x%08X this=0x%08X args=[0x%08X,0x%08X,0x%08X,0x%08X,0x%08X,0x%08X,0x%08X] result=%d active=%d",
            callerRet,
            (DWORD)(uintptr_t)thisPtr,
            arg1, arg2, arg3, arg4, arg5, arg6, arg7,
            resultValue,
            SkillOverlayBridgeHasLocalIndependentPotentialDisplayBonuses() ? 1 : 0);

        WriteLogFmt(
            "[AbilityRedMaster] 856C60 decA a3=%d/%d ok=%d/%d -> %d/%d ok=%d/%d a4=%d/%d ok=%d/%d -> %d/%d ok=%d/%d a5=%d/%d ok=%d/%d -> %d/%d ok=%d/%d",
            before3A, before3B, before3OkA ? 1 : 0, before3OkB ? 1 : 0, after3A, after3B, after3OkA ? 1 : 0, after3OkB ? 1 : 0,
            before4A, before4B, before4OkA ? 1 : 0, before4OkB ? 1 : 0, after4A, after4B, after4OkA ? 1 : 0, after4OkB ? 1 : 0,
            before5A, before5B, before5OkA ? 1 : 0, before5OkB ? 1 : 0, after5A, after5B, after5OkA ? 1 : 0, after5OkB ? 1 : 0);

        WriteLogFmt(
            "[AbilityRedMaster] 856C60 decB a6=%d/%d ok=%d/%d -> %d/%d ok=%d/%d a7=%d/%d ok=%d/%d -> %d/%d ok=%d/%d",
            before6A, before6B, before6OkA ? 1 : 0, before6OkB ? 1 : 0, after6A, after6B, after6OkA ? 1 : 0, after6OkB ? 1 : 0,
            before7A, before7B, before7OkA ? 1 : 0, before7OkB ? 1 : 0, after7A, after7B, after7OkA ? 1 : 0, after7OkB ? 1 : 0);

        WriteLogFmt(
            "[AbilityRedMaster] 856C60 def a3=%d/%d ok=%d/%d -> %d/%d ok=%d/%d a4=%d/%d ok=%d/%d -> %d/%d ok=%d/%d",
            before3Wdef, before3Mdef, before3WdefOk ? 1 : 0, before3MdefOk ? 1 : 0, after3Wdef, after3Mdef, after3WdefOk ? 1 : 0, after3MdefOk ? 1 : 0,
            before4Wdef, before4Mdef, before4WdefOk ? 1 : 0, before4MdefOk ? 1 : 0, after4Wdef, after4Mdef, after4WdefOk ? 1 : 0, after4MdefOk ? 1 : 0);

        WriteLogFmt(
            "[AbilityRedMaster] 856C60 def2 a5=%d/%d ok=%d/%d -> %d/%d ok=%d/%d a6=%d/%d ok=%d/%d -> %d/%d ok=%d/%d a7=%d/%d ok=%d/%d -> %d/%d ok=%d/%d",
            before5Wdef, before5Mdef, before5WdefOk ? 1 : 0, before5MdefOk ? 1 : 0, after5Wdef, after5Mdef, after5WdefOk ? 1 : 0, after5MdefOk ? 1 : 0,
            before6Wdef, before6Mdef, before6WdefOk ? 1 : 0, before6MdefOk ? 1 : 0, after6Wdef, after6Mdef, after6WdefOk ? 1 : 0, after6MdefOk ? 1 : 0,
            before7Wdef, before7Mdef, before7WdefOk ? 1 : 0, before7MdefOk ? 1 : 0, after7Wdef, after7Mdef, after7WdefOk ? 1 : 0, after7MdefOk ? 1 : 0);

        WriteLogFmt(
            "[AbilityRedMaster] 856C60 thisdecA 90=%d/%d->%d/%d E4=%d/%d->%d/%d 114=%d/%d->%d/%d 120=%d/%d->%d/%d",
            tripletBefore[0], tripletBeforeOk[0] ? 1 : 0, tripletAfter[0], tripletAfterOk[0] ? 1 : 0,
            tripletBefore[1], tripletBeforeOk[1] ? 1 : 0, tripletAfter[1], tripletAfterOk[1] ? 1 : 0,
            tripletBefore[2], tripletBeforeOk[2] ? 1 : 0, tripletAfter[2], tripletAfterOk[2] ? 1 : 0,
            tripletBefore[3], tripletBeforeOk[3] ? 1 : 0, tripletAfter[3], tripletAfterOk[3] ? 1 : 0);

        WriteLogFmt(
            "[AbilityRedMaster] 856C60 thisdecB 150=%d/%d->%d/%d 15C=%d/%d->%d/%d 18C=%d/%d->%d/%d 198=%d/%d->%d/%d",
            tripletBefore[4], tripletBeforeOk[4] ? 1 : 0, tripletAfter[4], tripletAfterOk[4] ? 1 : 0,
            tripletBefore[5], tripletBeforeOk[5] ? 1 : 0, tripletAfter[5], tripletAfterOk[5] ? 1 : 0,
            tripletBefore[6], tripletBeforeOk[6] ? 1 : 0, tripletAfter[6], tripletAfterOk[6] ? 1 : 0,
            tripletBefore[7], tripletBeforeOk[7] ? 1 : 0, tripletAfter[7], tripletAfterOk[7] ? 1 : 0);

        WriteLogFmt(
            "[AbilityRedMaster] 856C60 thisdecC 1C8=%d/%d->%d/%d 1D4=%d/%d->%d/%d 204=%d/%d->%d/%d 210=%d/%d->%d/%d 240=%d/%d->%d/%d",
            tripletBefore[8], tripletBeforeOk[8] ? 1 : 0, tripletAfter[8], tripletAfterOk[8] ? 1 : 0,
            tripletBefore[9], tripletBeforeOk[9] ? 1 : 0, tripletAfter[9], tripletAfterOk[9] ? 1 : 0,
            tripletBefore[10], tripletBeforeOk[10] ? 1 : 0, tripletAfter[10], tripletAfterOk[10] ? 1 : 0,
            tripletBefore[11], tripletBeforeOk[11] ? 1 : 0, tripletAfter[11], tripletAfterOk[11] ? 1 : 0,
            tripletBefore[12], tripletBeforeOk[12] ? 1 : 0, tripletAfter[12], tripletAfterOk[12] ? 1 : 0);

        WriteLogFmt(
            "[AbilityRedMaster] 856C60 move a4=0x%08X a6=0x%08X mount[a4=%d/%d user=%d/%d] speedAdd=%d/%d capBase=%d/%d override=%d/%d finalCap=%d/%d speed=%d/%d->%d/%d jump=%d/%d->%d/%d",
            arg2,
            arg4,
            movementAfter.mountItemIdFromA4, movementAfter.mountItemIdFromA4Ok ? 1 : 0,
            movementAfter.mountItemIdFromUser, movementAfter.mountItemIdFromUserOk ? 1 : 0,
            movementAfter.speedSourceAdd, movementAfter.speedSourceAddOk ? 1 : 0,
            movementAfter.speedCapBase, movementAfter.speedCapBaseOk ? 1 : 0,
            movementAfter.speedCapOverride, movementAfter.speedCapOverrideOk ? 1 : 0,
            movementFinalCap, movementFinalCapOk ? 1 : 0,
            movementBefore.currentSpeed, movementBefore.currentSpeedOk ? 1 : 0,
            movementAfter.currentSpeed, movementAfter.currentSpeedOk ? 1 : 0,
            movementBefore.currentJump, movementBefore.currentJumpOk ? 1 : 0,
            movementAfter.currentJump, movementAfter.currentJumpOk ? 1 : 0);

        if (patchedWatk || patchedMatk || patchedSpeed || patchedJump)
        {
            WriteLogFmt(
                "[AbilityRedMasterPatch] 856C60 this=0x%08X speed=%d->%d jump=%d->%d watk=%d->%d matk=%d->%d active=%d",
                (DWORD)(uintptr_t)thisPtr,
                patchedSpeedBefore,
                patchedSpeedAfter,
                patchedJumpBefore,
                patchedJumpAfter,
                patchedWatkBefore,
                patchedWatkAfter,
                patchedMatkBefore,
                patchedMatkAfter,
                SkillOverlayBridgeHasLocalIndependentPotentialDisplayBonuses() ? 1 : 0);
        }
    }

    return resultValue;
}

static int AdjustAbilityRedFinalDisplayValue(int resultValue, int deltaOffset, int* outDelta)
{
    if (outDelta)
        *outDelta = 0;

    if (!SkillOverlayBridgeHasLocalIndependentPotentialDisplayDeltaValue(deltaOffset))
        return resultValue;

    const int delta = SkillOverlayBridgeGetLocalIndependentPotentialDisplayDeltaValue(deltaOffset);
    if (outDelta)
        *outDelta = delta;
    if (delta == 0)
        return resultValue;

    long adjusted = static_cast<long>(resultValue) + static_cast<long>(delta);
    if (adjusted < 0)
        adjusted = 0;
    if (adjusted > 9999)
        adjusted = 9999;
    return static_cast<int>(adjusted);
}

static int __fastcall hkAbilityRedFinalCalc84BE40(
    void *thisPtr,
    void *edxUnused,
    DWORD arg1,
    DWORD arg2,
    DWORD arg3,
    DWORD arg4,
    DWORD arg5,
    DWORD arg6,
    DWORD arg7)
{
    (void)edxUnused;
    const bool wantDiagnosticLog = EnableAbilityRedDiagnosticLogs();
    const DWORD callerRet = wantDiagnosticLog ? (DWORD)(uintptr_t)_ReturnAddress() : 0;
    DWORD ptrMaskBefore = 0;
    DWORD ptrBefore[7] = {};

    int before120 = 0, before150 = 0, before15C = 0;
    bool before120Ok = false, before150Ok = false, before15COk = false;
    if (wantDiagnosticLog)
    {
        const DWORD args[7] = { arg1, arg2, arg3, arg4, arg5, arg6, arg7 };
        for (int i = 0; i < 7; ++i)
            ptrBefore[i] = ReadAbilityRedDisplayPointerValue(args[i], (1u << i), &ptrMaskBefore);
        DecodeAbilityRedTripletAtOffset((uintptr_t)thisPtr, 0x120, &before120, &before120Ok);
        DecodeAbilityRedTripletAtOffset((uintptr_t)thisPtr, 0x150, &before150, &before150Ok);
        DecodeAbilityRedTripletAtOffset((uintptr_t)thisPtr, 0x15C, &before15C, &before15COk);
    }

    const int resultValue = oAbilityRedFinalCalc84BE40Fn
        ? oAbilityRedFinalCalc84BE40Fn(thisPtr, edxUnused, arg1, arg2, arg3, arg4, arg5, arg6, arg7)
        : 0;

    DWORD ptrMaskAfter = 0;
    DWORD ptrAfter[7] = {};

    int after120 = 0, after150 = 0, after15C = 0;
    bool after120Ok = false, after150Ok = false, after15COk = false;
    int displayDelta = 0;
    const int adjustedValue = AdjustAbilityRedFinalDisplayValue(
        resultValue,
        0x40,
        wantDiagnosticLog ? &displayDelta : nullptr);
    const int activeState = wantDiagnosticLog
        ? (SkillOverlayBridgeHasLocalIndependentPotentialDisplayBonuses() ? 1 : 0)
        : 0;
    if (wantDiagnosticLog)
    {
        const DWORD args[7] = { arg1, arg2, arg3, arg4, arg5, arg6, arg7 };
        for (int i = 0; i < 7; ++i)
            ptrAfter[i] = ReadAbilityRedDisplayPointerValue(args[i], (1u << i), &ptrMaskAfter);
        DecodeAbilityRedTripletAtOffset((uintptr_t)thisPtr, 0x120, &after120, &after120Ok);
        DecodeAbilityRedTripletAtOffset((uintptr_t)thisPtr, 0x150, &after150, &after150Ok);
        DecodeAbilityRedTripletAtOffset((uintptr_t)thisPtr, 0x15C, &after15C, &after15COk);
    }
    if (wantDiagnosticLog &&
        ShouldLogAbilityRedFinalCalculator(
            &g_AbilityRedFinal84BE40LastCaller,
            &g_AbilityRedFinal84BE40LastThis,
            &g_AbilityRedFinal84BE40LastTick,
            &g_AbilityRedFinal84BE40LastActive,
            callerRet,
            (uintptr_t)thisPtr,
            activeState))
    {
        WriteLogFmt(
            "[AbilityRedFinal] 84BE40 caller=0x%08X this=0x%08X result=%d adjusted=%d delta=%d active=%d args=[0x%08X,0x%08X,0x%08X,0x%08X,0x%08X,0x%08X,0x%08X]",
            callerRet,
            (DWORD)(uintptr_t)thisPtr,
            resultValue,
            adjustedValue,
            displayDelta,
            activeState,
            arg1, arg2, arg3, arg4, arg5, arg6, arg7);
        WriteLogFmt(
            "[AbilityRedFinal] 84BE40 ptrB(mask=0x%02X)=[0x%08X,0x%08X,0x%08X,0x%08X,0x%08X,0x%08X,0x%08X] ptrA(mask=0x%02X)=[0x%08X,0x%08X,0x%08X,0x%08X,0x%08X,0x%08X,0x%08X]",
            ptrMaskBefore,
            ptrBefore[0], ptrBefore[1], ptrBefore[2], ptrBefore[3], ptrBefore[4], ptrBefore[5], ptrBefore[6],
            ptrMaskAfter,
            ptrAfter[0], ptrAfter[1], ptrAfter[2], ptrAfter[3], ptrAfter[4], ptrAfter[5], ptrAfter[6]);
        WriteLogFmt(
            "[AbilityRedFinal] 84BE40 slots 120=%d/%d->%d/%d 150=%d/%d->%d/%d 15C=%d/%d->%d/%d",
            before120, before120Ok ? 1 : 0, after120, after120Ok ? 1 : 0,
            before150, before150Ok ? 1 : 0, after150, after150Ok ? 1 : 0,
            before15C, before15COk ? 1 : 0, after15C, after15COk ? 1 : 0);
    }

    return adjustedValue;
}

static int __fastcall hkAbilityRedFinalCalc84C470(
    void *thisPtr,
    void *edxUnused,
    DWORD arg1,
    DWORD arg2,
    DWORD arg3,
    DWORD arg4,
    DWORD arg5,
    DWORD arg6)
{
    (void)edxUnused;
    const bool wantDiagnosticLog = EnableAbilityRedDiagnosticLogs();
    const DWORD callerRet = wantDiagnosticLog ? (DWORD)(uintptr_t)_ReturnAddress() : 0;
    DWORD ptrMaskBefore = 0;
    DWORD ptrBefore[6] = {};

    int before198 = 0, before1C8 = 0, before1D4 = 0, before210 = 0;
    bool before198Ok = false, before1C8Ok = false, before1D4Ok = false, before210Ok = false;
    if (wantDiagnosticLog)
    {
        const DWORD args[6] = { arg1, arg2, arg3, arg4, arg5, arg6 };
        for (int i = 0; i < 6; ++i)
            ptrBefore[i] = ReadAbilityRedDisplayPointerValue(args[i], (1u << i), &ptrMaskBefore);
        DecodeAbilityRedTripletAtOffset((uintptr_t)thisPtr, 0x198, &before198, &before198Ok);
        DecodeAbilityRedTripletAtOffset((uintptr_t)thisPtr, 0x1C8, &before1C8, &before1C8Ok);
        DecodeAbilityRedTripletAtOffset((uintptr_t)thisPtr, 0x1D4, &before1D4, &before1D4Ok);
        DecodeAbilityRedTripletAtOffset((uintptr_t)thisPtr, 0x210, &before210, &before210Ok);
    }

    const int resultValue = oAbilityRedFinalCalc84C470Fn
        ? oAbilityRedFinalCalc84C470Fn(thisPtr, edxUnused, arg1, arg2, arg3, arg4, arg5, arg6)
        : 0;

    DWORD ptrMaskAfter = 0;
    DWORD ptrAfter[6] = {};

    int after198 = 0, after1C8 = 0, after1D4 = 0, after210 = 0;
    bool after198Ok = false, after1C8Ok = false, after1D4Ok = false, after210Ok = false;
    int displayDelta = 0;
    const int adjustedValue = AdjustAbilityRedFinalDisplayValue(
        resultValue,
        0x44,
        wantDiagnosticLog ? &displayDelta : nullptr);
    const int activeState = wantDiagnosticLog
        ? (SkillOverlayBridgeHasLocalIndependentPotentialDisplayBonuses() ? 1 : 0)
        : 0;
    if (wantDiagnosticLog)
    {
        const DWORD args[6] = { arg1, arg2, arg3, arg4, arg5, arg6 };
        for (int i = 0; i < 6; ++i)
            ptrAfter[i] = ReadAbilityRedDisplayPointerValue(args[i], (1u << i), &ptrMaskAfter);
        DecodeAbilityRedTripletAtOffset((uintptr_t)thisPtr, 0x198, &after198, &after198Ok);
        DecodeAbilityRedTripletAtOffset((uintptr_t)thisPtr, 0x1C8, &after1C8, &after1C8Ok);
        DecodeAbilityRedTripletAtOffset((uintptr_t)thisPtr, 0x1D4, &after1D4, &after1D4Ok);
        DecodeAbilityRedTripletAtOffset((uintptr_t)thisPtr, 0x210, &after210, &after210Ok);
    }
    if (wantDiagnosticLog &&
        ShouldLogAbilityRedFinalCalculator(
            &g_AbilityRedFinal84C470LastCaller,
            &g_AbilityRedFinal84C470LastThis,
            &g_AbilityRedFinal84C470LastTick,
            &g_AbilityRedFinal84C470LastActive,
            callerRet,
            (uintptr_t)thisPtr,
            activeState))
    {
        WriteLogFmt(
            "[AbilityRedFinal] 84C470 caller=0x%08X this=0x%08X result=%d adjusted=%d delta=%d active=%d args=[0x%08X,0x%08X,0x%08X,0x%08X,0x%08X,0x%08X]",
            callerRet,
            (DWORD)(uintptr_t)thisPtr,
            resultValue,
            adjustedValue,
            displayDelta,
            activeState,
            arg1, arg2, arg3, arg4, arg5, arg6);
        WriteLogFmt(
            "[AbilityRedFinal] 84C470 ptrB(mask=0x%02X)=[0x%08X,0x%08X,0x%08X,0x%08X,0x%08X,0x%08X] ptrA(mask=0x%02X)=[0x%08X,0x%08X,0x%08X,0x%08X,0x%08X,0x%08X]",
            ptrMaskBefore,
            ptrBefore[0], ptrBefore[1], ptrBefore[2], ptrBefore[3], ptrBefore[4], ptrBefore[5],
            ptrMaskAfter,
            ptrAfter[0], ptrAfter[1], ptrAfter[2], ptrAfter[3], ptrAfter[4], ptrAfter[5]);
        WriteLogFmt(
            "[AbilityRedFinal] 84C470 slots 198=%d/%d->%d/%d 1C8=%d/%d->%d/%d 1D4=%d/%d->%d/%d 210=%d/%d->%d/%d",
            before198, before198Ok ? 1 : 0, after198, after198Ok ? 1 : 0,
            before1C8, before1C8Ok ? 1 : 0, after1C8, after1C8Ok ? 1 : 0,
            before1D4, before1D4Ok ? 1 : 0, after1D4, after1D4Ok ? 1 : 0,
            before210, before210Ok ? 1 : 0, after210, after210Ok ? 1 : 0);
    }

    return adjustedValue;
}

static int __fastcall hkAbilityRedFinalCalc84CA90(
    void *thisPtr,
    void *edxUnused,
    DWORD arg1,
    DWORD arg2,
    DWORD arg3,
    DWORD arg4,
    DWORD arg5)
{
    (void)edxUnused;
    const bool wantDiagnosticLog = EnableAbilityRedDiagnosticLogs();
    if (!wantDiagnosticLog)
    {
        return oAbilityRedFinalCalc84CA90Fn
            ? oAbilityRedFinalCalc84CA90Fn(thisPtr, edxUnused, arg1, arg2, arg3, arg4, arg5)
            : 0;
    }

    const DWORD callerRet = (DWORD)(uintptr_t)_ReturnAddress();
    const DWORD args[5] = { arg1, arg2, arg3, arg4, arg5 };
    DWORD ptrMaskBefore = 0;
    DWORD ptrBefore[5] = {};
    for (int i = 0; i < 5; ++i)
        ptrBefore[i] = ReadAbilityRedDisplayPointerValue(args[i], (1u << i), &ptrMaskBefore);

    int before198 = 0, before1D4 = 0, before204 = 0;
    bool before198Ok = false, before1D4Ok = false, before204Ok = false;
    DecodeAbilityRedTripletAtOffset((uintptr_t)thisPtr, 0x198, &before198, &before198Ok);
    DecodeAbilityRedTripletAtOffset((uintptr_t)thisPtr, 0x1D4, &before1D4, &before1D4Ok);
    DecodeAbilityRedTripletAtOffset((uintptr_t)thisPtr, 0x204, &before204, &before204Ok);

    const int resultValue = oAbilityRedFinalCalc84CA90Fn
        ? oAbilityRedFinalCalc84CA90Fn(thisPtr, edxUnused, arg1, arg2, arg3, arg4, arg5)
        : 0;

    DWORD ptrMaskAfter = 0;
    DWORD ptrAfter[5] = {};
    for (int i = 0; i < 5; ++i)
        ptrAfter[i] = ReadAbilityRedDisplayPointerValue(args[i], (1u << i), &ptrMaskAfter);

    int after198 = 0, after1D4 = 0, after204 = 0;
    bool after198Ok = false, after1D4Ok = false, after204Ok = false;
    DecodeAbilityRedTripletAtOffset((uintptr_t)thisPtr, 0x198, &after198, &after198Ok);
    DecodeAbilityRedTripletAtOffset((uintptr_t)thisPtr, 0x1D4, &after1D4, &after1D4Ok);
    DecodeAbilityRedTripletAtOffset((uintptr_t)thisPtr, 0x204, &after204, &after204Ok);

    const int activeState = SkillOverlayBridgeHasLocalIndependentPotentialDisplayBonuses() ? 1 : 0;
    if (ShouldLogAbilityRedFinalCalculator(
            &g_AbilityRedFinal84CA90LastCaller,
            &g_AbilityRedFinal84CA90LastThis,
            &g_AbilityRedFinal84CA90LastTick,
            &g_AbilityRedFinal84CA90LastActive,
            callerRet,
            (uintptr_t)thisPtr,
            activeState))
    {
        WriteLogFmt(
            "[AbilityRedFinal] 84CA90 caller=0x%08X this=0x%08X result=%d active=%d args=[0x%08X,0x%08X,0x%08X,0x%08X,0x%08X]",
            callerRet,
            (DWORD)(uintptr_t)thisPtr,
            resultValue,
            activeState,
            arg1, arg2, arg3, arg4, arg5);
        WriteLogFmt(
            "[AbilityRedFinal] 84CA90 ptrB(mask=0x%02X)=[0x%08X,0x%08X,0x%08X,0x%08X,0x%08X] ptrA(mask=0x%02X)=[0x%08X,0x%08X,0x%08X,0x%08X,0x%08X]",
            ptrMaskBefore,
            ptrBefore[0], ptrBefore[1], ptrBefore[2], ptrBefore[3], ptrBefore[4],
            ptrMaskAfter,
            ptrAfter[0], ptrAfter[1], ptrAfter[2], ptrAfter[3], ptrAfter[4]);
        WriteLogFmt(
            "[AbilityRedFinal] 84CA90 slots 198=%d/%d->%d/%d 1D4=%d/%d->%d/%d 204=%d/%d->%d/%d",
            before198, before198Ok ? 1 : 0, after198, after198Ok ? 1 : 0,
            before1D4, before1D4Ok ? 1 : 0, after1D4, after1D4Ok ? 1 : 0,
            before204, before204Ok ? 1 : 0, after204, after204Ok ? 1 : 0);
    }

    return resultValue;
}

static int __fastcall hkAbilityRedFinalCalc84CBD0(
    void *thisPtr,
    void *edxUnused,
    DWORD arg1,
    DWORD arg2,
    DWORD arg3,
    DWORD arg4,
    DWORD arg5)
{
    (void)edxUnused;
    const bool wantDiagnosticLog = EnableAbilityRedDiagnosticLogs();
    if (!wantDiagnosticLog)
    {
        return oAbilityRedFinalCalc84CBD0Fn
            ? oAbilityRedFinalCalc84CBD0Fn(thisPtr, edxUnused, arg1, arg2, arg3, arg4, arg5)
            : 0;
    }

    const DWORD callerRet = (DWORD)(uintptr_t)_ReturnAddress();
    const DWORD args[5] = { arg1, arg2, arg3, arg4, arg5 };
    DWORD ptrMaskBefore = 0;
    DWORD ptrBefore[5] = {};
    for (int i = 0; i < 5; ++i)
        ptrBefore[i] = ReadAbilityRedDisplayPointerValue(args[i], (1u << i), &ptrMaskBefore);

    int before198 = 0, before210 = 0, before240 = 0;
    bool before198Ok = false, before210Ok = false, before240Ok = false;
    DecodeAbilityRedTripletAtOffset((uintptr_t)thisPtr, 0x198, &before198, &before198Ok);
    DecodeAbilityRedTripletAtOffset((uintptr_t)thisPtr, 0x210, &before210, &before210Ok);
    DecodeAbilityRedTripletAtOffset((uintptr_t)thisPtr, 0x240, &before240, &before240Ok);

    const int resultValue = oAbilityRedFinalCalc84CBD0Fn
        ? oAbilityRedFinalCalc84CBD0Fn(thisPtr, edxUnused, arg1, arg2, arg3, arg4, arg5)
        : 0;

    DWORD ptrMaskAfter = 0;
    DWORD ptrAfter[5] = {};
    for (int i = 0; i < 5; ++i)
        ptrAfter[i] = ReadAbilityRedDisplayPointerValue(args[i], (1u << i), &ptrMaskAfter);

    int after198 = 0, after210 = 0, after240 = 0;
    bool after198Ok = false, after210Ok = false, after240Ok = false;
    DecodeAbilityRedTripletAtOffset((uintptr_t)thisPtr, 0x198, &after198, &after198Ok);
    DecodeAbilityRedTripletAtOffset((uintptr_t)thisPtr, 0x210, &after210, &after210Ok);
    DecodeAbilityRedTripletAtOffset((uintptr_t)thisPtr, 0x240, &after240, &after240Ok);

    const int activeState = SkillOverlayBridgeHasLocalIndependentPotentialDisplayBonuses() ? 1 : 0;
    if (ShouldLogAbilityRedFinalCalculator(
            &g_AbilityRedFinal84CBD0LastCaller,
            &g_AbilityRedFinal84CBD0LastThis,
            &g_AbilityRedFinal84CBD0LastTick,
            &g_AbilityRedFinal84CBD0LastActive,
            callerRet,
            (uintptr_t)thisPtr,
            activeState))
    {
        WriteLogFmt(
            "[AbilityRedFinal] 84CBD0 caller=0x%08X this=0x%08X result=%d active=%d args=[0x%08X,0x%08X,0x%08X,0x%08X,0x%08X]",
            callerRet,
            (DWORD)(uintptr_t)thisPtr,
            resultValue,
            activeState,
            arg1, arg2, arg3, arg4, arg5);
        WriteLogFmt(
            "[AbilityRedFinal] 84CBD0 ptrB(mask=0x%02X)=[0x%08X,0x%08X,0x%08X,0x%08X,0x%08X] ptrA(mask=0x%02X)=[0x%08X,0x%08X,0x%08X,0x%08X,0x%08X]",
            ptrMaskBefore,
            ptrBefore[0], ptrBefore[1], ptrBefore[2], ptrBefore[3], ptrBefore[4],
            ptrMaskAfter,
            ptrAfter[0], ptrAfter[1], ptrAfter[2], ptrAfter[3], ptrAfter[4]);
        WriteLogFmt(
            "[AbilityRedFinal] 84CBD0 slots 198=%d/%d->%d/%d 210=%d/%d->%d/%d 240=%d/%d->%d/%d",
            before198, before198Ok ? 1 : 0, after198, after198Ok ? 1 : 0,
            before210, before210Ok ? 1 : 0, after210, after210Ok ? 1 : 0,
            before240, before240Ok ? 1 : 0, after240, after240Ok ? 1 : 0);
    }

    return resultValue;
}

static void __cdecl hkObserveAbilityRedDisplayCallsite(
    DWORD esiValue,
    DWORD ecxValue,
    DWORD ebpValue,
    DWORD critPtr,
    DWORD option31Ptr,
    DWORD damagePtr,
    DWORD bossDamagePtr,
    DWORD ignoreDefensePtr,
    DWORD retAddr)
{
    if (!EnableAbilityRedDiagnosticLogs())
        return;
    const DWORD now = GetTickCount();
    if (now - g_AbilityRedDisplayCallsiteLastTick <= 1000)
        return;
    g_AbilityRedDisplayCallsiteLastTick = now;

    DWORD critValue = 0;
    DWORD option31Value = 0;
    DWORD damageValue = 0;
    DWORD bossDamageValue = 0;
    DWORD ignoreDefenseValue = 0;

    if (critPtr && !SafeIsBadReadPtr(reinterpret_cast<void*>(critPtr), 4))
        critValue = *reinterpret_cast<DWORD*>(critPtr);
    if (option31Ptr && !SafeIsBadReadPtr(reinterpret_cast<void*>(option31Ptr), 4))
        option31Value = *reinterpret_cast<DWORD*>(option31Ptr);
    if (damagePtr && !SafeIsBadReadPtr(reinterpret_cast<void*>(damagePtr), 4))
        damageValue = *reinterpret_cast<DWORD*>(damagePtr);
    if (bossDamagePtr && !SafeIsBadReadPtr(reinterpret_cast<void*>(bossDamagePtr), 4))
        bossDamageValue = *reinterpret_cast<DWORD*>(bossDamagePtr);
    if (ignoreDefensePtr && !SafeIsBadReadPtr(reinterpret_cast<void*>(ignoreDefensePtr), 4))
        ignoreDefenseValue = *reinterpret_cast<DWORD*>(ignoreDefensePtr);

    WriteLogFmt(
        "[AbilityRedDisplayCall] AE6C21 esi=0x%08X ecx=0x%08X ebp=0x%08X ptrs=[0x%08X,0x%08X,0x%08X,0x%08X,0x%08X] vals=[%u,%u,%u,%u,%u] ret=0x%08X",
        esiValue,
        ecxValue,
        ebpValue,
        critPtr,
        option31Ptr,
        damagePtr,
        bossDamagePtr,
        ignoreDefensePtr,
        critValue,
        option31Value,
        damageValue,
        bossDamageValue,
        ignoreDefenseValue,
        retAddr);
}

__declspec(naked) static void hkAbilityRedDisplayCallsiteNaked()
{
    __asm {
        pushfd
        pushad
        push dword ptr [esp + 0x24] // retAddr
        push dword ptr [esp + 0x68] // ignoreDefensePtr
        push dword ptr [esp + 0x68] // bossDamagePtr
        push dword ptr [esp + 0x68] // damagePtr
        push dword ptr [esp + 0x68] // option31Ptr
        push dword ptr [esp + 0x68] // critPtr
        push dword ptr [esp + 0x20] // ebp
        push dword ptr [esp + 0x34] // ecx
        push dword ptr [esp + 0x24] // esi
        call hkObserveAbilityRedDisplayCallsite
        add esp, 36
        popad
        popfd
        jmp dword ptr [g_AbilityRedDisplayCallsiteOriginalTarget]
    }
}

static void __cdecl hkObserveAbilityRedLevelReadFrame(DWORD *frame)
{
    if (!EnableAbilityRedDiagnosticLogs())
        return;
    if (!frame)
        return;

    const DWORD ediValue = frame[0];
    const DWORD esiValue = frame[1];
    const DWORD ebpValue = frame[2];
    const DWORD pushfdEsp = frame[3];
    const DWORD ebxValue = frame[4];
    const DWORD edxValue = frame[5];
    const DWORD ecxValue = frame[6];
    const DWORD eaxValue = frame[7];
    const DWORD originalEsp = pushfdEsp + 4;

    const DWORD now = GetTickCount();
    if (now - g_AbilityRedLevelReadLastTick <= 1000)
        return;
    g_AbilityRedLevelReadLastTick = now;

    DWORD stack18 = 0;
    DWORD stack1C = 0;
    DWORD stack20 = 0;
    DWORD stack24 = 0;
    DWORD stack24v0 = 0;
    DWORD stack24v4 = 0;
    DWORD stack24v8 = 0;
    DWORD stack24vC = 0;
    if (originalEsp && !SafeIsBadReadPtr(reinterpret_cast<void*>(originalEsp + 0x18), 16))
    {
        stack18 = *reinterpret_cast<DWORD*>(originalEsp + 0x18);
        stack1C = *reinterpret_cast<DWORD*>(originalEsp + 0x1C);
        stack20 = *reinterpret_cast<DWORD*>(originalEsp + 0x20);
        stack24 = *reinterpret_cast<DWORD*>(originalEsp + 0x24);
    }
    if (stack24 && !SafeIsBadReadPtr(reinterpret_cast<void*>(stack24), 16))
    {
        stack24v0 = *reinterpret_cast<DWORD*>(stack24 + 0x0);
        stack24v4 = *reinterpret_cast<DWORD*>(stack24 + 0x4);
        stack24v8 = *reinterpret_cast<DWORD*>(stack24 + 0x8);
        stack24vC = *reinterpret_cast<DWORD*>(stack24 + 0xC);
    }

    WriteLogFmt(
        "[AbilityRedLevelRead] AE43D5 esp=0x%08X regs=[eax=0x%08X ecx=0x%08X edx=0x%08X ebx=0x%08X ebp=0x%08X esi=0x%08X edi=0x%08X] stack=[+18=0x%08X +1C=0x%08X +20=0x%08X +24=0x%08X] ptr24=[0x%08X,0x%08X,0x%08X,0x%08X]",
        originalEsp,
        eaxValue,
        ecxValue,
        edxValue,
        ebxValue,
        ebpValue,
        esiValue,
        ediValue,
        stack18,
        stack1C,
        stack20,
        stack24,
        stack24v0,
        stack24v4,
        stack24v8,
        stack24vC);

    LogAbilityRedDecodedSnapshot("AE43D5");
}

__declspec(naked) static void hkAbilityRedLevelReadNaked()
{
    __asm {
        pushfd
        pushad
        mov eax, esp
        push eax
        call hkObserveAbilityRedLevelReadFrame
        add esp, 4
        popad
        popfd
        jmp [oAbilityRedLevelReadHook]
    }
}

static void __cdecl hkObserveAbilityRedHashLookupFrame(DWORD *frame)
{
    if (!frame)
        return;

    const DWORD ecxValue = frame[6];
    const DWORD pushfdEsp = frame[3];
    const DWORD originalEsp = pushfdEsp + 4;
    DWORD returnAddr = 0;
    DWORD keyPtr = 0;
    DWORD outValuePtr = 0;
    if (originalEsp && !SafeIsBadReadPtr(reinterpret_cast<void*>(originalEsp), 12))
    {
        returnAddr = *reinterpret_cast<DWORD*>(originalEsp + 0x0);
        keyPtr = *reinterpret_cast<DWORD*>(originalEsp + 0x4);
        outValuePtr = *reinterpret_cast<DWORD*>(originalEsp + 0x8);
    }

    DWORD keyValue = 0;
    DWORD outBefore = 0;
    DWORD bucketBase = 0;
    DWORD bucketCount = 0;
    DWORD entryCount = 0;
    if (keyPtr && !SafeIsBadReadPtr(reinterpret_cast<void*>(keyPtr), sizeof(DWORD)))
        keyValue = *reinterpret_cast<DWORD*>(keyPtr);
    if (outValuePtr && !SafeIsBadReadPtr(reinterpret_cast<void*>(outValuePtr), sizeof(DWORD)))
        outBefore = *reinterpret_cast<DWORD*>(outValuePtr);

    if (!ShouldLogAbilityRedHashLookup(returnAddr, (uintptr_t)ecxValue, keyValue))
        return;

    ReadAbilityRedHashContainerMeta((uintptr_t)ecxValue, &bucketBase, &bucketCount, &entryCount);
    WriteLogFmt(
        "[AbilityRedHashLookup] caller=0x%08X this=0x%08X keyPtr=0x%08X key=%u outPtr=0x%08X outBefore=%u buckets=0x%08X bucketCount=%u entryCount=%u active=%d",
        returnAddr,
        ecxValue,
        keyPtr,
        keyValue,
        outValuePtr,
        outBefore,
        bucketBase,
        bucketCount,
        entryCount,
        SkillOverlayBridgeHasLocalIndependentPotentialDisplayBonuses() ? 1 : 0);
}

__declspec(naked) static void hkAbilityRedHashLookupNaked()
{
    __asm {
        pushfd
        pushad
        mov eax, esp
        push eax
        call hkObserveAbilityRedHashLookupFrame
        add esp, 4
        popad
        popfd
        jmp [oAbilityRedHashLookupHook]
    }
}

static void __cdecl hkObserveAbilityRedHashInsertFrame(DWORD *frame)
{
    if (!frame)
        return;

    const DWORD ecxValue = frame[6];
    const DWORD pushfdEsp = frame[3];
    const DWORD originalEsp = pushfdEsp + 4;
    DWORD returnAddr = 0;
    DWORD keyPtr = 0;
    DWORD valuePtr = 0;
    if (originalEsp && !SafeIsBadReadPtr(reinterpret_cast<void*>(originalEsp), 12))
    {
        returnAddr = *reinterpret_cast<DWORD*>(originalEsp + 0x0);
        keyPtr = *reinterpret_cast<DWORD*>(originalEsp + 0x4);
        valuePtr = *reinterpret_cast<DWORD*>(originalEsp + 0x8);
    }

    DWORD keyValue = 0;
    DWORD valueValue = 0;
    DWORD bucketBase = 0;
    DWORD bucketCount = 0;
    DWORD entryCount = 0;
    if (keyPtr && !SafeIsBadReadPtr(reinterpret_cast<void*>(keyPtr), sizeof(DWORD)))
        keyValue = *reinterpret_cast<DWORD*>(keyPtr);
    if (valuePtr && !SafeIsBadReadPtr(reinterpret_cast<void*>(valuePtr), sizeof(DWORD)))
        valueValue = *reinterpret_cast<DWORD*>(valuePtr);

    if (!ShouldLogAbilityRedHashInsert(returnAddr, (uintptr_t)ecxValue, keyValue, valueValue))
        return;

    ReadAbilityRedHashContainerMeta((uintptr_t)ecxValue, &bucketBase, &bucketCount, &entryCount);
    WriteLogFmt(
        "[AbilityRedHashInsert] caller=0x%08X this=0x%08X keyPtr=0x%08X valuePtr=0x%08X key=%u value=%u buckets=0x%08X bucketCount=%u entryCount=%u active=%d",
        returnAddr,
        ecxValue,
        keyPtr,
        valuePtr,
        keyValue,
        valueValue,
        bucketBase,
        bucketCount,
        entryCount,
        SkillOverlayBridgeHasLocalIndependentPotentialDisplayBonuses() ? 1 : 0);
}

__declspec(naked) static void hkAbilityRedHashInsertNaked()
{
    __asm {
        pushfd
        pushad
        mov eax, esp
        push eax
        call hkObserveAbilityRedHashInsertFrame
        add esp, 4
        popad
        popfd
        jmp [oAbilityRedHashInsertHook]
    }
}

static void __cdecl hkObserveAbilityRedSkillWrite(
    const char *tag,
    DWORD destPtr,
    DWORD sourcePtr,
    DWORD sourceValue,
    DWORD carrierA,
    DWORD carrierB)
{
    if (!EnableAbilityRedDiagnosticLogs())
        return;
    const DWORD now = GetTickCount();
    if (now - g_AbilityRedSkillWriteLastTick <= 1000)
        return;
    g_AbilityRedSkillWriteLastTick = now;

    DWORD destValue = 0;
    DWORD destVtable = 0;
    DWORD dest04 = 0;
    DWORD dest08 = 0;
    DWORD dest0C = 0;
    DWORD sourceBase = sourcePtr >= 0x0C ? (sourcePtr - 0x0C) : 0;
    DWORD sourceBase0 = 0;
    DWORD sourceBase4 = 0;
    DWORD sourceBase8 = 0;
    DWORD sourceBaseC = 0;
    if (destPtr && !SafeIsBadReadPtr(reinterpret_cast<void*>(destPtr), 4))
        destValue = *reinterpret_cast<DWORD*>(destPtr);
    if (destPtr && !SafeIsBadReadPtr(reinterpret_cast<void*>(destPtr), 16))
    {
        destVtable = *reinterpret_cast<DWORD*>(destPtr + 0x0);
        dest04 = *reinterpret_cast<DWORD*>(destPtr + 0x4);
        dest08 = *reinterpret_cast<DWORD*>(destPtr + 0x8);
        dest0C = *reinterpret_cast<DWORD*>(destPtr + 0xC);
    }
    if (sourceBase && !SafeIsBadReadPtr(reinterpret_cast<void*>(sourceBase), 16))
    {
        sourceBase0 = *reinterpret_cast<DWORD*>(sourceBase + 0x0);
        sourceBase4 = *reinterpret_cast<DWORD*>(sourceBase + 0x4);
        sourceBase8 = *reinterpret_cast<DWORD*>(sourceBase + 0x8);
        sourceBaseC = *reinterpret_cast<DWORD*>(sourceBase + 0xC);
    }

    WriteLogFmt(
        "[AbilityRedSkillWrite] %s dest=0x%08X destVal=0x%08X destFields=[0x%08X,0x%08X,0x%08X,0x%08X] src=0x%08X srcVal=0x%08X srcBase=0x%08X srcFields=[0x%08X,0x%08X,0x%08X,0x%08X] carry=[0x%08X,0x%08X] active=%d",
        tag ? tag : "unknown",
        destPtr,
        destValue,
        destVtable,
        dest04,
        dest08,
        dest0C,
        sourcePtr,
        sourceValue,
        sourceBase,
        sourceBase0,
        sourceBase4,
        sourceBase8,
        sourceBaseC,
        carrierA,
        carrierB,
        SkillOverlayBridgeHasLocalIndependentPotentialDisplayBonuses() ? 1 : 0);
}

static void __cdecl hkObserveAbilityRedSkillWrite52FE14Frame(DWORD *frame)
{
    if (!frame)
        return;

    const DWORD ecxValue = frame[6];
    const DWORD eaxValue = frame[7];
    DWORD sourceValue = 0;
    if (ecxValue && !SafeIsBadReadPtr(reinterpret_cast<void*>(ecxValue), 4))
        sourceValue = *reinterpret_cast<DWORD*>(ecxValue);
    hkObserveAbilityRedSkillWrite("52FE14", eaxValue + 0x0C, ecxValue, sourceValue, eaxValue, ecxValue);
}

static void __cdecl hkObserveAbilityRedSkillWrite6226CEFrame(DWORD *frame)
{
    if (!frame)
        return;

    const DWORD esiValue = frame[1];
    const DWORD eaxValue = frame[7];
    DWORD sourceValue = 0;
    if (esiValue && !SafeIsBadReadPtr(reinterpret_cast<void*>(esiValue + 0x0C), 4))
        sourceValue = *reinterpret_cast<DWORD*>(esiValue + 0x0C);
    hkObserveAbilityRedSkillWrite("6226CE", eaxValue + 0x0C, esiValue + 0x0C, sourceValue, eaxValue, esiValue);
}

static void __cdecl hkObserveAbilityRedSkillWrite49CA01Frame(DWORD *frame)
{
    if (!frame)
        return;

    const DWORD edxValue = frame[5];
    const DWORD eaxValue = frame[7];
    DWORD sourceValue = 0;
    if (edxValue && !SafeIsBadReadPtr(reinterpret_cast<void*>(edxValue + 0x0C), 4))
        sourceValue = *reinterpret_cast<DWORD*>(edxValue + 0x0C);
    hkObserveAbilityRedSkillWrite("49CA01", eaxValue, edxValue + 0x0C, sourceValue, eaxValue, edxValue);
}

static void __cdecl hkPatchAbilityRedSkillWrite49CA01Frame(DWORD *frame)
{
    if (!frame)
        return;
    if (!SkillOverlayBridgeHasLocalIndependentPotentialDisplayDeltaValue(0x44))
        return;

    const int mdefDelta = SkillOverlayBridgeGetLocalIndependentPotentialDisplayDeltaValue(0x44);
    if (mdefDelta <= 0)
        return;

    const DWORD pushfdEsp = frame[3];
    const DWORD originalEsp = pushfdEsp + 4;
    DWORD callerRet = 0;
    if (originalEsp && !SafeIsBadReadPtr(reinterpret_cast<void*>(originalEsp + 0x08), sizeof(DWORD)))
        callerRet = *reinterpret_cast<DWORD*>(originalEsp + 0x08);

    const DWORD edxValue = frame[5];
    if (!edxValue || SafeIsBadWritePtr(reinterpret_cast<void*>(edxValue + 0x0C), sizeof(DWORD)))
        return;
    if (SafeIsBadReadPtr(reinterpret_cast<void*>(edxValue + 0x08), sizeof(DWORD)))
        return;

    DWORD *const sourceBase = reinterpret_cast<DWORD*>(edxValue);
    const DWORD nodeKey = sourceBase[2];
    const DWORD currentValue = sourceBase[3];
    if (nodeKey == 0 || nodeKey == currentValue)
        return;

    static DWORD s_lastAbilityRed49PatchLogTick = 0;
    const DWORD now = GetTickCount();
    if (now - s_lastAbilityRed49PatchLogTick > 1000)
    {
        s_lastAbilityRed49PatchLogTick = now;
        WriteLogFmt("[AbilityRedSkillWrite] 49CA01 observe nodeKey caller=0x%08X srcBase=0x%08X key=0x%08X value=0x%08X mdefDelta=%d",
            callerRet,
            edxValue,
            nodeKey,
            currentValue,
            mdefDelta);
    }
}

__declspec(naked) static void hkAbilityRedSkillWrite52FE14Naked()
{
    __asm {
        pushfd
        pushad
        mov eax, esp
        push eax
        call hkObserveAbilityRedSkillWrite52FE14Frame
        add esp, 4
        popad
        popfd
        jmp [oAbilityRedSkillWrite52FE14Hook]
    }
}

__declspec(naked) static void hkAbilityRedSkillWrite6226CENaked()
{
    __asm {
        pushfd
        pushad
        mov eax, esp
        push eax
        call hkObserveAbilityRedSkillWrite6226CEFrame
        add esp, 4
        popad
        popfd
        jmp [oAbilityRedSkillWrite6226CEHook]
    }
}

__declspec(naked) static void hkAbilityRedSkillWrite49CA01Naked()
{
    __asm {
        pushfd
        pushad
        mov eax, esp
        push eax
        call hkObserveAbilityRedSkillWrite49CA01Frame
        add esp, 4
        mov eax, esp
        push eax
        call hkPatchAbilityRedSkillWrite49CA01Frame
        add esp, 4
        popad
        popfd
        jmp [oAbilityRedSkillWrite49CA01Hook]
    }
}

static BYTE* __stdcall hkPotentialTextFormat(int* src, BYTE* statsPtr)
{
    BYTE* displayPtr = statsPtr;
    if (statsPtr && SkillOverlayBridgeHasLocalIndependentPotentialDisplayBonuses())
    {
        uintptr_t prepared = SkillOverlayBridgePrepareLocalIndependentPotentialDisplayBuffer((uintptr_t)statsPtr);
        if (prepared && !SafeIsBadReadPtr(reinterpret_cast<void*>(prepared), 0xD8))
        {
            displayPtr = reinterpret_cast<BYTE*>(prepared);

            static DWORD s_lastPotentialDisplayHookLogTick = 0;
            const DWORD now = GetTickCount();
            if (now - s_lastPotentialDisplayHookLogTick > 1000)
            {
                s_lastPotentialDisplayHookLogTick = now;
                WriteLogFmt("[PotentialTextHook] A4CA60 src=0x%08X stats=0x%08X prepared=0x%08X",
                    (DWORD)(uintptr_t)src,
                    (DWORD)(uintptr_t)statsPtr,
                    (DWORD)prepared);
            }
        }
    }

    return oPotentialTextFormat ? oPotentialTextFormat(src, displayPtr) : nullptr;
}

__declspec(naked) static void hkLocalIndependentPotentialSkillLevelDisplayNaked()
{
    __asm {
        pushad
        push dword ptr [esp + 0x4C]
        call hkApplyLocalIndependentPotentialSkillLevelDisplay
        add esp, 4
        popad
        jmp [oLocalIndependentPotentialSkillLevelDisplay]
    }
}

__declspec(naked) static void hkLocalIndependentPotentialDamageDisplayNaked()
{
    __asm {
        pushad
        mov eax, [esp + 0x5C]
        mov ecx, [esp + 0x58]
        mov edx, [esp + 0x54]
        mov ebx, [esp + 0x50]
        mov esi, [esp + 0x4C]
        push eax
        push ecx
        push edx
        push ebx
        push esi
        call hkApplyLocalIndependentPotentialDamageDisplay
        add esp, 20
        popad
        jmp [oLocalIndependentPotentialDamageDisplay]
    }
}

static int __stdcall hkLocalIndependentPotentialSkillLevelDisplayFunction(int a1, int a2, DWORD *a3)
{
    const int result = oLocalIndependentPotentialSkillLevelDisplayFn
        ? oLocalIndependentPotentialSkillLevelDisplayFn(a1, a2, a3)
        : 0;
    if (EnableIndependentBuffOverlayDiagnosticLogs())
    {
        static DWORD s_lastSkillLevelCallLogTick = 0;
        const DWORD now = GetTickCount();
        if (now - s_lastSkillLevelCallLogTick > 1000)
        {
            s_lastSkillLevelCallLogTick = now;
            const int delta88 = SkillOverlayBridgeGetLocalIndependentPotentialDisplayDeltaValue(0x88);
            WriteLogFmt("[IndependentBuffLocalDisplayCall] AE0A70 a1=0x%08X a2=%d out=0x%08X delta88=%d active=%d",
                (DWORD)a1,
                a2,
                (DWORD)(uintptr_t)a3,
                delta88,
                SkillOverlayBridgeHasLocalIndependentPotentialDisplayBonuses() ? 1 : 0);
        }
    }
    LogAbilityRedDecodedSnapshot("AE0A70");
    hkApplyLocalIndependentPotentialSkillLevelDisplay(reinterpret_cast<uintptr_t>(a3));
    return result;
}

static LONG __cdecl hkLocalIndependentPotentialPercentQuadDisplayFunction(
    int a1,
    int a2,
    DWORD *a3,
    DWORD *a4,
    DWORD *a5,
    DWORD *a6)
{
    const LONG result = oLocalIndependentPotentialPercentQuadDisplayFn
        ? oLocalIndependentPotentialPercentQuadDisplayFn(a1, a2, a3, a4, a5, a6)
        : 0;
    if (EnableIndependentBuffOverlayDiagnosticLogs())
    {
        static DWORD s_lastPercentQuadCallLogTick = 0;
        const DWORD now = GetTickCount();
        if (now - s_lastPercentQuadCallLogTick > 1000)
        {
            s_lastPercentQuadCallLogTick = now;
            const int d48 = SkillOverlayBridgeGetLocalIndependentPotentialDisplayDeltaValue(0x48);
            const int d4C = SkillOverlayBridgeGetLocalIndependentPotentialDisplayDeltaValue(0x4C);
            const int d50 = SkillOverlayBridgeGetLocalIndependentPotentialDisplayDeltaValue(0x50);
            const int d54 = SkillOverlayBridgeGetLocalIndependentPotentialDisplayDeltaValue(0x54);
            WriteLogFmt("[IndependentBuffLocalDisplayCall] 8538C0 a1=0x%08X a2=%d outs=[0x%08X,0x%08X,0x%08X,0x%08X] deltas=[%d,%d,%d,%d] active=%d",
                (DWORD)a1,
                a2,
                (DWORD)(uintptr_t)a3,
                (DWORD)(uintptr_t)a4,
                (DWORD)(uintptr_t)a5,
                (DWORD)(uintptr_t)a6,
                d48,
                d4C,
                d50,
                d54,
                SkillOverlayBridgeHasLocalIndependentPotentialDisplayBonuses() ? 1 : 0);
        }
    }
    LogAbilityRedDecodedSnapshot("8538C0");
    hkApplyLocalIndependentPotentialPercentQuadDisplay(
        reinterpret_cast<uintptr_t>(a3),
        reinterpret_cast<uintptr_t>(a4),
        reinterpret_cast<uintptr_t>(a5),
        reinterpret_cast<uintptr_t>(a6));
    return result;
}

static LONG __fastcall hkLocalIndependentPotentialPercentFullDisplayFunction(
    DWORD *thisPtr,
    void * /*edxUnused*/,
    int pExceptionObject,
    int a3,
    DWORD *a4)
{
    const LONG result = oLocalIndependentPotentialPercentFullDisplayFn
        ? oLocalIndependentPotentialPercentFullDisplayFn(thisPtr, pExceptionObject, a3, a4)
        : 0;
    if (EnableIndependentBuffOverlayDiagnosticLogs())
    {
        static DWORD s_lastPercentFullCallLogTick = 0;
        const DWORD now = GetTickCount();
        if (now - s_lastPercentFullCallLogTick > 1000)
        {
            s_lastPercentFullCallLogTick = now;
            const int d48 = SkillOverlayBridgeGetLocalIndependentPotentialDisplayDeltaValue(0x48);
            const int d4C = SkillOverlayBridgeGetLocalIndependentPotentialDisplayDeltaValue(0x4C);
            const int d50 = SkillOverlayBridgeGetLocalIndependentPotentialDisplayDeltaValue(0x50);
            const int d54 = SkillOverlayBridgeGetLocalIndependentPotentialDisplayDeltaValue(0x54);
            const int d58 = SkillOverlayBridgeGetLocalIndependentPotentialDisplayDeltaValue(0x58);
            const int d5C = SkillOverlayBridgeGetLocalIndependentPotentialDisplayDeltaValue(0x5C);
            WriteLogFmt("[IndependentBuffLocalDisplayCall] 853E10 this=0x%08X exc=0x%08X a3=%d out=0x%08X deltas=[%d,%d,%d,%d,%d,%d] active=%d",
                (DWORD)(uintptr_t)thisPtr,
                (DWORD)pExceptionObject,
                a3,
                (DWORD)(uintptr_t)a4,
                d48,
                d4C,
                d50,
                d54,
                d58,
                d5C,
                SkillOverlayBridgeHasLocalIndependentPotentialDisplayBonuses() ? 1 : 0);
        }
    }
    LogAbilityRedDecodedSnapshot("853E10");
    hkApplyLocalIndependentPotentialPercentFullDisplay(reinterpret_cast<uintptr_t>(a4));
    return result;
}

static LONG __fastcall hkLocalIndependentPotentialFlatBasicDisplayFunction(
    DWORD *thisPtr,
    void * /*edxUnused*/,
    int pExceptionObject,
    int a3)
{
    const LONG result = oLocalIndependentPotentialFlatBasicDisplayFn
        ? oLocalIndependentPotentialFlatBasicDisplayFn(thisPtr, pExceptionObject, a3)
        : 0;
    if (EnableIndependentBuffOverlayDiagnosticLogs())
    {
        static DWORD s_lastFlatBasicCallLogTick = 0;
        const DWORD now = GetTickCount();
        if (now - s_lastFlatBasicCallLogTick > 1000)
        {
            s_lastFlatBasicCallLogTick = now;
            const int d08 = SkillOverlayBridgeGetLocalIndependentPotentialDisplayDeltaValue(0x08);
            const int d0C = SkillOverlayBridgeGetLocalIndependentPotentialDisplayDeltaValue(0x0C);
            const int d10 = SkillOverlayBridgeGetLocalIndependentPotentialDisplayDeltaValue(0x10);
            const int d14 = SkillOverlayBridgeGetLocalIndependentPotentialDisplayDeltaValue(0x14);
            const int d20 = SkillOverlayBridgeGetLocalIndependentPotentialDisplayDeltaValue(0x20);
            const int d24 = SkillOverlayBridgeGetLocalIndependentPotentialDisplayDeltaValue(0x24);
            WriteLogFmt("[IndependentBuffLocalDisplayCall] 853B00 this=0x%08X exc=0x%08X a3=%d deltas=[%d,%d,%d,%d,%d,%d] active=%d",
                (DWORD)(uintptr_t)thisPtr,
                (DWORD)pExceptionObject,
                a3,
                d08,
                d0C,
                d10,
                d14,
                d20,
                d24,
                SkillOverlayBridgeHasLocalIndependentPotentialDisplayBonuses() ? 1 : 0);
        }
    }
    LogAbilityRedDecodedSnapshot("853B00");
    hkApplyLocalIndependentPotentialFlatBasicDisplay(reinterpret_cast<uintptr_t>(thisPtr));
    return result;
}

static LONG __fastcall hkLocalIndependentPotentialFlatExtendedDisplayFunction(
    DWORD *thisPtr,
    void * /*edxUnused*/,
    int pExceptionObject,
    int a3)
{
    const LONG result = oLocalIndependentPotentialFlatExtendedDisplayFn
        ? oLocalIndependentPotentialFlatExtendedDisplayFn(thisPtr, pExceptionObject, a3)
        : 0;
    if (EnableIndependentBuffOverlayDiagnosticLogs())
    {
        static DWORD s_lastFlatExtendedCallLogTick = 0;
        const DWORD now = GetTickCount();
        if (now - s_lastFlatExtendedCallLogTick > 1000)
        {
            s_lastFlatExtendedCallLogTick = now;
            const int d28 = SkillOverlayBridgeGetLocalIndependentPotentialDisplayDeltaValue(0x28);
            const int d2C = SkillOverlayBridgeGetLocalIndependentPotentialDisplayDeltaValue(0x2C);
            const int d30 = SkillOverlayBridgeGetLocalIndependentPotentialDisplayDeltaValue(0x30);
            const int d34 = SkillOverlayBridgeGetLocalIndependentPotentialDisplayDeltaValue(0x34);
            const int d38 = SkillOverlayBridgeGetLocalIndependentPotentialDisplayDeltaValue(0x38);
            const int d3C = SkillOverlayBridgeGetLocalIndependentPotentialDisplayDeltaValue(0x3C);
            const int d40 = SkillOverlayBridgeGetLocalIndependentPotentialDisplayDeltaValue(0x40);
            const int d44 = SkillOverlayBridgeGetLocalIndependentPotentialDisplayDeltaValue(0x44);
            const int dC8 = SkillOverlayBridgeGetLocalIndependentPotentialDisplayDeltaValue(0xC8);
            const int dCC = SkillOverlayBridgeGetLocalIndependentPotentialDisplayDeltaValue(0xCC);
            const int dD0 = SkillOverlayBridgeGetLocalIndependentPotentialDisplayDeltaValue(0xD0);
            const int dD4 = SkillOverlayBridgeGetLocalIndependentPotentialDisplayDeltaValue(0xD4);
            WriteLogFmt("[IndependentBuffLocalDisplayCall] 856830 this=0x%08X exc=0x%08X a3=%d deltas=[%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d] active=%d",
                (DWORD)(uintptr_t)thisPtr,
                (DWORD)pExceptionObject,
                a3,
                d28, d2C, d30, d34, d38, d3C, d40, d44, dC8, dCC, dD0, dD4,
                SkillOverlayBridgeHasLocalIndependentPotentialDisplayBonuses() ? 1 : 0);
        }
    }
    LogAbilityRedDecodedSnapshot("856830");
    hkApplyLocalIndependentPotentialFlatExtendedDisplay(reinterpret_cast<uintptr_t>(thisPtr));
    return result;
}

