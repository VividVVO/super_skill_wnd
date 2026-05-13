static HWND GetRealGameWindow()
{
    struct Param
    {
        HWND hwnd;
        DWORD pid;
    };
    Param p = {NULL, GetCurrentProcessId()};

    EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL
                {
        Param* pp = (Param*)lParam;
        DWORD pid;
        GetWindowThreadProcessId(hwnd, &pid);
        if (pid == pp->pid && IsWindowVisible(hwnd)) {
            char cn[256];
            GetClassNameA(hwnd, cn, sizeof(cn));
            if (strcmp(cn, "ConsoleWindowClass") != 0) {
                pp->hwnd = hwnd;
                return FALSE;
            }
        }
        return TRUE; }, (LPARAM)&p);
    return p.hwnd;
}

// ============================================================================
// PNG纹理从DLL资源加载（面板背景用，后续迁移到原生后可移除）
// ============================================================================
static IDirect3DTexture9 *LoadTextureFromResource(IDirect3DDevice9 *dev, int resID)
{
    if (!dev)
        return nullptr;

    HRSRC hRes = FindResourceA(g_hModule, MAKEINTRESOURCEA(resID), RT_RCDATA);
    if (!hRes)
    {
        WriteLogFmt("[Tex] FindResource(%d) failed", resID);
        return nullptr;
    }

    HGLOBAL hMem = LoadResource(g_hModule, hRes);
    DWORD sz = SizeofResource(g_hModule, hRes);
    if (!hMem || !sz)
    {
        WriteLogFmt("[Tex] LoadResource(%d) failed", resID);
        return nullptr;
    }

    void *pData = LockResource(hMem);
    if (!pData)
        return nullptr;

    int w, h, ch;
    unsigned char *pixels = stbi_load_from_memory((const unsigned char *)pData, (int)sz, &w, &h, &ch, 4);
    if (!pixels)
    {
        WriteLogFmt("[Tex] stbi_load(%d) failed", resID);
        return nullptr;
    }

    const bool hardenSuperBtnEdges =
        resID == IDR_BTN_NORMAL ||
        resID == IDR_BTN_HOVER ||
        resID == IDR_BTN_PRESSED ||
        resID == IDR_BTN_DISABLED;
    if (hardenSuperBtnEdges)
    {
        HardenPixelArtAlphaEdgesRgba(pixels, w, h);
    }

    IDirect3DTexture9 *tex = nullptr;
    if (FAILED(dev->CreateTexture(w, h, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &tex, nullptr)))
    {
        stbi_image_free(pixels);
        return nullptr;
    }

    D3DLOCKED_RECT lr;
    if (SUCCEEDED(tex->LockRect(0, &lr, nullptr, 0)))
    {
        for (int y2 = 0; y2 < h; y2++)
        {
            unsigned char *src = pixels + y2 * w * 4;
            unsigned char *dst = (unsigned char *)lr.pBits + y2 * lr.Pitch;
            for (int x2 = 0; x2 < w; x2++)
            {
                dst[x2 * 4 + 0] = src[x2 * 4 + 2]; // B
                dst[x2 * 4 + 1] = src[x2 * 4 + 1]; // G
                dst[x2 * 4 + 2] = src[x2 * 4 + 0]; // R
                dst[x2 * 4 + 3] = src[x2 * 4 + 3]; // A
            }
        }
        tex->UnlockRect(0);
    }

    stbi_image_free(pixels);
    WriteLogFmt("[Tex] Loaded #%d: %dx%d", resID, w, h);
    return tex;
}

static IDirect3DTexture9 *LoadTextureFromFilePath(IDirect3DDevice9 *dev, const char *path)
{
    if (!dev || !path || !path[0])
        return nullptr;

    int w = 0, h = 0, ch = 0;
    unsigned char *pixels = stbi_load(path, &w, &h, &ch, 4);
    if (!pixels)
    {
        WriteLogFmt("[Tex] stbi_load(file) failed path=%s", path);
        return nullptr;
    }

    IDirect3DTexture9 *tex = nullptr;
    if (FAILED(dev->CreateTexture(w, h, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &tex, nullptr)))
    {
        stbi_image_free(pixels);
        return nullptr;
    }

    D3DLOCKED_RECT lr = {};
    if (SUCCEEDED(tex->LockRect(0, &lr, nullptr, 0)))
    {
        for (int y2 = 0; y2 < h; y2++)
        {
            unsigned char *src = pixels + y2 * w * 4;
            unsigned char *dst = (unsigned char *)lr.pBits + y2 * lr.Pitch;
            for (int x2 = 0; x2 < w; x2++)
            {
                dst[x2 * 4 + 0] = src[x2 * 4 + 2];
                dst[x2 * 4 + 1] = src[x2 * 4 + 1];
                dst[x2 * 4 + 2] = src[x2 * 4 + 0];
                dst[x2 * 4 + 3] = src[x2 * 4 + 3];
            }
        }
        tex->UnlockRect(0);
    }

    stbi_image_free(pixels);
    WriteLogFmt("[Tex] Loaded file: %s -> %dx%d", path, w, h);
    return tex;
}

static D3D8Texture LoadTextureFromResourceD3D8(void *pDevice8, int resID)
{
    D3D8Texture tex = {};
    if (!pDevice8)
        return tex;

    HRSRC hRes = FindResourceA(g_hModule, MAKEINTRESOURCEA(resID), RT_RCDATA);
    if (!hRes)
    {
        WriteLogFmt("[D3D8Tex] FindResource(%d) failed", resID);
        return tex;
    }

    HGLOBAL hMem = LoadResource(g_hModule, hRes);
    DWORD sz = SizeofResource(g_hModule, hRes);
    if (!hMem || !sz)
    {
        WriteLogFmt("[D3D8Tex] LoadResource(%d) failed", resID);
        return tex;
    }

    void *pData = LockResource(hMem);
    if (!pData)
    {
        WriteLogFmt("[D3D8Tex] LockResource(%d) failed", resID);
        return tex;
    }

    int w = 0;
    int h = 0;
    int ch = 0;
    unsigned char *rgba = stbi_load_from_memory((const unsigned char *)pData, (int)sz, &w, &h, &ch, 4);
    if (!rgba)
    {
        WriteLogFmt("[D3D8Tex] stbi_load(%d) failed", resID);
        return tex;
    }

    const bool hardenSuperBtnEdges =
        resID == IDR_BTN_NORMAL ||
        resID == IDR_BTN_HOVER ||
        resID == IDR_BTN_PRESSED ||
        resID == IDR_BTN_DISABLED;
    if (hardenSuperBtnEdges)
    {
        HardenPixelArtAlphaEdgesRgba(rgba, w, h);
    }

    tex = D3D8_CreateTextureFromRGBA(pDevice8, rgba, w, h);
    stbi_image_free(rgba);
    if (tex.pTexture8)
    {
        WriteLogFmt("[D3D8Tex] Loaded #%d: %dx%d", resID, tex.width, tex.height);
    }
    return tex;
}

static void LoadAllTextures(IDirect3DDevice9 *dev)
{
    if (g_TexturesLoaded || !dev)
        return;
    g_texPanelBg = LoadTextureFromResource(dev, IDR_PANEL_BG);
    g_texBtnNormal = LoadTextureFromResource(dev, IDR_BTN_NORMAL);
    g_texBtnHover = LoadTextureFromResource(dev, IDR_BTN_HOVER);
    g_texBtnPressed = LoadTextureFromResource(dev, IDR_BTN_PRESSED);
    g_texBtnDisabled = LoadTextureFromResource(dev, IDR_BTN_DISABLED);
    g_texCursorNormal = LoadTextureFromResource(dev, IDR_CURSOR_NORMAL);
    g_texCursorHoverA = LoadTextureFromResource(dev, IDR_CURSOR_HOVER_A);
    g_texCursorHoverB = LoadTextureFromResource(dev, IDR_CURSOR_HOVER_B);
    g_texCursorPressed = LoadTextureFromResource(dev, IDR_CURSOR_PRESSED);
    g_TexturesLoaded = true;
    WriteLogFmt("[Tex] loaded panel=%p btn=[%p,%p,%p,%p] cursor=[%p,%p,%p,%p]",
                g_texPanelBg, g_texBtnNormal, g_texBtnHover, g_texBtnPressed, g_texBtnDisabled,
                g_texCursorNormal, g_texCursorHoverA, g_texCursorHoverB, g_texCursorPressed);
}

static void ReleaseAllD3D8Textures(const char *reason)
{
    if (g_D3D8TexturesLoaded)
    {
        WriteLogFmt("[D3D8Tex] release reason=%s", reason ? reason : "unknown");
    }
    D3D8_ReleaseTexture(g_d3d8TexBtnNormal);
    D3D8_ReleaseTexture(g_d3d8TexBtnHover);
    D3D8_ReleaseTexture(g_d3d8TexBtnPressed);
    D3D8_ReleaseTexture(g_d3d8TexBtnDisabled);
    D3D8_ReleaseTexture(g_d3d8TexCursorNormal);
    D3D8_ReleaseTexture(g_d3d8TexCursorHoverA);
    D3D8_ReleaseTexture(g_d3d8TexCursorHoverB);
    D3D8_ReleaseTexture(g_d3d8TexCursorPressed);
    g_D3D8TextureDevice = nullptr;
    g_D3D8TexturesLoaded = false;
}

