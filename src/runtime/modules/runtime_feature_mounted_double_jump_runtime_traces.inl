#include "runtime_feature_mounted_double_jump_runtime_trace_entrypoints.inl"
#include "runtime_feature_mounted_double_jump_runtime_trace_prelocal.inl"
#include "runtime_feature_mounted_double_jump_runtime_trace_action.inl"

static DWORD RewriteMountedDemonJumpActionRouteB29C70SkillEntry(
    DWORD callerRet,
    int runtimeSkillId,
    int mountItemId,
    DWORD originalEntryPtr,
    int *originalSkillIdOut,
    int *rewrittenSkillIdOut)
{
    if (originalSkillIdOut)
    {
        *originalSkillIdOut = 0;
    }
    if (rewrittenSkillIdOut)
    {
        *rewrittenSkillIdOut = 0;
    }

    if (callerRet != 0x00B32235 ||
        originalEntryPtr == 0 ||
        !IsMountedDemonJumpRuntimeChildSkillId(runtimeSkillId) ||
        mountItemId <= 0 ||
        ResolveMountedRuntimeSkillIdForKind(
            MountedRuntimeSkillKind_DemonJump,
            mountItemId) != 30010110 ||
        SafeIsBadReadPtr(reinterpret_cast<void *>(originalEntryPtr), 0x20))
    {
        return originalEntryPtr;
    }

    int originalSkillId = 0;
    __try
    {
        originalSkillId = *reinterpret_cast<int *>(originalEntryPtr);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return originalEntryPtr;
    }

    if (originalSkillIdOut)
    {
        *originalSkillIdOut = originalSkillId;
    }

    if (originalSkillId != 30010110)
    {
        if (rewrittenSkillIdOut)
        {
            *rewrittenSkillIdOut = originalSkillId;
        }
        return originalEntryPtr;
    }

    int rootSkillId = 0;
    int currentSkillId = 0;
    if (!TryReadMountedDemonJumpEffectiveContextState(
            mountItemId,
            &rootSkillId,
            &currentSkillId) ||
        rootSkillId != 30010110)
    {
        return originalEntryPtr;
    }

    int childSkillId = 0;
    if (IsMountedDemonJumpRuntimeChildSkillId(currentSkillId))
    {
        childSkillId = currentSkillId;
    }
    else if (IsMountedDemonJumpRuntimeChildSkillId(runtimeSkillId))
    {
        childSkillId = runtimeSkillId;
    }
    else
    {
        int recentChildSkillId = 0;
        if (TryGetRecentMountedDemonJumpNativeChildSkill(
                mountItemId,
                &recentChildSkillId,
                nullptr,
                1500) &&
            IsMountedDemonJumpRuntimeChildSkillId(recentChildSkillId))
        {
            childSkillId = recentChildSkillId;
        }
    }

    if (!IsMountedDemonJumpRuntimeChildSkillId(childSkillId))
    {
        return originalEntryPtr;
    }

    const uintptr_t childEntryPtr =
        SkillOverlayBridgeLookupSkillEntryPointer(childSkillId);
    if (!childEntryPtr ||
        SafeIsBadReadPtr(reinterpret_cast<void *>(childEntryPtr), 0x20))
    {
        return originalEntryPtr;
    }

    int verifiedChildSkillId = 0;
    __try
    {
        verifiedChildSkillId = *reinterpret_cast<int *>(childEntryPtr);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return originalEntryPtr;
    }

    if (verifiedChildSkillId != childSkillId)
    {
        return originalEntryPtr;
    }

    if (rewrittenSkillIdOut)
    {
        *rewrittenSkillIdOut = childSkillId;
    }
    return static_cast<DWORD>(childEntryPtr);
}

