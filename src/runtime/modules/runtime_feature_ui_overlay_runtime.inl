static void UpdateSuperCWnd()
{
    if (!g_SkillWndThis || !g_SuperExpanded)
    {
        g_PanelDrawX = -9999;
        g_PanelDrawY = -9999;
#if defined(SSW_ENABLE_SECOND_CHILD_CARRIER_PROBE_RUNTIME)
        PollSecondChildCarrierProbeTick(0x0000E001, false);
#endif
        return;
    }

    static int s_lastPanelX = 0x7FFFFFFF;
    static int s_lastPanelY = 0x7FFFFFFF;
    static int s_missPosLogCount = 0;

    int panelX = -9999, panelY = -9999;
    const char *src = "none";
    if (!ComputeSuperPanelPos(&panelX, &panelY, &src))
    {
        if (s_missPosLogCount < 16)
        {
            WriteLog("[UpdatePos] FAIL: ComputeSuperPanelPos");
            s_missPosLogCount++;
        }
        return;
    }

    g_PanelDrawX = panelX;
    g_PanelDrawY = panelY;

    if (ENABLE_IMGUI_OVERLAY_PANEL)
    {
        if (g_IsD3D8Mode)
            SuperD3D8OverlaySetAnchor(g_PanelDrawX, g_PanelDrawY);
        else
            SuperImGuiOverlaySetAnchor(g_PanelDrawX, g_PanelDrawY);
        if ((g_PanelDrawX != s_lastPanelX || g_PanelDrawY != s_lastPanelY) && g_UpdatePosLogCount < 200)
        {
            int swComX = 0, swComY = 0, swVtX = 0, swVtY = 0;
            bool hasCom = GetSkillWndComPos(g_SkillWndThis, &swComX, &swComY);
            bool hasVt = GetSkillWndAnchorPos(g_SkillWndThis, &swVtX, &swVtY);
            WriteLogFmt("[UpdatePos] src=%s panel=(%d,%d) swCom=%s(%d,%d) swVt=%s(%d,%d) overlay=%s",
                        src, g_PanelDrawX, g_PanelDrawY,
                        hasCom ? "Y" : "N", swComX, swComY,
                        hasVt ? "Y" : "N", swVtX, swVtY,
                        g_IsD3D8Mode ? "imgui_d3d8" : "imgui_d3d9");
            g_UpdatePosLogCount++;
        }
        s_lastPanelX = g_PanelDrawX;
        s_lastPanelY = g_PanelDrawY;
        return;
    }

    if (g_NativeWndCreated && g_SuperCWnd)
    {
        int vtX = 0, vtY = 0;
        if (GetSkillWndAnchorPos(g_SkillWndThis, &vtX, &vtY))
        {
            g_PanelDrawX = vtX - PANEL_W + SUPER_CHILD_VT_DELTA_X;
            g_PanelDrawY = vtY + SUPER_CHILD_VT_DELTA_Y;
            src = "skill_vt_child";
        }
        else
        {
            g_PanelDrawX += SUPER_CHILD_OFFSET_X;
            g_PanelDrawY += SUPER_CHILD_OFFSET_Y;
        }
    }

    if (g_SuperCWnd && !SafeIsBadReadPtr((void *)g_SuperCWnd, 0x30))
    {
        if (g_SuperUsesSkillWndSecondSlot)
        {
            LogOfficialSecondChildState(g_SuperCWnd, "UpdatePos:BeforeMove");
        }
        SetSuperWndVisible(g_SuperCWnd, 1);
        MoveNativeChildWnd(g_SuperCWnd, g_PanelDrawX, g_PanelDrawY, "UpdatePosMove");
        MarkSuperWndDirty(g_SuperCWnd, "UpdatePosDirty");
        if (g_SuperUsesSkillWndSecondSlot)
        {
            LogOfficialSecondChildState(g_SuperCWnd, "UpdatePos:AfterMove");
        }
    }

    if ((g_PanelDrawX != s_lastPanelX || g_PanelDrawY != s_lastPanelY) && g_UpdatePosLogCount < 200)
    {
        int swComX = 0, swComY = 0, swVtX = 0, swVtY = 0;
        bool hasCom = GetSkillWndComPos(g_SkillWndThis, &swComX, &swComY);
        bool hasVt = GetSkillWndAnchorPos(g_SkillWndThis, &swVtX, &swVtY);
        WriteLogFmt("[UpdatePos] src=%s panel=(%d,%d) raw=(%d,%d) childOff=(%d,%d) vtChildDelta=(%d,%d) swCom=%s(%d,%d) swVt=%s(%d,%d)",
                    src, g_PanelDrawX, g_PanelDrawY, panelX, panelY, SUPER_CHILD_OFFSET_X, SUPER_CHILD_OFFSET_Y,
                    SUPER_CHILD_VT_DELTA_X, SUPER_CHILD_VT_DELTA_Y,
                    hasCom ? "Y" : "N", swComX, swComY,
                    hasVt ? "Y" : "N", swVtX, swVtY);
        g_UpdatePosLogCount++;
    }

    s_lastPanelX = g_PanelDrawX;
    s_lastPanelY = g_PanelDrawY;

#if defined(SSW_ENABLE_SECOND_CHILD_CARRIER_PROBE_RUNTIME)
    PollSecondChildCarrierProbeTick(0x0000E002, false);
#endif
}

