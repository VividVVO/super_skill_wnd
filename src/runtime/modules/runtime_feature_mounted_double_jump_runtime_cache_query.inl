static void ObserveMountedDemonJumpLevelQueryCaller(
    const char *hookTag,
    DWORD callerRet,
    int skillId,
    int lookupSkillId,
    int flags,
    int rawResult,
    int finalResult)
{
    if (!kEnableMountedDemonJumpRuntimeHooks)
        return;

    if (!IsMountedDemonJumpRelatedSkillId(skillId) &&
        !IsMountedDemonJumpRelatedSkillId(lookupSkillId))
    {
        return;
    }

    if (IsAddressInCurrentModule(callerRet))
    {
        return;
    }

    static DWORD s_lastLogTick = 0;
    static DWORD s_lastCallerRet = 0;
    static int s_lastSkillId = 0;
    static int s_lastLookupSkillId = 0;
    static int s_lastFlags = 0;
    const DWORD nowTick = GetTickCount();
    if (callerRet == s_lastCallerRet &&
        skillId == s_lastSkillId &&
        lookupSkillId == s_lastLookupSkillId &&
        flags == s_lastFlags &&
        nowTick - s_lastLogTick < 1000)
    {
        return;
    }

    s_lastLogTick = nowTick;
    s_lastCallerRet = callerRet;
    s_lastSkillId = skillId;
    s_lastLookupSkillId = lookupSkillId;
    s_lastFlags = flags;
    WriteLogFmt("[MountDemonJump] %s caller=0x%08X query=%d lookup=%d flags=%d raw=%d final=%d",
                hookTag ? hookTag : "skill-level",
                callerRet,
                skillId,
                lookupSkillId,
                flags,
                rawResult,
                finalResult);
}

