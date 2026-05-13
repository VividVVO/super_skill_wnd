#include "runtime_feature_mounted_double_jump_runtime_release_classifier.inl"
#include "runtime_feature_mounted_double_jump_runtime_mount_common.inl"

#include "runtime_feature_mounted_double_jump_state.inl"

static bool TryReadCurrentUserLocalPtr(void **userLocalOut)
{
    if (userLocalOut)
    {
        *userLocalOut = nullptr;
    }

    if (!userLocalOut ||
        SafeIsBadReadPtr(reinterpret_cast<void *>(ADDR_UserLocal), sizeof(DWORD)))
    {
        return false;
    }

    DWORD userLocal = 0;
    __try
    {
        userLocal = *reinterpret_cast<DWORD *>(ADDR_UserLocal);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }

    if (!userLocal)
    {
        return false;
    }

    *userLocalOut = reinterpret_cast<void *>(static_cast<uintptr_t>(userLocal));
    return true;
}

static bool TryReadMountedDemonJumpContextState(
    int *rootSkillIdOut,
    int *currentSkillIdOut,
    DWORD *userLocalOut)
{
    if (rootSkillIdOut)
    {
        *rootSkillIdOut = 0;
    }
    if (currentSkillIdOut)
    {
        *currentSkillIdOut = 0;
    }
    if (userLocalOut)
    {
        *userLocalOut = 0;
    }

    void *userLocal = nullptr;
    if (!TryReadCurrentUserLocalPtr(&userLocal))
    {
        return false;
    }

    const uintptr_t userLocalAddr = reinterpret_cast<uintptr_t>(userLocal);
    const uintptr_t contextAddr = userLocalAddr + kMountedDemonJumpContextOffset;
    const uintptr_t rootSkillAddr = userLocalAddr + kMountedDemonJumpContextRootSkillOffset;
    if (SafeIsBadReadPtr(reinterpret_cast<void *>(contextAddr), sizeof(DWORD) * 2))
    {
        return false;
    }

    DWORD contextHead = 0;
    int rootSkillId = 0;
    __try
    {
        contextHead = *reinterpret_cast<DWORD *>(contextAddr);
        rootSkillId = *reinterpret_cast<int *>(rootSkillAddr);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }

    if (!contextHead || rootSkillId <= 0)
    {
        return false;
    }

    int currentSkillId = 0;
    if (currentSkillIdOut)
    {
        tMountedDemonJumpContextCurrentSkillFn currentSkillFn =
            reinterpret_cast<tMountedDemonJumpContextCurrentSkillFn>(
                ADDR_MountedDemonJumpContextCurrentSkill4300A0);
        if (currentSkillFn)
        {
            __try
            {
                currentSkillId = currentSkillFn(
                    reinterpret_cast<void *>(contextAddr));
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                currentSkillId = 0;
            }
        }
    }

    if (rootSkillIdOut)
    {
        *rootSkillIdOut = rootSkillId;
    }
    if (currentSkillIdOut)
    {
        *currentSkillIdOut = currentSkillId > 0 ? currentSkillId : rootSkillId;
    }
    if (userLocalOut)
    {
        *userLocalOut = static_cast<DWORD>(userLocalAddr);
    }
    return true;
}

