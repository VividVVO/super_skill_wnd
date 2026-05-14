static void __cdecl hkSkillReleaseClassifierRootDispatch(int skillId)
{
    DWORD overrideSkillId = 0;
    if (SkillOverlayBridgeIsEchoOfHeroSkillId(skillId))
    {
        g_ClassifierOverrideSkillId = 0;
        static LONG s_echoRootPassthroughLogBudget = 24;
        if (InterlockedDecrement(&s_echoRootPassthroughLogBudget) >= 0)
        {
            WriteLogFmt("[SkillReleaseHook] B31349 echo native-only skillId=%d", skillId);
        }
        return;
    }

    int mountItemId = 0;
    int rootSkillId = 0;
    int currentSkillId = 0;
    const char *localSkillSource = nullptr;
    const int localSkillId = ResolveMountedDemonJumpClassifierLocalSkillId(
        skillId,
        &mountItemId,
        &rootSkillId,
        &currentSkillId,
        &localSkillSource);
    if (localSkillId > 0)
    {
        if (localSkillId != skillId)
        {
            overrideSkillId = static_cast<DWORD>(localSkillId);
        }
        static LONG s_mountedDemonJumpRootClassifierLocalSkillLogBudget = 32;
        const LONG budgetAfterDecrement = InterlockedDecrement(
            &s_mountedDemonJumpRootClassifierLocalSkillLogBudget);
        if (budgetAfterDecrement >= 0)
        {
            WriteLogFmt(
                "[MountDemonJump] B31349 local skill observed=%d local=%d mount=%d root=%d current=%d source=%s",
                skillId,
                localSkillId,
                mountItemId,
                rootSkillId,
                currentSkillId,
                localSkillSource ? localSkillSource : "none");
        }
    }
    else
    {
        overrideSkillId =
            (DWORD)SkillOverlayBridgeResolveNativeClassifierOverrideSkillId(
                skillId);
        if (overrideSkillId == 0)
        {
            const int contextOverrideSkillId =
                ResolveMountedDemonJumpContextFallbackOverrideSkillId(
                    skillId,
                    &mountItemId,
                    &rootSkillId,
                    &currentSkillId);
            if (contextOverrideSkillId > 0)
            {
                overrideSkillId = static_cast<DWORD>(contextOverrideSkillId);
                static LONG s_mountedDemonJumpContextRootOverrideLogBudget = 24;
                const LONG budgetAfterDecrement =
                    InterlockedDecrement(
                        &s_mountedDemonJumpContextRootOverrideLogBudget);
                if (budgetAfterDecrement >= 0)
                {
                    WriteLogFmt(
                        "[MountDemonJump] B31349 context fallback skill=%d mount=%d root=%d current=%d override=%d",
                        skillId,
                        mountItemId,
                        rootSkillId,
                        currentSkillId,
                        contextOverrideSkillId);
                }
            }
        }
    }
    if (IsMountedDemonJumpRelatedSkillId(skillId))
    {
        static LONG s_mountedDemonJumpReleaseRootLogBudget = 24;
        const LONG budgetAfterDecrement =
            InterlockedDecrement(&s_mountedDemonJumpReleaseRootLogBudget);
        if (budgetAfterDecrement >= 0)
        {
            WriteLogFmt("[MountDemonJump] B31349 root enter skill=%d override=%d",
                        skillId,
                        (int)overrideSkillId);
        }
    }
    g_ClassifierOverrideSkillId = overrideSkillId;
}
