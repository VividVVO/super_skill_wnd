static bool IsAddressInCurrentModule(DWORD address)
{
    if (!address)
    {
        return false;
    }

    static HMODULE s_module = nullptr;
    static DWORD s_sizeOfImage = 0;
    if (!s_module)
    {
        s_module = GetModuleHandleW(L"SS.dll");
        if (s_module)
        {
            const uintptr_t base = reinterpret_cast<uintptr_t>(s_module);
            if (!SafeIsBadReadPtr(reinterpret_cast<void *>(base), sizeof(IMAGE_DOS_HEADER)))
            {
                const IMAGE_DOS_HEADER *dosHeader =
                    reinterpret_cast<const IMAGE_DOS_HEADER *>(base);
                const uintptr_t ntHeaderAddr = base + static_cast<uintptr_t>(dosHeader->e_lfanew);
                if (!SafeIsBadReadPtr(reinterpret_cast<void *>(ntHeaderAddr), sizeof(IMAGE_NT_HEADERS32)))
                {
                    const IMAGE_NT_HEADERS32 *ntHeader =
                        reinterpret_cast<const IMAGE_NT_HEADERS32 *>(ntHeaderAddr);
                    s_sizeOfImage = ntHeader->OptionalHeader.SizeOfImage;
                }
            }
        }
    }

    if (!s_module || s_sizeOfImage == 0)
    {
        return false;
    }

    const uintptr_t base = reinterpret_cast<uintptr_t>(s_module);
    const uintptr_t addr = static_cast<uintptr_t>(address);
    return addr >= base && addr < (base + static_cast<uintptr_t>(s_sizeOfImage));
}

static int ResolveMountedDemonJumpCacheEntrySkillId(
    int skillId,
    int lookupSkillId,
    int mountItemId)
{
    if (IsMountedDemonJumpRuntimeChildSkillId(lookupSkillId) &&
        SkillOverlayBridgeCanUseMountedDemonJumpRuntimeSkill(
            mountItemId,
            lookupSkillId))
    {
        return lookupSkillId;
    }

    if (IsMountedDemonJumpRuntimeChildSkillId(skillId) &&
        SkillOverlayBridgeCanUseMountedDemonJumpRuntimeSkill(
            mountItemId,
            skillId))
    {
        return skillId;
    }

    int rootSkillId = 0;
    int currentSkillId = 0;
    if (TryReadMountedDemonJumpContextState(
            &rootSkillId,
            &currentSkillId,
            nullptr) &&
        rootSkillId == 30010110 &&
        IsMountedDemonJumpRuntimeChildSkillId(currentSkillId) &&
        SkillOverlayBridgeCanUseMountedDemonJumpRuntimeSkill(
            mountItemId,
            currentSkillId))
    {
        return currentSkillId;
    }

    int recentChildSkillId = 0;
    if (TryGetRecentMountedDemonJumpNativeChildSkill(
            mountItemId,
            &recentChildSkillId,
            nullptr,
            1500) &&
        IsMountedDemonJumpRuntimeChildSkillId(recentChildSkillId) &&
        SkillOverlayBridgeCanUseMountedDemonJumpRuntimeSkill(
            mountItemId,
            recentChildSkillId))
    {
        return recentChildSkillId;
    }

    int gateProbePreferredChildSkillId = 0;
    if (TryGetRecentMountedDemonJumpGateProbePreferredChildSkillId(
            mountItemId,
            &gateProbePreferredChildSkillId,
            400) &&
        SkillOverlayBridgeCanUseMountedDemonJumpRuntimeSkill(
            mountItemId,
            gateProbePreferredChildSkillId))
    {
        return gateProbePreferredChildSkillId;
    }

    const int latchedUpChildSkillId =
        ResolveMountedDemonJumpLatchedUpChildSkillId(
            mountItemId,
            rootSkillId,
            currentSkillId);
    if (IsMountedDemonJumpRuntimeChildSkillId(latchedUpChildSkillId) &&
        SkillOverlayBridgeCanUseMountedDemonJumpRuntimeSkill(
            mountItemId,
            latchedUpChildSkillId))
    {
        return latchedUpChildSkillId;
    }

    return 30010110;
}

