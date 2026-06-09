static void ApplyVirtualStatusBarBuffShift(uintptr_t statusBar, StatusBarObservedBuffSlot* slots, const char* reason)
{
    if (!statusBar || !slots)
        return;

    std::vector<IndependentBuffOverlayEntry> entries;
    SkillOverlayBridgeGetIndependentBuffOverlayEntriesLite(entries);
    const int virtualCount = (int)entries.size();
    if (virtualCount <= 0 || virtualCount >= 6)
        return;

    struct VisibleSlotRef
    {
        int slotIndex = 0;
        int x = 0;
        int y = 0;
        int renderX = 0;
        int renderY = 0;
        int w = 0;
        int h = 0;
        uintptr_t child = 0;
    };

    std::vector<VisibleSlotRef> visibleSlots;
    visibleSlots.reserve(6);
    for (int i = 0; i < 6; ++i)
    {
        if (!slots[i].child || slots[i].w <= 0 || slots[i].h <= 0)
            continue;

        VisibleSlotRef slot = {};
        slot.slotIndex = i;
        slot.x = slots[i].x;
        slot.y = slots[i].y;
        slot.renderX = slots[i].renderX;
        slot.renderY = slots[i].renderY;
        slot.w = slots[i].w;
        slot.h = slots[i].h;
        slot.child = slots[i].child;
        visibleSlots.push_back(slot);
    }

    if (visibleSlots.empty() || (int)visibleSlots.size() + virtualCount > 6)
        return;

    std::sort(visibleSlots.begin(), visibleSlots.end(),
        [](const VisibleSlotRef& left, const VisibleSlotRef& right)
        {
            return left.slotIndex < right.slotIndex;
        });

    int stepX = 0;
    int stepRenderX = 0;
    for (size_t i = 1; i < visibleSlots.size(); ++i)
    {
        const int deltaIndex = visibleSlots[i].slotIndex - visibleSlots[i - 1].slotIndex;
        const int deltaX = visibleSlots[i].x - visibleSlots[i - 1].x;
        const int deltaRenderX = visibleSlots[i].renderX - visibleSlots[i - 1].renderX;
        if (deltaIndex > 0 && deltaX > 0)
        {
            const int candidateX = deltaX / deltaIndex;
            if (candidateX > 0 && (stepX == 0 || candidateX < stepX))
                stepX = candidateX;
        }
        if (deltaIndex > 0 && deltaRenderX > 0)
        {
            const int candidateRenderX = deltaRenderX / deltaIndex;
            if (candidateRenderX > 0 && (stepRenderX == 0 || candidateRenderX < stepRenderX))
                stepRenderX = candidateRenderX;
        }
    }

    if (stepX <= 0)
        stepX = visibleSlots[0].w + 2;
    if (stepX <= 0)
        return;

    if (stepRenderX <= 0)
        stepRenderX = stepX;

    const int baseX = visibleSlots[0].x - visibleSlots[0].slotIndex * stepX;
    const int baseRenderX = visibleSlots[0].renderX - visibleSlots[0].slotIndex * stepRenderX;
    const int baseY = visibleSlots[0].y;
    const int baseRenderY = visibleSlots[0].renderY;

    for (int order = (int)visibleSlots.size() - 1; order >= 0; --order)
    {
        const VisibleSlotRef& slot = visibleSlots[(size_t)order];
        const uintptr_t child = slot.child;
        if (!child || SafeIsBadReadPtr((void*)child, 0x4C))
            continue;

        const int targetIndex = order + virtualCount;
        const int targetX = baseX + targetIndex * stepX;
        const int targetRenderX = baseRenderX + targetIndex * stepRenderX;
        if (slot.x == targetX && slot.renderX == targetRenderX)
            continue;

        CWnd_SetComPos(child, targetX, baseY);
        CWnd_SetRenderPos(child, targetRenderX, baseRenderY);
        WriteLogFmt("[StatusBarBuffSlotShift] reason=%s child=0x%08X fromSlot=%d toSlot=%d x=%d->%d renderX=%d->%d virtualCount=%d",
            reason ? reason : "unknown",
            (DWORD)child,
            slot.slotIndex,
            targetIndex,
            slot.x,
            targetX,
            slot.renderX,
            targetRenderX,
            virtualCount);

        slots[slot.slotIndex].x = targetX;
        slots[slot.slotIndex].renderX = targetRenderX;
        slots[slot.slotIndex].y = baseY;
        slots[slot.slotIndex].renderY = baseRenderY;
    }
}