// ============================================================================
// WndProc Hook — F9切换 + 面板内点击（原生按钮不需要坐标检测了）
// ============================================================================
static LRESULT CALLBACK GameWndProc(HWND h, UINT m, WPARAM w, LPARAM l)
{
#if defined(SSW_ENABLE_SECOND_CHILD_CARRIER_PROBE_RUNTIME)
    SSW_SecondChildCarrierProbe_ObserveWndProc(m, w, l);
#endif

    if (ENABLE_IMGUI_OVERLAY_PANEL)
    {
        switch (m)
        {
        case WM_CAPTURECHANGED:
        case WM_KILLFOCUS:
            if (g_IsD3D8Mode)
                SuperD3D8OverlayCancelMouseCapture();
            else
                SuperImGuiOverlayCancelMouseCapture();
            break;
        case WM_ACTIVATEAPP:
            if (!w)
            {
                if (g_IsD3D8Mode)
                    SuperD3D8OverlayCancelMouseCapture();
                else
                    SuperImGuiOverlayCancelMouseCapture();
            }
            break;
        default:
            break;
        }
    }

    // F9：切换面板
    if (m == WM_KEYDOWN && w == VK_F9)
    {
        if (g_SkillWndThis)
        {
            ToggleSuperWnd();
        }
        return 0;
    }

#if defined(SSW_ENABLE_SECOND_CHILD_CARRIER_PROBE_RUNTIME)
    if (m == WM_KEYDOWN && w == VK_F10)
    {
        RunSecondChildCarrierProbeHotkey();
        return 0;
    }

    if (m == WM_KEYDOWN && w == VK_F11)
    {
        PollSecondChildCarrierProbeTick(0x0000F011, true);
        return 0;
    }

    if (m == WM_KEYDOWN && w == VK_F12)
    {
        ReleaseSecondChildCarrierProbeHotkey();
        return 0;
    }
#endif

    if (m == WM_SETCURSOR && g_MouseSuppressFallbackActive)
    {
        SetCursor(nullptr);
        return TRUE;
    }

    if (!ENABLE_IMGUI_OVERLAY_PANEL && HandleSuperBtnD3DWndProc(h, m, w, l))
        return 0;

    // 兜底：即使原生消息没分发到sub_9ECFD0，也保证按钮可点
    // 注意：不能吞掉 WM_LBUTTONUP，否则原生按钮状态机会丢失 mouseUp，光标可能卡在 pressed
    if (g_Ready && g_NativeBtnCreated && !oSkillWndMsg)
    {
        if (m == WM_LBUTTONUP)
        {
            int mx = (short)LOWORD(l), my = (short)HIWORD(l);
            if (TryToggleByMousePoint(mx, my, "wndproc"))
            {
                static DWORD s_lastPassThroughLogTick = 0;
                DWORD now = GetTickCount();
                if (now - s_lastPassThroughLogTick > 200)
                {
                    s_lastPassThroughLogTick = now;
                    WriteLog("[WndProc] fallback toggle hit, pass WM_LBUTTONUP through");
                }
                // 不 return：继续传给原WndProc，让原生按钮完成 mouseUp 收尾
            }
        }
    }

    if (ENABLE_IMGUI_OVERLAY_PANEL)
    {
        if (m == WM_MOUSEACTIVATE && (g_IsD3D8Mode ? SuperD3D8OverlayShouldSuppressGameMouse() : SuperImGuiOverlayShouldSuppressGameMouse()))
        {
            // 点击非前台游戏里的 overlay 时，先激活游戏窗口，但吃掉这一下，避免穿透到原生窗口。
            return MA_ACTIVATEANDEAT;
        }

        bool overlayHandled = g_IsD3D8Mode ? SuperD3D8OverlayHandleWndProc(h, m, w, l) : SuperImGuiOverlayHandleWndProc(h, m, w, l);
        const bool overlayToggleRequested = g_IsD3D8Mode ? SuperD3D8OverlayConsumeToggleRequested() : SuperImGuiOverlayConsumeToggleRequested();
        if (overlayToggleRequested)
        {
            g_LastNativeMsgToggleTick = GetTickCount();
            ToggleSuperWnd("overlay_btn");
            overlayHandled = true;
        }
        bool suppressGameMouse = g_IsD3D8Mode ? SuperD3D8OverlayShouldSuppressGameMouse() : SuperImGuiOverlayShouldSuppressGameMouse();

        if (!overlayHandled)
        {
            switch (m)
            {
            case WM_LBUTTONDOWN:
            case WM_LBUTTONUP:
            case WM_LBUTTONDBLCLK:
            case WM_RBUTTONDOWN:
            case WM_RBUTTONUP:
            case WM_RBUTTONDBLCLK:
            case WM_MBUTTONDOWN:
            case WM_MBUTTONUP:
            case WM_MBUTTONDBLCLK:
            case WM_XBUTTONDOWN:
            case WM_XBUTTONUP:
            case WM_XBUTTONDBLCLK:
            case WM_MOUSEWHEEL:
            case WM_MOUSEHWHEEL:
                if (g_IsD3D8Mode)
                    SuperD3D8OverlayCancelMouseCapture();
                else
                    SuperImGuiOverlayCancelMouseCapture();
                if (Win32InputSpoofIsInstalled())
                {
                    Win32InputSpoofSetSuppressMouse(false);
                }
                break;
            default:
                break;
            }
        }

        if (overlayHandled && suppressGameMouse)
        {
            auto forwardOffscreenMouseToGame = [&]()
            {
                if (g_OriginalWndProc)
                {
                    CallWindowProc(g_OriginalWndProc, h, m, w, Win32InputSpoofMakeOffscreenMouseLParam());
                }
            };

            switch (m)
            {
            default:
                break;
            }

            if (m == WM_SETCURSOR)
            {
                SetCursor(nullptr);
                return TRUE;
            }

            if (m == WM_MOUSEMOVE)
            {
                forwardOffscreenMouseToGame();
                return 0;
            }

            switch (m)
            {
            case WM_LBUTTONDOWN:
            case WM_LBUTTONUP:
            case WM_LBUTTONDBLCLK:
            case WM_RBUTTONDOWN:
            case WM_RBUTTONUP:
            case WM_RBUTTONDBLCLK:
            case WM_MBUTTONDOWN:
            case WM_MBUTTONUP:
            case WM_MBUTTONDBLCLK:
            case WM_XBUTTONDOWN:
            case WM_XBUTTONUP:
            case WM_XBUTTONDBLCLK:
            case WM_MOUSEWHEEL:
            case WM_MOUSEHWHEEL:
                return 0;
            default:
                break;
            }
        }

        if (overlayHandled)
        {
            return 0;
        }
    }

    // 面板内点击（当前native child 仍未接上原生输入协议，先由WndProc兜底接管，避免点击穿透到底层窗口）
    if (!ENABLE_IMGUI_OVERLAY_PANEL && g_Ready && g_SkillWndThis && g_SuperExpanded)
    {
        if (m == WM_LBUTTONDOWN || m == WM_LBUTTONUP)
        {
            if (g_PanelDrawX <= -9000 || g_PanelDrawY <= -9000)
            {
                UpdateSuperCWnd();
            }
            int mx = (short)LOWORD(l), my = (short)HIWORD(l);
            int cx = g_PanelDrawX;
            int cy = g_PanelDrawY;
            if (cx > -9000 && cy > -9000)
            {
                int relX = mx - cx;
                int relY = my - cy;
                if (relX >= 0 && relX < PANEL_W && relY >= 0 && relY < PANEL_H)
                {
                    static int s_panelHitLogCount = 0;
                    if (s_panelHitLogCount < 40)
                    {
                        WriteLogFmt("[PanelHit:wndproc] msg=%u mx=%d my=%d rel=(%d,%d) panel=(%d,%d,%d,%d)",
                                    m, mx, my, relX, relY, cx, cy, PANEL_W, PANEL_H);
                        s_panelHitLogCount++;
                    }
                    if (m == WM_LBUTTONDOWN)
                    {
                        if (relY < 28)
                        {
                            static int s_panelTabLogCount = 0;
                            if (s_panelTabLogCount < 40)
                            {
                                WriteLogFmt("[PanelAction] tab=%d rel=(%d,%d)", relX / 56, relX, relY);
                                s_panelTabLogCount++;
                            }
                            g_SkillMgr.SetTab(relX / 56);
                        }
                        else
                        {
                            int rowIdx = (relY - 28) / 34;
                            SkillTab *tab = g_SkillMgr.GetCurrentTab();
                            if (tab && rowIdx >= 0 && rowIdx < tab->count)
                            {
                                static int s_panelSkillLogCount = 0;
                                if (s_panelSkillLogCount < 60)
                                {
                                    WriteLogFmt("[PanelAction] skillRow=%d rel=(%d,%d) tabCount=%d",
                                                rowIdx, relX, relY, tab->count);
                                    s_panelSkillLogCount++;
                                }
                                tab->skills[rowIdx].Use();
                            }
                        }
                    }
                    return 0;
                }
            }
        }
    }

    return CallWindowProc(g_OriginalWndProc, h, m, w, l);
}

// ============================================================================
// D3D9 Present Hook — 仅用于设备获取、纹理加载和 overlay 面板更新
// ============================================================================
typedef HRESULT(__stdcall *tPresent)(IDirect3DDevice9 *, const RECT *, const RECT *, HWND, const RGNDATA *);
typedef HRESULT(__stdcall *tReset)(IDirect3DDevice9 *, D3DPRESENT_PARAMETERS *);
typedef HRESULT(__stdcall *tResetEx)(IDirect3DDevice9Ex *, D3DPRESENT_PARAMETERS *, D3DDISPLAYMODEEX *);
static HRESULT __stdcall hkReset(IDirect3DDevice9 *pDevice, D3DPRESENT_PARAMETERS *pPresentationParameters);
static HRESULT __stdcall hkResetEx(IDirect3DDevice9Ex *pDevice, D3DPRESENT_PARAMETERS *pPresentationParameters, D3DDISPLAYMODEEX *pFullscreenDisplayMode);
static HRESULT __stdcall hkPresent(IDirect3DDevice9 *pDevice, const RECT *pSourceRect, const RECT *pDestRect, HWND hDestWindowOverride, const RGNDATA *pDirtyRegion);
static tPresent oPresent = nullptr;
static tReset oReset = nullptr;
static tResetEx oResetEx = nullptr;
typedef HRESULT(__stdcall *tEndScene)(IDirect3DDevice9 *);
static tEndScene oEndScene = nullptr;
static tPresent g_LiveDevicePresent = nullptr;
static bool g_LivePresentVtableHooked = false;
static tReset g_LiveDeviceReset = nullptr;
static tResetEx g_LiveDeviceResetEx = nullptr;
static void **g_LiveDeviceVTable9 = nullptr;
static void **g_LiveDeviceVTable9Ex = nullptr;

// Forward declarations for CreateDevice interception strategy
static HRESULT __stdcall hkEndScene(IDirect3DDevice9 *pDevice);
static bool PatchLiveDeviceVTableEntry(void **vtable, int index, void *hookFunc, void **outOriginal);
static void EnsureLiveDeviceResetHooks(IDirect3DDevice9 *pDevice);

