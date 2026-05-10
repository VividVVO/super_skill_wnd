static int __fastcall hkMountedUseFailPromptAE6260(
    void *thisPtr,
    void * /*edxUnused*/,
    int a2)
{
    int mountItemId = 0;
    int configuredSkillId = 0;
    MountedRuntimeSkillKind kind = MountedRuntimeSkillKind_DoubleJump;
    const DWORD callerRet = (DWORD)(uintptr_t)_ReturnAddress();
    if (ShouldSuppressMountedConfiguredUseFailPrompt(
            thisPtr,
            a2,
            callerRet,
            &kind,
            &mountItemId,
            &configuredSkillId))
    {
        static LONG s_mountUseFailPromptSuppressLogBudget = 24;
        if (InterlockedDecrement(&s_mountUseFailPromptSuppressLogBudget) >= 0)
        {
            WriteLogFmt(
                "[%s] AE6260 suppress prompt mount=%d skill=%d reason=%d caller=0x%08X",
                GetMountedRuntimeSkillLogTag(kind),
                mountItemId,
                configuredSkillId,
                a2,
                callerRet);
        }
        return 1;
    }

    int debugMountItemId = 0;
    if (TryResolveCurrentUserMountItemIdWithFallback(&debugMountItemId, nullptr))
    {
        MountedMovementOverride mountedOverride = {};
        if (debugMountItemId > 0 &&
            SkillOverlayBridgeResolveMountedMovementOverride(
                debugMountItemId,
                0,
                mountedOverride) &&
            mountedOverride.matched)
        {
            static LONG s_mountUseFailPromptObserveLogBudget = 24;
            if (InterlockedDecrement(&s_mountUseFailPromptObserveLogBudget) >= 0)
            {
                WriteLogFmt(
                    "[MountUseFail] AE6260 passthrough mount=%d reason=%d caller=0x%08X doubleIntent=%d doubleSkill=%d demonIntent=%d demonSkill=%d",
                    debugMountItemId,
                    a2,
                    callerRet,
                    HasRecentMountedDoubleJumpIntent(debugMountItemId, 1200) ? 1 : 0,
                    SkillOverlayBridgeResolveMountedDoubleJumpSkillId(debugMountItemId),
                    HasRecentMountedDemonJumpIntent(debugMountItemId, 1200) ? 1 : 0,
                    SkillOverlayBridgeResolveMountedDemonJumpSkillId(debugMountItemId));
            }
        }
    }

    return oMountedUseFailPromptAE6260
               ? oMountedUseFailPromptAE6260(thisPtr, a2)
               : 0;
}
