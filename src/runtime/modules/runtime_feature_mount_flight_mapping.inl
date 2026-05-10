static bool SetupMountFlightMappingFeatureHooks()
{
    bool anyOk = false;

    if (ssw::runtime::IsFeatureEnabled(ssw::runtime::FeatureSwitchId::MountFlightMappingHooks))
    {
        oMountNativeFlightSkillMap7CF370 = (tMountNativeFlightSkillMapFn)InstallInlineHook(
            ADDR_7CF370, (void *)hkMountNativeFlightSkillMap7CF370);
        if (oMountNativeFlightSkillMap7CF370)
        {
            anyOk = true;
            WriteLogFmt("[MountFlightMap] OK(7CF370): tramp=0x%08X", (DWORD)(uintptr_t)oMountNativeFlightSkillMap7CF370);
        }
        else
        {
            WriteLog("[MountFlightMap] hook failed: 7CF370");
        }

        oMountContextIsFlyingFamily7D4CD0 = (tMountContextIsFlyingFamilyFn)InstallInlineHook(
            ADDR_7D4CD0, (void *)hkMountContextIsFlyingFamily7D4CD0);
        if (oMountContextIsFlyingFamily7D4CD0)
        {
            anyOk = true;
            WriteLogFmt("[MountFamily] OK(7D4CD0): tramp=0x%08X",
                        (DWORD)(uintptr_t)oMountContextIsFlyingFamily7D4CD0);
        }
        else
        {
            WriteLog("[MountFamily] hook failed: 7D4CD0");
        }

        oMountSoaringGate7DC1B0 = (tMountSoaringGateFn)InstallInlineHook(
            ADDR_7DC1B0, (void *)hkMountSoaringGate7DC1B0);
        if (oMountSoaringGate7DC1B0)
        {
            anyOk = true;
            WriteLogFmt("[MountSoaringGate] OK(7DC1B0): tramp=0x%08X",
                        (DWORD)(uintptr_t)oMountSoaringGate7DC1B0);
        }
        else
        {
            WriteLog("[MountSoaringGate] hook failed: 7DC1B0");
        }

        BYTE *pMountNativeSoaringRelease = FollowJmpChain((void *)ADDR_B26290);
        if (pMountNativeSoaringRelease)
        {
            oMountNativeSoaringReleaseB26290 = (tMountNativeSoaringReleaseFn)GenericInlineHook5(
                pMountNativeSoaringRelease,
                (void *)hkMountNativeSoaringReleaseB26290,
                13);
        }
        else
        {
            oMountNativeSoaringReleaseB26290 = nullptr;
        }
        if (oMountNativeSoaringReleaseB26290)
        {
            anyOk = true;
            WriteLogFmt("[MountSoaringNative] OK(B26290): tramp=0x%08X",
                        (DWORD)(uintptr_t)oMountNativeSoaringReleaseB26290);
        }
        else
        {
            WriteLog("[MountSoaringNative] hook failed: B26290");
        }

        oMountFamilyGateA9AAA0 = (tMountFamilyGateFn)InstallInlineHook(
            ADDR_A9AAA0, (void *)hkMountFamilyGateA9AAA0);
        if (oMountFamilyGateA9AAA0)
        {
            anyOk = true;
            WriteLogFmt("[MountFamilyGate] OK(A9AAA0): tramp=0x%08X",
                        (DWORD)(uintptr_t)oMountFamilyGateA9AAA0);
        }
        else
        {
            WriteLog("[MountFamilyGate] hook failed: A9AAA0");
        }
    }
    else
    {
        oMountNativeFlightSkillMap7CF370 = nullptr;
        oMountContextIsFlyingFamily7D4CD0 = nullptr;
        oMountSoaringGate7DC1B0 = nullptr;
        oMountNativeSoaringReleaseB26290 = nullptr;
        oMountFamilyGateA9AAA0 = nullptr;
        WriteLog("[MountFlightMap] flight mapping hooks disabled");
    }

    return anyOk;
}