static void ObserveStatusBarBuffSlots(uintptr_t statusBar, const char* reason)
{
    if (!statusBar || SafeIsBadReadPtr((void*)statusBar, 0xB30 + 4))
        return;

    SkillOverlayBridgeSetObservedStatusBarPtr(statusBar);

    StatusBarObservedBuffSlot current[9] = {};
    for (int i = 0; i < 9; ++i)
    {
        const uintptr_t slotAddr = statusBar + (i < 6 ? (0xAE8 + i * 8) : (0xB18 + (i - 6) * 8));
        if (SafeIsBadReadPtr((void*)slotAddr, 4))
            continue;

        const uintptr_t wrapper = *(uintptr_t*)slotAddr;
        current[i].wrapper = wrapper;
        if (!wrapper || SafeIsBadReadPtr((void*)wrapper, 8))
            continue;

        const uintptr_t child = wrapper + 4;
        current[i].child = child;
        if (!child || SafeIsBadReadPtr((void*)child, 0x4C))
            continue;

        current[i].x = CWnd_GetX(child);
        current[i].y = CWnd_GetY(child);
        current[i].w = CWnd_GetWidth(child);
        current[i].h = CWnd_GetHeight(child);
        current[i].renderX = CWnd_GetRenderX(child);
        current[i].renderY = CWnd_GetRenderY(child);
    }

    int activeVisibleCount = 0;
    int firstVisibleX = -1;
    for (int i = 0; i < 9; ++i)
    {
        const bool slotLooksVisible =
            current[i].child &&
            current[i].w >= 16 &&
            current[i].h >= 16 &&
            (current[i].x != 0 || current[i].y != 0 || current[i].renderX != 0 || current[i].renderY != 0);
        if (slotLooksVisible)
        {
            ++activeVisibleCount;
            if (firstVisibleX < 0 || current[i].x < firstVisibleX)
                firstVisibleX = current[i].x;
        }
    }
    if (activeVisibleCount > 0)
        SkillOverlayBridgeSetObservedNativeVisibleBuffVisualCount(activeVisibleCount);
    else
        SkillOverlayBridgeSetObservedNativeVisibleBuffVisualCount(-1);
    if (activeVisibleCount > 0 && firstVisibleX >= 0)
        SkillOverlayBridgeSetObservedNativeVisibleBuffAnchorX(firstVisibleX);
    else
        SkillOverlayBridgeSetObservedNativeVisibleBuffAnchorX(-1);

    static DWORD s_lastStatusBarBuffSummaryLogTick = 0;
    const DWORD nowTick = GetTickCount();
    if (nowTick - s_lastStatusBarBuffSummaryLogTick > 1000)
    {
        s_lastStatusBarBuffSummaryLogTick = nowTick;
        unsigned int wrapperMask = 0;
        unsigned int childMask = 0;
        unsigned int visibleMask = 0;
        for (int i = 0; i < 6; ++i)
        {
            if (current[i].wrapper)
                wrapperMask |= (1u << i);
            if (current[i].child)
                childMask |= (1u << i);
            if (current[i].child &&
                current[i].w >= 16 &&
                current[i].h >= 16 &&
                (current[i].x != 0 || current[i].y != 0 || current[i].renderX != 0 || current[i].renderY != 0))
                visibleMask |= (1u << i);
        }
        WriteLogFmt("[StatusBarBuffSlotSummary] reason=%s statusBar=0x%08X topWr=0x%02X topChild=0x%02X topVisible=0x%02X activeVisible=%d firstVisibleX=%d B30=0x%08X",
            reason ? reason : "unknown",
            (DWORD)statusBar,
            wrapperMask,
            childMask,
            visibleMask,
            activeVisibleCount,
            firstVisibleX,
            *(DWORD*)(statusBar + 0xB30));
    }

    bool changed = false;
    for (int i = 0; i < 9; ++i)
    {
        const StatusBarObservedBuffSlot& prev = g_StatusBarObservedBuffSlots[i];
        const StatusBarObservedBuffSlot& now = current[i];
        if (prev.wrapper != now.wrapper ||
            prev.child != now.child ||
            prev.x != now.x || prev.y != now.y ||
            prev.w != now.w || prev.h != now.h ||
            prev.renderX != now.renderX || prev.renderY != now.renderY)
        {
            changed = true;
            WriteLogFmt("[StatusBarBuffSlot] reason=%s slot=%d wrapper=0x%08X child=0x%08X rect=(%d,%d,%d,%d) render=(%d,%d)",
                reason ? reason : "unknown",
                i,
                (DWORD)now.wrapper,
                (DWORD)now.child,
                now.x,
                now.y,
                now.w,
                now.h,
                now.renderX,
                now.renderY);
        }
    }

    if (changed)
    {
        for (int i = 0; i < 9; ++i)
            g_StatusBarObservedBuffSlots[i] = current[i];
    }

    // Keep native buff icons in their original slots. Overlay icons render in a
    // separate block on the left, so shifting native children here only scrambles
    // the observed order during status-bar refreshes.
}