static bool HasMountedDemonJumpContextPrimedForMount(
    int mountItemId,
    int *currentSkillIdOut,
    int *rootSkillIdOut)
{
    if (currentSkillIdOut)
    {
        *currentSkillIdOut = 0;
    }
    if (rootSkillIdOut)
    {
        *rootSkillIdOut = 0;
    }

    if (mountItemId <= 0)
    {
        return false;
    }

    int currentMountItemId = 0;
    if (!TryReadCurrentUserMountItemId(&currentMountItemId) ||
        currentMountItemId != mountItemId)
    {
        return false;
    }

    const int configuredSkillId = ResolveMountedRuntimeSkillIdForKind(
        MountedRuntimeSkillKind_DemonJump,
        mountItemId);
    if (configuredSkillId <= 0)
    {
        return false;
    }

    int rootSkillId = 0;
    int currentSkillId = 0;
    if (!TryReadMountedDemonJumpContextState(
            &rootSkillId,
            &currentSkillId,
            nullptr) ||
        rootSkillId != configuredSkillId)
    {
        return false;
    }

    if (!IsMountedDemonJumpRuntimeChildSkillId(currentSkillId) &&
        (currentSkillId <= 0 ||
         currentSkillId == configuredSkillId) &&
        HasRecentMountedDemonJumpIntent(
            mountItemId,
            kMountedDemonJumpLateChildCacheMatchMaxAgeMs))
    {
        int recentChildSkillId = 0;
        if (TryGetRecentMountedDemonJumpNativeChildSkill(
                mountItemId,
                &recentChildSkillId,
                nullptr,
                kMountedDemonJumpLateChildCacheMatchMaxAgeMs) &&
            IsMountedDemonJumpRuntimeChildSkillId(recentChildSkillId) &&
            SkillOverlayBridgeCanUseMountedDemonJumpRuntimeSkill(
                mountItemId,
                recentChildSkillId))
        {
            // Evidence from v23.04 job-100 logs:
            // B300AC can already reroute root/proxy release to child=30010183,
            // but the immediate action chain still re-reads root/current as
            // 30010110/30010110 and loses the local up-jump motion.
            // Promote the fresh native child cache only inside the short
            // recent-intent window so the action/late path sees the same
            // child-shaped context as the previously successful build.
            static LONG s_mountedDemonJumpContextPromoteFreshChildLogBudget = 48;
            if (InterlockedDecrement(
                    &s_mountedDemonJumpContextPromoteFreshChildLogBudget) >= 0)
            {
                WriteLogFmt(
                    "[MountDemonJumpContext] promote fresh child mount=%d root=%d current=%d -> child=%d intent=1",
                    mountItemId,
                    rootSkillId,
                    currentSkillId,
                    recentChildSkillId);
            }
            currentSkillId = recentChildSkillId;
        }
        else
        {
            int probeChildSkillId = 0;
            if (TryGetRecentMountedDemonJumpGateProbePreferredChildSkillId(
                    mountItemId,
                    &probeChildSkillId,
                    kMountedDemonJumpIntentMaxAgeMs) &&
                probeChildSkillId == 30010183 &&
                SkillOverlayBridgeCanUseMountedDemonJumpRuntimeSkill(
                    mountItemId,
                    probeChildSkillId))
            {
                // v23.43 logs show a slow first mounted up-jump can age past
                // the short recent-child cache while the up gate-probe is still
                // fresh. Keep this fallback narrow to the verified up-child so
                // later late-route helpers still see the intended child context.
                static LONG s_mountedDemonJumpContextPromoteProbeChildLogBudget =
                    32;
                if (InterlockedDecrement(
                        &s_mountedDemonJumpContextPromoteProbeChildLogBudget) >=
                    0)
                {
                    WriteLogFmt(
                        "[MountDemonJumpContext] promote probe child mount=%d root=%d current=%d -> child=%d intent=1",
                        mountItemId,
                        rootSkillId,
                        currentSkillId,
                        probeChildSkillId);
                }
                currentSkillId = probeChildSkillId;
            }
        }
    }

    if (currentSkillIdOut)
    {
        *currentSkillIdOut = currentSkillId > 0 ? currentSkillId : rootSkillId;
    }
    if (rootSkillIdOut)
    {
        *rootSkillIdOut = rootSkillId;
    }
    return true;
}

static bool HasFreshMountedRuntimeSkillNativeReleaseRaw(
    MountedRuntimeSkillKind kind,
    int expectedSkillId,
    int expectedMountItemId,
    DWORD maxAgeMs = 450)
{
    if (!IsMountedRuntimeSkillHooksEnabledForKind(kind) ||
        expectedSkillId <= 0 ||
        expectedMountItemId <= 0)
    {
        return false;
    }

    const LONG recentSkillId =
        InterlockedCompareExchange(
            &g_recentMountedRuntimeSkillNativeReleaseSkillId[kind],
            0,
            0);
    const LONG recentMountItemId =
        InterlockedCompareExchange(
            &g_recentMountedRuntimeSkillNativeReleaseItemId[kind],
            0,
            0);
    const LONG recentTick =
        InterlockedCompareExchange(
            &g_recentMountedRuntimeSkillNativeReleaseTick[kind],
            0,
            0);
    if (recentSkillId != expectedSkillId ||
        recentMountItemId != expectedMountItemId ||
        recentTick <= 0)
    {
        return false;
    }

    const DWORD nowTick = GetTickCount();
    return nowTick - static_cast<DWORD>(recentTick) <= maxAgeMs;
}

