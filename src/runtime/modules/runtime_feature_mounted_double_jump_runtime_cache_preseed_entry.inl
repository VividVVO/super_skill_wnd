static bool TryPreseedMountedDemonJumpLevelCache(
    const char *hookTag,
    DWORD callerRet,
    int skillId,
    int lookupSkillId,
    void *cachePtr)
{
    if (!kEnableMountedDemonJumpRuntimeHooks)
        return false;

    if (!cachePtr ||
        SafeIsBadWritePtr(cachePtr, sizeof(uintptr_t)))
    {
        return false;
    }

    const bool directMountedDemonSkillQuery =
        IsMountedDemonJumpRelatedSkillId(skillId) ||
        IsMountedDemonJumpRelatedSkillId(lookupSkillId);

    uintptr_t existingEntry = 0;
    __try
    {
        existingEntry = *reinterpret_cast<uintptr_t *>(cachePtr);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        existingEntry = 0;
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
            mountItemId) != 30010110)
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

    int gateProbePreferredChildSkillId = 0;
    if (hasRecentProbe)
    {
        TryGetRecentMountedDemonJumpGateProbePreferredChildSkillId(
            mountItemId,
            &gateProbePreferredChildSkillId,
            400);
    }

    const bool allowGateProbeSeedRefresh =
        hasRecentProbe &&
        IsMountedDemonJumpGateProbeLevelSeedCaller(callerRet);
    if (!directMountedDemonSkillQuery && !allowGateProbeSeedRefresh)
    {
        LogMountedDemonJumpCachePreseedReject(
            hookTag,
            "caller-not-whitelisted",
            callerRet,
            skillId,
            lookupSkillId,
            mountItemId,
            directMountedDemonSkillQuery,
            hasRecentProbe,
            allowGateProbeSeedRefresh,
            existingEntry,
            0,
            gateProbePreferredChildSkillId);
        return false;
    }

    if (existingEntry)
    {
        if (!allowGateProbeSeedRefresh)
        {
            if (hasRecentProbe)
            {
                LogMountedDemonJumpCachePreseedReject(
                    hookTag,
                    "existing-entry-refresh-not-allowed",
                    callerRet,
                    skillId,
                    lookupSkillId,
                    mountItemId,
                    directMountedDemonSkillQuery,
                    hasRecentProbe,
                    allowGateProbeSeedRefresh,
                    existingEntry,
                    0,
                    gateProbePreferredChildSkillId);
            }
            return false;
        }

        int existingEntrySkillId = 0;
        if (!TryReadMountedDemonJumpCacheEntrySkillId(
                cachePtr,
                &existingEntrySkillId))
        {
            LogMountedDemonJumpCachePreseedReject(
                hookTag,
                "existing-entry-read-failed",
                callerRet,
                skillId,
                lookupSkillId,
                mountItemId,
                directMountedDemonSkillQuery,
                hasRecentProbe,
                allowGateProbeSeedRefresh,
                existingEntry,
                0,
                gateProbePreferredChildSkillId);
            return false;
        }

        if (existingEntrySkillId != skillId &&
            existingEntrySkillId != lookupSkillId)
        {
            LogMountedDemonJumpCachePreseedReject(
                hookTag,
                "existing-entry-mismatch",
                callerRet,
                skillId,
                lookupSkillId,
                mountItemId,
                directMountedDemonSkillQuery,
                hasRecentProbe,
                allowGateProbeSeedRefresh,
                existingEntry,
                existingEntrySkillId,
                gateProbePreferredChildSkillId);
            return false;
        }

        if (existingEntrySkillId == 30010110 ||
            IsMountedDemonJumpRuntimeChildSkillId(existingEntrySkillId))
        {
            LogMountedDemonJumpCachePreseedReject(
                hookTag,
                "existing-entry-already-runtime",
                callerRet,
                skillId,
                lookupSkillId,
                mountItemId,
                directMountedDemonSkillQuery,
                hasRecentProbe,
                allowGateProbeSeedRefresh,
                existingEntry,
                existingEntrySkillId,
                gateProbePreferredChildSkillId);
            return false;
        }
    }

    int entrySkillId = ResolveMountedDemonJumpCacheEntrySkillId(
        skillId,
        lookupSkillId,
        mountItemId);
    uintptr_t bridgeEntry =
        SkillOverlayBridgeLookupSkillEntryPointer(entrySkillId);
    if ((!bridgeEntry ||
         SafeIsBadReadPtr(reinterpret_cast<void *>(bridgeEntry), 0x40)) &&
        entrySkillId != 30010110)
    {
        entrySkillId = 30010110;
        bridgeEntry = SkillOverlayBridgeLookupSkillEntryPointer(entrySkillId);
    }
    if (!bridgeEntry ||
        SafeIsBadReadPtr(reinterpret_cast<void *>(bridgeEntry), 0x40))
    {
        return false;
    }

    __try
    {
        *reinterpret_cast<uintptr_t *>(cachePtr) = bridgeEntry;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }

    static LONG s_mountedDemonJumpCachePreseedLogBudget = 96;
    const LONG budgetAfterDecrement =
        InterlockedDecrement(&s_mountedDemonJumpCachePreseedLogBudget);
    if (budgetAfterDecrement >= 0)
    {
        WriteLogFmt(
            "[MountDemonJumpCache] %s stage=%s query=%d lookup=%d entrySkill=%d mount=%d cache=0x%08X entry=0x%08X caller=0x%08X",
            hookTag ? hookTag : "skill-level",
            existingEntry ? "probe-refresh" : "preseed",
            skillId,
            lookupSkillId,
            entrySkillId,
            mountItemId,
            (DWORD)(uintptr_t)cachePtr,
            static_cast<DWORD>(bridgeEntry),
            callerRet);
    }
    return true;
}