// --- CreateDevice interception strategy ---
// When inline hooks on Present/EndScene don't fire (e.g., D3D9on12, wrapper layers),
// we hook Direct3DCreate9 -> vtable-hook IDirect3D9::CreateDevice -> vtable-hook the real device's Present.
typedef IDirect3D9 *(WINAPI *tDirect3DCreate9Fn)(UINT SDKVersion);
typedef HRESULT(__stdcall *tCreateDevice)(IDirect3D9 *, UINT, D3DDEVTYPE, HWND, DWORD, D3DPRESENT_PARAMETERS *, IDirect3DDevice9 **);
static tDirect3DCreate9Fn oDirectCreate9 = nullptr;
static tCreateDevice oCreateDevice = nullptr;
static bool g_DeviceCapturedViaCreateHook = false;

static HRESULT __stdcall hkCreateDevice(IDirect3D9 *pD3D, UINT Adapter, D3DDEVTYPE DeviceType,
                                        HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS *pPP, IDirect3DDevice9 **ppDevice)
{
    HRESULT hr = oCreateDevice(pD3D, Adapter, DeviceType, hFocusWindow, BehaviorFlags, pPP, ppDevice);
    if (SUCCEEDED(hr) && ppDevice && *ppDevice && !g_DeviceCapturedViaCreateHook)
    {
        IDirect3DDevice9 *pDevice = *ppDevice;
        g_pDevice = pDevice;
        g_DeviceCapturedViaCreateHook = true;
        WriteLogFmt("[D3D9-CreateHook] Captured real device=0x%08X type=%d", (DWORD)(uintptr_t)pDevice, (int)DeviceType);

        // Vtable-hook Present on the real device
        void **vtable = *(void ***)pDevice;
        if (vtable)
        {
            void *origPresent = nullptr;
            if (PatchLiveDeviceVTableEntry(vtable, 17, (void *)hkPresent, &origPresent))
            {
                if (origPresent && origPresent != (void *)hkPresent)
                    g_LiveDevicePresent = (tPresent)origPresent;
                g_LivePresentVtableHooked = true;
                WriteLogFmt("[D3D9-CreateHook] Present vtable hooked orig=0x%08X", (DWORD)(uintptr_t)origPresent);
            }

            void *origEndScene = nullptr;
            if (PatchLiveDeviceVTableEntry(vtable, 42, (void *)hkEndScene, &origEndScene))
            {
                if (origEndScene && origEndScene != (void *)hkEndScene && !oEndScene)
                    oEndScene = (tEndScene)origEndScene;
                WriteLogFmt("[D3D9-CreateHook] EndScene vtable hooked orig=0x%08X", (DWORD)(uintptr_t)origEndScene);
            }
        }
        EnsureLiveDeviceResetHooks(pDevice);
    }
    return hr;
}

static IDirect3D9 *WINAPI hkDirect3DCreate9(UINT SDKVersion)
{
    IDirect3D9 *pD3D = oDirectCreate9(SDKVersion);
    if (pD3D)
    {
        WriteLogFmt("[D3D9-CreateHook] Direct3DCreate9 called SDK=%d result=0x%08X", SDKVersion, (DWORD)(uintptr_t)pD3D);
        // Vtable-hook CreateDevice (index 16) on the real IDirect3D9
        void **vtable = *(void ***)pD3D;
        if (vtable)
        {
            void *origCreateDevice = nullptr;
            if (PatchLiveDeviceVTableEntry(vtable, 16, (void *)hkCreateDevice, &origCreateDevice))
            {
                if (origCreateDevice && origCreateDevice != (void *)hkCreateDevice)
                    oCreateDevice = (tCreateDevice)origCreateDevice;
                WriteLogFmt("[D3D9-CreateHook] CreateDevice vtable hooked orig=0x%08X", (DWORD)(uintptr_t)origCreateDevice);
            }
        }
    }
    return pD3D;
}

static bool PatchLiveDeviceVTableEntry(void **vtable, int index, void *hookFunc, void **outOriginal)
{
    if (!vtable || index < 0)
        return false;

    void **slot = &vtable[index];
    if (SafeIsBadReadPtr(slot, sizeof(void *)))
        return false;

    if (*slot == hookFunc)
        return true;

    DWORD oldProtect = 0;
    if (!VirtualProtect(slot, sizeof(void *), PAGE_EXECUTE_READWRITE, &oldProtect))
        return false;

    if (outOriginal)
        *outOriginal = *slot;
    *slot = hookFunc;
    VirtualProtect(slot, sizeof(void *), oldProtect, &oldProtect);
    FlushInstructionCache(GetCurrentProcess(), slot, sizeof(void *));
    return true;
}

static void EnsureLiveDeviceResetHooks(IDirect3DDevice9 *pDevice)
{
    if (!pDevice)
        return;

    void **vtable9 = *(void ***)pDevice;
    if (vtable9 && vtable9 != g_LiveDeviceVTable9)
    {
        void *originalReset = nullptr;
        if (PatchLiveDeviceVTableEntry(vtable9, 16, (void *)hkReset, &originalReset))
        {
            if (originalReset && originalReset != (void *)hkReset)
                g_LiveDeviceReset = (tReset)originalReset;
            g_LiveDeviceVTable9 = vtable9;
            WriteLogFmt("[D3D9] live vtbl Reset patched vtbl=0x%08X orig=0x%08X",
                        (DWORD)(uintptr_t)vtable9, (DWORD)(uintptr_t)g_LiveDeviceReset);
        }
    }

    IDirect3DDevice9Ex *pDeviceEx = nullptr;
    if (SUCCEEDED(pDevice->QueryInterface(__uuidof(IDirect3DDevice9Ex), (void **)&pDeviceEx)) && pDeviceEx)
    {
        void **vtableEx = *(void ***)pDeviceEx;
        if (vtableEx && vtableEx != g_LiveDeviceVTable9Ex)
        {
            void *originalResetEx = nullptr;
            if (PatchLiveDeviceVTableEntry(vtableEx, 132, (void *)hkResetEx, &originalResetEx))
            {
                if (originalResetEx && originalResetEx != (void *)hkResetEx)
                    g_LiveDeviceResetEx = (tResetEx)originalResetEx;
                g_LiveDeviceVTable9Ex = vtableEx;
                WriteLogFmt("[D3D9Ex] live vtbl ResetEx patched vtbl=0x%08X orig=0x%08X",
                            (DWORD)(uintptr_t)vtableEx, (DWORD)(uintptr_t)g_LiveDeviceResetEx);
            }
        }
        pDeviceEx->Release();
    }
}

static volatile bool g_InReset = false;

static HRESULT __stdcall hkReset(IDirect3DDevice9 *pDevice, D3DPRESENT_PARAMETERS *pPresentationParameters)
{
    // Guard against infinite recursion: inline hook on function body + vtable hook
    // both redirect to hkReset. If re-entered, go straight to trampoline.
    if (g_InReset)
    {
        tReset resetFn = g_LiveDeviceReset ? g_LiveDeviceReset : oReset;
        return resetFn ? resetFn(pDevice, pPresentationParameters) : D3DERR_INVALIDCALL;
    }
    g_InReset = true;

    PrepareForD3DDeviceReset("reset");

    tReset resetFn = g_LiveDeviceReset ? g_LiveDeviceReset : oReset;
    HRESULT hr = resetFn ? resetFn(pDevice, pPresentationParameters) : D3DERR_INVALIDCALL;

    if (SUCCEEDED(hr))
    {
        g_pDevice = pDevice;
        if (ENABLE_IMGUI_OVERLAY_PANEL)
            SuperImGuiOverlayOnDeviceReset(pDevice);
        WriteLog("[D3D9] Reset OK: hard rebuild pending on next Present");
    }
    else
    {
        WriteLogFmt("[D3D9] Reset FAIL hr=0x%08X", (DWORD)hr);
    }

    g_InReset = false;
    return hr;
}

static volatile bool g_InResetEx = false;

