static void __cdecl hkSkillReleaseClassifierB2F370Dispatch(int skillId)
{
    int overrideSkillId = 0;
    int classifierMountItemId = 0;
    int rootSkillId = 0;
    int currentSkillId = 0;
    const char *localSkillSource = nullptr;
    const int localSkillId = ResolveMountedDemonJumpClassifierLocalSkillId(
        skillId,
        &classifierMountItemId,
        &rootSkillId,
        &currentSkillId,
        &localSkillSource);
    if (localSkillId > 0)
    {
        if (localSkillId != skillId)
        {
            overrideSkillId = localSkillId;
        }
        static LONG s_mountedDemonJumpBranchClassifierLocalSkillLogBudget = 32;
        const LONG budgetAfterDecrement = InterlockedDecrement(
            &s_mountedDemonJumpBranchClassifierLocalSkillLogBudget);
        if (budgetAfterDecrement >= 0)
        {
            WriteLogFmt(
                "[MountDemonJump] B2F370 local skill observed=%d local=%d mount=%d root=%d current=%d source=%s",
                skillId,
                localSkillId,
                classifierMountItemId,
                rootSkillId,
                currentSkillId,
                localSkillSource ? localSkillSource : "none");
        }

        if (classifierMountItemId > 0 &&
            rootSkillId == 30010110 &&
            IsMountedDemonJumpRuntimeChildSkillId(localSkillId) &&
            ResolveMountedRuntimeSkillIdForKind(
                MountedRuntimeSkillKind_DemonJump,
                classifierMountItemId) == 30010110 &&
            SkillOverlayBridgeCanUseMountedDemonJumpRuntimeSkill(
                classifierMountItemId,
                localSkillId) &&
            !HasRecentMountedDemonJumpIntent(classifierMountItemId, 250))
        {
            RememberMountedDemonJumpNativeChildSkill(
                classifierMountItemId,
                localSkillId,
                "B2F370");
            ObserveMountedDemonJumpIntent(
                classifierMountItemId,
                "B2F370-arm-missing-intent");
            static LONG s_mountedDemonJumpClassifierArmMissingIntentLogBudget = 24;
            if (InterlockedDecrement(
                    &s_mountedDemonJumpClassifierArmMissingIntentLogBudget) >= 0)
            {
                WriteLogFmt(
                    "[MountDemonJump] B2F370 arm missing intent mount=%d local=%d root=%d current=%d source=%s",
                    classifierMountItemId,
                    localSkillId,
                    rootSkillId,
                    currentSkillId,
                    localSkillSource ? localSkillSource : "none");
            }
        }
    }
    else
    {
        overrideSkillId =
            SkillOverlayBridgeResolveNativeClassifierOverrideSkillId(skillId);
        if (overrideSkillId == 0)
        {
            const int contextOverrideSkillId =
                ResolveMountedDemonJumpContextFallbackOverrideSkillId(
                    skillId,
                    &classifierMountItemId,
                    &rootSkillId,
                    &currentSkillId);
            if (contextOverrideSkillId > 0)
            {
                overrideSkillId = contextOverrideSkillId;
                static LONG s_mountedDemonJumpContextBranchOverrideLogBudget = 24;
                const LONG budgetAfterDecrement =
                    InterlockedDecrement(
                        &s_mountedDemonJumpContextBranchOverrideLogBudget);
                if (budgetAfterDecrement >= 0)
                {
                    WriteLogFmt(
                        "[MountDemonJump] B2F370 context fallback skill=%d mount=%d root=%d current=%d override=%d",
                        skillId,
                        classifierMountItemId,
                        rootSkillId,
                        currentSkillId,
                        contextOverrideSkillId);
                }
            }
        }
    }
    g_ClassifierOverrideSkillId = (DWORD)overrideSkillId;
    if (overrideSkillId > 0 && overrideSkillId != skillId)
    {
        WriteLogFmt("[SkillReleaseHook] B2F370 override skillId=%d -> %d", skillId, overrideSkillId);
    }

    if (!kEnableMountedDoubleJumpRuntimeHooks)
    {
        return;
    }

    int mountItemId = 0;
    if (skillId > 0 &&
        TryResolveMountedDoubleJumpMountItemIdWithFallback(
            nullptr,
            &mountItemId,
            nullptr,
            1200) &&
        HasRecentMountedDoubleJumpIntent(mountItemId) &&
        SkillOverlayBridgeCanUseMountedDoubleJumpRuntimeSkill(mountItemId, skillId))
    {
        ObserveMountedDoubleJumpNativeRelease(mountItemId, skillId);
        static LONG s_mountedDoubleJumpNativeReleaseObserveLogBudget = 24;
        const LONG budgetAfterDecrement =
            InterlockedDecrement(&s_mountedDoubleJumpNativeReleaseObserveLogBudget);
        if (budgetAfterDecrement >= 0)
        {
            WriteLogFmt("[MountDoubleJump] B2F370 observe native release mount=%d skill=%d",
                        mountItemId,
                        skillId);
        }
    }

    mountItemId = 0;
    if (skillId > 0 &&
        TryResolveMountedDemonJumpMountItemIdWithFallback(
            nullptr,
            &mountItemId,
            nullptr,
            1200) &&
        HasRecentMountedDemonJumpIntent(mountItemId) &&
        SkillOverlayBridgeCanUseMountedDemonJumpRuntimeSkill(mountItemId, skillId))
    {
        ObserveMountedDemonJumpNativeRelease(mountItemId, skillId);
        static LONG s_mountedDemonJumpNativeReleaseObserveLogBudget = 24;
        const LONG budgetAfterDecrement =
            InterlockedDecrement(&s_mountedDemonJumpNativeReleaseObserveLogBudget);
        if (budgetAfterDecrement >= 0)
        {
            WriteLogFmt("[MountDemonJump] B2F370 observe native release mount=%d skill=%d",
                        mountItemId,
                        skillId);
        }
    }
}