static int __fastcall hkMountedDemonJumpActionRouteB29C70(
    void *thisPtr,
    void * /*edxUnused*/,
    DWORD arg1,
    DWORD arg2,
    DWORD arg3)
{
    const DWORD callerRet = (DWORD)(uintptr_t)_ReturnAddress();
    int runtimeSkillId = 0;
    int mountItemId = 0;
    int originalEntrySkillId = 0;
    int rewrittenEntrySkillId = 0;
    DWORD routeArg1 = arg1;
    const bool trace =
        IsMountedDemonJumpCrashTraceFresh(&runtimeSkillId, &mountItemId);
    if (trace)
    {
        routeArg1 = RewriteMountedDemonJumpActionRouteB29C70SkillEntry(
            callerRet,
            runtimeSkillId,
            mountItemId,
            arg1,
            &originalEntrySkillId,
            &rewrittenEntrySkillId);
    }
    if (trace)
    {
        WriteLogFmt(
            "[MountDemonJumpAction] B29C70 enter caller=0x%08X runtime=%d mount=%d this=0x%08X args=[0x%08X,0x%08X,0x%08X]",
            callerRet,
            runtimeSkillId,
            mountItemId,
            (DWORD)(uintptr_t)thisPtr,
            arg1,
            arg2,
            arg3);
        if (routeArg1 != arg1 &&
            rewrittenEntrySkillId > 0)
        {
            WriteLogFmt(
                "[MountDemonJumpAction] B29C70 rewrite entry mount=%d runtime=%d skill=%d -> child=%d ptr=0x%08X->0x%08X",
                mountItemId,
                runtimeSkillId,
                originalEntrySkillId,
                rewrittenEntrySkillId,
                arg1,
                routeArg1);
        }
    }

    const int result = oMountedDemonJumpActionRouteB29C70
                           ? oMountedDemonJumpActionRouteB29C70(
                                 thisPtr,
                                 routeArg1,
                                 arg2,
                                 arg3)
                           : 0;
    if (trace)
    {
        WriteLogFmt(
            "[MountDemonJumpAction] B29C70 leave caller=0x%08X result=%d mount=%d",
            callerRet,
            result,
            mountItemId);
    }
    return result;
}

static int __fastcall hkMountedDemonJumpActionRouteB24010(
    void *thisPtr,
    void * /*edxUnused*/,
    DWORD arg1,
    DWORD arg2)
{
    const DWORD callerRet = (DWORD)(uintptr_t)_ReturnAddress();
    int runtimeSkillId = 0;
    int mountItemId = 0;
    const bool trace =
        callerRet == 0x00B31320 &&
        IsMountedDemonJumpCrashTraceFresh(&runtimeSkillId, &mountItemId);
    if (trace)
    {
        WriteLogFmt(
            "[MountDemonJumpAction] B24010 enter caller=0x%08X runtime=%d mount=%d this=0x%08X args=[0x%08X,0x%08X]",
            callerRet,
            runtimeSkillId,
            mountItemId,
            (DWORD)(uintptr_t)thisPtr,
            arg1,
            arg2);
    }

    const int result = oMountedDemonJumpActionRouteB24010
                           ? oMountedDemonJumpActionRouteB24010(thisPtr, arg1, arg2)
                           : 0;
    if (trace)
    {
        WriteLogFmt(
            "[MountDemonJumpAction] B24010 leave caller=0x%08X result=%d mount=%d",
            callerRet,
            result,
            mountItemId);
    }
    return result;
}

static int __fastcall hkMountedDemonJumpActionRouteB24EA0(
    void *thisPtr,
    void * /*edxUnused*/,
    DWORD arg1,
    DWORD arg2)
{
    const DWORD callerRet = (DWORD)(uintptr_t)_ReturnAddress();
    int runtimeSkillId = 0;
    int mountItemId = 0;
    const bool trace =
        callerRet == 0x00B31337 &&
        IsMountedDemonJumpCrashTraceFresh(&runtimeSkillId, &mountItemId);
    if (trace)
    {
        WriteLogFmt(
            "[MountDemonJumpAction] B24EA0 enter caller=0x%08X runtime=%d mount=%d this=0x%08X args=[0x%08X,0x%08X]",
            callerRet,
            runtimeSkillId,
            mountItemId,
            (DWORD)(uintptr_t)thisPtr,
            arg1,
            arg2);
    }

    const int result = oMountedDemonJumpActionRouteB24EA0
                           ? oMountedDemonJumpActionRouteB24EA0(thisPtr, arg1, arg2)
                           : 0;
    if (trace)
    {
        WriteLogFmt(
            "[MountDemonJumpAction] B24EA0 leave caller=0x%08X result=%d mount=%d",
            callerRet,
            result,
            mountItemId);
    }
    return result;
}