static HRESULT __stdcall hkResetEx(IDirect3DDevice9Ex *pDevice, D3DPRESENT_PARAMETERS *pPresentationParameters, D3DDISPLAYMODEEX *pFullscreenDisplayMode)
{
    if (g_InResetEx)
    {
        tResetEx resetFn = g_LiveDeviceResetEx ? g_LiveDeviceResetEx : oResetEx;
        return resetFn ? resetFn(pDevice, pPresentationParameters, pFullscreenDisplayMode) : D3DERR_INVALIDCALL;
    }
    g_InResetEx = true;

    PrepareForD3DDeviceReset("reset_ex");

    tResetEx resetFn = g_LiveDeviceResetEx ? g_LiveDeviceResetEx : oResetEx;
    HRESULT hr = resetFn ? resetFn(pDevice, pPresentationParameters, pFullscreenDisplayMode) : D3DERR_INVALIDCALL;

    if (SUCCEEDED(hr))
    {
        g_pDevice = (IDirect3DDevice9 *)pDevice;
        if (ENABLE_IMGUI_OVERLAY_PANEL)
            SuperImGuiOverlayOnDeviceReset((IDirect3DDevice9 *)pDevice);
        WriteLog("[D3D9Ex] ResetEx OK: hard rebuild pending on next Present");
    }
    else
    {
        WriteLogFmt("[D3D9Ex] ResetEx FAIL hr=0x%08X", (DWORD)hr);
    }

    g_InResetEx = false;
    return hr;
}

// EndScene hook — fallback device capture when Present inline hook misses
// (e.g., game uses D3D9Ex or wrapper d3d9.dll with different Present address)
static HRESULT __stdcall hkEndScene(IDirect3DDevice9 *pDevice)
{
    if (pDevice && !g_pDevice)
    {
        g_pDevice = pDevice;
        WriteLogFmt("[D3D9] EndScene captured device=0x%08X", (DWORD)(uintptr_t)pDevice);

        // Vtable-hook Present on the live device so hkPresent fires from now on
        if (!g_LivePresentVtableHooked)
        {
            void **vtable = *(void ***)pDevice;
            if (vtable)
            {
                void *origPresent = nullptr;
                if (PatchLiveDeviceVTableEntry(vtable, 17, (void *)hkPresent, &origPresent))
                {
                    if (origPresent && origPresent != (void *)hkPresent)
                        g_LiveDevicePresent = (tPresent)origPresent;
                    g_LivePresentVtableHooked = true;
                    WriteLogFmt("[D3D9] live vtbl Present patched vtbl=0x%08X orig=0x%08X",
                                (DWORD)(uintptr_t)vtable, (DWORD)(uintptr_t)g_LiveDevicePresent);
                }
            }
        }

        EnsureLiveDeviceResetHooks(pDevice);
    }

    return oEndScene(pDevice);
}

static HRESULT __stdcall hkPresent(IDirect3DDevice9 *pDevice,
                                   const RECT *pSourceRect, const RECT *pDestRect,
                                   HWND hDestWindowOverride, const RGNDATA *pDirtyRegion)
{
    g_pDevice = pDevice;

    // Choose the correct original Present to call at the end:
    // - oPresent = inline hook trampoline (works when inline hook caught the right function)
    // - g_LiveDevicePresent = vtable original (works when EndScene fallback patched vtable)
    tPresent fnOrigPresent = oPresent ? oPresent : g_LiveDevicePresent;
    EnsureLiveDeviceResetHooks(pDevice);

    // 纹理加载（一次性，面板用）
    if (!g_TexturesLoaded)
    {
        LoadAllTextures(pDevice);
    }

    // 每帧同步SkillWnd全局指针，防止角色切换/重开窗口后悬空指针
    bool wasReady = g_Ready;
    uintptr_t swGlobal = 0;
    if (!SafeIsBadReadPtr((void *)ADDR_SkillWndEx, 4))
    {
        swGlobal = *(uintptr_t *)ADDR_SkillWndEx;
    }
    OnSkillWndPointerObserved(swGlobal, "present");

    if (!wasReady && g_Ready && g_SkillWndThis)
    {
        WriteLogFmt("[Present] SkillWnd: 0x%08X", (DWORD)g_SkillWndThis);
        WriteLog("[Present] Ready");
    }

    if (g_Ready && g_SkillWndThis && !g_NativeBtnCreated)
    {
        static DWORD s_lastPresentButtonRetryTick = 0;
        DWORD now = GetTickCount();
        if (now - s_lastPresentButtonRetryTick > 500)
        {
            s_lastPresentButtonRetryTick = now;
            WriteLogFmt("[Present] retry create native button hookInstalled=%d", oSkillWndInitChildren ? 1 : 0);
            if (CreateSuperButton(g_SkillWndThis))
                WriteLog("[Present] retry native button OK");
            else
                WriteLog("[Present] retry native button FAILED");
        }
    }

    if (ENABLE_IMGUI_OVERLAY_PANEL)
    {
        const bool hasIndependentBuffOverlay = SkillOverlayBridgeHasIndependentBuffOverlayEntries();
        const bool overlayRuntimeReady = (g_Ready || hasIndependentBuffOverlay) && g_GameHwnd && pDevice;
        if (overlayRuntimeReady && !SuperImGuiOverlayIsInitialized() && g_GameHwnd && pDevice)
        {
            if (!SuperImGuiOverlayEnsureInitialized(g_GameHwnd, pDevice, 1.0f, IMGUI_PANEL_ASSET_PATH))
            {
                static DWORD s_lastOverlayInitFailLogTick = 0;
                DWORD now = GetTickCount();
                if (now - s_lastOverlayInitFailLogTick > 1000)
                {
                    WriteLogFmt("[ImGuiOverlay] ensure init failed in Present device=0x%08X hwnd=0x%08X",
                                (DWORD)(uintptr_t)pDevice, (DWORD)(uintptr_t)g_GameHwnd);
                    s_lastOverlayInitFailLogTick = now;
                }
            }
        }

        if (SuperImGuiOverlayIsInitialized())
        {
            RECT superBtnRect = {};
            const bool hasSuperBtnRect = GetSuperButtonBaseRectForD3D(&superBtnRect);
            SuperImGuiOverlaySetPanelExpanded(g_SuperExpanded);
            SuperImGuiOverlaySetSuperButtonVisible(hasSuperBtnRect);
            SuperImGuiOverlaySetSuperButtonRect(hasSuperBtnRect ? &superBtnRect : nullptr);
        }

        if (overlayRuntimeReady && SuperImGuiOverlayIsInitialized())
        {
            if (g_SuperExpanded)
                UpdateSuperCWnd();
            SuperImGuiOverlaySetVisible(true);
            SuperImGuiOverlayRender(pDevice);
        }
        else if (SuperImGuiOverlayIsInitialized())
        {
            SuperImGuiOverlaySetVisible(false);
        }

        const bool suppressMouse = SuperImGuiOverlayShouldSuppressGameMouse();
        if (Win32InputSpoofIsInstalled())
        {
            Win32InputSpoofSetSuppressMouse(suppressMouse);
        }
        UpdateGameMouseSuppressionFallback(suppressMouse);
        if (g_LastOverlaySuppressMouse && !suppressMouse)
        {
            RefreshGameCursorImmediately();
        }
        g_LastOverlaySuppressMouse = suppressMouse;
        HRESULT presentHr = fnOrigPresent ? fnOrigPresent(pDevice, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion) : D3D_OK;
        SkillOverlayBridgeBeginFrameObservation();
        return presentHr;
    }

    // 每帧刷新扩展层锚点（真正绘制在 sub_9DEE30 里做）
    // v10.4+: native child 已有 move/refresh/toggle 三条同步链，这里先停掉 Present 中的每帧搬运，
    // 避免与原生移动链互相打架，导致拖动抽搐和视口轻微漂移。
    if (g_SkillWndThis && g_SuperExpanded)
    {
        if (!g_NativeWndCreated || !g_SuperCWnd || ENABLE_PRESENT_NATIVE_CHILD_UPDATE)
        {
            UpdateSuperCWnd();
        }
    }

    // v7.6: 拖动过程中原生dirty链不会稳定重画，直接在Present里按最新锚点绘制面板纹理。
    if (ENABLE_PRESENT_PANEL_DRAW && g_SuperExpanded && g_texPanelBg)
    {
        int drawX = g_PanelDrawX;
        int drawY = g_PanelDrawY;
        if ((drawX <= -9000 || drawY <= -9000) && g_SuperCWnd)
        {
            drawX = CWnd_GetRenderX(g_SuperCWnd);
            drawY = CWnd_GetRenderY(g_SuperCWnd);
        }
        if (drawX > -9000 && drawY > -9000)
        {
            static int s_presentDrawLogCount = 0;
            if (s_presentDrawLogCount < 20)
            {
                WriteLogFmt("[PresentDraw] panel=(%d,%d)", drawX, drawY);
                s_presentDrawLogCount++;
            }
            DrawTexturedQuad(pDevice, g_texPanelBg,
                             (float)drawX, (float)drawY, (float)PANEL_W, (float)PANEL_H);
        }
    }

    DrawSuperButtonTextureInPresent(pDevice);

    // v7.2: 默认关闭Present点击轮询，避免与WndProc路径双触发
    if (ENABLE_PRESENT_CLICK_POLL && g_Ready && g_NativeBtnCreated)
    {
        static bool s_prevLBtnDown = false;
        bool isDown = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
        if (s_prevLBtnDown && !isDown)
        {
            POINT pt = {};
            if (GetCursorPos(&pt))
            {
                HWND hwndForClient = g_GameHwnd ? g_GameHwnd : hDestWindowOverride;
                if (hwndForClient)
                {
                    ScreenToClient(hwndForClient, &pt);
                }
                TryToggleByMousePoint(pt.x, pt.y, "present");
            }
        }
        s_prevLBtnDown = isDown;
    }

    HRESULT presentHr = fnOrigPresent ? fnOrigPresent(pDevice, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion) : D3D_OK;
    SkillOverlayBridgeBeginFrameObservation();
    return presentHr;
}

