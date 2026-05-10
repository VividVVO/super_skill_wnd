static bool TryBackfillMountedDemonJumpLevelCache(
    const char *hookTag,
    int skillId,
    int lookupSkillId,
    void *cachePtr,
    int mountItemId)
{
    if (!cachePtr ||
        mountItemId <= 0 ||
        SafeIsBadWritePtr(cachePtr, sizeof(uintptr_t)) ||
        (!IsMountedDemonJumpRelatedSkillId(skillId) &&
         !IsMountedDemonJumpRelatedSkillId(lookupSkillId)))
    {
        return false;
    }

    if (ResolveMountedRuntimeSkillIdForKind(
            MountedRuntimeSkillKind_DemonJump,
            mountItemId) != 30010110)
    {
        return false;
    }

    uintptr_t existingEntry = 0;
    __try
    {
        existingEntry = *reinterpret_cast<uintptr_t *>(cachePtr);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        existingEntry = 0;
    }

    if (existingEntry)
    {
        return false;
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
            "[MountDemonJumpCache] %s stage=backfill query=%d lookup=%d entrySkill=%d mount=%d cache=0x%08X entry=0x%08X",
            hookTag ? hookTag : "skill-level",
            skillId,
            lookupSkillId,
            entrySkillId,
            mountItemId,
            (DWORD)(uintptr_t)cachePtr,
            static_cast<DWORD>(bridgeEntry));
    }
    return true;
}

