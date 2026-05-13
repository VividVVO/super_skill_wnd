// 骑宠二段跳/恶魔跳跃共享装配模块：
// - `doubleJumpHooks` 负责二段跳主链，同时也承载两条链共用的基础 gate。
// - `demonJumpHooks` 只负责恶魔跳跃专属的 trace / packet / late / context 分支。
static bool SetupMountedDoubleJumpRuntimeFeatureHooks()
{
    bool anyOk = false;

    if (kEnableMountedDoubleJumpRuntimeHooks ||
        kEnableMountedDemonJumpRuntimeHooks)
    {
        oMountedSkillWhitelist7CF270 = (tSkillNativeIdGateFn)InstallInlineHook(
            ADDR_7CF270, (void *)hkMountedSkillWhitelist7CF270);
        if (oMountedSkillWhitelist7CF270)
        {
            anyOk = true;
            WriteLogFmt("[MountDoubleJump] OK(7CF270): tramp=0x%08X",
                        (DWORD)(uintptr_t)oMountedSkillWhitelist7CF270);
        }
        else
        {
            WriteLog("[MountDoubleJump] hook failed: 7CF270");
        }

        // A9BF40 entry starts with a rel32 call, so keep the safe callsite patch route.
        oMountedSkillContextGateA9BF40 = nullptr;
        WriteLog("[MountDoubleJump] hook disabled: A9BF40 rel32-call trampoline unsafe");
        if (SetupMountedSkillContextGateCallsiteHook())
            anyOk = true;
        if (SetupMountedUnknownSkillReleaseBranchHook())
            anyOk = true;
        if (SetupMountedUseFailPromptSuppressHook())
            anyOk = true;

        oMountedStateGate42DE20 = (tMountedStateGateFn)InstallInlineHook(
            ADDR_42DE20, (void *)hkMountedStateGate42DE20);
        if (oMountedStateGate42DE20)
        {
            anyOk = true;
            WriteLogFmt("[MountDoubleJump] OK(42DE20): tramp=0x%08X",
                        (DWORD)(uintptr_t)oMountedStateGate42DE20);
        }
        else
        {
            WriteLog("[MountDoubleJump] hook failed: 42DE20");
        }
    }
    else
    {
        oMountedSkillWhitelist7CF270 = nullptr;
        oMountedSkillContextGateA9BF40 = nullptr;
        oMountedStateGate42DE20 = nullptr;
        oMountedUseFailPromptAE6260 = nullptr;
        g_MountedSkillContextGateCallsiteOriginalTarget = 0;
        g_MountedUnknownSkillReleaseBranchOriginalTarget = 0;
        WriteLog("[MountDoubleJump] runtime hooks disabled");
    }

    return anyOk;
}

static bool SetupMountedDemonJumpRuntimeFeatureHooks()
{
    bool anyOk = false;

    if (kEnableMountedDemonJumpRuntimeHooks)
    {
        if (SetupMountedDemonJumpCrashTraceHooks())
            anyOk = true;
        if (SetupMountedDemonJumpPacketObserveHooks())
            anyOk = true;
        if (SetupMountedDemonJumpLatePathHooks())
            anyOk = true;
        if (SetupMountedDemonJumpActionTraceHooks())
            anyOk = true;
        if (SetupMountedDemonJumpRequirementBypassHook())
            anyOk = true;

        if (!oMountedDemonJumpContextClear433380)
        {
            oMountedDemonJumpContextClear433380 =
                (tMountedDemonJumpContextClearFn)InstallInlineHook(
                    ADDR_MountedDemonJumpContextClear433380,
                    (void *)hkMountedDemonJumpContextClear433380);
            if (oMountedDemonJumpContextClear433380)
            {
                anyOk = true;
                WriteLogFmt(
                    "[MountDemonJumpContext] OK(433380): tramp=0x%08X",
                    (DWORD)(uintptr_t)oMountedDemonJumpContextClear433380);
            }
            else
            {
                WriteLog("[MountDemonJumpContext] hook failed: 433380");
            }
        }
        else
        {
            anyOk = true;
        }
    }
    else
    {
        oMountedDemonJumpContextClear433380 = nullptr;
        oMountedSkillPacketDispatchB26760 = nullptr;
        oMountedSkillAttackPacketB28A00 = nullptr;
        oMountedDemonJumpLateRoute575D60 = nullptr;
        oMountedDemonJumpLateTick576020 = nullptr;
        oMountedDemonJumpContextInputB22630 = nullptr;
        oMountedDemonJumpMoveB1DB10 = nullptr;
        oMountedDemonJumpMoveB1C9E0 = nullptr;
        oMountedDemonJumpPrimeAE8F70 = nullptr;
        oMountedDemonJumpUpActionAFB710 = nullptr;
        oMountedDemonJumpAfbState42E170 = nullptr;
        oMountedDemonJumpAfbGateADB240 = nullptr;
        oMountedDemonJumpAfbGateAD9500 = nullptr;
        oMountedDemonJumpAfbLookup773500 = nullptr;
        oMountedDemonJumpBranchADEDA0 = nullptr;
        oMountedDemonJumpFilterBDBFD0 = nullptr;
        oMountedDemonJumpKeyState7BECF0 = nullptr;
        oMountedDemonJumpActionGate7DC870 = nullptr;
        oMountedDemonJumpActionGate7DC810 = nullptr;
        oMountedDemonJumpActionGate7DC7B0 = nullptr;
        oMountedDemonJumpActionGate7DC8D0 = nullptr;
        oMountedDemonJumpActionGate7DC710 = nullptr;
        oMountedDemonJumpActionGate7DC900 = nullptr;
        oMountedDemonJumpActionGate7CF840 = nullptr;
        oMountedDemonJumpActionGate7DC750 = nullptr;
        oMountedDemonJumpActionGate7DC8A0 = nullptr;
        oMountedDemonJumpActionKind7CE210 = nullptr;
        oMountedDemonJumpActionUsable7DAAF0 = nullptr;
        oMountedDemonJumpActionJobGate7D7D20 = nullptr;
        oMountedDemonJumpActionBattlegroundSkillGate4E1D30 = nullptr;
        oMountedDemonJumpActionAbilityGateAE5870 = nullptr;
        oMountedDemonJumpActionPromptReason42DAF0 = nullptr;
        oMountedDemonJumpActionPrepareB273B0 = nullptr;
        oMountedDemonJumpActionGateA9B710 = nullptr;
        oMountedDemonJumpActionKind52BAD0 = nullptr;
        oMountedDemonJumpActionRouteB29C70 = nullptr;
        oMountedDemonJumpActionRouteB24010 = nullptr;
        oMountedDemonJumpActionRouteB24EA0 = nullptr;
        oMountedDemonJumpActionRouteB26550 = nullptr;
        oMountedDemonJumpActionRouteB26050 = nullptr;
        WriteLog("[MountDemonJump] runtime hooks disabled");
    }

    return anyOk;
}