// ============================================================================
// D3D9 Hook安装
// ============================================================================
static bool SetupD3D9Hook()
{
    WriteLog("[D3D9] Setup...");

    WNDCLASSEXA wc = {sizeof(WNDCLASSEX), CS_CLASSDC, DefWindowProc, 0L, 0L,
                      GetModuleHandle(NULL), NULL, NULL, NULL, NULL, "SSWDummy", NULL};
    RegisterClassExA(&wc);
    HWND hWnd = CreateWindowA("SSWDummy", "", WS_OVERLAPPEDWINDOW, 0, 0, 100, 100,
                              NULL, NULL, wc.hInstance, NULL);

    // Use the d3d9.dll that the game actually loaded (may be a wrapper/proxy in the game dir)
    // rather than always calling the system Direct3DCreate9
    typedef IDirect3D9 *(WINAPI * tDirect3DCreate9)(UINT);
    tDirect3DCreate9 pfnCreate9 = nullptr;
    HMODULE hD3D9 = ::GetModuleHandleA("d3d9.dll");
    if (hD3D9)
        pfnCreate9 = (tDirect3DCreate9)::GetProcAddress(hD3D9, "Direct3DCreate9");
    if (!pfnCreate9)
        pfnCreate9 = Direct3DCreate9; // fallback to linked import

    IDirect3D9 *pD3D = pfnCreate9(D3D_SDK_VERSION);
    if (!pD3D)
    {
        WriteLog("[D3D9] FAIL: Direct3DCreate9");
        DestroyWindow(hWnd);
        UnregisterClassA("SSWDummy", wc.hInstance);
        return false;
    }

    WriteLogFmt("[D3D9] d3d9.dll=0x%08X create9=0x%08X",
                (DWORD)(uintptr_t)hD3D9, (DWORD)(uintptr_t)pfnCreate9);

    D3DPRESENT_PARAMETERS d3dpp = {};
    d3dpp.Windowed = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
    d3dpp.hDeviceWindow = hWnd;

    IDirect3DDevice9 *pDev = nullptr;
    // Try HAL first (shares vtable with real game device), fall back to NULLREF
    HRESULT hr = pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
                                    hWnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pDev);
    if (FAILED(hr) || !pDev)
    {
        WriteLogFmt("[D3D9] HAL device failed (hr=0x%08X), trying NULLREF", (DWORD)hr);
        hr = pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_NULLREF,
                                hWnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pDev);
    }

    if (FAILED(hr) || !pDev)
    {
        pD3D->Release();
        DestroyWindow(hWnd);
        UnregisterClassA("SSWDummy", wc.hInstance);
        return false;
    }

    DWORD *vtable = *(DWORD **)pDev;
    DWORD presentAddr = vtable[17];
    DWORD endSceneAddr = vtable[42];
    BYTE *pPresent = FollowJmpChain((void *)presentAddr);
    BYTE *pEndScene = FollowJmpChain((void *)endSceneAddr);

    WriteLogFmt("[D3D9] vtable=0x%08X Present=[0x%08X->0x%08X] EndScene=[0x%08X->0x%08X]",
                (DWORD)(uintptr_t)vtable,
                presentAddr, (DWORD)(uintptr_t)pPresent,
                endSceneAddr, (DWORD)(uintptr_t)pEndScene);

    // Detect which module owns these functions
    {
        HMODULE hModPresent = nullptr, hModEndScene = nullptr;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           (LPCWSTR)pPresent, &hModPresent);
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           (LPCWSTR)pEndScene, &hModEndScene);
        char modNameP[260] = {}, modNameE[260] = {};
        if (hModPresent)
            GetModuleFileNameA(hModPresent, modNameP, sizeof(modNameP));
        if (hModEndScene)
            GetModuleFileNameA(hModEndScene, modNameE, sizeof(modNameE));
        WriteLogFmt("[D3D9] Present in: %s", modNameP[0] ? modNameP : "(unknown)");
        WriteLogFmt("[D3D9] EndScene in: %s", modNameE[0] ? modNameE : "(unknown)");
    }

    int copyLen = CalcMinCopyLen(pPresent);
    if (copyLen < 5)
        copyLen = 5;
    oReset = nullptr;
    oResetEx = nullptr;
    g_LiveDeviceReset = nullptr;
    g_LiveDeviceResetEx = nullptr;

    oPresent = (tPresent)GenericInlineHook5(pPresent, (void *)hkPresent, copyLen);
    if (!oPresent)
    {
        WriteLog("[D3D9] Present inline hook failed (will rely on EndScene fallback)");
    }

    // EndScene fallback: hook vtable[42] to capture device even if Present inline hook
    // doesn't match the game's actual Present function (e.g., D3D9Ex / wrapper d3d9.dll)
    int esLen = CalcMinCopyLen(pEndScene);
    if (esLen < 5)
        esLen = 5;
    oEndScene = (tEndScene)GenericInlineHook5(pEndScene, (void *)hkEndScene, esLen);
    if (!oEndScene)
    {
        WriteLog("[D3D9] EndScene inline hook failed");
    }

    if (!oPresent && !oEndScene)
    {
        WriteLog("[D3D9] FAIL: both Present and EndScene hooks failed");
        pDev->Release();
        pD3D->Release();
        DestroyWindow(hWnd);
        UnregisterClassA("SSWDummy", wc.hInstance);
        return false;
    }

    WriteLogFmt("[D3D9] Hooked OK, present=0x%08X endScene=0x%08X reset=vtable_only",
                (DWORD)(uintptr_t)oPresent, (DWORD)(uintptr_t)oEndScene);

    // --- Strategy 3: Hook Direct3DCreate9 to intercept device creation ---
    // When inline hooks on Present/EndScene install but never fire (D3D9on12, wrapper layers,
    // or the game's device uses a different vtable than our dummy device), this catches the
    // real device at creation time and vtable-hooks its Present directly.
    {
        HMODULE hD3D9Live = ::GetModuleHandleA("d3d9.dll");
        if (hD3D9Live)
        {
            BYTE *pCreate9 = (BYTE *)::GetProcAddress(hD3D9Live, "Direct3DCreate9");
            if (pCreate9)
            {
                pCreate9 = FollowJmpChain(pCreate9);
                int createLen = CalcMinCopyLen(pCreate9);
                if (createLen < 5)
                    createLen = 5;
                oDirectCreate9 = (tDirect3DCreate9Fn)GenericInlineHook5(pCreate9, (void *)hkDirect3DCreate9, createLen);
                if (oDirectCreate9)
                    WriteLogFmt("[D3D9] Direct3DCreate9 hooked at 0x%08X (CreateDevice interception ready)", (DWORD)(uintptr_t)pCreate9);
                else
                    WriteLog("[D3D9] Direct3DCreate9 inline hook failed (non-fatal, game may have already called it)");
            }
        }
    }

    // --- Strategy 4: Vtable-patch the dummy device's vtable directly ---
    // If the game creates a HAL device from the same d3d9.dll, it will share this vtable.
    // The vtable lives in d3d9.dll's data segment and persists after pDev->Release().
    // This catches D3D9on12 scenarios where inline hooks on the function body don't fire
    // but the vtable entries are still used by the game device.
    {
        void **dummyVtable = *(void ***)pDev;
        if (dummyVtable)
        {
            void *origPresentVtbl = nullptr;
            void *origEndSceneVtbl = nullptr;
            if (PatchLiveDeviceVTableEntry(dummyVtable, 17, (void *)hkPresent, &origPresentVtbl))
            {
                // If inline hook already installed, the original function entry is patched with jmp->hkPresent
                // so calling it would recurse. Only use vtable original when we DON'T have an inline trampoline.
                if (!oPresent && !g_LiveDevicePresent && origPresentVtbl && origPresentVtbl != (void *)hkPresent)
                    g_LiveDevicePresent = (tPresent)origPresentVtbl;
                WriteLogFmt("[D3D9] dummy vtable Present patched orig=0x%08X (using=%s)",
                            (DWORD)(uintptr_t)origPresentVtbl,
                            oPresent ? "inline_tramp" : (g_LiveDevicePresent ? "vtbl_orig" : "none"));
            }
            if (PatchLiveDeviceVTableEntry(dummyVtable, 42, (void *)hkEndScene, &origEndSceneVtbl))
            {
                if (!oEndScene && origEndSceneVtbl && origEndSceneVtbl != (void *)hkEndScene)
                    oEndScene = (tEndScene)origEndSceneVtbl;
                WriteLogFmt("[D3D9] dummy vtable EndScene patched orig=0x%08X", (DWORD)(uintptr_t)origEndSceneVtbl);
            }

            // Also patch Reset (index 16) on the dummy vtable
            void *origResetVtbl = nullptr;
            if (PatchLiveDeviceVTableEntry(dummyVtable, 16, (void *)hkReset, &origResetVtbl))
            {
                if (!oReset && origResetVtbl && origResetVtbl != (void *)hkReset)
                    oReset = (tReset)origResetVtbl;
                WriteLogFmt("[D3D9] dummy vtable Reset patched orig=0x%08X", (DWORD)(uintptr_t)origResetVtbl);
            }
        }
    }

    pDev->Release();
    pD3D->Release();
    DestroyWindow(hWnd);
    UnregisterClassA("SSWDummy", wc.hInstance);
    return true;
}