static bool TryReadMountedDemonJumpCacheEntrySkillId(
    void *cachePtr,
    int *entrySkillIdOut)
{
    if (entrySkillIdOut)
    {
        *entrySkillIdOut = 0;
    }

    if (!cachePtr ||
        !entrySkillIdOut ||
        SafeIsBadReadPtr(cachePtr, sizeof(uintptr_t)))
    {
        return false;
    }

    uintptr_t entryPtr = 0;
    __try
    {
        entryPtr = *reinterpret_cast<uintptr_t *>(cachePtr);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        entryPtr = 0;
    }

    if (!entryPtr ||
        SafeIsBadReadPtr(reinterpret_cast<void *>(entryPtr), sizeof(int)))
    {
        return false;
    }

    int entrySkillId = 0;
    __try
    {
        entrySkillId = *reinterpret_cast<int *>(entryPtr);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        entrySkillId = 0;
    }

    if (entrySkillId <= 0)
    {
        return false;
    }

    *entrySkillIdOut = entrySkillId;
    return true;
}

static void LogMountedDemonJumpCachePreseedReject(
    const char *hookTag,
    const char *reasonTag,
    DWORD callerRet,
    int skillId,
    int lookupSkillId,
    int mountItemId,
    bool directMountedDemonSkillQuery,
    bool hasRecentProbe,
    bool allowGateProbeSeedRefresh,
    uintptr_t existingEntry,
    int existingEntrySkillId,
    int gateProbePreferredChildSkillId)
{
    static LONG s_mountedDemonJumpCachePreseedRejectLogBudget = 96;
    static DWORD s_lastLogTick = 0;
    static DWORD s_lastCallerRet = 0;
    static int s_lastSkillId = 0;
    static int s_lastLookupSkillId = 0;
    static int s_lastMountItemId = 0;
    static int s_lastExistingEntrySkillId = 0;
    static const char *s_lastReasonTag = nullptr;

    const DWORD nowTick = GetTickCount();
    if (callerRet == s_lastCallerRet &&
        skillId == s_lastSkillId &&
        lookupSkillId == s_lastLookupSkillId &&
        mountItemId == s_lastMountItemId &&
        existingEntrySkillId == s_lastExistingEntrySkillId &&
        reasonTag == s_lastReasonTag &&
        nowTick - s_lastLogTick < 120)
    {
        return;
    }

    const LONG budgetAfterDecrement =
        InterlockedDecrement(&s_mountedDemonJumpCachePreseedRejectLogBudget);
    if (budgetAfterDecrement < 0)
    {
        return;
    }

    s_lastLogTick = nowTick;
    s_lastCallerRet = callerRet;
    s_lastSkillId = skillId;
    s_lastLookupSkillId = lookupSkillId;
    s_lastMountItemId = mountItemId;
    s_lastExistingEntrySkillId = existingEntrySkillId;
    s_lastReasonTag = reasonTag;

    WriteLogFmt(
        "[MountDemonJumpCache] %s stage=preseed-reject reason=%s query=%d lookup=%d mount=%d caller=0x%08X direct=%d recentProbe=%d allowProbeRefresh=%d existingEntry=0x%08X existingSkill=%d gateProbeChild=%d",
        hookTag ? hookTag : "skill-level",
        reasonTag ? reasonTag : "unknown",
        skillId,
        lookupSkillId,
        mountItemId,
        callerRet,
        directMountedDemonSkillQuery ? 1 : 0,
        hasRecentProbe ? 1 : 0,
        allowGateProbeSeedRefresh ? 1 : 0,
        static_cast<DWORD>(existingEntry),
        existingEntrySkillId,
        gateProbePreferredChildSkillId);
}