// 状态栏 BUFF 槽模块：负责 BUFF 栏刷新、清理和 transient slot 观测。
static void LogStatusBarHookSeen(const char* reason, uintptr_t thisPtr)
{
    static DWORD s_lastStatusBarHookSeenLogTick = 0;
    const DWORD nowTick = GetTickCount();
    if (nowTick - s_lastStatusBarHookSeenLogTick < 1000)
        return;
    s_lastStatusBarHookSeenLogTick = nowTick;
    WriteLogFmt("[StatusBarHookSeen] reason=%s this=0x%08X readable=%d",
        reason ? reason : "unknown",
        (DWORD)thisPtr,
        (!thisPtr || SafeIsBadReadPtr((void*)thisPtr, 0xB30 + 4)) ? 0 : 1);
}

static void __cdecl hkStatusBarRefreshSlotsPrimaryHandler(uintptr_t thisPtr)
{
    LogStatusBarHookSeen("9F4F00", thisPtr);
    SkillOverlayBridgeSetObservedStatusBarPtr(thisPtr);
    if (oStatusBarRefreshSlotsPrimary)
        oStatusBarRefreshSlotsPrimary(thisPtr);
    ObserveStatusBarBuffSlots(thisPtr, "9F4F00");
}

static void __cdecl hkStatusBarRefreshSlotsSecondaryHandler(uintptr_t thisPtr)
{
    LogStatusBarHookSeen("9F4C30", thisPtr);
    SkillOverlayBridgeSetObservedStatusBarPtr(thisPtr);
    if (oStatusBarRefreshSlotsSecondary)
        oStatusBarRefreshSlotsSecondary(thisPtr);
    ObserveStatusBarBuffSlots(thisPtr, "9F4C30");
}

static void __cdecl hkStatusBarRefreshInternalHandler(uintptr_t thisPtr)
{
    LogStatusBarHookSeen("9F5FE0", thisPtr);
    SkillOverlayBridgeSetObservedStatusBarPtr(thisPtr);
    if (oStatusBarRefreshInternal)
        oStatusBarRefreshInternal(thisPtr);
    ObserveStatusBarBuffSlots(thisPtr, "9F5FE0");
}

static void __cdecl hkStatusBarCleanupTransientHandler(uintptr_t thisPtr)
{
    LogStatusBarHookSeen("9FCAE0", thisPtr);
    SkillOverlayBridgeSetObservedStatusBarPtr(thisPtr);
    if (oStatusBarCleanupTransient)
        oStatusBarCleanupTransient(thisPtr);
    ObserveStatusBarBuffSlots(thisPtr, "9FCAE0");
}

static void LogStatusBarTransientState(uintptr_t thisPtr, const char* reason, int extra)
{
    if (!thisPtr || SafeIsBadReadPtr((void*)thisPtr, 0xB70))
        return;

    const DWORD slotAD8 = *(DWORD*)(thisPtr + 0xAD8);
    const DWORD ptrB2C0 = *(DWORD*)(thisPtr + 0xB2C);
    const DWORD ptrB2C4 = *(DWORD*)(thisPtr + 0xB30);
    const DWORD ptrB20 = *(DWORD*)(thisPtr + 0xB20);
    const DWORD ptrB24 = *(DWORD*)(thisPtr + 0xB24);
    const DWORD ptrB28 = *(DWORD*)(thisPtr + 0xB28);
    const DWORD ptrB5C = *(DWORD*)(thisPtr + 0xB5C);

    int childX = 0, childY = 0, childW = 0, childH = 0, childRenderX = 0, childRenderY = 0;
    if (ptrB2C4 && !SafeIsBadReadPtr((void*)ptrB2C4, 0x4C))
    {
        childX = CWnd_GetX(ptrB2C4);
        childY = CWnd_GetY(ptrB2C4);
        childW = CWnd_GetWidth(ptrB2C4);
        childH = CWnd_GetHeight(ptrB2C4);
        childRenderX = CWnd_GetRenderX(ptrB2C4);
        childRenderY = CWnd_GetRenderY(ptrB2C4);
    }

    WriteLogFmt(
        "[StatusBarTransient] %s this=0x%08X extra=%d AD8=%u B20=0x%08X B24=0x%08X B28=0x%08X B2C=0x%08X B30=0x%08X B5C=0x%08X childRect=(%d,%d,%d,%d) render=(%d,%d)",
        reason ? reason : "unknown",
        (DWORD)thisPtr,
        extra,
        slotAD8,
        ptrB20,
        ptrB24,
        ptrB28,
        ptrB2C0,
        ptrB2C4,
        ptrB5C,
        childX,
        childY,
        childW,
        childH,
        childRenderX,
        childRenderY);
}