// ============================================================================
// D3D8 兼容层 — 运行时检测 + D3D8 Present hook + shared ImGui panel
// ============================================================================

// D3D8 function pointer types (用 void* 代替 IDirect3DDevice8* 因为不包含 d3d8.h)
typedef HRESULT(__stdcall *tD3D8Present)(void *pDevice8, const RECT *, const RECT *, HWND, const RGNDATA *);
typedef HRESULT(__stdcall *tD3D8Reset)(void *pDevice8, void *pPresentationParameters);
static tD3D8Present oD3D8Present = nullptr;
static tD3D8Reset oD3D8Reset = nullptr;

static bool TryInstallD3D8Rel32ThunkHook(BYTE *entry, void *hookFunc, void **outOriginal, const char *tag)
{
    if (!entry || !hookFunc || !outOriginal)
        return false;
    if (SafeIsBadReadPtr(entry, 5))
        return false;
    if (entry[0] != 0xE9)
        return false;

    BYTE *resolved = FollowJmpChain(entry);
    if (!resolved || resolved == entry)
        return false;

    DWORD oldProtect = 0;
    if (!VirtualProtect(entry, 5, PAGE_EXECUTE_READWRITE, &oldProtect))
        return false;

    *outOriginal = (void *)resolved;
    entry[0] = 0xE9;
    *(DWORD *)(entry + 1) = (DWORD)(uintptr_t)hookFunc - (DWORD)(uintptr_t)entry - 5;
    VirtualProtect(entry, 5, oldProtect, &oldProtect);
    FlushInstructionCache(GetCurrentProcess(), entry, 5);

    WriteLogFmt("[D3D8] %s rel32 thunk patched entry=0x%08X resolved=0x%08X",
                tag ? tag : "hook",
                (DWORD)(uintptr_t)entry,
                (DWORD)(uintptr_t)resolved);
    return true;
}