static int __fastcall hkMountedDemonJumpActionRouteB26550(
    void *thisPtr,
    void * /*edxUnused*/,
    int skillId)
{
    const DWORD callerRet = (DWORD)(uintptr_t)_ReturnAddress();
    int runtimeSkillId = 0;
    int mountItemId = 0;
    const bool trace =
        callerRet == 0x00B31345 &&
        IsMountedDemonJumpCrashTraceFresh(&runtimeSkillId, &mountItemId);
    if (trace)
    {
        WriteLogFmt(
            "[MountDemonJumpAction] B26550 enter caller=0x%08X runtime=%d mount=%d this=0x%08X argSkill=%d",
            callerRet,
            runtimeSkillId,
            mountItemId,
            (DWORD)(uintptr_t)thisPtr,
            skillId);
    }

    const int result = oMountedDemonJumpActionRouteB26550
                           ? oMountedDemonJumpActionRouteB26550(thisPtr, skillId)
                           : 0;
    if (trace)
    {
        WriteLogFmt(
            "[MountDemonJumpAction] B26550 leave caller=0x%08X result=%d mount=%d",
            callerRet,
            result,
            mountItemId);
    }
    return result;
}

static int __fastcall hkMountedDemonJumpActionRouteB26050(
    void *thisPtr,
    void * /*edxUnused*/,
    int skillId)
{
    const DWORD callerRet = (DWORD)(uintptr_t)_ReturnAddress();
    int runtimeSkillId = 0;
    int mountItemId = 0;
    const bool trace =
        callerRet == 0x00B313F6 &&
        IsMountedDemonJumpCrashTraceFresh(&runtimeSkillId, &mountItemId);
    if (trace)
    {
        WriteLogFmt(
            "[MountDemonJumpAction] B26050 enter caller=0x%08X runtime=%d mount=%d this=0x%08X argSkill=%d",
            callerRet,
            runtimeSkillId,
            mountItemId,
            (DWORD)(uintptr_t)thisPtr,
            skillId);
    }

    const int result = oMountedDemonJumpActionRouteB26050
                           ? oMountedDemonJumpActionRouteB26050(thisPtr, skillId)
                           : 0;
    if (trace)
    {
        WriteLogFmt(
            "[MountDemonJumpAction] B26050 leave caller=0x%08X result=%d mount=%d",
            callerRet,
            result,
            mountItemId);
    }
    return result;
}

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

