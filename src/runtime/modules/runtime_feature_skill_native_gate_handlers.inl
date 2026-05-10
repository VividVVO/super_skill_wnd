static BOOL __cdecl hkSkillNativeIdGate7CE790(int skillId)
{
    const int mappedSkillId = SkillOverlayBridgeResolveNativeGateSkillId(skillId);
    BOOL result = oSkillNativeIdGate7CE790
                      ? oSkillNativeIdGate7CE790(mappedSkillId)
                      : FALSE;
    if (!result && ResolveForcedNativeSkillGateAllow(skillId, mappedSkillId))
    {
        result = TRUE;
        static LONG s_skillGate7CE790ForceAllowLogBudget = 24;
        const LONG budgetAfterDecrement = InterlockedDecrement(&s_skillGate7CE790ForceAllowLogBudget);
        if (budgetAfterDecrement >= 0)
        {
            WriteLogFmt("[SkillGate] 7CE790 force allow skill=%d mapped=%d",
                        skillId, mappedSkillId);
        }
    }
    if (!result)
    {
        int resolvedMountItemId = 0;
        MountedRuntimeSkillKind kind = MountedRuntimeSkillKind_DoubleJump;
        if (ResolveMountedConfiguredSkillGateAllow(
                skillId,
                mappedSkillId,
                &kind,
                &resolvedMountItemId))
        {
            result = TRUE;
            static LONG s_skillGate7CE790MountedDoubleJumpForceAllowLogBudget = 24;
            const LONG budgetAfterDecrement =
                InterlockedDecrement(&s_skillGate7CE790MountedDoubleJumpForceAllowLogBudget);
            if (budgetAfterDecrement >= 0)
            {
                WriteLogFmt("[%s] 7CE790 force allow skill=%d mapped=%d mount=%d",
                            GetMountedRuntimeSkillLogTag(kind),
                            skillId,
                            mappedSkillId,
                            resolvedMountItemId);
            }
        }
    }
    if (!result)
    {
        int resolvedMountItemId = 0;
        bool fromUserLocal = false;
        if (ResolveRecentMountSoaringSkillGateAllow(
                skillId,
                mappedSkillId,
                &resolvedMountItemId,
                &fromUserLocal))
        {
            result = TRUE;
            static LONG s_skillGate7CE790RecentMountForceAllowLogBudget = 24;
            const LONG budgetAfterDecrement =
                InterlockedDecrement(&s_skillGate7CE790RecentMountForceAllowLogBudget);
            if (budgetAfterDecrement >= 0)
            {
                WriteLogFmt("[SkillGate] 7CE790 recent mount force allow skill=%d mapped=%d mount=%d source=%s",
                            skillId,
                            mappedSkillId,
                            resolvedMountItemId,
                            fromUserLocal ? "userlocal" : "recent");
            }
        }
    }
    if (mappedSkillId != skillId)
    {
        WriteLogFmt("[SkillGate] 7CE790 map custom=%d donor=%d result=%d",
                    skillId, mappedSkillId, result ? 1 : 0);
    }
    if (skillId == 80001089 || mappedSkillId == 80001089)
    {
        static LONG s_skillGate7CE790FinalLogBudget = 48;
        const LONG budgetAfterDecrement = InterlockedDecrement(&s_skillGate7CE790FinalLogBudget);
        if (budgetAfterDecrement >= 0)
        {
            WriteLogFmt("[SkillGate] 7CE790 final skill=%d mapped=%d result=%d",
                        skillId, mappedSkillId, result ? 1 : 0);
        }
    }
    return result;
}

static int __fastcall hkNativeGlyphLookup(void *thisPtr, void * /*edxUnused*/, unsigned int codepoint, RECT *outRectOrNull)
{
    if (thisPtr && codepoint > 0 && codepoint <= 0xFFFF)
        RetroSkillDWriteObserveGlyphLookup(thisPtr, codepoint);

    if (!oNativeGlyphLookup)
        return 0;

    return oNativeGlyphLookup(thisPtr, codepoint, outRectOrNull);
}

static BOOL __cdecl hkSkillNativeIdGate7D0000(int skillId)
{
    const int mappedSkillId = SkillOverlayBridgeResolveNativeGateSkillId(skillId);
    BOOL result = oSkillNativeIdGate7D0000
                      ? oSkillNativeIdGate7D0000(mappedSkillId)
                      : FALSE;
    if (!result && ResolveForcedNativeSkillGateAllow(skillId, mappedSkillId))
    {
        result = TRUE;
        static LONG s_skillGate7D0000ForceAllowLogBudget = 24;
        const LONG budgetAfterDecrement = InterlockedDecrement(&s_skillGate7D0000ForceAllowLogBudget);
        if (budgetAfterDecrement >= 0)
        {
            WriteLogFmt("[SkillGate] 7D0000 force allow skill=%d mapped=%d",
                        skillId, mappedSkillId);
        }
    }
    if (!result)
    {
        int resolvedMountItemId = 0;
        MountedRuntimeSkillKind kind = MountedRuntimeSkillKind_DoubleJump;
        if (ResolveMountedConfiguredSkillGateAllow(
                skillId,
                mappedSkillId,
                &kind,
                &resolvedMountItemId))
        {
            result = TRUE;
            static LONG s_skillGate7D0000MountedDoubleJumpForceAllowLogBudget = 24;
            const LONG budgetAfterDecrement =
                InterlockedDecrement(&s_skillGate7D0000MountedDoubleJumpForceAllowLogBudget);
            if (budgetAfterDecrement >= 0)
            {
                WriteLogFmt("[%s] 7D0000 force allow skill=%d mapped=%d mount=%d",
                            GetMountedRuntimeSkillLogTag(kind),
                            skillId,
                            mappedSkillId,
                            resolvedMountItemId);
            }
        }
    }
    if (!result)
    {
        int resolvedMountItemId = 0;
        bool fromUserLocal = false;
        if (ResolveRecentMountSoaringSkillGateAllow(
                skillId,
                mappedSkillId,
                &resolvedMountItemId,
                &fromUserLocal))
        {
            result = TRUE;
            static LONG s_skillGate7D0000RecentMountForceAllowLogBudget = 24;
            const LONG budgetAfterDecrement =
                InterlockedDecrement(&s_skillGate7D0000RecentMountForceAllowLogBudget);
            if (budgetAfterDecrement >= 0)
            {
                WriteLogFmt("[SkillGate] 7D0000 recent mount force allow skill=%d mapped=%d mount=%d source=%s",
                            skillId,
                            mappedSkillId,
                            resolvedMountItemId,
                            fromUserLocal ? "userlocal" : "recent");
            }
        }
    }
    if (mappedSkillId != skillId)
    {
        WriteLogFmt("[SkillGate] 7D0000 map custom=%d donor=%d result=%d",
                    skillId, mappedSkillId, result ? 1 : 0);
    }
    if (skillId == 80001089 || mappedSkillId == 80001089)
    {
        static LONG s_skillGate7D0000FinalLogBudget = 48;
        const LONG budgetAfterDecrement = InterlockedDecrement(&s_skillGate7D0000FinalLogBudget);
        if (budgetAfterDecrement >= 0)
        {
            WriteLogFmt("[SkillGate] 7D0000 final skill=%d mapped=%d result=%d",
                        skillId, mappedSkillId, result ? 1 : 0);
        }
    }
    return result;
}
