static void ObserveMountedDemonJumpGateProbeLevelLookup(
    const char *hookTag,
    const char *stageTag,
    DWORD callerRet,
    int skillId,
    int lookupSkillId,
    void *cachePtr,
    int flags,
    int rawResult,
    int finalResult)
{
    int gateProbeMountItemId = 0;
    if (!TryGetRecentMountedDemonJumpGateProbeMountItemId(
            &gateProbeMountItemId,
            400) ||
        gateProbeMountItemId <= 0)
    {
        return;
    }

    int cacheEntrySkillId = 0;
    const bool hasCacheEntrySkillId = TryReadMountedDemonJumpCacheEntrySkillId(
        cachePtr,
        &cacheEntrySkillId);

    int resolvedMountItemId = 0;
    int resolvedChildSkillId = 0;
    const bool freshChildMatch =
        TryResolveMountedDemonJumpFreshChildLevelLookup(
            callerRet,
            skillId,
            lookupSkillId,
            cachePtr,
            &resolvedMountItemId,
            &resolvedChildSkillId);

    static LONG s_mountedDemonJumpGateProbeLevelLookupLogBudget = 160;
    static DWORD s_lastLogTick = 0;
    static DWORD s_lastCallerRet = 0;
    static int s_lastSkillId = 0;
    static int s_lastLookupSkillId = 0;
    static int s_lastCacheEntrySkillId = 0;
    static int s_lastFlags = 0;
    static int s_lastFinalResult = 0;
    static int s_lastStage = 0;
    const DWORD nowTick = GetTickCount();
    const int stageKey =
        stageTag && lstrcmpA(stageTag, "post") == 0 ? 2 : 1;
    if (callerRet == s_lastCallerRet &&
        skillId == s_lastSkillId &&
        lookupSkillId == s_lastLookupSkillId &&
        cacheEntrySkillId == s_lastCacheEntrySkillId &&
        flags == s_lastFlags &&
        finalResult == s_lastFinalResult &&
        stageKey == s_lastStage &&
        nowTick - s_lastLogTick < 120)
    {
        return;
    }

    s_lastLogTick = nowTick;
    s_lastCallerRet = callerRet;
    s_lastSkillId = skillId;
    s_lastLookupSkillId = lookupSkillId;
    s_lastCacheEntrySkillId = cacheEntrySkillId;
    s_lastFlags = flags;
    s_lastFinalResult = finalResult;
    s_lastStage = stageKey;

    if (InterlockedDecrement(
            &s_mountedDemonJumpGateProbeLevelLookupLogBudget) >= 0)
    {
        WriteLogFmt(
            "[MountDemonJumpProbe] %s %s caller=0x%08X query=%d lookup=%d flags=%d cache=0x%08X entry=%d entryOk=%d raw=%d final=%d fresh=%d freshMount=%d freshChild=%d probeMount=%d",
            hookTag ? hookTag : "skill-level",
            stageTag ? stageTag : "unknown",
            callerRet,
            skillId,
            lookupSkillId,
            flags,
            (DWORD)(uintptr_t)cachePtr,
            cacheEntrySkillId,
            hasCacheEntrySkillId ? 1 : 0,
            rawResult,
            finalResult,
            freshChildMatch ? 1 : 0,
            freshChildMatch ? resolvedMountItemId : 0,
            freshChildMatch ? resolvedChildSkillId : 0,
            gateProbeMountItemId);
    }
}