static void ReleaseAllD3D9Textures(const char *reason)
{
    if (g_texPanelBg)
    {
        WriteLogFmt("[Tex] release panel=%p reason=%s", g_texPanelBg, reason ? reason : "unknown");
        g_texPanelBg->Release();
        g_texPanelBg = nullptr;
    }
    if (g_texBtnNormal)
    {
        g_texBtnNormal->Release();
        g_texBtnNormal = nullptr;
    }
    if (g_texBtnHover)
    {
        g_texBtnHover->Release();
        g_texBtnHover = nullptr;
    }
    if (g_texBtnPressed)
    {
        g_texBtnPressed->Release();
        g_texBtnPressed = nullptr;
    }
    if (g_texBtnDisabled)
    {
        g_texBtnDisabled->Release();
        g_texBtnDisabled = nullptr;
    }
    if (g_texCursorNormal)
    {
        g_texCursorNormal->Release();
        g_texCursorNormal = nullptr;
    }
    if (g_texCursorHoverA)
    {
        g_texCursorHoverA->Release();
        g_texCursorHoverA = nullptr;
    }
    if (g_texCursorHoverB)
    {
        g_texCursorHoverB->Release();
        g_texCursorHoverB = nullptr;
    }
    if (g_texCursorPressed)
    {
        g_texCursorPressed->Release();
        g_texCursorPressed = nullptr;
    }
    g_TexturesLoaded = false;
}

static void EnsureD3D8SuperTexturesLoaded(void *pDevice8)
{
    if (!pDevice8)
        return;

    if (g_D3D8TexturesLoaded && g_D3D8TextureDevice == pDevice8)
        return;

    if (g_D3D8TexturesLoaded && g_D3D8TextureDevice != pDevice8)
        ReleaseAllD3D8Textures("device_changed");

    g_d3d8TexBtnNormal = LoadTextureFromResourceD3D8(pDevice8, IDR_BTN_NORMAL);
    g_d3d8TexBtnHover = LoadTextureFromResourceD3D8(pDevice8, IDR_BTN_HOVER);
    g_d3d8TexBtnPressed = LoadTextureFromResourceD3D8(pDevice8, IDR_BTN_PRESSED);
    g_d3d8TexBtnDisabled = LoadTextureFromResourceD3D8(pDevice8, IDR_BTN_DISABLED);
    g_d3d8TexCursorNormal = LoadTextureFromResourceD3D8(pDevice8, IDR_CURSOR_NORMAL);
    g_d3d8TexCursorHoverA = LoadTextureFromResourceD3D8(pDevice8, IDR_CURSOR_HOVER_A);
    g_d3d8TexCursorHoverB = LoadTextureFromResourceD3D8(pDevice8, IDR_CURSOR_HOVER_B);
    g_d3d8TexCursorPressed = LoadTextureFromResourceD3D8(pDevice8, IDR_CURSOR_PRESSED);
    g_D3D8TextureDevice = pDevice8;
    g_D3D8TexturesLoaded =
        g_d3d8TexBtnNormal.pTexture8 &&
        g_d3d8TexBtnHover.pTexture8 &&
        g_d3d8TexBtnPressed.pTexture8 &&
        g_d3d8TexCursorNormal.pTexture8;

    WriteLogFmt("[D3D8Tex] loaded=%d btn=[0x%08X,0x%08X,0x%08X,0x%08X] cursor=[0x%08X,0x%08X,0x%08X,0x%08X]",
                g_D3D8TexturesLoaded ? 1 : 0,
                (DWORD)(uintptr_t)g_d3d8TexBtnNormal.pTexture8,
                (DWORD)(uintptr_t)g_d3d8TexBtnHover.pTexture8,
                (DWORD)(uintptr_t)g_d3d8TexBtnPressed.pTexture8,
                (DWORD)(uintptr_t)g_d3d8TexBtnDisabled.pTexture8,
                (DWORD)(uintptr_t)g_d3d8TexCursorNormal.pTexture8,
                (DWORD)(uintptr_t)g_d3d8TexCursorHoverA.pTexture8,
                (DWORD)(uintptr_t)g_d3d8TexCursorHoverB.pTexture8,
                (DWORD)(uintptr_t)g_d3d8TexCursorPressed.pTexture8);
}

static void PrepareForD3DDeviceReset(const char *reason)
{
    WriteLogFmt("[D3D9] hard rebuild begin reason=%s overlay=%d textures=%d",
                reason ? reason : "unknown",
                SuperImGuiOverlayIsInitialized() ? 1 : 0,
                g_TexturesLoaded ? 1 : 0);

    if (Win32InputSpoofIsInstalled())
        Win32InputSpoofSetSuppressMouse(false);
    g_LastOverlaySuppressMouse = false;

    if (ENABLE_IMGUI_OVERLAY_PANEL)
        SuperImGuiOverlayOnDeviceLost();

    // Keep managed textures alive across Reset. Releasing them inside Reset has
    // proven fragile and is unnecessary for D3DPOOL_MANAGED resources.
    g_pDevice = nullptr;
}

// ============================================================================
// 自定义Draw函数（替换vtable1[11]，即byte offset +44）
// 在RenderAll的dirty list遍历中调用
// ============================================================================
static int g_DrawCallCount = 0;
static int g_UpdatePosLogCount = 0;
static int g_LastMsgID = -1;
static DWORD g_LastMsgTick = 0;

static void __fastcall SuperCWndDraw(uintptr_t thisPtr, void * /*edx_unused*/, int *clipRegion)
{
    if (!thisPtr)
        return;

    int drawX = g_PanelDrawX;
    int drawY = g_PanelDrawY;
    if (drawX <= -9000 || drawY <= -9000)
    {
        drawX = CWnd_GetX(thisPtr);
        drawY = CWnd_GetY(thisPtr);
    }

    if (!g_SuperExpanded)
        return;

    if (g_DrawCallCount < 20)
    {
        int cx = CWnd_GetX(thisPtr);
        int cy = CWnd_GetY(thisPtr);
        int rx = CWnd_GetRenderX(thisPtr);
        int ry = CWnd_GetRenderY(thisPtr);
        int w = *(int *)(thisPtr + 10 * 4);
        int h = *(int *)(thisPtr + 11 * 4);
        if (IsOfficialSecondChildObject(thisPtr, true, true))
        {
            int refCount = *(int *)(thisPtr + CWND_OFF_REFCNT * 4);
            WriteLogFmt("[Draw] #%d officialSecond=1 ref=%d com=(%d,%d) render=(%d,%d) size=%dx%d",
                        g_DrawCallCount, refCount, cx, cy, rx, ry, w, h);
        }
        else
        {
            int hx = CWnd_GetHomeX(thisPtr);
            int hy = CWnd_GetHomeY(thisPtr);
            WriteLogFmt("[Draw] #%d officialSecond=0 home=(%d,%d) com=(%d,%d) render=(%d,%d) size=%dx%d",
                        g_DrawCallCount, hx, hy, cx, cy, rx, ry, w, h);
        }
        g_DrawCallCount++;
    }

    DWORD *surface = nullptr;
    __try
    {
        ((tGetSurface)ADDR_435A50)(thisPtr, &surface);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        static int s_surfaceExceptLog = 0;
        if (s_surfaceExceptLog < 12)
        {
            WriteLogFmt("[SuperWndDraw] EXCEPTION: sub_435A50 0x%08X", GetExceptionCode());
            s_surfaceExceptLog++;
        }
        return;
    }

    if (!surface)
    {
        static int s_surfaceNullLog = 0;
        if (s_surfaceNullLog < 12)
        {
            WriteLog("[SuperWndDraw] FAIL: surface null");
            s_surfaceNullLog++;
        }
        return;
    }

    static int s_superNativeDrawLogCount = 0;
    bool emitLog = (s_superNativeDrawLogCount < 40);
    if (DrawNativePanelOnSurface(surface, 0, 0, "SuperWndDraw", emitLog))
    {
        if (emitLog)
            s_superNativeDrawLogCount++;
    }
}