// D3D8 Present hook — 直接在游戏 D3D8 设备上渲染 overlay
static HRESULT __stdcall hkD3D8Present(void *pDevice8,
                                       const RECT *pSourceRect, const RECT *pDestRect,
                                       HWND hDestWindowOverride, const RGNDATA *pDirtyRegion)
{
    const char *d3d8Stage = "begin";
    __try
    {
        d3d8Stage = "resolve_hwnd";
        if (!g_D3D8GameHwnd)
        {
            HWND hwnd = hDestWindowOverride;
            if (!hwnd)
                hwnd = g_GameHwnd;
            if (!hwnd)
            {
                DWORD pid = GetCurrentProcessId();
                struct FindCtx
                {
                    DWORD pid;
                    HWND result;
                };
                FindCtx ctx = {pid, nullptr};
                EnumWindows([](HWND h, LPARAM lp) -> BOOL
                            {
                    FindCtx* c = (FindCtx*)lp;
                    DWORD wp;
                    GetWindowThreadProcessId(h, &wp);
                    if (wp == c->pid && IsWindowVisible(h)) {
                        c->result = h;
                        return FALSE;
                    }
                    return TRUE; }, (LPARAM)&ctx);
                hwnd = ctx.result;
            }
            if (hwnd)
            {
                g_D3D8GameHwnd = hwnd;
                if (!g_GameHwnd)
                    g_GameHwnd = hwnd;
                WriteLogFmt("[D3D8] first Present: hwnd=0x%08X", (DWORD)(uintptr_t)hwnd);
            }
        }

        d3d8Stage = "observe_skillwnd";
        bool wasReady = g_Ready;
        uintptr_t swGlobal = 0;
        if (!SafeIsBadReadPtr((void *)ADDR_SkillWndEx, 4))
        {
            swGlobal = *(uintptr_t *)ADDR_SkillWndEx;
        }
        OnSkillWndPointerObserved(swGlobal, "d3d8_present");

        if (!wasReady && g_Ready && g_SkillWndThis)
        {
            WriteLogFmt("[D3D8-Present] SkillWnd: 0x%08X", (DWORD)g_SkillWndThis);
            WriteLog("[D3D8-Present] Ready");
        }

        d3d8Stage = "retry_native_button";
        if (g_Ready && g_SkillWndThis && !g_NativeBtnCreated)
        {
            static DWORD s_lastD3D8PresentButtonRetryTick = 0;
            DWORD now = GetTickCount();
            if (now - s_lastD3D8PresentButtonRetryTick > 500)
            {
                s_lastD3D8PresentButtonRetryTick = now;
                WriteLogFmt("[D3D8-Present] retry create native button hookInstalled=%d", oSkillWndInitChildren ? 1 : 0);
                if (CreateSuperButton(g_SkillWndThis))
                    WriteLog("[D3D8-Present] retry native button OK");
                else
                    WriteLog("[D3D8-Present] retry native button FAILED");
            }
        }

        d3d8Stage = "overlay_present";
        if (ENABLE_IMGUI_OVERLAY_PANEL)
        {
            const bool hasIndependentBuffOverlay = SkillOverlayBridgeHasIndependentBuffOverlayEntries();
            const bool overlayActivationReady =
                (g_Ready || hasIndependentBuffOverlay) &&
                g_D3D8GameHwnd &&
                pDevice8;

            d3d8Stage = "ensure_d3d8_textures";
            if (g_Ready && g_NativeBtnCreated)
                EnsureD3D8SuperTexturesLoaded(pDevice8);

            d3d8Stage = "overlay_init";
            if (overlayActivationReady && !SuperD3D8OverlayIsInitialized() && g_D3D8GameHwnd && pDevice8)
            {
                if (!SuperD3D8OverlayEnsureInitialized(g_D3D8GameHwnd, pDevice8, 1.0f, IMGUI_PANEL_ASSET_PATH))
                {
                    static DWORD s_lastD3D8OverlayInitFailLogTick = 0;
                    DWORD now = GetTickCount();
                    if (now - s_lastD3D8OverlayInitFailLogTick > 1000)
                    {
                        WriteLogFmt("[D3D8ImGuiOverlay] ensure init failed in Present device=0x%08X hwnd=0x%08X",
                                    (DWORD)(uintptr_t)pDevice8, (DWORD)(uintptr_t)g_D3D8GameHwnd);
                        s_lastD3D8OverlayInitFailLogTick = now;
                    }
                }
            }

            d3d8Stage = "overlay_render";
            if (SuperD3D8OverlayIsInitialized())
            {
                RECT superBtnRect = {};
                const bool hasSuperBtnRect = GetSuperButtonBaseRectForD3D(&superBtnRect);
                SuperD3D8OverlaySetPanelExpanded(g_SuperExpanded);
                SuperD3D8OverlaySetSuperButtonVisible(hasSuperBtnRect);
                SuperD3D8OverlaySetSuperButtonRect(hasSuperBtnRect ? &superBtnRect : nullptr);
            }
            if (overlayActivationReady && SuperD3D8OverlayIsInitialized())
            {
                if (g_SuperExpanded)
                    UpdateSuperCWnd();
                SuperD3D8OverlaySetVisible(true);
                SuperD3D8OverlayRender(pDevice8);
            }
            else if (SuperD3D8OverlayIsInitialized())
            {
                SuperD3D8OverlaySetVisible(false);
            }

            d3d8Stage = "mouse_suppress";
            const bool suppressMouse = SuperD3D8OverlayShouldSuppressGameMouse();
            if (Win32InputSpoofIsInstalled())
            {
                Win32InputSpoofSetSuppressMouse(suppressMouse);
            }
            UpdateGameMouseSuppressionFallback(suppressMouse);
            if (g_LastOverlaySuppressMouse && !suppressMouse)
            {
                RefreshGameCursorImmediately();
            }
            g_LastOverlaySuppressMouse = suppressMouse;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        WriteLogFmt("[D3D8] EXCEPTION in Present stage=%s code=0x%08X",
                    d3d8Stage ? d3d8Stage : "unknown",
                    GetExceptionCode());
        ResetSuperBtnD3DInteractionState();
        g_LastOverlaySuppressMouse = false;
        if (ENABLE_IMGUI_OVERLAY_PANEL && SuperD3D8OverlayIsInitialized())
        {
            SuperD3D8OverlaySetVisible(false);
        }
    }

    HRESULT hr = D3D_OK;
    __try
    {
        d3d8Stage = "call_orig_present";
        hr = oD3D8Present ? oD3D8Present(pDevice8, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion) : D3D_OK;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        WriteLogFmt("[D3D8] EXCEPTION in Present stage=%s code=0x%08X",
                    d3d8Stage ? d3d8Stage : "call_orig_present",
                    GetExceptionCode());
        hr = D3D_OK;
    }
    SkillOverlayBridgeBeginFrameObservation();
    return hr;
}

// D3D8 Reset hook — 释放 D3D8 纹理
static HRESULT __stdcall hkD3D8Reset(void *pDevice8, void *pPresentationParameters)
{
    WriteLog("[D3D8] Reset called, releasing D3D8 textures");
    if (ENABLE_IMGUI_OVERLAY_PANEL)
    {
        SuperD3D8OverlayOnDeviceLost();
    }

    HRESULT hr = oD3D8Reset ? oD3D8Reset(pDevice8, pPresentationParameters) : D3DERR_INVALIDCALL;

    if (SUCCEEDED(hr))
    {
        if (ENABLE_IMGUI_OVERLAY_PANEL)
        {
            SuperD3D8OverlayOnDeviceReset(pDevice8);
        }
        WriteLog("[D3D8] Reset OK, overlay device objects refreshed");
    }
    else
    {
        WriteLogFmt("[D3D8] Reset FAIL hr=0x%08X", (DWORD)hr);
    }
    return hr;
}

// 安装 D3D8 hooks
static bool SetupD3D8Hook()
{
    WriteLog("[D3D8] Setup...");

    HMODULE hD3D8 = ::GetModuleHandleA("d3d8.dll");
    if (!hD3D8)
    {
        WriteLog("[D3D8] FAIL: d3d8.dll not loaded");
        return false;
    }

    typedef void *(__stdcall * tDirect3DCreate8)(UINT);
    tDirect3DCreate8 pfnCreate8 = (tDirect3DCreate8)::GetProcAddress(hD3D8, "Direct3DCreate8");
    if (!pfnCreate8)
    {
        WriteLog("[D3D8] FAIL: Direct3DCreate8 not found");
        return false;
    }

    WriteLogFmt("[D3D8] d3d8.dll=0x%08X Direct3DCreate8=0x%08X",
                (DWORD)(uintptr_t)hD3D8, (DWORD)(uintptr_t)pfnCreate8);

    // 创建 dummy 窗口和 D3D8 设备来获取 vtable
    WNDCLASSEXA wc = {sizeof(WNDCLASSEX), CS_CLASSDC, DefWindowProc, 0L, 0L,
                      GetModuleHandle(NULL), NULL, NULL, NULL, NULL, "SSWDummyD3D8", NULL};
    RegisterClassExA(&wc);
    HWND hWnd = CreateWindowA("SSWDummyD3D8", "", WS_OVERLAPPEDWINDOW, 0, 0, 100, 100,
                              NULL, NULL, wc.hInstance, NULL);

    // 调用 Direct3DCreate8(220) — D3D8 SDK version
    void *pD3D8 = pfnCreate8(220);
    if (!pD3D8)
    {
        WriteLog("[D3D8] FAIL: Direct3DCreate8 returned null");
        DestroyWindow(hWnd);
        UnregisterClassA("SSWDummyD3D8", wc.hInstance);
        return false;
    }

    // IDirect3D8::CreateDevice = vtable[15]
    // IDirect3D8 vtable: 0=QI, 1=AddRef, 2=Release, ..., 15=CreateDevice
    DWORD *vtableD3D8 = *(DWORD **)pD3D8;

    // 查询当前显示模式以获取有效的 BackBufferFormat
    // IDirect3D8::GetAdapterDisplayMode = vtable[8]
    // HRESULT GetAdapterDisplayMode(UINT Adapter, D3DDISPLAYMODE* pMode)
    // D3DDISPLAYMODE: { Width(4), Height(4), RefreshRate(4), Format(4) } = 16 bytes
    typedef HRESULT(__stdcall * tGetAdapterDisplayMode8)(void *, UINT, void *);
    tGetAdapterDisplayMode8 pfnGetDisplayMode = (tGetAdapterDisplayMode8)vtableD3D8[8];
    BYTE displayMode[16] = {};
    DWORD backBufferFormat = 22; // D3DFMT_X8R8G8B8 fallback
    if (pfnGetDisplayMode)
    {
        HRESULT hrDM = pfnGetDisplayMode(pD3D8, 0, displayMode);
        if (SUCCEEDED(hrDM))
        {
            DWORD dmFormat = *(DWORD *)(displayMode + 12); // Format at offset 12
            if (dmFormat != 0)
            {
                backBufferFormat = dmFormat;
                WriteLogFmt("[D3D8] display mode: %dx%d fmt=%d",
                            *(DWORD *)(displayMode + 0), *(DWORD *)(displayMode + 4), dmFormat);
            }
        }
    }

    // D3D8 D3DPRESENT_PARAMETERS layout (不同于 D3D9，没有 MultiSampleQuality):
    //   0x00: BackBufferWidth (DWORD)
    //   0x04: BackBufferHeight (DWORD)
    //   0x08: BackBufferFormat (D3DFORMAT)  <-- D3D8 不支持 D3DFMT_UNKNOWN(0)！必须指定有效格式
    //   0x0C: BackBufferCount (DWORD)
    //   0x10: MultiSampleType (D3DMULTISAMPLE_TYPE)
    //   0x14: SwapEffect (D3DSWAPEFFECT)
    //   0x18: hDeviceWindow (HWND)
    //   0x1C: Windowed (BOOL)
    //   0x20: EnableAutoDepthStencil (BOOL)
    //   0x24: AutoDepthStencilFormat (D3DFORMAT)
    //   0x28: Flags (DWORD)
    //   0x2C: FullScreen_RefreshRateInHz (UINT)
    //   0x30: FullScreen_PresentationInterval (UINT)
    BYTE d3dpp8[64] = {};
    *(DWORD *)(d3dpp8 + 0x08) = backBufferFormat; // BackBufferFormat — 必须是有效格式
    *(DWORD *)(d3dpp8 + 0x14) = 1;                // SwapEffect = D3DSWAPEFFECT_DISCARD
    *(HWND *)(d3dpp8 + 0x18) = hWnd;              // hDeviceWindow
    *(DWORD *)(d3dpp8 + 0x1C) = 1;                // Windowed = TRUE

    // 调用 IDirect3D8::CreateDevice (vtable[15])
    // HRESULT CreateDevice(UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow,
    //                      DWORD BehaviorFlags, D3DPRESENT_PARAMETERS8* pPP, IDirect3DDevice8** ppDevice)
    typedef HRESULT(__stdcall * tD3D8CreateDevice)(void *pD3D8, UINT, DWORD, HWND, DWORD, void *, void **);
    tD3D8CreateDevice pfnCreateDevice8 = (tD3D8CreateDevice)vtableD3D8[15];

    void *pDummyDevice8 = nullptr;
    HRESULT hr = pfnCreateDevice8(pD3D8, 0, 1 /*D3DDEVTYPE_HAL*/, hWnd,
                                  0x20 /*D3DCREATE_SOFTWARE_VERTEXPROCESSING*/, d3dpp8, &pDummyDevice8);

    if (FAILED(hr) || !pDummyDevice8)
    {
        WriteLogFmt("[D3D8] dummy device creation failed hr=0x%08X, trying NULLREF", (DWORD)hr);
        // D3D8 没有 D3DDEVTYPE_NULLREF，尝试 REF (2)
        hr = pfnCreateDevice8(pD3D8, 0, 2 /*D3DDEVTYPE_REF*/, hWnd,
                              0x20, d3dpp8, &pDummyDevice8);
    }

    if (FAILED(hr) || !pDummyDevice8)
    {
        WriteLogFmt("[D3D8] FAIL: all device creation attempts failed hr=0x%08X", (DWORD)hr);
        // 释放 IDirect3D8 (vtable[2] = Release)
        typedef ULONG(__stdcall * tRelease)(void *);
        ((tRelease)vtableD3D8[2])(pD3D8);
        DestroyWindow(hWnd);
        UnregisterClassA("SSWDummyD3D8", wc.hInstance);
        return false;
    }

    // 从 dummy device 获取 vtable
    DWORD *vtableDevice8 = *(DWORD **)pDummyDevice8;
    // IDirect3DDevice8 vtable indices:
    //   14 = Reset
    //   15 = Present
    DWORD presentAddr = vtableDevice8[15];
    DWORD resetAddr = vtableDevice8[14];

    WriteLogFmt("[D3D8] device vtable=0x%08X Present=[%d]=0x%08X Reset=[%d]=0x%08X",
                (DWORD)(uintptr_t)vtableDevice8, 15, presentAddr, 14, resetAddr);

    // Inline hook D3D8 Present
    BYTE *pPresentEntry8 = (BYTE *)(uintptr_t)presentAddr;
    BYTE *pPresent8 = FollowJmpChain((void *)presentAddr);
    if (TryInstallD3D8Rel32ThunkHook(pPresentEntry8, (void *)hkD3D8Present, (void **)&oD3D8Present, "Present"))
    {
        WriteLogFmt("[D3D8] Present thunk-entry hooked entry=0x%08X orig=0x%08X",
                    (DWORD)(uintptr_t)pPresentEntry8,
                    (DWORD)(uintptr_t)oD3D8Present);
    }
    else
    {
        int copyLenPresent = CalcMinCopyLen(pPresent8);
        if (copyLenPresent < 5)
            copyLenPresent = 5;
        oD3D8Present = (tD3D8Present)GenericInlineHook5(pPresent8, (void *)hkD3D8Present, copyLenPresent);
        if (!oD3D8Present)
        {
            WriteLog("[D3D8] Present inline hook failed, trying vtable patch");
            // Fallback: vtable patch
            DWORD oldProt;
            VirtualProtect(&vtableDevice8[15], sizeof(void *), PAGE_READWRITE, &oldProt);
            oD3D8Present = (tD3D8Present)vtableDevice8[15];
            vtableDevice8[15] = (DWORD)(uintptr_t)hkD3D8Present;
            VirtualProtect(&vtableDevice8[15], sizeof(void *), oldProt, &oldProt);
            WriteLogFmt("[D3D8] Present vtable patched, orig=0x%08X", (DWORD)(uintptr_t)oD3D8Present);
        }
        else
        {
            WriteLogFmt("[D3D8] Present inline hooked at 0x%08X tramp=0x%08X",
                        (DWORD)(uintptr_t)pPresent8, (DWORD)(uintptr_t)oD3D8Present);
        }
    }

    // Inline hook D3D8 Reset
    BYTE *pResetEntry8 = (BYTE *)(uintptr_t)resetAddr;
    BYTE *pReset8 = FollowJmpChain((void *)resetAddr);
    if (TryInstallD3D8Rel32ThunkHook(pResetEntry8, (void *)hkD3D8Reset, (void **)&oD3D8Reset, "Reset"))
    {
        WriteLogFmt("[D3D8] Reset thunk-entry hooked entry=0x%08X orig=0x%08X",
                    (DWORD)(uintptr_t)pResetEntry8,
                    (DWORD)(uintptr_t)oD3D8Reset);
    }
    else
    {
        int copyLenReset = CalcMinCopyLen(pReset8);
        if (copyLenReset < 5)
            copyLenReset = 5;
        oD3D8Reset = (tD3D8Reset)GenericInlineHook5(pReset8, (void *)hkD3D8Reset, copyLenReset);
        if (!oD3D8Reset)
        {
            WriteLog("[D3D8] Reset inline hook failed, trying vtable patch");
            DWORD oldProt;
            VirtualProtect(&vtableDevice8[14], sizeof(void *), PAGE_READWRITE, &oldProt);
            oD3D8Reset = (tD3D8Reset)vtableDevice8[14];
            vtableDevice8[14] = (DWORD)(uintptr_t)hkD3D8Reset;
            VirtualProtect(&vtableDevice8[14], sizeof(void *), oldProt, &oldProt);
            WriteLogFmt("[D3D8] Reset vtable patched, orig=0x%08X", (DWORD)(uintptr_t)oD3D8Reset);
        }
        else
        {
            WriteLogFmt("[D3D8] Reset inline hooked at 0x%08X tramp=0x%08X",
                        (DWORD)(uintptr_t)pReset8, (DWORD)(uintptr_t)oD3D8Reset);
        }
    }

    // 释放 dummy 设备和 IDirect3D8
    typedef ULONG(__stdcall * tRelease)(void *);
    ((tRelease)(*(DWORD **)pDummyDevice8)[2])(pDummyDevice8);
    ((tRelease)vtableD3D8[2])(pD3D8);
    DestroyWindow(hWnd);
    UnregisterClassA("SSWDummyD3D8", wc.hInstance);

    WriteLog("[D3D8] Hook setup complete");
    return true;
}

// ============================================================================
// SkillWnd Hook安装
// ============================================================================
static bool SetupSkillWndHook()
{
    oSkillWndInitChildren = (tSkillWndInitChildren)InstallInlineHook(
        ADDR_9E17D0, (void *)hkSkillWndInitChildren);
    if (!oSkillWndInitChildren)
    {
        WriteLog("[SkillHook] Init hook failed");
        return false;
    }
    WriteLogFmt("[SkillHook] Init: tramp=0x%08X", (DWORD)oSkillWndInitChildren);
    return true;
}

// ============================================================================
// 消息处理Hook安装
// ============================================================================
static bool SetupMsgHook()
{
    oSkillWndMsg = (tSkillWndMsg)InstallInlineHook(
        ADDR_9DDB30, (void *)hkSkillWndMsgNaked);
    if (!oSkillWndMsg)
    {
        WriteLog("[MsgHook] Hook failed");
        return false;
    }
    WriteLogFmt("[MsgHook] OK(9DDB30): tramp=0x%08X", (DWORD)oSkillWndMsg);
    return true;
}

// ============================================================================
// 发包 Hook安装
// ============================================================================
#include "runtime_feature_mount_movement_ability.inl"
#include "runtime_feature_mounted_double_jump.inl"
#include "runtime_feature_mount_climb_gate.inl"
#include "runtime_feature_mount_flight_mapping.inl"
#include "runtime_feature_mount_movement_runtime.inl"

