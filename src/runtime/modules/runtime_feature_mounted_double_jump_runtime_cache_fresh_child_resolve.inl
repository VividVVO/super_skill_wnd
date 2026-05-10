static bool TryResolveMountedDemonJumpFreshChildLevelLookup(
    DWORD callerRet,
    int skillId,
    int lookupSkillId,
    void *cachePtr,
    int *mountItemIdOut,
    int *childSkillIdOut)
{
    if (mountItemIdOut)
    {
        *mountItemIdOut = 0;
    }
    if (childSkillIdOut)
    {
        *childSkillIdOut = 0;
    }

    // Current runtime evidence:
    // 1. Failing root-path samples already reach 7DBC50 with
    //    query/lookup=30010183 at caller 0x00433B4C, but B300AC still sees
    //    recent=23001002 and drops back to keep-root.
    // 2. Natural demon-jump success reaches 7DBC50 with
    //    query/lookup=30010184 at caller 0x00B30B55 before the full child
    //    release chain continues through 8057F0/B28A00.
    // 3. Mounted left-jump success trace export-20260503-125544.csv also shows
    //    7CF270 -> 7DC1CD -> 7DBC50 return=0x007DC1D2 with query/lookup still
    //    resolved to child=30010184 before B31349/B28A00.
    // 4. Mounted up-jump failure trace export-20260506-072846.csv shows
    //    7DBC50 returning to 0x0086538F / 0x008653BE / 0x008545F6 while the
    //    looked-up entry is already child=0x01C9EB47(30010183). These are the
    //    earliest observable points in that failing chain where native child
    //    resolution is concrete, but our mounted recent-intent/context had not
    //    yet been re-armed.
    //
    // 5. Current v23.19 mounted job-100 logs show 42DE20 caller=0x00ADF02B
    //    immediately followed by 7DBC50 returning to 0x79AEB002 / 0x00A04B7A
    //    with query/lookup still parked on the mounted source skill
    //    (80001099). Once that probe cache is refreshed to the real child,
    //    these two callers become the earliest stable gate-probe seed points
    //    before the proxy classifier decides whether to stay on mount
    //    behavior or enter demon-jump release.
    //
    // Keep this whitelist narrow to only the verified early callers above.
    // Later lookups such as AE0479/B28AD1/434034 remain too late and should
    // not resurrect stale child state on their own.
    if (callerRet != 0x00433B4C &&
        callerRet != 0x00B30B55 &&
        callerRet != 0x007DC1D2 &&
        callerRet != 0x0086538F &&
        callerRet != 0x008653BE &&
        callerRet != 0x008545F6 &&
        callerRet != 0x79AEB002 &&
        callerRet != 0x00A04B7A)
    {
        return false;
    }

    int childSkillId = 0;
    if (IsMountedDemonJumpRuntimeChildSkillId(lookupSkillId))
    {
        childSkillId = lookupSkillId;
    }
    else if (IsMountedDemonJumpRuntimeChildSkillId(skillId))
    {
        childSkillId = skillId;
    }
    else
    {
        int cacheEntrySkillId = 0;
        if (TryReadMountedDemonJumpCacheEntrySkillId(
                cachePtr,
                &cacheEntrySkillId) &&
            IsMountedDemonJumpRuntimeChildSkillId(cacheEntrySkillId))
        {
            childSkillId = cacheEntrySkillId;
        }
    }
    if (!IsMountedDemonJumpRuntimeChildSkillId(childSkillId))
    {
        return false;
    }

    int mountItemId = 0;
    bool hasRecentProbe = false;
    if (!TryResolveMountedDemonJumpMountItemIdWithFallback(
            nullptr,
            &mountItemId,
            nullptr,
            1200) &&
        !TryGetRecentMountedDemonJumpIntentItemId(&mountItemId, 1200))
    {
        if (!TryGetRecentMountedDemonJumpGateProbeMountItemId(
                &mountItemId,
                400))
        {
            return false;
        }
        hasRecentProbe = true;
    }
    if (mountItemId <= 0 ||
        ResolveMountedRuntimeSkillIdForKind(
            MountedRuntimeSkillKind_DemonJump,
            mountItemId) != 30010110 ||
        !SkillOverlayBridgeCanUseMountedDemonJumpRuntimeSkill(
            mountItemId,
            childSkillId))
    {
        return false;
    }

    if (!hasRecentProbe)
    {
        int probeMountItemId = 0;
        hasRecentProbe =
            TryGetRecentMountedDemonJumpGateProbeMountItemId(
                &probeMountItemId,
                400) &&
            probeMountItemId == mountItemId;
    }

    if (!HasRecentMountedDemonJumpIntent(mountItemId, 1200) &&
        !hasRecentProbe)
    {
        return false;
    }

    if (mountItemIdOut)
    {
        *mountItemIdOut = mountItemId;
    }
    if (childSkillIdOut)
    {
        *childSkillIdOut = childSkillId;
    }
    return true;
}