// ============================================================================
// 原生按钮创建（复刻 BtMacro 模式）
//
// 证据：
//   sub_66A770 是 __thiscall, ECX = SkillWndEx+0xBEC（控件容器地址）
//   参数：(resultBuf, off_资源路径, 控件ID, X偏移, Y偏移, 0, 0, alpha=0xFF)
//   asm 009E1B67~009E1B88 直接确认
//
// 这次改动会不会影响原本稳定逻辑：不会，新增独立按钮，不覆盖任何原有偏移
// 这次新增call的证据是否足够：A级，asm直接确认调用约定和参数
// this/ecx/edx/参数/返回值是否确认：ECX=lea容器地址，7个push参数，返回值[+4]是对象
// 新增了哪些空指针和时机保护：SkillWndThis非空、+0xBEC可读
// 仍不确定需补查的数据：无
// ============================================================================
static bool CreateSuperButton(uintptr_t skillWndThis)
{
    if (!skillWndThis)
        return false;

    for (int i = 0; i < 5; ++i)
    {
        InterlockedExchange(&g_SuperBtnMetricOverrideX[i], LONG_MIN);
        InterlockedExchange(&g_SuperBtnMetricOverrideY[i], LONG_MIN);
    }

    // 控件容器 = SkillWndEx + 0xBEC（lea，不是解引用）
    uintptr_t ctrlContainer = skillWndThis + 0xBEC;

    // 安全检查：控件容器的前3个DWORD应该已被 sub_6688B0 初始化
    if (SafeIsBadReadPtr((void *)ctrlContainer, 12))
    {
        WriteLog("[NativeBtn] FAIL: ctrl container unreadable");
        return false;
    }
    // 检查 ctrlContainer[0] 应该是父窗口指针（即 SkillWndThis 本身）
    DWORD parentPtr = *(DWORD *)ctrlContainer;
    if (parentPtr != (DWORD)skillWndThis)
    {
        WriteLogFmt("[NativeBtn] WARNING: container[0]=%08X != this=%08X, continue anyway",
                    parentPtr, (DWORD)skillWndThis);
    }

    DWORD createdObj = 0;
    const unsigned short *usedPath = reinterpret_cast<const unsigned short *>(SUPER_BTN_RES_PATH);
    bool createCallOk = CreateNativeButtonInstance(
        skillWndThis,
        usedPath,
        SUPER_BTN_ID,
        BTN_X_OFFSET,
        BTN_Y_OFFSET,
        false,
        &createdObj);

    if (createCallOk)
    {
        WriteLogFmt("[NativeBtn] primary create OK path=%S obj=0x%08X",
                    reinterpret_cast<const wchar_t *>(usedPath),
                    createdObj);
    }
    else
    {
        usedPath = reinterpret_cast<const unsigned short *>(SUPER_BTN_RES_PATH_ALT);
        createCallOk = CreateNativeButtonInstance(
            skillWndThis,
            usedPath,
            SUPER_BTN_ID,
            BTN_X_OFFSET,
            BTN_Y_OFFSET,
            false,
            &createdObj);
        if (createCallOk)
        {
            WriteLogFmt("[NativeBtn] primary alt create OK path=%S obj=0x%08X",
                        reinterpret_cast<const wchar_t *>(usedPath),
                        createdObj);
        }
    }

    if (!createCallOk)
    {
        usedPath = reinterpret_cast<const unsigned short *>(ADDR_OFF_SkillEx_BtMacro);
        createCallOk = CreateNativeButtonInstance(
            skillWndThis,
            usedPath,
            SUPER_BTN_ID,
            BTN_X_OFFSET,
            BTN_Y_OFFSET,
            true,
            &createdObj);
        if (!createCallOk)
        {
            WriteLogFmt("[NativeBtn] create returned null obj path=%S",
                        reinterpret_cast<const wchar_t *>(usedPath));
            return false;
        }
        WriteLogFmt("[NativeBtn] fallback BtMacro create OK path=%S obj=0x%08X",
                    reinterpret_cast<const wchar_t *>(usedPath),
                    createdObj);
    }

    g_SuperBtnObj = createdObj;
    if (!g_SuperBtnObj)
    {
        WriteLog("[NativeBtn] FAIL: resultBuf[1] is NULL");
        return false;
    }

    WriteLogFmt("[NativeBtn] OK: obj=0x%08X", (DWORD)g_SuperBtnObj);
    WriteLogFmt("[NativeBtn] basePath=%S", reinterpret_cast<const wchar_t *>(usedPath));
    LogNativeButtonCoreFields(g_SuperBtnObj, "BtnCoreCreate");

    __try
    {
        ((tRefreshButtonState)ADDR_5095A0)(reinterpret_cast<DWORD *>(g_SuperBtnObj), nullptr);
        WriteLogFmt("[NativeBtn] state refresh OK obj=0x%08X", (DWORD)g_SuperBtnObj);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        WriteLogFmt("[NativeBtn] state refresh EXCEPTION obj=0x%08X code=0x%08X",
                    (DWORD)g_SuperBtnObj, GetExceptionCode());
    }

    if (ENABLE_SUPERBTN_D3D_BUTTON_MODE)
    {
        MoveNativeButtonRaw(g_SuperBtnObj, -4096, -4096, "BtnHideD3D");
        ResetSuperBtnD3DInteractionState();
    }
    else
    {
        MoveNativeButtonRaw(g_SuperBtnObj, BTN_X_OFFSET, BTN_Y_OFFSET, "BtnMoveCreate");
    }
    SeedSuperBtnMetricOverridesIfEmpty(BTN_METRIC_FALLBACK_X, BTN_METRIC_FALLBACK_Y, "BtnMetricSeedCreate");
    if (ENABLE_SUPERBTN_SELF_DRAWOBJ_PATCH)
    {
        PatchSuperBtnOwnDrawObjectsFromResources(g_SuperBtnObj, "create");
    }
    if (ENABLE_SUPERBTN_RUNTIME_WRAPPER_PATCH)
    {
        PatchSuperBtnCurrentWrapperFromResources(g_SuperBtnObj, "create");
    }

    if (!ENABLE_SUPERBTN_D3D_BUTTON_MODE && (ENABLE_SUPERBTN_STATE_DRAWOBJ_OVERRIDE || ENABLE_SUPERBTN_DRAWOBJ_AB_FALLBACK || ENABLE_SUPERBTN_NATIVE_DONOR_DRAWOBJ) &&
        (usedPath == reinterpret_cast<const unsigned short *>(SUPER_BTN_RES_PATH) ||
         usedPath == reinterpret_cast<const unsigned short *>(SUPER_BTN_RES_PATH_ALT)))
    {
        DWORD compareObj = 0;
        if (CreateNativeButtonInstance(
                skillWndThis,
                reinterpret_cast<const unsigned short *>(ADDR_OFF_SkillEx_BtMacro),
                SUPER_BTN_ID + 1,
                BTN_X_OFFSET,
                BTN_Y_OFFSET,
                false,
                &compareObj) &&
            compareObj)
        {
            g_SuperBtnSkinDonorObj = compareObj;
            for (int i = 0; i < 5; ++i)
            {
                g_SuperBtnStateDonorObj[i] = 0;
                g_SuperBtnStateDonorPatched[i] = false;
            }
            DWORD compareState = 0;
            if (!SafeIsBadReadPtr((void *)(compareObj + 0x34), 4))
                compareState = *(DWORD *)(compareObj + 0x34);
            if (compareState < 5 && oButtonMetric507DF0 && oButtonMetric507ED0)
            {
                int mx = 0;
                int my = 0;
                __try
                {
                    mx = oButtonMetric507DF0(compareObj);
                    my = oButtonMetric507ED0(compareObj);
                    if (IsReasonableButtonMetric(mx) && IsReasonableButtonMetric(my))
                    {
                        InterlockedExchange(&g_SuperBtnMetricOverrideX[compareState], mx);
                        InterlockedExchange(&g_SuperBtnMetricOverrideY[compareState], my);
                        WriteLogFmt("[BtnMetricPrime] state=%u x=%d y=%d", compareState, mx, my);
                    }
                    else
                    {
                        WriteLogFmt("[BtnMetricPrimeSkip] state=%u x=%d y=%d", compareState, mx, my);
                    }
                }
                __except (EXCEPTION_EXECUTE_HANDLER)
                {
                    WriteLogFmt("[BtnMetricPrime] EXCEPTION state=%u code=0x%08X", compareState, GetExceptionCode());
                }
            }
            g_SuperBtnCompareObj = compareObj;
            if (ENABLE_DEBUG_VISIBLE_COMPARE_BUTTON)
            {
                MoveNativeButtonRaw(compareObj, BTN_X_OFFSET + BTN_COMPARE_DEBUG_DX, BTN_Y_OFFSET, "BtnCompareDebug");
            }
            else
            {
                MoveNativeButtonRaw(compareObj, -4096, -4096, "BtnCompareHide");
                if (!ENABLE_SUPERBTN_NATIVE_DONOR_DRAWOBJ)
                {
                    // compare按钮只用于取一次原版metric，隐藏后不再参与跟踪，避免离屏值持续污染缓存。
                    g_SuperBtnSkinDonorObj = 0;
                }
            }
            LogNativeButtonCoreFields(compareObj, "BtnCoreCompareBtMacro");
            if (ENABLE_SUPERBTN_NATIVE_DONOR_DRAWOBJ)
            {
                for (DWORD donorState = 0; donorState <= 4; ++donorState)
                {
                    DWORD donorObj = 0;
                    if (CreateNativeButtonInstance(
                            skillWndThis,
                            reinterpret_cast<const unsigned short *>(ADDR_OFF_SkillEx_BtMacro),
                            SUPER_BTN_ID + 1 + donorState,
                            BTN_X_OFFSET,
                            BTN_Y_OFFSET,
                            false,
                            &donorObj) &&
                        donorObj)
                    {
                        g_SuperBtnStateDonorObj[donorState] = donorObj;
                        MoveNativeButtonRaw(donorObj, -4096, -4096, "BtnDonorHide");
                        WriteLogFmt("[BtnDonorCreate] state=%u obj=0x%08X", donorState, donorObj);
                    }
                    else
                    {
                        WriteLogFmt("[BtnDonorCreate] state=%u FAILED", donorState);
                    }
                }
                if (!g_SuperBtnStateDonorObj[0])
                {
                    g_SuperBtnStateDonorObj[0] = compareObj;
                    WriteLogFmt("[BtnDonorCreate] state=0 FALLBACK compare=0x%08X", compareObj);
                }
                PatchSuperBtnDonorDrawObjectsFromResources();
                // v17.6: 把所有 5 个 slot 钉成 normal donor，然后启用 stableNormal 模式。
                // 这样 hkButtonRefreshState5095A0 会拦截后续所有状态刷新（包括 hover），
                // 按钮保持 state=0，draw 链不会被破坏。
                if (g_SuperBtnObj)
                {
                    ForceSuperButtonAllStatesToNormalDonor(g_SuperBtnObj);
                }
                // v17.7b: slot 值已经复制到 SuperBtn，现在把 donor/compare 宽高清零
                // 防止 sub_529640 在 UI tree 遍历时为 donor 生成 -4096 矩形覆盖 SuperBtn
                for (int di = 0; di < 5; ++di)
                {
                    uintptr_t dObj = g_SuperBtnStateDonorObj[di];
                    if (dObj)
                    {
                        __try
                        {
                            if (!SafeIsBadReadPtr((void *)(dObj + 0x1C), 8))
                            {
                                *(DWORD *)(dObj + 0x1C) = 0;
                                *(DWORD *)(dObj + 0x20) = 0;
                            }
                        }
                        __except (EXCEPTION_EXECUTE_HANDLER)
                        {
                        }
                    }
                }
                if (g_SuperBtnCompareObj)
                {
                    __try
                    {
                        uintptr_t cObj = g_SuperBtnCompareObj;
                        if (!SafeIsBadReadPtr((void *)(cObj + 0x1C), 8))
                        {
                            *(DWORD *)(cObj + 0x1C) = 0;
                            *(DWORD *)(cObj + 0x20) = 0;
                        }
                    }
                    __except (EXCEPTION_EXECUTE_HANDLER)
                    {
                    }
                }
                WriteLog("[BtnDonorPostPatch] donor+compare wh zeroed");
            }
        }
        else
        {
            WriteLog("[BtnCoreCompareBtMacro] create FAILED");
        }
    }

    LogSuperButtonGeometry("BtnGeomCreate");

    g_NativeBtnCreated = true;
    return true;
}

