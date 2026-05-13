// 骑宠攀爬门禁模块：负责攀爬/绳索动作白名单 hook 装配。
static bool SetupMountClimbGateFeatureHooks()
{
    bool anyOk = false;

    if (ssw::runtime::IsFeatureEnabled(ssw::runtime::FeatureSwitchId::MountClimbGateHooks))
    {
        oMountActionGate4069E0 = (tMountActionGateFn)InstallInlineHook(
            ADDR_4069E0, (void *)hkMountActionGate4069E0);
        if (oMountActionGate4069E0)
        {
            anyOk = true;
            WriteLogFmt("[MountGate] OK(4069E0): tramp=0x%08X", (DWORD)(uintptr_t)oMountActionGate4069E0);
        }
        else
        {
            WriteLog("[MountGate] hook failed: 4069E0");
        }

        oMountActionGate406AB0 = (tMountActionGateFn)InstallInlineHook(
            ADDR_406AB0, (void *)hkMountActionGate406AB0);
        if (oMountActionGate406AB0)
        {
            WriteLogFmt("[MountGate] OK(406AB0): tramp=0x%08X",
                        (DWORD)(uintptr_t)oMountActionGate406AB0);
        }
        else
        {
            WriteLog("[MountGate] hook failed: 406AB0");
        }
    }
    else
    {
        oMountActionGate4069E0 = nullptr;
        oMountActionGate406AB0 = nullptr;
        WriteLog("[MountGate] climb gate hooks disabled");
    }

    return anyOk;
}