static int __cdecl hkStatusBarTransientRefreshHandler(uintptr_t thisPtr, int a2)
{
    LogStatusBarHookSeen("9FC110", thisPtr);
    SkillOverlayBridgeSetObservedStatusBarPtr(thisPtr);
    const int result = oStatusBarTransientRefresh ? oStatusBarTransientRefresh(thisPtr, a2) : 0;
    LogStatusBarTransientState(thisPtr, "9FC110", result);
    ObserveStatusBarBuffSlots(thisPtr, "9FC110");
    return result;
}

static void __cdecl hkStatusBarTransientDispatchHandler(uintptr_t thisPtr, int a2)
{
    LogStatusBarHookSeen("9FCC10", thisPtr);
    SkillOverlayBridgeSetObservedStatusBarPtr(thisPtr);
    if (oStatusBarTransientDispatch)
        oStatusBarTransientDispatch(thisPtr, a2);
    LogStatusBarTransientState(thisPtr, "9FCC10", a2);
    ObserveStatusBarBuffSlots(thisPtr, "9FCC10");
}

static LONG* __cdecl hkStatusBarTransientToggleHandler(int a1)
{
    LONG* result = oStatusBarTransientToggle ? oStatusBarTransientToggle(a1) : nullptr;
    DWORD statusBar = 0;
    if (!SafeIsBadReadPtr((void*)ADDR_StatusBar, 4))
        statusBar = *(DWORD*)ADDR_StatusBar;
    if (statusBar)
    {
        LogStatusBarHookSeen("9FCBD0", statusBar);
        SkillOverlayBridgeSetObservedStatusBarPtr(statusBar);
        LogStatusBarTransientState(statusBar, "9FCBD0", a1);
        ObserveStatusBarBuffSlots(statusBar, "9FCBD0");
    }
    return result;
}

__declspec(naked) static void hkStatusBarRefreshSlotsPrimaryNaked()
{
    __asm {
        push ecx
        call hkStatusBarRefreshSlotsPrimaryHandler
        add esp, 4
        ret
    }
}

__declspec(naked) static void hkStatusBarRefreshSlotsSecondaryNaked()
{
    __asm {
        push ecx
        call hkStatusBarRefreshSlotsSecondaryHandler
        add esp, 4
        ret
    }
}

__declspec(naked) static void hkStatusBarRefreshInternalNaked()
{
    __asm {
        push ecx
        call hkStatusBarRefreshInternalHandler
        add esp, 4
        ret
    }
}

__declspec(naked) static void hkStatusBarCleanupTransientNaked()
{
    __asm {
        push ecx
        call hkStatusBarCleanupTransientHandler
        add esp, 4
        ret
    }
}

__declspec(naked) static void hkStatusBarTransientRefreshNaked()
{
    __asm {
        mov eax, [esp + 4]
        push eax
        push ecx
        call hkStatusBarTransientRefreshHandler
        add esp, 8
        ret 4
    }
}

__declspec(naked) static void hkStatusBarTransientDispatchNaked()
{
    __asm {
        mov eax, [esp + 4]
        push eax
        push ecx
        call hkStatusBarTransientDispatchHandler
        add esp, 8
        ret 4
    }
}

__declspec(naked) static void hkStatusBarTransientToggleNaked()
{
    __asm {
        mov eax, [esp + 4]
        push eax
        call hkStatusBarTransientToggleHandler
        add esp, 4
        ret 4
    }
}

