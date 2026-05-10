static bool SetupWndProcHook()
{
    if (g_OriginalWndProc && g_GameHwnd)
        return true;

    g_GameHwnd = GetRealGameWindow();
    if (!g_GameHwnd)
    {
        WriteLog("[WndProc] Game window not found");
        return false;
    }
    g_OriginalWndProc = (WNDPROC)SetWindowLongPtrA(g_GameHwnd, GWLP_WNDPROC, (LONG_PTR)GameWndProc);
    WriteLogFmt("[WndProc] Hooked: 0x%08X", (DWORD)g_OriginalWndProc);
    return g_OriginalWndProc != nullptr;
}

static void EnsureDeferredInteractionHooks(const char *reason)
{
    static DWORD s_lastWndProcRetryTick = 0;
    static bool s_inputSpoofAttempted = false;

    const DWORD now = GetTickCount();

    if (!g_OriginalWndProc && (now - s_lastWndProcRetryTick >= 1000))
    {
        s_lastWndProcRetryTick = now;
        if (SetupWndProcHook())
        {
            WriteLogFmt("[WndProc] deferred install OK reason=%s", reason ? reason : "unknown");
        }
        else
        {
            WriteLogFmt("[WndProc] deferred install pending reason=%s", reason ? reason : "unknown");
        }
    }

    if (!s_inputSpoofAttempted)
    {
        s_inputSpoofAttempted = true;
        if (!Win32InputSpoofInstall())
            WriteLogFmt("[InputSpoof] deferred install failed reason=%s", reason ? reason : "unknown");
        else
            WriteLogFmt("[InputSpoof] deferred install OK reason=%s", reason ? reason : "unknown");
    }
}