static bool TryPreseedMountedDemonJumpLevelCache(
    const char *hookTag,
    DWORD callerRet,
    int skillId,
    int lookupSkillId,
    void *cachePtr)
{
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

static void ObserveMountedDemonJumpLevelQueryCaller(
    const char *hookTag,
    DWORD callerRet,
    int skillId,
    int lookupSkillId,
    int flags,
    int rawResult,
    int finalResult)
{
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

static void ObserveMountedDemonJumpFreshChildLevelLookup(
    const char *hookTag,
    DWORD callerRet,
    int skillId,
    int lookupSkillId,
    void *cachePtr)
{
    int mountItemId = 0;
    int childSkillId = 0;
    if (!TryResolveMountedDemonJumpFreshChildLevelLookup(
            callerRet,
            skillId,
            lookupSkillId,
            cachePtr,
            &mountItemId,
            &childSkillId))
    {
        return;
    }

    int recentChildSkillId = 0;
    const bool hasMatchingRecentChild =
        TryGetRecentMountedDemonJumpNativeChildSkill(
            mountItemId,
            &recentChildSkillId,
            nullptr,
            250) &&
        recentChildSkillId == childSkillId;

    if (!hasMatchingRecentChild)
    {
        RememberMountedDemonJumpNativeChildSkill(
            mountItemId,
            childSkillId,
            hookTag ? hookTag : "skill-level");
    }

    const bool isSkillLevel7DBC50Hook =
        hookTag &&
        strcmp(hookTag, "7DBC50") == 0;
    const bool shouldUseRawContextOnlyFor7DBC50 =
        isSkillLevel7DBC50Hook &&
        // v23.53 failure logs showed the repeated virtual child promotion flood
        // came from the support lookup loop at 00A04B7A (80001099/2301014/
        // 80001095/30001902), not from the primary 00433B4C child lookup that
        // is still needed to seed the first 30010183 up-child correctly.
        callerRet == 0x00A04B7A;
    int rootSkillId = 0;
    int currentSkillId = 0;
    const bool hasContext =
        shouldUseRawContextOnlyFor7DBC50
            ? TryReadMountedDemonJumpContextState(
                  &rootSkillId,
                  &currentSkillId,
                  nullptr)
            : TryReadMountedDemonJumpEffectiveContextState(
                  mountItemId,
                  &rootSkillId,
                  &currentSkillId);
    const bool hasMatchingContext =
        hasContext &&
        rootSkillId == 30010110 &&
        currentSkillId == childSkillId;
    const bool hasRecentIntent =
        HasRecentMountedDemonJumpIntent(mountItemId, 250);
    const bool resolveActive =
        SkillOverlayBridgeIsMountedRuntimeDefinitionResolveActive();
    const bool contextClearActive =
        IsMountedDemonJumpContextClearActive();
    const bool suppressIntentRearm =
        contextClearActive ||
        resolveActive;
    const bool allowResolveSafeUpPrime =
        resolveActive &&
        !contextClearActive &&
        childSkillId == 30010183 &&
        hasRecentIntent &&
        !isSkillLevel7DBC50Hook &&
        !hasMatchingContext;
    const bool shouldRefresh7DBC50UpIntentOnly =
        !contextClearActive &&
        childSkillId == 30010183 &&
        hasRecentIntent &&
        !hasMatchingContext &&
        isSkillLevel7DBC50Hook &&
        (callerRet == 0x00433B4C ||
         callerRet == 0x00A04B7A ||
         callerRet == 0x00AE0479);
    const bool shouldRefreshResolveActiveUpIntentOnly =
        resolveActive &&
        shouldRefresh7DBC50UpIntentOnly;
    bool primedFreshUpChildContext = false;
    int primedFreshUpChildCurrentSkillId = 0;

    if (shouldRefresh7DBC50UpIntentOnly &&
        !shouldRefreshResolveActiveUpIntentOnly)
    {
        // v23.47 logs show the very first mounted jump can stall before the
        // real B22630/575D60 path even begins because 7DBC50 keeps rearming
        // full demon intent and re-priming the same up child dozens of times:
        //   skip eager intent
        //   reason=7DBC50 ... primed=1
        // Preserve the fresh up-child probe and only refresh the narrow
        // demon recent-intent tick here; let the later late-route hook do the
        // actual prime exactly once closer to the native action chain.
        ObserveMountedRuntimeSkillIntent(
            MountedRuntimeSkillKind_DemonJump,
            mountItemId);
        static LONG
            s_mountedDemonJumpFreshChildPreserveGateIntentRefreshLogBudget =
                48;
        if (InterlockedDecrement(
                &s_mountedDemonJumpFreshChildPreserveGateIntentRefreshLogBudget) >=
            0)
        {
            WriteLogFmt(
                "[MountDemonJumpLevelSeed] %s caller=0x%08X query=%d lookup=%d mount=%d child=%d preserve-gate intent-refresh=1",
                hookTag ? hookTag : "skill-level",
                callerRet,
                skillId,
                lookupSkillId,
                mountItemId,
                childSkillId);
        }
    }
    else if ((!hasRecentIntent || !hasMatchingContext) &&
        (!suppressIntentRearm || allowResolveSafeUpPrime))
    {
        if (!suppressIntentRearm)
        {
            ObserveMountedDemonJumpIntent(
                mountItemId,
                hookTag ? hookTag : "skill-level-fresh-child");
        }
        if (childSkillId == 30010183 &&
            !hasMatchingContext)
        {
            if (allowResolveSafeUpPrime)
            {
                int primedFreshUpChildRootSkillId = 0;
                primedFreshUpChildContext =
                    TryManualPrimeMountedDemonJumpContextResolveSafe(
                        mountItemId,
                        hookTag ? hookTag : "skill-level-up-child-resolve-safe",
                        &primedFreshUpChildCurrentSkillId,
                        &primedFreshUpChildRootSkillId);
            }
            else
            {
                primedFreshUpChildContext =
                    PrimeMountedDemonJumpContextIfNeeded(
                        mountItemId,
                        hookTag ? hookTag : "skill-level-up-child",
                        &primedFreshUpChildCurrentSkillId);
            }
        }
    }
    else if (shouldRefreshResolveActiveUpIntentOnly)
    {
        // Pure failure logs can stop at `7DBC50 remember child` without ever
        // reaching 575D60/B22630. Refresh only the narrow demon recent-intent
        // tick here so the next real up pair can still consume the fresh child
        // without recursively forcing another prime inside the 7DBC50 hook.
        ObserveMountedRuntimeSkillIntent(
            MountedRuntimeSkillKind_DemonJump,
            mountItemId);
        static LONG s_mountedDemonJumpFreshChildResolveIntentRefreshLogBudget =
            48;
        if (InterlockedDecrement(
                &s_mountedDemonJumpFreshChildResolveIntentRefreshLogBudget) >=
            0)
        {
            WriteLogFmt(
                "[MountDemonJumpLevelSeed] %s caller=0x%08X query=%d lookup=%d mount=%d child=%d resolve-active intent-refresh=1",
                hookTag ? hookTag : "skill-level",
                callerRet,
                skillId,
                lookupSkillId,
                mountItemId,
                childSkillId);
        }
    }
    else if (suppressIntentRearm)
    {
        static LONG s_mountedDemonJumpFreshChildSkipIntentLogBudget = 48;
        if (InterlockedDecrement(
                &s_mountedDemonJumpFreshChildSkipIntentLogBudget) >= 0)
        {
            WriteLogFmt(
                "[MountDemonJumpLevelSeed] %s caller=0x%08X query=%d lookup=%d mount=%d child=%d recentMatch=%d intent=%d root=%d current=%d context=%d skip-intent-rearm=%s",
                hookTag ? hookTag : "skill-level",
                callerRet,
                skillId,
                lookupSkillId,
                mountItemId,
                childSkillId,
                hasMatchingRecentChild ? 1 : 0,
                hasRecentIntent ? 1 : 0,
                hasContext ? rootSkillId : 0,
                hasContext ? currentSkillId : 0,
                hasContext ? 1 : 0,
                resolveActive ? "resolve-active" : "context-clear");
        }
    }

    static LONG s_mountedDemonJumpFreshChildLevelLookupLogBudget = 64;
    if (InterlockedDecrement(
            &s_mountedDemonJumpFreshChildLevelLookupLogBudget) >= 0)
    {
        WriteLogFmt(
            "[MountDemonJumpLevelSeed] %s caller=0x%08X query=%d lookup=%d mount=%d child=%d recentMatch=%d intent=%d root=%d current=%d context=%d prime-up=%d primedCurrent=%d ctxRead=%s",
            hookTag ? hookTag : "skill-level",
            callerRet,
            skillId,
            lookupSkillId,
            mountItemId,
            childSkillId,
            hasMatchingRecentChild ? 1 : 0,
            hasRecentIntent ? 1 : 0,
            hasContext ? rootSkillId : 0,
            hasContext ? currentSkillId : 0,
            hasContext ? 1 : 0,
            primedFreshUpChildContext ? 1 : 0,
            primedFreshUpChildContext
                ? primedFreshUpChildCurrentSkillId
                : 0,
            shouldUseRawContextOnlyFor7DBC50 ? "raw" : "effective");
    }
}

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

