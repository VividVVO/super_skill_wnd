static void __cdecl hkSkillReleaseClassifierDispatch(int skillId)
{
    const DWORD forcedJump =
        SkillOverlayBridgeResolveNativeReleaseJumpTarget(skillId);
    if (IsMountedDemonJumpRelatedSkillId(skillId))
    {
        static LONG s_mountedDemonJumpReleaseClassifierLogBudget = 24;
        const LONG budgetAfterDecrement =
            InterlockedDecrement(&s_mountedDemonJumpReleaseClassifierLogBudget);
        if (budgetAfterDecrement >= 0)
        {
            WriteLogFmt("[MountDemonJump] B3144D classifier enter skill=%d jump=0x%08X",
                        skillId,
                        forcedJump);
        }
    }
    g_ForcedNativeReleaseJump = forcedJump;
}