// ============================================================================
// 原生子窗口创建（v12.0：改走 SkillWndEx 官方 second-child 包装链）
//
// 证据：
// 1. 9DDB30 对 3001~3004 直接走 9DC220(this, a2-750)
// 2. 9DC220 会分配 0x84，调用 9DB2B0，并把 child 存到 this+3048
// 3. CE 已证明 generic child 从未进入 SkillWnd 的 +3048 槽位，这是它和真实 child 的核心差异
// 4. 因此这次不再自己 gameMalloc+family ctor，而是直接复用官方 second-child create/replace 包装链
// ============================================================================
static uintptr_t GetSkillWndSecondChildPtr(uintptr_t skillWndThis)
{
    if (!skillWndThis || SafeIsBadReadPtr((void *)(skillWndThis + 3048), 4))
        return 0;
    return *(DWORD *)(skillWndThis + 3048);
}

static bool ReleaseSkillWndSecondChild(uintptr_t skillWndThis, const char *reason)
{
    if (!skillWndThis)
        return false;

    uintptr_t child = GetSkillWndSecondChildPtr(skillWndThis);
    if (!child)
        return false;

    RestoreSuperChildCustomMouseGuardVTable(child, reason ? reason : "release");
    LogOfficialSecondChildState(child, "Lifecycle:BeforeSecondChildRelease");

    WriteLogFmt("[Lifecycle] releasing second child (reason=%s) ptr=0x%08X",
                reason ? reason : "unknown", (DWORD)child);

    DWORD fnClose = ADDR_B9E880;
    __try
    {
        __asm {
            mov ecx, [child]
            call [fnClose]
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        WriteLogFmt("[Lifecycle] EXCEPTION in second-child close: 0x%08X", GetExceptionCode());
    }

    uintptr_t childAfterClose = GetSkillWndSecondChildPtr(skillWndThis);
    if (childAfterClose)
    {
        DWORD fnRelease = ADDR_9D93A0;
        uintptr_t wrapPtr = skillWndThis + 3044;
        int zero = 0;
        __try
        {
            __asm {
                push [zero]
                mov ecx, [wrapPtr]
                call [fnRelease]
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            WriteLogFmt("[Lifecycle] EXCEPTION in second-child release: 0x%08X", GetExceptionCode());
        }
        if (!SafeIsBadReadPtr((void *)(skillWndThis + 3048), 4))
        {
            *(DWORD *)(skillWndThis + 3048) = 0;
        }
    }

    return true;
}

static bool CreateSuperWnd(uintptr_t skillWndThis)
{
    if (!skillWndThis)
        return false;

    if (ENABLE_IMGUI_OVERLAY_PANEL)
    {
        if (!g_GameHwnd || !g_pDevice)
        {
            WriteLogFmt("[ImGuiOverlay] FAIL: hwnd/device not ready (hwnd=0x%08X device=0x%08X)", (DWORD)(uintptr_t)g_GameHwnd, (DWORD)(uintptr_t)g_pDevice);
            return false;
        }

        if (!SuperImGuiOverlayEnsureInitialized(g_GameHwnd, g_pDevice, 1.0f, IMGUI_PANEL_ASSET_PATH))
        {
            WriteLog("[ImGuiOverlay] FAIL: initialization failed");
            return false;
        }

        g_SuperCWnd = 1;
        g_NativeWndCreated = true;
        g_SuperUsesSkillWndSecondSlot = false;
        SuperImGuiOverlaySetVisible(false);
        WriteLog("[ImGuiOverlay] overlay route ready");
        return true;
    }

    if (!g_SuperChildHooksReady)
    {
        WriteLog("[NativeWnd] FAIL: route-B child hooks not ready");
        return false;
    }

    uintptr_t existingSecond = GetSkillWndSecondChildPtr(skillWndThis);
    if (existingSecond)
    {
        WriteLogFmt("[NativeWnd] second-child slot busy: ptr=0x%08X, abort create", (DWORD)existingSecond);
        return false;
    }

    // Step 1: 走 SkillWnd 官方 second-child 包装链。
    // 关键修正：
    //   9DDB30 伪代码把 ctrlID 反编译成 int* a2，"a2 - 750" 是按指针步长算的，
    //   所以 3001..3004 实际映射到的有效模式是 1..4，而不是 2251..2254。
    //   之前传 2251 会让 9DB2B0 在 sub_419110(..., (char*)a2 - 1) 这条链上直接异常。
    DWORD fnCreateSlot = ADDR_9DC220;
    int mode = SUPER_CHILD_DONOR_MODE;
    __try
    {
        __asm {
            push [mode]
            mov ecx, [skillWndThis]
            call [fnCreateSlot]
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        WriteLogFmt("[NativeWnd] EXCEPTION in second-child wrapper 0x%08X", GetExceptionCode());
        return false;
    }

    uintptr_t wndObj = GetSkillWndSecondChildPtr(skillWndThis);
    if (!wndObj)
    {
        WriteLog("[NativeWnd] FAIL: second-child wrapper returned null slot");
        return false;
    }

    if (SafeIsBadReadPtr((void *)wndObj, 0x84))
    {
        WriteLog("[NativeWnd] FAIL: second-child slot object unreadable");
        return false;
    }

    WriteLogFmt("[NativeWnd] official second-child OK: slot=0x%08X mode=%d", (DWORD)wndObj, mode);
    LogNativeChildSurfaceShape(wndObj, "NativeWndCtor");
    LogOfficialSecondChildState(wndObj, "NativeWndCtorState");

    // Step 2: 算初始锚点，并尝试把官方 second-child 的壳收缩到我们的目标尺寸。
    int swX = 0, swY = 0;
    bool fromVTable = GetSkillWndAnchorPos(skillWndThis, &swX, &swY);
    if (!fromVTable)
    {
        swX = CWnd_GetX(skillWndThis);
        swY = CWnd_GetY(skillWndThis);
    }
    int xPos = swX - PANEL_W - PANEL_LEFT_GAP;
    int yPos = swY;
    WriteLogFmt("[NativeWnd] second-child pos (%s): sw=(%d,%d) -> x=%d y=%d",
                fromVTable ? "vtable" : "com", swX, swY, xPos, yPos);

    RebuildNativeChildSurface(wndObj, xPos, yPos, PANEL_W, PANEL_H, "NativeWndSecondSlotResize");
    LogNativeChildSurfaceShape(wndObj, "NativeWndInitSecondSlot");

    // Step 3: 同步官方 0x84 child 内部安全坐标，避免 draw/move 初期取到旧值。
    // 注意：official second-child 不是 A996B0-family，不能写 +2756/+2760 home 坐标。
    CWnd_SetRenderPos(wndObj, xPos, yPos);
    CWnd_SetComPos(wndObj, xPos, yPos);

    // Step 4: 替换 VT1 draw 槽位，接入我们的面板绘制
    if (!ApplySuperChildCustomDrawVTable(wndObj))
    {
        WriteLog("[NativeWnd] FAIL: custom draw vtable install failed");
        ReleaseSkillWndSecondChild(skillWndThis, "custom_vt_fail");
        return false;
    }
    if (!ApplySuperChildCustomMouseGuardVTable(wndObj))
    {
        WriteLog("[NativeWnd] WARN: custom VT2 mouse guard install failed");
    }

    // Step 5: 再补一次 move，确保初始化后逻辑位置与我们的锚点一致
    MoveNativeChildWnd(wndObj, xPos, yPos, "NativeWndInitMove");

    // Step 6: official second-child 没有可靠 show/hide 槽位；收起时走 close+release。
    MarkSuperWndDirty(wndObj, "NativeWndInit");
    LogOfficialSecondChildState(wndObj, "NativeWndAfterInit");

    g_SuperCWnd = wndObj;
    g_NativeWndCreated = true;
    g_SuperUsesSkillWndSecondSlot = true;
    WriteLogFmt("[NativeWnd] === SUCCESS(second_child_slot): 0x%08X ===", (DWORD)wndObj);
    return true;
}

static void SetSuperWndVisible(uintptr_t wndObj, int showVal)
{
    if (ENABLE_IMGUI_OVERLAY_PANEL)
    {
        SuperImGuiOverlaySetVisible(showVal != 0);
        return;
    }

    if (!wndObj)
        return;

    if (IsOfficialSecondChildObject(wndObj, true, true))
    {
        static int s_officialSecondVisibleNoopLogCount = 0;
        if (s_officialSecondVisibleNoopLogCount < 8)
        {
            WriteLogFmt("[Visible] official second-child has no reliable show/hide slot; no-op show=%d wnd=0x%08X",
                        showVal, (DWORD)wndObj);
            s_officialSecondVisibleNoopLogCount++;
        }
        return;
    }

    uintptr_t thisForVT2 = wndObj + 4; // 与sub_9E9B50一致
    if (SafeIsBadReadPtr((void *)thisForVT2, 4))
        return;

    DWORD vtable2 = *(DWORD *)thisForVT2;
    if (!vtable2 || SafeIsBadReadPtr((void *)(vtable2 + 0x28), 4))
        return;

    DWORD fnShow = *(DWORD *)(vtable2 + 0x28);
    DWORD fnVis = *(DWORD *)(vtable2 + 0x20);
    DWORD ecxVal = (DWORD)thisForVT2;

    if (!fnShow || !fnVis)
        return;

    __try
    {
        __asm {
            push [showVal]
            mov ecx, [ecxVal]
            call [fnShow]
        }
        __asm
        {
            push [showVal]
            mov ecx, [ecxVal]
            call [fnVis]
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        WriteLogFmt("[Visible] EXCEPTION: 0x%08X", GetExceptionCode());
    }
}

static void SafeCloseSuperWnd(const char *reason)
{
    if (ENABLE_IMGUI_OVERLAY_PANEL)
    {
        WriteLogFmt("[Lifecycle] hiding imgui overlay (reason=%s)", reason ? reason : "unknown");
        SuperImGuiOverlaySetVisible(false);
        return;
    }

    if (!g_SuperCWnd)
        return;

    uintptr_t oldWnd = g_SuperCWnd;
    WriteLogFmt("[Lifecycle] closing super wnd (reason=%s) ptr=0x%08X",
                reason ? reason : "unknown", (DWORD)oldWnd);

    SetSuperWndVisible(oldWnd, 0);

    if (g_SuperUsesSkillWndSecondSlot && g_SkillWndThis)
    {
        uintptr_t slotChild = GetSkillWndSecondChildPtr(g_SkillWndThis);
        if (slotChild == oldWnd)
        {
            ReleaseSkillWndSecondChild(g_SkillWndThis, reason ? reason : "unknown");
            return;
        }
    }

    if (!SafeIsBadReadPtr((void *)oldWnd, 4))
    {
        DWORD fnClose = ADDR_B9E880;
        __try
        {
            __asm {
                mov ecx, [oldWnd]
                call [fnClose]
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            WriteLogFmt("[Lifecycle] EXCEPTION in close: 0x%08X", GetExceptionCode());
        }
    }
}

static void DestroySuperWndOnly(const char *reason)
{
    if (ENABLE_IMGUI_OVERLAY_PANEL)
    {
        SetSuperWndVisible(g_SuperCWnd, 0);
        g_PanelDrawX = -9999;
        g_PanelDrawY = -9999;
        return;
    }

    if (!g_SuperCWnd)
        return;
    SafeCloseSuperWnd(reason);
    g_SuperCWnd = 0;
    g_NativeWndCreated = false;
    g_SuperUsesSkillWndSecondSlot = false;
    g_PanelDrawX = -9999;
    g_PanelDrawY = -9999;
}

static void ResetSuperRuntimeState(bool closeWnd, const char *reason)
{
    if (ENABLE_IMGUI_OVERLAY_PANEL)
    {
        if (g_IsD3D8Mode)
        {
            SuperD3D8OverlaySetVisible(false);
            SuperD3D8OverlayResetPanelState();
        }
        else
        {
            SuperImGuiOverlaySetVisible(false);
            SuperImGuiOverlayResetPanelState();
        }
    }

    if (closeWnd && g_SuperCWnd)
    {
        SafeCloseSuperWnd(reason);
    }

    g_SuperExpanded = false;
    g_LastToggleTick = 0;
    g_LastNativeMsgToggleTick = 0;
    g_LastFallbackHitLogTick = 0;
    g_LastSkillWndSeenTick = 0;
    g_PanelDrawX = -9999;
    g_PanelDrawY = -9999;

    g_SuperBtnObj = 0;
    g_SuperBtnSkinDonorObj = 0;
    g_SuperBtnCompareObj = 0;
    g_SuperBtnForcedStableNormalMode = false;
    for (int i = 0; i < 5; ++i)
    {
        g_SuperBtnSelfStatePatched[i] = false;
    }
    for (int i = 0; i < 5; ++i)
    {
        g_SuperBtnStateDonorObj[i] = 0;
        g_SuperBtnStateDonorPatched[i] = false;
        g_SuperBtnStateDonorRetryTick[i] = 0;
    }
    g_SuperCWnd = 0;
    g_NativeBtnCreated = false;
    ResetSuperBtnD3DInteractionState();
    g_NativeWndCreated = false;
    g_SuperUsesSkillWndSecondSlot = false;
}

static void OnSkillWndPointerObserved(uintptr_t observed, const char *srcTag)
{
    DWORD now = GetTickCount();
    if (observed && SafeIsBadReadPtr((void *)observed, 0x20))
    {
        observed = 0;
    }

    if (observed == g_SkillWndThis)
    {
        if (observed)
            g_LastSkillWndSeenTick = now;
        return;
    }

    if (!observed)
    {
        if (g_SkillWndThis && g_LastSkillWndSeenTick && (now - g_LastSkillWndSeenTick) < SKILLWND_GONE_DEBOUNCE_MS)
        {
            return;
        }
        if (g_SkillWndThis)
        {
            WriteLogFmt("[Lifecycle] SkillWnd gone (src=%s old=0x%08X)",
                        srcTag ? srcTag : "unknown", (DWORD)g_SkillWndThis);
            ResetSuperRuntimeState(g_SuperCWnd != 0, "skillwnd_gone");
        }
        g_SkillWndThis = 0;
        SkillOverlayBridgeSetSkillWnd(0);
        g_Ready = false;
        return;
    }

    if (g_SkillWndThis && g_SkillWndThis != observed)
    {
        WriteLogFmt("[Lifecycle] SkillWnd switched (src=%s old=0x%08X new=0x%08X)",
                    srcTag ? srcTag : "unknown", (DWORD)g_SkillWndThis, (DWORD)observed);
        ResetSuperRuntimeState(g_SuperCWnd != 0, "skillwnd_switched");
    }

    g_SkillWndThis = observed;
    SkillOverlayBridgeSetSkillWnd(g_SkillWndThis);
    g_Ready = true;
    g_LastSkillWndSeenTick = now;

    if (g_IsD3D8Mode && ENABLE_IMGUI_OVERLAY_PANEL)
        EnsureDeferredInteractionHooks("skillwnd_ready");
}

// ============================================================================
// 切换超级技能栏窗口显示/隐藏
// 复刻 sub_9E9B50 的逻辑
// ============================================================================
static void ToggleSuperWnd(const char *srcTag)
{
    if (!g_SkillWndThis)
        return;

    DWORD now = GetTickCount();
    if (now - g_LastToggleTick < 120)
    {
        return;
    }
    g_LastToggleTick = now;

    g_SuperExpanded = !g_SuperExpanded;
    WriteLogFmt("[Toggle:%s] expanded=%d", srcTag ? srcTag : "unknown", g_SuperExpanded);

    if (ENABLE_IMGUI_OVERLAY_PANEL)
    {
        if (g_IsD3D8Mode)
            SuperD3D8OverlaySetPanelExpanded(g_SuperExpanded);
        else
            SuperImGuiOverlaySetPanelExpanded(g_SuperExpanded);
    }

    // D3D8 mode owns the shared ImGui panel from hkD3D8Present.
    // It does not need the D3D9 CreateSuperWnd route or a native child window.
    if (g_IsD3D8Mode && ENABLE_IMGUI_OVERLAY_PANEL)
    {
        WriteLogFmt("[Toggle] D3D8 mode: panel %s", g_SuperExpanded ? "ON" : "OFF");
        return;
    }

    if (g_SuperExpanded && !g_NativeWndCreated)
    {
        WriteLog(ENABLE_IMGUI_OVERLAY_PANEL ? "[Toggle] creating imgui overlay panel..." : "[Toggle] creating official second-slot super child...");
        if (CreateSuperWnd(g_SkillWndThis))
        {
            WriteLog(ENABLE_IMGUI_OVERLAY_PANEL ? "[Toggle] imgui overlay panel ready" : "[Toggle] official second-slot super child created OK");
        }
        else
        {
            WriteLog(ENABLE_IMGUI_OVERLAY_PANEL ? "[Toggle] imgui overlay panel create FAILED" : "[Toggle] official second-slot super child create FAILED");
            g_SuperExpanded = false;
            g_PanelDrawX = -9999;
            g_PanelDrawY = -9999;
            WriteLog("[Toggle] rollback: create failed, expanded reset to 0");
            return;
        }
    }
    if (g_SuperCWnd)
    {
        SetSuperWndVisible(g_SuperCWnd, g_SuperExpanded ? 1 : 0);
    }
    if (g_SuperExpanded)
    {
        UpdateSuperCWnd();
        if (ENABLE_TOGGLE_FOCUS_SYNC)
        {
            SyncSkillWndActiveFocus("ToggleFocusSync", true);
        }
    }
    if (!g_SuperExpanded)
    {
        DestroySuperWndOnly("toggle_hide");
    }
}

#if defined(SSW_ENABLE_SECOND_CHILD_CARRIER_PROBE_RUNTIME)
static void RunSecondChildCarrierProbeHotkey()
{
    if (!g_SkillWndThis)
    {
        WriteLog("[CarrierProbe] F10 ignored: SkillWnd not ready");
        return;
    }

    WriteLogFmt("[CarrierProbe] F10 run-once skillWnd=0x%08X flags=0x%08X",
                (DWORD)g_SkillWndThis,
                SECOND_CHILD_CARRIER_PROBE_FLAGS);
    SSW_SecondChildCarrierProbe_RunOnce((DWORD)g_SkillWndThis, SECOND_CHILD_CARRIER_PROBE_FLAGS, -9999, -9999);
}

static void PollSecondChildCarrierProbeTick(DWORD reasonCode, bool force)
{
    if (!g_SkillWndThis)
        return;

    DWORD now = GetTickCount();
    if (!force && (now - g_LastCarrierProbePollTick) < 250)
        return;

    g_LastCarrierProbePollTick = now;
    SSW_SecondChildCarrierProbe_Poll((DWORD)g_SkillWndThis, reasonCode);
}

static void ReleaseSecondChildCarrierProbeHotkey()
{
    if (!g_SkillWndThis)
    {
        WriteLog("[CarrierProbe] F12 ignored: SkillWnd not ready");
        return;
    }

    WriteLogFmt("[CarrierProbe] F12 release skillWnd=0x%08X", (DWORD)g_SkillWndThis);
    SSW_SecondChildCarrierProbe_Release((DWORD)g_SkillWndThis);
}
#endif

// SkillWnd 核心运行时模块：负责面板生命周期、消息、移动、刷新、绘制和列表过滤。
static bool IsPointInRectPad(int mx, int my, int x, int y, int w, int h, int pad)
{
    return (mx >= (x - pad) && mx < (x + w + pad) &&
            my >= (y - pad) && my < (y + h + pad));
}

static bool TryToggleByMousePoint(int mx, int my, const char *srcTag)
{
    if (ENABLE_SUPERBTN_D3D_BUTTON_MODE)
        return false;
    if (!g_Ready || !g_NativeBtnCreated || !g_SkillWndThis)
        return false;

    DWORD now = GetTickCount();
    if (now - g_LastNativeMsgToggleTick < 180)
    {
        return false;
    }

    int bxObj = 0, byObj = 0, bwObj = 0, bhObj = 0;
    int bxCom = 0, byCom = 0, bwCom = 0, bhCom = 0;
    int bxVt = 0, byVt = 0, bwVt = 0, bhVt = 0;
    bool hasObj = GetSuperButtonScreenRect(&bxObj, &byObj, &bwObj, &bhObj);
    bool hasCom = GetExpectedButtonRectCom(&bxCom, &byCom, &bwCom, &bhCom);
    bool hasVt = GetExpectedButtonRectVt(&bxVt, &byVt, &bwVt, &bhVt);

    // resultBuf对象上的宽高有时不可信（曾出现高度=1），这里做一次兜底过滤
    if (hasObj && (bwObj < 20 || bhObj < 10 || bwObj > 256 || bhObj > 64))
    {
        hasObj = false;
    }

    const int kPad = 0;
    bool hitObj = hasObj && IsPointInRectPad(mx, my, bxObj, byObj, bwObj, bhObj, kPad);
    bool hitCom = !hitObj && hasCom && IsPointInRectPad(mx, my, bxCom, byCom, bwCom, bhCom, kPad);
    bool hitVt = !hitObj && !hitCom && hasVt && IsPointInRectPad(mx, my, bxVt, byVt, bwVt, bhVt, kPad);
    if (!(hitObj || hitCom || hitVt))
    {
        static DWORD s_lastMissLogTick = 0;
        if (now - s_lastMissLogTick > 200)
        {
            s_lastMissLogTick = now;
            WriteLogFmt("[BtnMiss:%s] mx=%d my=%d obj=%s(%d,%d,%d,%d) com=%s(%d,%d,%d,%d) vt=%s(%d,%d,%d,%d)",
                        srcTag ? srcTag : "unknown", mx, my,
                        hasObj ? "Y" : "N", bxObj, byObj, bxObj + bwObj, byObj + bhObj,
                        hasCom ? "Y" : "N", bxCom, byCom, bxCom + bwCom, byCom + bhCom,
                        hasVt ? "Y" : "N", bxVt, byVt, bxVt + bwVt, byVt + bhVt);
        }
        return false;
    }

    if (now - g_LastFallbackHitLogTick > 120)
    {
        g_LastFallbackHitLogTick = now;
        WriteLogFmt("[BtnHit:%s] mx=%d my=%d obj=%s(%d,%d,%d,%d) com=%s(%d,%d,%d,%d) vt=%s(%d,%d,%d,%d)",
                    srcTag ? srcTag : "unknown", mx, my,
                    hitObj ? "HIT" : "no", bxObj, byObj, bxObj + bwObj, byObj + bhObj,
                    hitCom ? "HIT" : "no", bxCom, byCom, bxCom + bwCom, byCom + bhCom,
                    hitVt ? "HIT" : "no", bxVt, byVt, bxVt + bwVt, byVt + bhVt);
    }

    // 与原生消息路径共用节流时间戳，避免同一点击在不同路径重复toggle
    g_LastNativeMsgToggleTick = now;
    ToggleSuperWnd("fallback_hit");
    return true;
}

// ============================================================================
// SkillWndEx子控件初始化Hook（sub_9E17D0）
// 策略：调用原函数后，追加创建我们的按钮和窗口
//
// sub_9E17D0: __thiscall(ecx=SkillWndEx, push a2), void, retn 4
// ============================================================================
typedef void(__thiscall *tSkillWndInitChildren)(uintptr_t thisptr, int **a2);
static tSkillWndInitChildren oSkillWndInitChildren = nullptr;

static void __cdecl hkSkillWndPostInit(uintptr_t skillWndThis)
{
    OnSkillWndPointerObserved(skillWndThis, "hook_postinit");
    WriteLogFmt("[Hook] SkillWndEx captured: 0x%08X", (DWORD)g_SkillWndThis);

    // 创建原生按钮
    if (!g_NativeBtnCreated)
    {
        WriteLog("[Hook] Creating native button...");
        if (CreateSuperButton(g_SkillWndThis))
        {
            WriteLog("[Hook] Native button created OK");
        }
        else
        {
            WriteLog("[Hook] Native button creation FAILED");
        }
    }

    if (!g_NativeWndCreated)
    {
        WriteLog("[Hook] Super child deferred: create on first toggle");
    }
}

// naked thunk: 保存ecx→调用原函数→调用post-init→恢复栈→ret
__declspec(naked) static void hkSkillWndInitChildren()
{
    __asm {
        // 保存寄存器
        push ebp
        mov ebp, esp
        push esi
        push edi
        mov esi, ecx // esi = SkillWndEx this

            // 调用原函数：__thiscall(ecx=this, push a2), retn 4
            // a2 在 [ebp+8] (因为我们push了ebp，原来的[esp+4]变成[ebp+8])
        mov eax, [ebp + 8] // a2
        push eax
        mov ecx, esi
        call [oSkillWndInitChildren]

        // 调用post-init回调（__cdecl, push this）
        push esi
        call hkSkillWndPostInit
        add esp, 4

        // 恢复并返回（原函数是 retn 4，我们也要 retn 4）
        pop edi
        pop esi
        pop ebp
        ret 4
    }
}

// ============================================================================
// 消息处理Hook（sub_9DDB30）
// __thiscall(ecx=SkillWndEx, push ctrlID), void, retn 4
// ============================================================================
typedef void(__thiscall *tSkillWndMsg)(uintptr_t thisptr, int ctrlID);
static tSkillWndMsg oSkillWndMsg = nullptr;

static void __cdecl hkMsgHandler(uintptr_t thisPtr, int ctrlID)
{
    DWORD now = GetTickCount();
    if (ctrlID != g_LastMsgID || (now - g_LastMsgTick) > 400)
    {
        WriteLogFmt("[Msg] ctrlID=0x%X this=0x%08X", ctrlID, (DWORD)thisPtr);
        g_LastMsgID = ctrlID;
        g_LastMsgTick = now;
    }

    if ((DWORD)ctrlID == SUPER_BTN_ID)
    {
        WriteLogFmt("[Msg] Super button clicked (ID=0x%X)", ctrlID);
        // WndProc fallback 可能已经在同一点击里先切过一次；这里直接复用节流时间戳，避免二次翻转
        bool skipToggle = ((now - g_LastNativeMsgToggleTick) < 180) || ((now - g_LastToggleTick) < 120);
        if (skipToggle)
        {
            if (now - g_LastNativeMsgSkipTick > 200)
            {
                g_LastNativeMsgSkipTick = now;
                WriteLog("[Msg] Super button native toggle skipped (already handled by fallback)");
            }
        }
        else
        {
            g_LastNativeMsgToggleTick = now;
            ToggleSuperWnd("native_msg");
        }

        // 调用 sub_A99550 消费消息（__thiscall, ecx=SkillWndEx, push ctrlID）
        DWORD fnConsume = ADDR_A99550;
        uintptr_t thisVal = thisPtr;
        int id = ctrlID;
        __asm {
            push [id]
            mov ecx, [thisVal]
            call [fnConsume]
        }
        return;
    }

    // 其他消息交给原函数
    DWORD fnOrig = (DWORD)oSkillWndMsg;
    uintptr_t thisVal = thisPtr;
    int idVal = ctrlID;
    __asm {
        push [idVal]
        mov ecx, [thisVal]
        call [fnOrig]
    }
}

__declspec(naked) static void hkSkillWndMsgNaked()
{
    __asm {
        // sub_9ECFD0: ecx=this, [esp+4]=ctrlID, retn 4
        mov eax, [esp + 4] // ctrlID
        push eax
        push ecx // this
        call hkMsgHandler
        add esp, 8
        ret 4
    }
}

// ============================================================================
// SkillWndEx 移动 hook（sub_9D95A0）
// 证据：
//   9D95A0 是 SkillWndEx 父窗移动时，原生同步 Macro child 的入口
//   原函数最终调用 sub_56D630(child, parentX+174, parentY)
// ============================================================================
typedef LONG(__thiscall *tSkillWndMove)(uintptr_t thisptr, int a2, int a3);
static tSkillWndMove oSkillWndMove = nullptr;

static LONG __cdecl hkSkillWndMoveHandler(uintptr_t thisPtr, int a2, int a3)
{
    if (thisPtr == g_SkillWndThis && g_SuperExpanded && g_SuperCWnd && g_SuperUsesSkillWndSecondSlot)
    {
        LogOfficialSecondChildState(g_SuperCWnd, "MoveHook:BeforeOrig");
    }

    LONG ret = oSkillWndMove ? oSkillWndMove(thisPtr, a2, a3) : 0;
    if (thisPtr == g_SkillWndThis && g_SuperExpanded && g_SuperCWnd)
    {
        if (g_SuperUsesSkillWndSecondSlot)
        {
            LogOfficialSecondChildState(g_SuperCWnd, "MoveHook:AfterOrig");
        }
        MoveSuperChildBySkillAnchor("MoveHookDirect", false);
        if (g_SuperUsesSkillWndSecondSlot)
        {
            LogOfficialSecondChildState(g_SuperCWnd, "MoveHook:AfterRetarget");
        }
        if (ENABLE_MOVE_FOCUS_SYNC)
        {
            SyncSkillWndActiveFocus("MoveFocusSync");
        }
    }
    return ret;
}

__declspec(naked) static void hkSkillWndMoveNaked()
{
    __asm {
        mov eax, [esp + 8]
        mov edx, [esp + 4]
        push eax
        push edx
        push ecx
        call hkSkillWndMoveHandler
        add esp, 12
        ret 8
    }
}

// ============================================================================
// SkillWndEx refresh hook（sub_9E1770）
// 证据：
//   9E1770 是 SkillWndEx 刷新 helper，末尾会 B9A5D0(0)
//   用它做“父窗静止但内部刷新后”的兜底位置同步
// ============================================================================
typedef int(__thiscall *tSkillWndRefresh)(uintptr_t thisptr);
static tSkillWndRefresh oSkillWndRefresh = nullptr;

static int __cdecl hkSkillWndRefreshHandler(uintptr_t thisPtr)
{
    if (thisPtr == g_SkillWndThis && g_SuperExpanded && g_SuperCWnd && g_SuperUsesSkillWndSecondSlot)
    {
        LogOfficialSecondChildState(g_SuperCWnd, "RefreshHook:BeforeOrig");
    }

    int ret = oSkillWndRefresh ? oSkillWndRefresh(thisPtr) : 0;
    if (thisPtr == g_SkillWndThis && g_SuperExpanded && g_SuperCWnd)
    {
        if (g_SuperUsesSkillWndSecondSlot)
        {
            LogOfficialSecondChildState(g_SuperCWnd, "RefreshHook:AfterOrig");
        }
        if (ENABLE_REFRESH_NATIVE_CHILD_UPDATE)
        {
            UpdateSuperCWnd();
        }
    }
    return ret;
}

__declspec(naked) static void hkSkillWndRefreshNaked()
{
    __asm {
        push ecx
        call hkSkillWndRefreshHandler
        add esp, 4
        ret
    }
}

// ============================================================================
// SkillWndEx 绘制 Hook（sub_9DEE30）
// 证据：
//   sub_9DEE30 是 __thiscall(ecx=this, push clipRegion), retn 4
//   asm 009DEE63~009DEE69 直接确认 ecx=this, [esp+4]=clip 参数
//   原函数一开始先 sub_B9B800(a2)，然后在同一帧继续绘制 SkillWnd 内容
//
// 这次改动会不会影响原本稳定逻辑：只在 SkillWnd 原函数返回后追加画我们自己的扩展层
// 这次新增 call 的证据是否足够：A级，asm/pseudo 直接确认 9DEE30 调用约定和时机
// this / ecx / edx / 参数 / 返回值 是否确认：确认，返回 int，retn 4
// 这次新增了哪些空指针和时机保护：仅当 this==当前 SkillWnd、expanded=1、device/texture 可用时绘制
// 目前仍不确定、需要我补查的数据：技能栏上游 hit-test/hover 入口仍需继续补
// ============================================================================
typedef int(__thiscall *tSkillWndDraw)(uintptr_t thisptr, int clipRegion);
static tSkillWndDraw oSkillWndDraw = nullptr;

static bool DrawSuperPanelNativeBackgrnd(uintptr_t skillWndThis)
{
    if (!skillWndThis || !g_SuperExpanded)
        return false;
    if (g_SuperChildHooksReady)
        return false; // v11.1: route-B 是主路线，创建失败时 fail-closed，不再偷偷退回 fallback surface draw
    if (g_NativeWndCreated && g_SuperCWnd)
        return false;

    UpdateSuperCWnd();
    if (g_PanelDrawX <= -9000 || g_PanelDrawY <= -9000)
        return false;

    DWORD *surface = nullptr;
    __try
    {
        ((tGetSurface)ADDR_435A50)(skillWndThis, &surface);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        WriteLog("[SkillDrawNative] EXCEPTION: sub_435A50");
        return false;
    }
    if (!surface)
    {
        WriteLog("[SkillDrawNative] FAIL: surface null");
        return false;
    }

    int skillComX = 0, skillComY = 0;
    bool hasSkillCom = GetSkillWndComPos(skillWndThis, &skillComX, &skillComY);
    int localX = g_PanelDrawX;
    int localY = g_PanelDrawY;
    if (hasSkillCom)
    {
        localX = g_PanelDrawX - skillComX;
        localY = g_PanelDrawY - skillComY;
    }

    bool ok = DrawNativePanelOnSurface(surface, localX, localY, "SkillDrawNative", false);

    static int s_nativeDrawLogCount = 0;
    if (s_nativeDrawLogCount < 60)
    {
        WriteLogFmt("[SkillDrawNative] %s screen=(%d,%d) local=(%d,%d) skillCom=%s(%d,%d) surface=0x%08X",
                    ok ? "OK" : "FAIL",
                    g_PanelDrawX, g_PanelDrawY,
                    localX, localY,
                    hasSkillCom ? "Y" : "N", skillComX, skillComY,
                    (DWORD)surface);
        s_nativeDrawLogCount++;
    }
    return ok;
}

static void DrawSuperPanelInSkillWnd(uintptr_t skillWndThis)
{
    if (ENABLE_IMGUI_OVERLAY_PANEL)
        return;
    if (!skillWndThis || !g_SuperExpanded)
        return;

    bool nativeOk = DrawSuperPanelNativeBackgrnd(skillWndThis);

    static int s_drawHookLogCount = 0;
    if (s_drawHookLogCount < 40)
    {
        WriteLogFmt("[SkillDraw] native=%d panel=(%d,%d) skill=0x%08X",
                    nativeOk ? 1 : 0, g_PanelDrawX, g_PanelDrawY, (DWORD)skillWndThis);
        s_drawHookLogCount++;
    }
}

static int __cdecl hkSkillWndDrawHandler(uintptr_t thisPtr, int clipRegion)
{
    int ret = 0;
    SkillOverlayBridgeFilterNativeSkillWindow(thisPtr);
    if (oSkillWndDraw)
    {
        ret = oSkillWndDraw(thisPtr, clipRegion);
    }

    OnSkillWndPointerObserved(thisPtr, "skill_draw");

    // v17.6b diag: 确认 draw handler 是否持续被调，以及 thisPtr 是否匹配
    {
        static LONG s_drawHandlerCallCount = 0;
        LONG count = InterlockedIncrement(&s_drawHandlerCallCount);
        if (ENABLE_HOTPATH_DIAGNOSTIC_LOGS && (count <= 20 || (count % 500 == 0)))
        {
            WriteLogFmt("[DrawHandler] #%d this=0x%08X g_this=0x%08X match=%d btn=0x%08X created=%d",
                        (int)count, (DWORD)thisPtr, (DWORD)g_SkillWndThis,
                        thisPtr == g_SkillWndThis ? 1 : 0,
                        (DWORD)g_SuperBtnObj, g_NativeBtnCreated ? 1 : 0);
        }
    }

    if (thisPtr == g_SkillWndThis)
    {
        if (g_NativeBtnCreated && g_SuperBtnObj && !ENABLE_SUPERBTN_D3D_BUTTON_MODE)
        {
            MoveSuperButtonToExpectedPos("BtnMoveDraw");
        }
        DrawSuperButtonTextureInSkillWndDraw(thisPtr);
        DrawSuperPanelInSkillWnd(thisPtr);
    }
    return ret;
}

__declspec(naked) static void hkSkillWndDrawNaked()
{
    __asm {
        mov eax, [esp + 4]
        push eax
        push ecx
        call hkSkillWndDrawHandler
        add esp, 8
        ret 4
    }
}

__declspec(naked) static void hkPostB9F6E0DrawNaked()
{
    __asm {
        pushfd
        pushad
        call DrawPostB9F6E0NativeTimingTest
        popad
        popfd
        mov eax, oPostB9F6E0DrawContinue
        jmp eax
    }
}

// ============================================================================
// 技能列表构建过滤 Hook（sub_7DD420 LABEL_42 入口 0x007DD67D）
// 证据：
//   sub_7DD420 是 __stdcall, retn 14h — 技能列表构建的核心函数
//   0x007DD67D 是 LABEL_42: 技能通过所有检查后、即将被加入 entries 数组的入口
//   此时 ebp = 技能数据指针, [ebp+0] = skillId
//   跳过时跳到 0x007DD6E8 (loop continue)
//   正常继续时执行原指令: mov eax,[esp+20h]; mov esi,[ebx+8] 然后跳到 0x007DD684
//
// 这次改动会不会影响原本稳定逻辑：不会，只在技能加入列表前做一次 skillId 检查
// 这次新增 call 的证据是否足够：A级，asm 直接确认 ebp=[skillData], [ebp+0]=skillId
// this / ecx / edx / 参数 / 返回值 是否确认：不涉及 call，只检查寄存器
// 这次新增了哪些空指针和时机保护：ebp 检查
// 目前仍不确定、需要补查的数据：无
// ============================================================================
static void *oSkillListBuildContinue = nullptr; // trampoline (原 7 字节: mov eax,[esp+20h]; mov esi,[ebx+8])

// C function called from naked hook — must be __cdecl, preserves no state
static int __cdecl CheckHideSkillFromNativeList(int skillId)
{
    return SkillOverlayBridgeShouldHideFromNativeList(skillId) ? 1 : 0;
}

// Storage for the indirect jmp target
static DWORD s_skipSkillAddr = ADDR_7DD6E8;

__declspec(naked) static void hkSkillListBuildFilterNaked()
{
    __asm {
        // At this point: ebp = skill data ptr, [ebp+0] = skillId
        // Save all registers we'll use
        push eax
        push ecx
        push edx

                    // Call our C check function with [ebp+0] as argument
        mov eax, [ebp]
        push eax
        call CheckHideSkillFromNativeList
        add esp, 4
        test eax, eax

            // Restore registers
        pop edx
        pop ecx
        pop eax

        jnz skip_skill

                            // Normal path: execute the original 7 bytes and continue
        jmp [oSkillListBuildContinue]

    skip_skill:
        // Skip this skill: jump to loop continue at 0x007DD6E8
        jmp dword ptr [s_skipSkillAddr]
    }
}

static bool SetupSkillListBuildFilterHook()
{
    // Hook at 0x007DD67D, need to copy 7 bytes:
    //   007DD67D: 8B 44 24 20   mov eax, [esp+20h]    (4 bytes)
    //   007DD681: 8B 73 08      mov esi, [ebx+8]       (3 bytes)
    oSkillListBuildContinue = GenericInlineHook5(
        (BYTE *)ADDR_7DD67D, (void *)hkSkillListBuildFilterNaked, 7);
    if (!oSkillListBuildContinue)
    {
        WriteLog("[SkillListFilter] Hook failed at 7DD67D");
        return false;
    }
    WriteLogFmt("[SkillListFilter] OK: tramp=0x%08X", (DWORD)oSkillListBuildContinue);
    return true;
}

// ============================================================================
// SkillWndEx 析构 Hook（sub_9E14D0）
// 证据：
//   sub_9E14D0 是 __thiscall(ecx=this), retn
//   asm 009E14F4 显示 esi=this，末尾是普通 retn；伪代码明确清理 MacroWnd 链和 dword_F6A0C0
//
// 这次改动会不会影响原本稳定逻辑：不会修改游戏析构顺序，只在调用原析构前清我们自己的外部状态
// 这次新增 call 的证据是否足够：A级，伪代码+asm 确认析构职责与调用约定
// this / ecx / edx / 参数 / 返回值 是否确认：确认，只有 this
// 这次新增了哪些空指针和时机保护：仅当 this==当前 SkillWnd 时才清理
// 目前仍不确定、需要我补查的数据：无
// ============================================================================
typedef int(__thiscall *tSkillWndDtor)(uintptr_t thisptr);
static tSkillWndDtor oSkillWndDtor = nullptr;

static int __cdecl hkSkillWndDtorHandler(uintptr_t thisPtr)
{
    if (thisPtr && thisPtr == g_SkillWndThis)
    {
        WriteLogFmt("[Lifecycle] SkillWnd dtor: this=0x%08X", (DWORD)thisPtr);
        ResetSuperRuntimeState(g_SuperCWnd != 0, "skillwnd_dtor");
        g_SkillWndThis = 0;
        g_Ready = false;
    }

    if (oSkillWndDtor)
    {
        return oSkillWndDtor(thisPtr);
    }
    return 0;
}

__declspec(naked) static void hkSkillWndDtorNaked()
{
    __asm {
        push ecx
        call hkSkillWndDtorHandler
        add esp, 4
        ret
    }
}

// ============================================================================
// 通用发包 Hook：在已知代理技能发包后，把 skillId 改写成自定义技能
// 入口证据：0043D94D，栈参数 [esp+4]=packetData, [esp+8]=packetLen
// ============================================================================
