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