static bool TryReadMountedDemonJumpEffectiveContextState(
    int mountItemId,
    int *rootSkillIdOut,
    int *currentSkillIdOut)
{
    if (rootSkillIdOut)
    {
        *rootSkillIdOut = 0;
    }
    if (currentSkillIdOut)
    {
        *currentSkillIdOut = 0;
    }

    if (mountItemId > 0 &&
        HasMountedDemonJumpContextPrimedForMount(
            mountItemId,
            currentSkillIdOut,
            rootSkillIdOut))
    {
        return true;
    }

    return TryReadMountedDemonJumpContextState(
        rootSkillIdOut,
        currentSkillIdOut,
        nullptr);
}


#include "runtime_feature_mounted_double_jump_runtime_context_prime.inl"

static bool TryReadCurrentUserMountItemId(int *mountItemIdOut)
{
    if (!mountItemIdOut || SafeIsBadReadPtr(reinterpret_cast<void *>(ADDR_UserLocal), sizeof(DWORD)))
    {
        return false;
    }

    DWORD userLocal = 0;
    __try
    {
        userLocal = *reinterpret_cast<DWORD *>(ADDR_UserLocal);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
    if (!userLocal)
    {
        return false;
    }

    const uintptr_t mountItemIdAddr = static_cast<uintptr_t>(userLocal) + 0x454;
    if (SafeIsBadReadPtr(reinterpret_cast<void *>(mountItemIdAddr), sizeof(DWORD)))
    {
        return false;
    }

    int mountItemId = 0;
    __try
    {
        mountItemId = *reinterpret_cast<int *>(mountItemIdAddr);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }

    if (mountItemId <= 0)
    {
        return false;
    }
    *mountItemIdOut = mountItemId;
    return true;
}

static bool TryGetActiveMountedSoaringFlightItemId(int *mountItemIdOut, DWORD maxAgeMs = 0)
{
    if (!mountItemIdOut)
    {
        return false;
    }

    const LONG activeMountItemId =
        InterlockedCompareExchange(&g_activeMountedSoaringFlightItemId, 0, 0);
    if (activeMountItemId <= 0)
    {
        return false;
    }

    const LONG activeTick =
        InterlockedCompareExchange(&g_activeMountedSoaringFlightTick, 0, 0);
    if (activeTick <= 0)
    {
        return false;
    }

    const DWORD nowTick = GetTickCount();
    const DWORD allowedAgeMs =
        maxAgeMs > 0 ? maxAgeMs : 10000;
    if (nowTick - static_cast<DWORD>(activeTick) > allowedAgeMs)
    {
        return false;
    }

    *mountItemIdOut = static_cast<int>(activeMountItemId);
    return true;
}

static bool TryGetRecentMountedFlightScaleSampleMountItemId(
    int *mountItemIdOut,
    DWORD maxAgeMs = 10000)
{
    if (!mountItemIdOut)
    {
        return false;
    }

    const MountedFlightPhysicsScaleSample sample = g_MountedFlightPhysicsScaleSample;
    if (sample.mountItemId <= 0 || sample.tick == 0)
    {
        return false;
    }

    const DWORD nowTick = GetTickCount();
    const DWORD allowedAgeMs = maxAgeMs > 0 ? maxAgeMs : 10000;
    if (nowTick - sample.tick > allowedAgeMs)
    {
        return false;
    }

    *mountItemIdOut = sample.mountItemId;
    return true;
}

static void ClearMountedSoaringRuntimeFallbackState();

static bool TryResolveCurrentUserMountItemIdWithFallback(
    int *mountItemIdOut,
    const char **sourceOut)
{
    if (mountItemIdOut)
    {
        *mountItemIdOut = 0;
    }
    if (sourceOut)
    {
        *sourceOut = nullptr;
    }

    int mountItemId = 0;
    const char *source = nullptr;
    bool shouldObserveExtendedMountContext = false;
    if (TryReadCurrentUserMountItemId(&mountItemId))
    {
        source = "user";
        shouldObserveExtendedMountContext = true;
    }
    else if (TryGetRecentExtendedMountContext(&mountItemId) &&
             IsExtendedMountSoaringContextMount(mountItemId))
    {
        source = "recent-extended";
    }
    else if (TryGetActiveMountedSoaringFlightItemId(
                 &mountItemId,
                 kMountedSoaringFallbackGraceMs) &&
             IsExtendedMountSoaringContextMount(mountItemId))
    {
        source = "active-soaring";
    }
    else
    {
        // Do not let the cached scale baseline masquerade as a live mount
        // source. That cache is useful to preserve the next legitimate flight
        // boost sample, but using it as "current mount" resurrects stale mount
        // context after dismount.
        ClearMountedSoaringRuntimeFallbackState();
        return false;
    }

    if (shouldObserveExtendedMountContext &&
        IsExtendedMountSoaringContextMount(mountItemId))
    {
        ObserveExtendedMountContext(mountItemId);
    }

    if (mountItemIdOut)
    {
        *mountItemIdOut = mountItemId;
    }
    if (sourceOut)
    {
        *sourceOut = source;
    }
    return true;
}

#include "runtime_feature_mounted_double_jump_resolve.inl"

static bool TryResolveMountItemIdFromContextPointer(void *mountContext, int *mountItemIdOut)
{
    if (!mountContext || !mountItemIdOut)
    {
        return false;
    }

    if (SafeIsBadReadPtr(mountContext, sizeof(DWORD)))
    {
        return false;
    }

    tMountContextGetItemIdFn getItemIdFn =
        reinterpret_cast<tMountContextGetItemIdFn>(ADDR_7D4CA0);
    if (!getItemIdFn)
    {
        return false;
    }

    int mountItemId = 0;
    __try
    {
        mountItemId = getItemIdFn(mountContext);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }

    if (mountItemId <= 0)
    {
        return false;
    }

    *mountItemIdOut = mountItemId;
    return true;
}

static bool TryReadMountItemIdFromPlayerObject(void *playerObj, int *mountItemIdOut)
{
    int mountItemId = 0;
    if (!TryReadMountItemIdFromPlayerObjectRaw(playerObj, &mountItemId) ||
        mountItemId <= 0)
    {
        return false;
    }

    *mountItemIdOut = mountItemId;
    return true;
}

static bool TryReadMountItemIdFromPlayerObjectRaw(void *playerObj, int *mountItemIdOut)
{
    if (!playerObj || !mountItemIdOut)
    {
        return false;
    }

    const uintptr_t mountItemIdAddr = reinterpret_cast<uintptr_t>(playerObj) + 0x454;
    if (SafeIsBadReadPtr(reinterpret_cast<void *>(mountItemIdAddr), sizeof(DWORD)))
    {
        return false;
    }

    int mountItemId = 0;
    __try
    {
        mountItemId = *reinterpret_cast<int *>(mountItemIdAddr);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }

    *mountItemIdOut = mountItemId;
    return true;
}

static bool TryResolveMountedMovementDataKeyFromMountItemId(int mountItemId, int *dataKeyOut)
{
    if (!dataKeyOut)
    {
        return false;
    }
    *dataKeyOut = 0;

    if (mountItemId <= 0)
    {
        return false;
    }

    tMountItemInfoLookupFn lookupFn =
        reinterpret_cast<tMountItemInfoLookupFn>(ADDR_6545A0);
    tMountItemInfoDataKeyFn dataKeyFn =
        reinterpret_cast<tMountItemInfoDataKeyFn>(ADDR_B22500);
    if (!lookupFn || !dataKeyFn ||
        SafeIsBadReadPtr(reinterpret_cast<void *>(static_cast<uintptr_t>(ADDR_F59D34)), sizeof(void *)))
    {
        return false;
    }

    void *mountInfoTable = nullptr;
    void *mountInfo = nullptr;
    int dataKey = 0;
    __try
    {
        mountInfoTable =
            *reinterpret_cast<void **>(static_cast<uintptr_t>(ADDR_F59D34));
        if (!mountInfoTable)
        {
            return false;
        }

        // Follow B92D10's native chain exactly:
        //   mountItemId -> sub_6545A0(dword_F59D34, itemId) -> sub_B22500(info)
        // This yields the same runtime data key later passed to 888B30, so we
        // can prime flight speed immediately instead of waiting for the first
        // natural mounted movement lookup to arrive a few seconds later.
        mountInfo = lookupFn(mountInfoTable, mountItemId);
        if (!mountInfo)
        {
            return false;
        }

        dataKey = dataKeyFn(mountInfo);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }

    if (dataKey <= 0)
    {
        return false;
    }

    *dataKeyOut = dataKey;

    static LONG s_mountDataKeyResolveLogBudget = 24;
    if (InterlockedDecrement(&s_mountDataKeyResolveLogBudget) >= 0)
    {
        WriteLogFmt(
            "[MountDataKeyResolve] mount=%d key=%d source=B92D10-native",
            mountItemId,
            dataKey);
    }
    return true;
}

