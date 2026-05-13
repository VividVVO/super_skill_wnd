static bool IsMountMovementObservedDataLookupCaller(DWORD callerRet)
{
    return callerRet == ADDR_B92F2C ||
           callerRet == ADDR_B930C6 ||
           callerRet == ADDR_B932B2;
}

static void RememberRecentMountedMovementRawSample(
    int mountItemId,
    int dataKey,
    int speed,
    int jump)
{
    if (mountItemId <= 0 || dataKey <= 0 || speed <= 0 || jump <= 0)
    {
        return;
    }

    const DWORD nowTick = GetTickCount();
    InterlockedExchange(&g_RecentMountedMovementRawMountItemId, mountItemId);
    InterlockedExchange(&g_RecentMountedMovementRawDataKey, dataKey);
    InterlockedExchange(&g_RecentMountedMovementRawSpeed, speed);
    InterlockedExchange(&g_RecentMountedMovementRawJump, jump);
    InterlockedExchange(&g_RecentMountedMovementRawTick, static_cast<LONG>(nowTick));
}

static bool TryGetRecentMountedMovementRawSample(
    int *mountItemIdOut,
    int *dataKeyOut,
    int *speedOut,
    int *jumpOut,
    DWORD maxAgeMs)
{
    if (mountItemIdOut)
    {
        *mountItemIdOut = 0;
    }
    if (dataKeyOut)
    {
        *dataKeyOut = 0;
    }
    if (speedOut)
    {
        *speedOut = 0;
    }
    if (jumpOut)
    {
        *jumpOut = 0;
    }

    const int mountItemId = InterlockedCompareExchange(
        &g_RecentMountedMovementRawMountItemId,
        0,
        0);
    const int dataKey = InterlockedCompareExchange(
        &g_RecentMountedMovementRawDataKey,
        0,
        0);
    const int speed = InterlockedCompareExchange(
        &g_RecentMountedMovementRawSpeed,
        0,
        0);
    const int jump = InterlockedCompareExchange(
        &g_RecentMountedMovementRawJump,
        0,
        0);
    const DWORD tick = static_cast<DWORD>(InterlockedCompareExchange(
        &g_RecentMountedMovementRawTick,
        0,
        0));
    if (mountItemId <= 0 || dataKey <= 0 || speed <= 0 || jump <= 0 || tick == 0)
    {
        return false;
    }

    const DWORD allowedAgeMs = maxAgeMs > 0 ? maxAgeMs : 1000;
    const DWORD nowTick = GetTickCount();
    if (nowTick - tick > allowedAgeMs)
    {
        return false;
    }

    int currentMountItemId = 0;
    if (!TryReadCurrentUserMountItemId(&currentMountItemId) ||
        currentMountItemId <= 0)
    {
        // Mounted raw samples should only affect the live mounted output path.
        // Once dismounted, keeping the last sample around leaks boosted speed
        // and jump for a short window before the next stat refresh catches up.
        ClearRecentMountedMovementRawSample();
        return false;
    }

    if (currentMountItemId != mountItemId)
    {
        return false;
    }

    if (mountItemIdOut)
    {
        *mountItemIdOut = mountItemId;
    }
    if (dataKeyOut)
    {
        *dataKeyOut = dataKey;
    }
    if (speedOut)
    {
        *speedOut = speed;
    }
    if (jumpOut)
    {
        *jumpOut = jump;
    }

    return true;
}

static void ClearRecentMountedMovementRawSample()
{
    InterlockedExchange(&g_RecentMountedMovementRawMountItemId, 0);
    InterlockedExchange(&g_RecentMountedMovementRawDataKey, 0);
    InterlockedExchange(&g_RecentMountedMovementRawSpeed, 0);
    InterlockedExchange(&g_RecentMountedMovementRawJump, 0);
    InterlockedExchange(&g_RecentMountedMovementRawTick, 0);
}

static bool ShouldAttemptMountedMovementNativeCacheRebuild(
    const MountedMovementOverride &mountedOverride,
    int mountItemId,
    int dataKey,
    int currentSpeed,
    int currentJump)
{
    if (!mountedOverride.matched ||
        !mountedOverride.useNativeMovement ||
        mountItemId <= 0 ||
        dataKey <= 0)
    {
        return false;
    }

    // Pure native mode should keep the client img/cache values. If a lookup for
    // that route still lands on the historical 190/123 edge, we are most likely
    // looking at a cache that was built before our cap-removal patches landed.
    if (currentSpeed != 190 && currentJump != 123)
    {
        return false;
    }

    const DWORD nowTick = GetTickCount();
    const LONG lastDataKey = InterlockedCompareExchange(
        &g_LastMountMovementNativeCacheRebuildDataKey,
        0,
        0);
    const DWORD lastTick = static_cast<DWORD>(InterlockedCompareExchange(
        &g_LastMountMovementNativeCacheRebuildTick,
        0,
        0));
    return lastDataKey != dataKey ||
           lastTick == 0 ||
           nowTick - lastTick > 2000;
}

static void RememberMountedMovementNativeCacheRebuildAttempt(int dataKey)
{
    InterlockedExchange(&g_LastMountMovementNativeCacheRebuildDataKey, dataKey);
    InterlockedExchange(
        &g_LastMountMovementNativeCacheRebuildTick,
        static_cast<LONG>(GetTickCount()));
}

static bool RebuildMountedMovementNativeCache()
{
    if (SafeIsBadReadPtr(reinterpret_cast<void *>(static_cast<uintptr_t>(ADDR_D68820)), 5) ||
        SafeIsBadReadPtr(reinterpret_cast<void *>(static_cast<uintptr_t>(ADDR_889410)), 5))
    {
        return false;
    }

    if (InterlockedCompareExchange(&g_MountMovementNativeCacheRebuildInProgress, 1, 0) != 0)
    {
        return false;
    }

    bool ok = false;
    DWORD exceptionCode = 0;
    __try
    {
        // D68820 is the table reset path for off_F56A40; 889410 is the
        // original client-side TamingMob cache population pass.
        reinterpret_cast<void (__cdecl *)()>(
            static_cast<uintptr_t>(ADDR_D68820))();
        __asm
        {
            xor ecx, ecx
            mov eax, ADDR_889410
            call eax
        }
        ClearMountedFlightPhysicsScaleSample();
        ClearRecentMountedMovementRawSample();
        ok = true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        exceptionCode = static_cast<DWORD>(GetExceptionCode());
        ok = false;
    }

    InterlockedExchange(&g_MountMovementNativeCacheRebuildInProgress, 0);

    if (InterlockedDecrement(&g_MountMovementNativeCacheRebuildLogBudget) >= 0)
    {
        WriteLogFmt(
            "[MountMoveNativeCache] rebuild ok=%d exception=0x%08X",
            ok ? 1 : 0,
            exceptionCode);
    }
    return ok;
}

static bool TryApplyMountedMovementOverrideToCachedData(
    int mountItemId,
    int dataKey,
    uintptr_t dataPtr,
    MountedMovementOverride *appliedOverrideOut)
{
    if (appliedOverrideOut)
        *appliedOverrideOut = MountedMovementOverride();

    if (mountItemId <= 0 || dataKey <= 0 || dataPtr == 0)
        return false;

    MountedMovementOverride mountedOverride = {};
    if (!SkillOverlayBridgeResolveMountedMovementOverride(mountItemId, dataKey, mountedOverride) ||
        !mountedOverride.matched)
    {
        if (InterlockedDecrement(&g_MountMovementOverrideMissLogBudget) >= 0)
        {
            WriteLogFmt("[MountMoveMiss] stage=resolve mount=%d key=%d data=0x%08X",
                        mountItemId,
                        dataKey,
                        static_cast<DWORD>(dataPtr));
        }
        return false;
    }

    MountedMovementOverride requestedOverride = mountedOverride;
    bool normalizedFsForLargeNativeUnits = false;
    bool normalizedSwimForLargeNativeUnits = false;

    __try
    {
        const double nativeFs = *reinterpret_cast<double *>(dataPtr + 0x20);
        const DWORD nativeSwim = *reinterpret_cast<DWORD *>(dataPtr + 0x28);

        if (mountedOverride.hasFs &&
            mountedOverride.fs >= 100.0 &&
            nativeFs >= 10000.0 &&
            mountedOverride.fs < nativeFs)
        {
            double adjustedFs = mountedOverride.fs;
            for (int i = 0; i < 3 && adjustedFs < nativeFs; ++i)
            {
                adjustedFs *= 10.0;
            }
            if (adjustedFs > mountedOverride.fs)
            {
                mountedOverride.fs = adjustedFs;
                normalizedFsForLargeNativeUnits = true;
            }
        }

        if (mountedOverride.hasSwim &&
            mountedOverride.swim >= 100.0 &&
            nativeSwim >= 10000u &&
            mountedOverride.swim < static_cast<double>(nativeSwim))
        {
            double adjustedSwim = mountedOverride.swim;
            for (int i = 0; i < 3 && adjustedSwim < static_cast<double>(nativeSwim); ++i)
            {
                adjustedSwim *= 10.0;
            }
            if (adjustedSwim > mountedOverride.swim)
            {
                mountedOverride.swim = adjustedSwim;
                normalizedSwimForLargeNativeUnits = true;
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }

    if (normalizedFsForLargeNativeUnits || normalizedSwimForLargeNativeUnits)
    {
        static LONG s_mountMoveUnitNormalizationLogBudget = 24;
        if (InterlockedDecrement(&s_mountMoveUnitNormalizationLogBudget) >= 0)
        {
            WriteLogFmt(
                "[MountMoveUnitFix] mount=%d key=%d requested=[fs=%.3f swim=%.3f] applied=[fs=%.3f swim=%.3f]",
                mountItemId,
                dataKey,
                requestedOverride.hasFs ? requestedOverride.fs : -1.0,
                requestedOverride.hasSwim ? requestedOverride.swim : -1.0,
                mountedOverride.hasFs ? mountedOverride.fs : -1.0,
                mountedOverride.hasSwim ? mountedOverride.swim : -1.0);
        }
    }

    bool wroteAny = false;

    __try
    {
        if (mountedOverride.hasSpeed)
        {
            *reinterpret_cast<int *>(dataPtr + 0x14) = mountedOverride.speed;
            wroteAny = true;
        }
        if (mountedOverride.hasJump)
        {
            *reinterpret_cast<int *>(dataPtr + 0x18) = mountedOverride.jump;
            wroteAny = true;
        }
        if (mountedOverride.hasFs)
        {
            *reinterpret_cast<double *>(dataPtr + 0x20) = mountedOverride.fs;
            wroteAny = true;
        }
        if (mountedOverride.hasSwim)
        {
            const double clampedSwim = mountedOverride.swim > 0.0 ? mountedOverride.swim : 0.0;
            *reinterpret_cast<DWORD *>(dataPtr + 0x28) =
                static_cast<DWORD>(clampedSwim + 0.5);
            wroteAny = true;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        if (InterlockedDecrement(&g_MountMovementOverrideMissLogBudget) >= 0)
        {
            WriteLogFmt("[MountMoveMiss] stage=write-exception mount=%d key=%d data=0x%08X code=0x%08X",
                        mountItemId,
                        dataKey,
                        static_cast<DWORD>(dataPtr),
                        static_cast<DWORD>(GetExceptionCode()));
        }
        wroteAny = false;
    }

    if (appliedOverrideOut && mountedOverride.matched)
        *appliedOverrideOut = mountedOverride;
    return wroteAny;
}

static const char *DescribeMountMovementObservedDataLookupCaller(DWORD callerRet)
{
    switch (callerRet)
    {
    case ADDR_B92F2C:
        return "normal";
    case ADDR_B930C6:
        return "special";
    case ADDR_B932B2:
        return "mechanic";
    default:
        return "unknown";
    }
}

static int __cdecl hkMountMovementDataLookup888B30(int dataKey)
{
    const DWORD callerRet = (DWORD)(uintptr_t)_ReturnAddress();
    int result = oMountMovementDataLookup888B30
                     ? oMountMovementDataLookup888B30(dataKey)
                     : 0;
    if (!IsMountMovementObservedDataLookupCaller(callerRet))
    {
        return result;
    }

    const bool shouldLog = InterlockedDecrement(&g_MountMovementObserveLogBudget) >= 0;

    int mountItemId = 0;
    if (!TryReadCurrentUserMountItemId(&mountItemId) || mountItemId <= 0)
    {
        // Ground movement lookup must not reuse soaring/extended fallback
        // context. After dismount those fallback windows can linger briefly and
        // keep refreshing the last mounted raw sample, which makes B93B80
        // continue raising speed/jump for a few seconds on foot.
        ClearRecentMountedMovementRawSample();
        return result;
    }

    MountedMovementOverride resolvedOverride = {};
    const bool hasResolvedOverride =
        mountItemId > 0 &&
        SkillOverlayBridgeResolveMountedMovementOverride(
            mountItemId,
            dataKey,
            resolvedOverride) &&
        resolvedOverride.matched;

    bool readable = false;
    int rawSpeed = 0;
    int rawJump = 0;
    double rawFs = 0.0;
    DWORD rawSwim = 0;
    DWORD rawSpeedOverrideFlag = 0;
    DWORD rawJumpOverrideFlag = 0;
    int beforePatchSpeed = 0;
    int beforePatchJump = 0;
    double beforePatchFs = 0.0;
    DWORD beforePatchSwim = 0;
    bool hadBeforePatchSnapshot = false;
    if (result > 0 &&
        !SafeIsBadReadPtr(reinterpret_cast<void *>(static_cast<uintptr_t>(result)), 0x38))
    {
        __try
        {
            const uintptr_t dataPtr = static_cast<uintptr_t>(result);
            rawSpeed = *reinterpret_cast<int *>(dataPtr + 0x14);
            rawJump = *reinterpret_cast<int *>(dataPtr + 0x18);
            rawFs = *reinterpret_cast<double *>(dataPtr + 0x20);
            rawSwim = *reinterpret_cast<DWORD *>(dataPtr + 0x28);
            rawSpeedOverrideFlag = *reinterpret_cast<DWORD *>(dataPtr + 0x30);
            rawJumpOverrideFlag = *reinterpret_cast<DWORD *>(dataPtr + 0x34);
            readable = true;
            beforePatchSpeed = rawSpeed;
            beforePatchJump = rawJump;
            beforePatchFs = rawFs;
            beforePatchSwim = rawSwim;
            hadBeforePatchSnapshot = true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            readable = false;
        }
    }

    if (hadBeforePatchSnapshot &&
        hasResolvedOverride &&
        ShouldAttemptMountedMovementNativeCacheRebuild(
            resolvedOverride,
            mountItemId,
            dataKey,
            beforePatchSpeed,
            beforePatchJump))
    {
        RememberMountedMovementNativeCacheRebuildAttempt(dataKey);
        const bool rebuilt = RebuildMountedMovementNativeCache();
        const int rebuiltResult = rebuilt && oMountMovementDataLookup888B30
                                      ? oMountMovementDataLookup888B30(dataKey)
                                      : 0;
        if (rebuilt && rebuiltResult > 0)
        {
            result = rebuiltResult;
            readable = false;
            rawSpeed = 0;
            rawJump = 0;
            rawFs = 0.0;
            rawSwim = 0;
            rawSpeedOverrideFlag = 0;
            rawJumpOverrideFlag = 0;
            beforePatchSpeed = 0;
            beforePatchJump = 0;
            beforePatchFs = 0.0;
            beforePatchSwim = 0;
            hadBeforePatchSnapshot = false;

            if (!SafeIsBadReadPtr(reinterpret_cast<void *>(static_cast<uintptr_t>(result)), 0x38))
            {
                __try
                {
                    const uintptr_t dataPtr = static_cast<uintptr_t>(result);
                    rawSpeed = *reinterpret_cast<int *>(dataPtr + 0x14);
                    rawJump = *reinterpret_cast<int *>(dataPtr + 0x18);
                    rawFs = *reinterpret_cast<double *>(dataPtr + 0x20);
                    rawSwim = *reinterpret_cast<DWORD *>(dataPtr + 0x28);
                    rawSpeedOverrideFlag = *reinterpret_cast<DWORD *>(dataPtr + 0x30);
                    rawJumpOverrideFlag = *reinterpret_cast<DWORD *>(dataPtr + 0x34);
                    readable = true;
                    beforePatchSpeed = rawSpeed;
                    beforePatchJump = rawJump;
                    beforePatchFs = rawFs;
                    beforePatchSwim = rawSwim;
                    hadBeforePatchSnapshot = true;
                }
                __except (EXCEPTION_EXECUTE_HANDLER)
                {
                    readable = false;
                }
            }
        }
    }

    MountedMovementOverride appliedOverride = {};
    const bool overrideApplied = TryApplyMountedMovementOverrideToCachedData(
        mountItemId,
        dataKey,
        static_cast<uintptr_t>(result),
        &appliedOverride);

    if (overrideApplied &&
        result > 0 &&
        !SafeIsBadReadPtr(reinterpret_cast<void *>(static_cast<uintptr_t>(result)), 0x38))
    {
        __try
        {
            const uintptr_t dataPtr = static_cast<uintptr_t>(result);
            rawSpeed = *reinterpret_cast<int *>(dataPtr + 0x14);
            rawJump = *reinterpret_cast<int *>(dataPtr + 0x18);
            rawFs = *reinterpret_cast<double *>(dataPtr + 0x20);
            rawSwim = *reinterpret_cast<DWORD *>(dataPtr + 0x28);
            rawSpeedOverrideFlag = *reinterpret_cast<DWORD *>(dataPtr + 0x30);
            rawJumpOverrideFlag = *reinterpret_cast<DWORD *>(dataPtr + 0x34);
            readable = true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
        }
    }

    if (overrideApplied && hadBeforePatchSnapshot)
    {
        RememberMountedFlightPhysicsScaleSample(
            mountItemId,
            dataKey,
            beforePatchFs,
            static_cast<double>(beforePatchSwim),
            appliedOverride);
    }
    if (hadBeforePatchSnapshot &&
        appliedOverride.matched &&
        appliedOverride.useNativeMovement)
    {
        MountedMovementOverride nativeScaleOverride = appliedOverride;
        nativeScaleOverride.hasFs = false;
        nativeScaleOverride.fs = 0.0;
        nativeScaleOverride.hasSwim = false;
        nativeScaleOverride.swim = 0.0;

        const double normalizedFs =
            NormalizeMountedFlightPhysicsHumanScalar(beforePatchFs);
        const double normalizedSwim =
            NormalizeMountedFlightPhysicsHumanScalar(static_cast<double>(beforePatchSwim));

        if (normalizedFs > 0.0)
        {
            nativeScaleOverride.hasFs = true;
            nativeScaleOverride.fs = normalizedFs;
        }
        if (normalizedSwim > 0.0)
        {
            nativeScaleOverride.hasSwim = true;
            nativeScaleOverride.swim = normalizedSwim;
        }

        if (nativeScaleOverride.hasFs || nativeScaleOverride.hasSwim)
        {
            RememberMountedFlightPhysicsScaleSample(
                mountItemId,
                dataKey,
                100.0,
                100.0,
                nativeScaleOverride);

            static LONG s_mountMoveNativeSampleLogBudget = 24;
            if (InterlockedDecrement(&s_mountMoveNativeSampleLogBudget) >= 0)
            {
                WriteLogFmt(
                    "[MountMoveNative] sourceSkill=%d mount=%d key=%d raw=[fs=%.3f swim=%u] normalized=[fs=%.3f swim=%.3f]",
                    nativeScaleOverride.sourceSkillId,
                    mountItemId,
                    dataKey,
                    beforePatchFs,
                    beforePatchSwim,
                    nativeScaleOverride.hasFs ? nativeScaleOverride.fs : -1.0,
                    nativeScaleOverride.hasSwim ? nativeScaleOverride.swim : -1.0);
            }
        }
    }

    if (readable)
    {
        RememberRecentMountedMovementRawSample(
            mountItemId,
            dataKey,
            rawSpeed,
            rawJump);
    }

    if (readable && shouldLog)
    {
        if (overrideApplied && hadBeforePatchSnapshot)
        {
            WriteLogFmt(
                "[MountMoveFix] sourceSkill=%d mount=%d key=%d applied=[speed=%d jump=%d fs=%.3f swim=%.3f] before=[speed=%d jump=%d fs=%.3f swim=%u] after=[speed=%d jump=%d fs=%.3f swim=%u]",
                appliedOverride.sourceSkillId,
                mountItemId,
                dataKey,
                appliedOverride.hasSpeed ? appliedOverride.speed : -1,
                appliedOverride.hasJump ? appliedOverride.jump : -1,
                appliedOverride.hasFs ? appliedOverride.fs : -1.0,
                appliedOverride.hasSwim ? appliedOverride.swim : -1.0,
                beforePatchSpeed,
                beforePatchJump,
                beforePatchFs,
                beforePatchSwim,
                rawSpeed,
                rawJump,
                rawFs,
                rawSwim);
        }
        WriteLogFmt(
            "[MountMoveObserve] path=%s caller=0x%08X mount=%d key=%d data=0x%08X raw=[speed=%d jump=%d fs=%.3f swim=%u speedOverride=%u jumpOverride=%u]",
            DescribeMountMovementObservedDataLookupCaller(callerRet),
            callerRet,
            mountItemId,
            dataKey,
            result,
            rawSpeed,
            rawJump,
            rawFs,
            rawSwim,
            rawSpeedOverrideFlag,
            rawJumpOverrideFlag);
    }
    else if (shouldLog)
    {
        WriteLogFmt(
            "[MountMoveObserve] path=%s caller=0x%08X mount=%d key=%d data=0x%08X raw=unreadable",
            DescribeMountMovementObservedDataLookupCaller(callerRet),
            callerRet,
            mountItemId,
            dataKey,
            result);
    }

    return result;
}

struct MountedFlightPhysicsSlotPatch
{
    uintptr_t slotAddr;
    double originalValue;
    double patchedValue;
    size_t relativeOffset;
    bool isStateSlot;
    bool active;
};

static void ClearMountedSoaringFlightActive()
{
    InterlockedExchange(&g_activeMountedSoaringFlightItemId, 0);
    InterlockedExchange(&g_activeMountedSoaringFlightTick, 0);
    InterlockedExchange(&g_activeMountedSoaringFlightStartTick, 0);
}

static bool TryForceClearMountedSoaringFlightActiveForMount(
    int mountItemId,
    const char *reasonTag)
{
    if (mountItemId <= 0)
    {
        return false;
    }

    const LONG activeMountItemId =
        InterlockedCompareExchange(&g_activeMountedSoaringFlightItemId, 0, 0);
    if (activeMountItemId <= 0 ||
        activeMountItemId != mountItemId)
    {
        return false;
    }

    const LONG activeTick =
        InterlockedCompareExchange(&g_activeMountedSoaringFlightTick, 0, 0);
    const LONG activeStartTick =
        InterlockedCompareExchange(&g_activeMountedSoaringFlightStartTick, 0, 0);
    const DWORD nowTick = GetTickCount();
    const DWORD ageMs =
        activeTick > 0 ? (nowTick - static_cast<DWORD>(activeTick)) : 0;
    const DWORD activeDurationMs =
        activeStartTick > 0
            ? (nowTick - static_cast<DWORD>(activeStartTick))
            : 0;

    ClearMountedSoaringFlightActive();

    static LONG s_mountedSoaringFlightForcedClearLogBudget = 32;
    if (InterlockedDecrement(
            &s_mountedSoaringFlightForcedClearLogBudget) >= 0)
    {
        WriteLogFmt(
            "[MountFlightState] clear active reason=%s mount=%d age=%u duration=%u",
            reasonTag ? reasonTag : "unknown",
            mountItemId,
            ageMs,
            activeDurationMs);
    }

    return true;
}

static void ClearMountedSoaringRuntimeFallbackState()
{
    ClearExtendedMountContext();
    ClearMountedSoaringFlightActive();
}

static void ObserveMountedSoaringFlightActive(int mountItemId)
{
    if (mountItemId <= 0)
    {
        return;
    }

    const DWORD nowTick = GetTickCount();
    const LONG previousMountItemId =
        InterlockedCompareExchange(&g_activeMountedSoaringFlightItemId, 0, 0);
    const LONG previousTick =
        InterlockedCompareExchange(&g_activeMountedSoaringFlightTick, 0, 0);
    const bool shouldResetStartTick =
        previousMountItemId != mountItemId ||
        previousTick <= 0 ||
        nowTick - static_cast<DWORD>(previousTick) > kMountedSoaringFlightActiveRefreshGapMs;

    InterlockedExchange(&g_activeMountedSoaringFlightItemId, mountItemId);
    InterlockedExchange(&g_activeMountedSoaringFlightTick, static_cast<LONG>(nowTick));
    if (shouldResetStartTick ||
        InterlockedCompareExchange(&g_activeMountedSoaringFlightStartTick, 0, 0) <= 0)
    {
        InterlockedExchange(&g_activeMountedSoaringFlightStartTick, static_cast<LONG>(nowTick));
    }
}

static bool TryResolveMountedFlightPhysicsScaleForAxis(
    bool preferFs,
    int *mountItemIdOut,
    int *dataKeyOut,
    double *scaleOut,
    const char **scaleSourceOut)
{
    if (mountItemIdOut)
        *mountItemIdOut = 0;
    if (dataKeyOut)
        *dataKeyOut = 0;
    if (scaleOut)
        *scaleOut = 0.0;
    if (scaleSourceOut)
        *scaleSourceOut = nullptr;

    int mountItemId = 0;
    if (!TryResolveCurrentUserMountItemIdWithFallback(&mountItemId, nullptr) ||
        !IsExtendedMountSoaringContextMount(mountItemId))
    {
        ClearMountedSoaringRuntimeFallbackState();
        return false;
    }

    MountedFlightPhysicsScaleSample sample = {};
    if (!TryPrimeMountedFlightPhysicsScaleSampleForMount(mountItemId, &sample) &&
        !(g_MountedFlightPhysicsScaleSample.mountItemId == mountItemId &&
          TryGetMountedFlightPhysicsScaleSampleForRoute(
              mountItemId,
              g_MountedFlightPhysicsScaleSample.dataKey,
              &sample)))
    {
        return false;
    }

    DWORD activeAgeMs = 0;
    DWORD activeDurationMs = 0;
    TryGetMountedSoaringFlightTiming(
        mountItemId,
        &activeAgeMs,
        &activeDurationMs);

    MountedMovementOverride soaringOverride = {};
    const bool hasSoaringOverride =
        SkillOverlayBridgeResolveMountedSoaringOverride(mountItemId, sample.dataKey, soaringOverride);
    const double resolvedOverrideFs =
        hasSoaringOverride && soaringOverride.hasFs && soaringOverride.fs > sample.overrideFs
            ? soaringOverride.fs
            : sample.overrideFs;
    const double resolvedOverrideSwim =
        hasSoaringOverride && soaringOverride.hasSwim && soaringOverride.swim > sample.overrideSwim
            ? soaringOverride.swim
            : sample.overrideSwim;

    const double swimScale =
        (resolvedOverrideSwim > sample.nativeSwim && sample.nativeSwim > 0.0)
            ? (resolvedOverrideSwim / sample.nativeSwim)
            : 0.0;
    const double fsScale =
        (resolvedOverrideFs > sample.nativeFs && sample.nativeFs > 0.0)
            ? (resolvedOverrideFs / sample.nativeFs)
            : 0.0;

    double scale = 0.0;
    const char *source = nullptr;
    if (preferFs)
    {
        if (fsScale > 1.001)
        {
            scale = fsScale;
            source = "fs";
        }
        else if (swimScale > 1.001)
        {
            scale = swimScale;
            source = "swim";
        }
    }
    else
    {
        if (swimScale > 1.001)
        {
            scale = swimScale;
            source = "swim";
        }
        else if (fsScale > 1.001)
        {
            scale = fsScale;
            source = "fs";
        }
    }

    if (!(scale > 1.001) || !(scale < 100000.0))
    {
        return false;
    }

    if (mountItemIdOut)
    {
        *mountItemIdOut = mountItemId;
    }
    if (dataKeyOut)
    {
        *dataKeyOut = sample.dataKey;
    }
    if (scaleOut)
    {
        *scaleOut = scale;
    }
    if (scaleSourceOut)
    {
        *scaleSourceOut = source;
    }
    return true;
}

static bool TryResolveMountedFlightPhysicsScale(
    int *mountItemIdOut,
    int *dataKeyOut,
    double *scaleOut,
    const char **scaleSourceOut)
{
    return TryResolveMountedFlightPhysicsScaleForAxis(
        false,
        mountItemIdOut,
        dataKeyOut,
        scaleOut,
        scaleSourceOut);
}

static bool TryReadEncodedDoubleSlot(uintptr_t slotAddr, double *outValue)
{
    if (!slotAddr || !outValue || SafeIsBadReadPtr(reinterpret_cast<void *>(slotAddr), 12))
    {
        return false;
    }

    tEncodedDoubleReadFn readFn =
        reinterpret_cast<tEncodedDoubleReadFn>(ADDR_445A20);
    if (!readFn)
    {
        return false;
    }

    __try
    {
        *outValue = readFn(reinterpret_cast<void *>(slotAddr));
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

static bool TryWriteEncodedDoubleSlot(uintptr_t slotAddr, double value)
{
    if (!slotAddr || SafeIsBadReadPtr(reinterpret_cast<void *>(slotAddr), 12))
    {
        return false;
    }

    tEncodedDoubleWriteFn writeFn =
        reinterpret_cast<tEncodedDoubleWriteFn>(ADDR_444CA0);
    if (!writeFn)
    {
        return false;
    }

    unsigned __int64 bits = 0;
    memcpy(&bits, &value, sizeof(bits));

    __try
    {
        writeFn(
            reinterpret_cast<void *>(slotAddr),
            static_cast<int>(bits & 0xFFFFFFFFu),
            static_cast<int>(bits >> 32));
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

static bool TryPatchMountedFlightPhysicsSlot(
    uintptr_t slotAddr,
    double scale,
    MountedFlightPhysicsSlotPatch *outPatch)
{
    if (outPatch)
    {
        outPatch->slotAddr = 0;
        outPatch->originalValue = 0.0;
        outPatch->patchedValue = 0.0;
        outPatch->relativeOffset = 0;
        outPatch->isStateSlot = false;
        outPatch->active = false;
    }

    if (!slotAddr || !(scale > 1.001))
    {
        return false;
    }

    double originalValue = 0.0;
    if (!TryReadEncodedDoubleSlot(slotAddr, &originalValue) ||
        !(originalValue > 0.0) ||
        !(originalValue < 1000000.0))
    {
        return false;
    }

    const double patchedValue = originalValue * scale;
    if (!(patchedValue > originalValue) || !(patchedValue < 1000000000.0))
    {
        return false;
    }

    if (!TryWriteEncodedDoubleSlot(slotAddr, patchedValue))
    {
        return false;
    }

    if (outPatch)
    {
        outPatch->slotAddr = slotAddr;
        outPatch->originalValue = originalValue;
        outPatch->patchedValue = patchedValue;
        outPatch->active = true;
    }
    return true;
}

static void RestoreMountedFlightPhysicsSlotPatches(
    MountedFlightPhysicsSlotPatch *patches,
    size_t patchCount)
{
    if (!patches || patchCount == 0)
    {
        return;
    }

    for (size_t i = 0; i < patchCount; ++i)
    {
        if (!patches[i].active)
        {
            continue;
        }

        TryWriteEncodedDoubleSlot(patches[i].slotAddr, patches[i].originalValue);
        patches[i].active = false;
    }
}

static size_t BeginMountedFlightPhysicsScalePatchForOffsetsWithScale(
    void *thisPtr,
    const char *label,
    double scale,
    int mountItemId,
    int dataKey,
    const char *scaleSource,
    const size_t *localSlotOffsets,
    size_t localSlotCount,
    const size_t *stateSlotOffsets,
    size_t stateSlotCount,
    MountedFlightPhysicsSlotPatch *patches,
    size_t patchCapacity)
{
    if (!thisPtr || !patches || patchCapacity == 0 || !(scale > 1.001))
    {
        return 0;
    }

    for (size_t i = 0; i < patchCapacity; ++i)
    {
        patches[i].slotAddr = 0;
        patches[i].originalValue = 0.0;
        patches[i].patchedValue = 0.0;
        patches[i].relativeOffset = 0;
        patches[i].isStateSlot = false;
        patches[i].active = false;
    }

    size_t patchCount = 0;
    const uintptr_t thisValue = reinterpret_cast<uintptr_t>(thisPtr);
    for (size_t offsetIndex = 0;
         localSlotOffsets &&
         offsetIndex < localSlotCount &&
         patchCount < patchCapacity;
         ++offsetIndex)
    {
        if (TryPatchMountedFlightPhysicsSlot(
                thisValue + localSlotOffsets[offsetIndex],
                scale,
                &patches[patchCount]))
        {
            patches[patchCount].relativeOffset = localSlotOffsets[offsetIndex];
            patches[patchCount].isStateSlot = false;
            ++patchCount;
        }
    }

    uintptr_t statePtr = 0;
    if (!SafeIsBadReadPtr(reinterpret_cast<void *>(thisValue + 332), sizeof(DWORD)))
    {
        __try
        {
            statePtr = *reinterpret_cast<uintptr_t *>(thisValue + 332);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            statePtr = 0;
        }
    }

    for (size_t offsetIndex = 0;
         statePtr &&
         stateSlotOffsets &&
         offsetIndex < stateSlotCount &&
         patchCount < patchCapacity;
         ++offsetIndex)
    {
        if (TryPatchMountedFlightPhysicsSlot(
                statePtr + stateSlotOffsets[offsetIndex],
                scale,
                &patches[patchCount]))
        {
            patches[patchCount].relativeOffset = stateSlotOffsets[offsetIndex];
            patches[patchCount].isStateSlot = true;
            ++patchCount;
        }
    }

    if (patchCount > 0 && InterlockedDecrement(&g_MountedFlightPhysicsScaleLogBudget) >= 0)
    {
        WriteLogFmt(
            "[MountFlightSpeed] %s mount=%d key=%d scale=%.3f source=%s count=%u s0=%c+%u %.6f->%.6f s1=%c+%u %.6f->%.6f s2=%c+%u %.6f->%.6f s3=%c+%u %.6f->%.6f",
            label ? label : "unknown",
            mountItemId,
            dataKey,
            scale,
            scaleSource ? scaleSource : "none",
            static_cast<unsigned int>(patchCount),
            patchCount >= 1 ? (patches[0].isStateSlot ? 'S' : 'L') : '-',
            patchCount >= 1 ? static_cast<unsigned int>(patches[0].relativeOffset) : 0,
            patchCount >= 1 ? patches[0].originalValue : 0.0,
            patchCount >= 1 ? patches[0].patchedValue : 0.0,
            patchCount >= 2 ? (patches[1].isStateSlot ? 'S' : 'L') : '-',
            patchCount >= 2 ? static_cast<unsigned int>(patches[1].relativeOffset) : 0,
            patchCount >= 2 ? patches[1].originalValue : 0.0,
            patchCount >= 2 ? patches[1].patchedValue : 0.0,
            patchCount >= 3 ? (patches[2].isStateSlot ? 'S' : 'L') : '-',
            patchCount >= 3 ? static_cast<unsigned int>(patches[2].relativeOffset) : 0,
            patchCount >= 3 ? patches[2].originalValue : 0.0,
            patchCount >= 3 ? patches[2].patchedValue : 0.0,
            patchCount >= 4 ? (patches[3].isStateSlot ? 'S' : 'L') : '-',
            patchCount >= 4 ? static_cast<unsigned int>(patches[3].relativeOffset) : 0,
            patchCount >= 4 ? patches[3].originalValue : 0.0,
            patchCount >= 4 ? patches[3].patchedValue : 0.0);
    }

    return patchCount;
}

static size_t BeginMountedFlightPhysicsScalePatchForOffsets(
    void *thisPtr,
    const char *label,
    const size_t *localSlotOffsets,
    size_t localSlotCount,
    const size_t *stateSlotOffsets,
    size_t stateSlotCount,
    MountedFlightPhysicsSlotPatch *patches,
    size_t patchCapacity)
{
    int mountItemId = 0;
    int dataKey = 0;
    double scale = 0.0;
    const char *scaleSource = nullptr;
    if (!TryResolveMountedFlightPhysicsScale(&mountItemId, &dataKey, &scale, &scaleSource))
    {
        return 0;
    }
    return BeginMountedFlightPhysicsScalePatchForOffsetsWithScale(
        thisPtr,
        label,
        scale,
        mountItemId,
        dataKey,
        scaleSource,
        localSlotOffsets,
        localSlotCount,
        stateSlotOffsets,
        stateSlotCount,
        patches,
        patchCapacity);
}

static size_t BeginMountedFlightPhysicsScalePatch(
    void *thisPtr,
    const char *label,
    MountedFlightPhysicsSlotPatch *patches,
    size_t patchCapacity)
{
    static const size_t kLocalSlotOffsets[] = {
        340, 352, 364, 376, 388, 400, 412, 424, 436, 448, 460
    };
    static const size_t kStateSlotOffsets[] = { 0, 12, 24, 36 };
    return BeginMountedFlightPhysicsScalePatchForOffsets(
        thisPtr,
        label,
        kLocalSlotOffsets,
        ARRAYSIZE(kLocalSlotOffsets),
        kStateSlotOffsets,
        ARRAYSIZE(kStateSlotOffsets),
        patches,
        patchCapacity);
}

static void ObserveMountedFlightPhysicsEntry(const char *label, void *thisPtr, size_t patchCount)
{
    int mountItemId = 0;
    const char *mountSource = nullptr;
    const bool hasMountContext =
        TryResolveCurrentUserMountItemIdWithFallback(&mountItemId, &mountSource);
    DWORD activeAgeMs = 0;
    DWORD activeDurationMs = 0;
    const bool hasActiveTiming =
        hasMountContext &&
        TryGetMountedSoaringFlightTiming(
            mountItemId,
            &activeAgeMs,
            &activeDurationMs);
    int scaleMountItemId = 0;
    int dataKey = 0;
    double scale = 0.0;
    const char *scaleSource = nullptr;
    const bool hasScale =
        TryResolveMountedFlightPhysicsScaleForAxis(
            true,
            &scaleMountItemId,
            &dataKey,
            &scale,
            &scaleSource);
    if (hasScale && scaleMountItemId > 0)
    {
        mountItemId = scaleMountItemId;
    }

    if (!hasScale && patchCount == 0 && !hasMountContext && !hasActiveTiming)
    {
        return;
    }

    if (InterlockedDecrement(&g_MountedFlightPhysicsEntryLogBudget) < 0)
    {
        return;
    }

    WriteLogFmt(
        "[MountFlightEntry] %s this=0x%08X mount=%d mountSource=%s hasActive=%d age=%u duration=%u key=%d hasScale=%d scale=%.3f source=%s patchCount=%u",
        label ? label : "unknown",
        (DWORD)(uintptr_t)thisPtr,
        mountItemId,
        mountSource ? mountSource : "none",
        hasActiveTiming ? 1 : 0,
        static_cast<unsigned int>(activeAgeMs),
        static_cast<unsigned int>(activeDurationMs),
        dataKey,
        hasScale ? 1 : 0,
        scale,
        scaleSource ? scaleSource : "none",
        static_cast<unsigned int>(patchCount));
}

struct MountedFlightPhysicsStateSnapshot
{
    double posX;
    double posY;
    double velX;
    double velY;
    bool valid;
};

static bool IsReasonableMountedFlightPhysicsValue(double value)
{
    return value == value &&
           value > -1000000000.0 &&
           value < 1000000000.0;
}

static bool TryReadMountedFlightPhysicsStateSnapshot(
    void *thisPtr,
    MountedFlightPhysicsStateSnapshot *outSnapshot)
{
    if (outSnapshot)
    {
        outSnapshot->posX = 0.0;
        outSnapshot->posY = 0.0;
        outSnapshot->velX = 0.0;
        outSnapshot->velY = 0.0;
        outSnapshot->valid = false;
    }

    if (!thisPtr || !outSnapshot)
    {
        return false;
    }

    const uintptr_t thisValue = reinterpret_cast<uintptr_t>(thisPtr);
    if (SafeIsBadReadPtr(reinterpret_cast<void *>(thisValue + 40), 32))
    {
        return false;
    }

    __try
    {
        outSnapshot->posX = *reinterpret_cast<double *>(thisValue + 40);
        outSnapshot->posY = *reinterpret_cast<double *>(thisValue + 48);
        outSnapshot->velX = *reinterpret_cast<double *>(thisValue + 56);
        outSnapshot->velY = *reinterpret_cast<double *>(thisValue + 64);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        outSnapshot->valid = false;
        return false;
    }

    outSnapshot->valid =
        IsReasonableMountedFlightPhysicsValue(outSnapshot->posX) &&
        IsReasonableMountedFlightPhysicsValue(outSnapshot->posY) &&
        IsReasonableMountedFlightPhysicsValue(outSnapshot->velX) &&
        IsReasonableMountedFlightPhysicsValue(outSnapshot->velY);
    return outSnapshot->valid;
}

struct MountedFlightPhysicsScalarSnapshot
{
    double pos;
    double vel;
    bool valid;
};

struct MountedFlightScalarBoostRecord
{
    uintptr_t thisPtr;
    int mountItemId;
    int dataKey;
    DWORD tick;
    MountedFlightPhysicsScalarSnapshot snapshot;
};

struct MountedFlightStateBoostRecord
{
    uintptr_t thisPtr;
    int mountItemId;
    int dataKey;
    DWORD tick;
    MountedFlightPhysicsStateSnapshot snapshot;
};

static const DWORD kMountedFlightBoostPreserveWindowMs = 120;
static volatile LONG g_MountedFlightBoostPreserveLogBudget = 32;
static MountedFlightScalarBoostRecord g_recentMountedFlightScalarBoost = {};
static MountedFlightStateBoostRecord g_recentMountedFlightStateBoost = {};

static bool IsMountedFlightVelocityDirectionCompatible(double currentValue, double boostedValue)
{
    if (fabs(currentValue) <= 0.000001 || fabs(boostedValue) <= 0.000001)
    {
        return true;
    }
    return (currentValue > 0.0 && boostedValue > 0.0) ||
           (currentValue < 0.0 && boostedValue < 0.0);
}

static void ObserveMountedFlightScalarBoost(
    void *thisPtr,
    int mountItemId,
    int dataKey,
    const MountedFlightPhysicsScalarSnapshot &snapshot)
{
    if (!thisPtr || mountItemId <= 0 || !snapshot.valid)
    {
        return;
    }

    g_recentMountedFlightScalarBoost.thisPtr = reinterpret_cast<uintptr_t>(thisPtr);
    g_recentMountedFlightScalarBoost.mountItemId = mountItemId;
    g_recentMountedFlightScalarBoost.dataKey = dataKey;
    g_recentMountedFlightScalarBoost.tick = GetTickCount();
    g_recentMountedFlightScalarBoost.snapshot = snapshot;
}

static bool TryRestoreRecentMountedFlightScalarBoost(
    void *thisPtr,
    int mountItemId,
    int dataKey,
    MountedFlightPhysicsScalarSnapshot *ioSnapshot)
{
    if (!thisPtr ||
        mountItemId <= 0 ||
        !ioSnapshot ||
        !ioSnapshot->valid)
    {
        return false;
    }

    const MountedFlightScalarBoostRecord recent = g_recentMountedFlightScalarBoost;
    if (!recent.snapshot.valid ||
        recent.thisPtr != reinterpret_cast<uintptr_t>(thisPtr) ||
        recent.mountItemId != mountItemId ||
        recent.dataKey != dataKey ||
        recent.tick == 0)
    {
        return false;
    }

    const DWORD nowTick = GetTickCount();
    if (nowTick - recent.tick > kMountedFlightBoostPreserveWindowMs)
    {
        return false;
    }

    const double currentAbsVel = fabs(ioSnapshot->vel);
    const double recentAbsVel = fabs(recent.snapshot.vel);
    if (!(recentAbsVel > 1.0) ||
        currentAbsVel >= recentAbsVel * 0.5 ||
        !IsMountedFlightVelocityDirectionCompatible(ioSnapshot->vel, recent.snapshot.vel))
    {
        return false;
    }

    const double oldVel = ioSnapshot->vel;
    ioSnapshot->vel = recent.snapshot.vel;
    ioSnapshot->valid = true;

    if (InterlockedDecrement(&g_MountedFlightBoostPreserveLogBudget) >= 0)
    {
        WriteLogFmt(
            "[MountFlightPreserve] scalar this=0x%08X mount=%d key=%d age=%u vel=%.6f->%.6f pos=%.6f",
            (DWORD)(uintptr_t)thisPtr,
            mountItemId,
            dataKey,
            nowTick - recent.tick,
            oldVel,
            ioSnapshot->vel,
            ioSnapshot->pos);
    }
    return true;
}

static void ObserveMountedFlightStateBoost(
    void *thisPtr,
    int mountItemId,
    int dataKey,
    const MountedFlightPhysicsStateSnapshot &snapshot)
{
    if (!thisPtr || mountItemId <= 0 || !snapshot.valid)
    {
        return;
    }

    g_recentMountedFlightStateBoost.thisPtr = reinterpret_cast<uintptr_t>(thisPtr);
    g_recentMountedFlightStateBoost.mountItemId = mountItemId;
    g_recentMountedFlightStateBoost.dataKey = dataKey;
    g_recentMountedFlightStateBoost.tick = GetTickCount();
    g_recentMountedFlightStateBoost.snapshot = snapshot;
}

static bool TryRestoreRecentMountedFlightStateBoost(
    void *thisPtr,
    int mountItemId,
    int dataKey,
    MountedFlightPhysicsStateSnapshot *ioSnapshot)
{
    if (!thisPtr ||
        mountItemId <= 0 ||
        !ioSnapshot ||
        !ioSnapshot->valid)
    {
        return false;
    }

    const MountedFlightStateBoostRecord recent = g_recentMountedFlightStateBoost;
    if (!recent.snapshot.valid ||
        recent.thisPtr != reinterpret_cast<uintptr_t>(thisPtr) ||
        recent.mountItemId != mountItemId ||
        recent.dataKey != dataKey ||
        recent.tick == 0)
    {
        return false;
    }

    const DWORD nowTick = GetTickCount();
    if (nowTick - recent.tick > kMountedFlightBoostPreserveWindowMs)
    {
        return false;
    }

    bool restoredAny = false;
    const double oldVelX = ioSnapshot->velX;
    const double oldVelY = ioSnapshot->velY;
    const double recentAbsVelX = fabs(recent.snapshot.velX);
    const double currentAbsVelX = fabs(ioSnapshot->velX);
    if (recentAbsVelX > 1.0 &&
        currentAbsVelX < recentAbsVelX * 0.5 &&
        IsMountedFlightVelocityDirectionCompatible(ioSnapshot->velX, recent.snapshot.velX))
    {
        ioSnapshot->velX = recent.snapshot.velX;
        restoredAny = true;
    }

    const double recentAbsVelY = fabs(recent.snapshot.velY);
    const double currentAbsVelY = fabs(ioSnapshot->velY);
    if (recentAbsVelY > 1.0 &&
        currentAbsVelY < recentAbsVelY * 0.5 &&
        IsMountedFlightVelocityDirectionCompatible(ioSnapshot->velY, recent.snapshot.velY))
    {
        ioSnapshot->velY = recent.snapshot.velY;
        restoredAny = true;
    }

    if (restoredAny && InterlockedDecrement(&g_MountedFlightBoostPreserveLogBudget) >= 0)
    {
        WriteLogFmt(
            "[MountFlightPreserve] state this=0x%08X mount=%d key=%d age=%u vel=(%.6f,%.6f)->(%.6f,%.6f) pos=(%.6f,%.6f)",
            (DWORD)(uintptr_t)thisPtr,
            mountItemId,
            dataKey,
            nowTick - recent.tick,
            oldVelX,
            oldVelY,
            ioSnapshot->velX,
            ioSnapshot->velY,
            ioSnapshot->posX,
            ioSnapshot->posY);
    }

    return restoredAny;
}

static bool TryReadMountedFlightPhysicsScalarSnapshot(
    void *thisPtr,
    size_t posOffset,
    size_t velOffset,
    MountedFlightPhysicsScalarSnapshot *outSnapshot)
{
    if (outSnapshot)
    {
        outSnapshot->pos = 0.0;
        outSnapshot->vel = 0.0;
        outSnapshot->valid = false;
    }

    if (!thisPtr || !outSnapshot)
    {
        return false;
    }

    const uintptr_t thisValue = reinterpret_cast<uintptr_t>(thisPtr);
    if (SafeIsBadReadPtr(reinterpret_cast<void *>(thisValue + posOffset), 16))
    {
        return false;
    }

    __try
    {
        outSnapshot->pos = *reinterpret_cast<double *>(thisValue + posOffset);
        outSnapshot->vel = *reinterpret_cast<double *>(thisValue + velOffset);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        outSnapshot->valid = false;
        return false;
    }

    outSnapshot->valid =
        IsReasonableMountedFlightPhysicsValue(outSnapshot->pos) &&
        IsReasonableMountedFlightPhysicsValue(outSnapshot->vel);
    return outSnapshot->valid;
}

static bool TryWriteMountedFlightPhysicsScalarSnapshot(
    void *thisPtr,
    size_t posOffset,
    size_t velOffset,
    const MountedFlightPhysicsScalarSnapshot &snapshot)
{
    if (!thisPtr ||
        !snapshot.valid ||
        !IsReasonableMountedFlightPhysicsValue(snapshot.pos) ||
        !IsReasonableMountedFlightPhysicsValue(snapshot.vel))
    {
        return false;
    }

    const uintptr_t thisValue = reinterpret_cast<uintptr_t>(thisPtr);
    if (SafeIsBadReadPtr(reinterpret_cast<void *>(thisValue + posOffset), 16))
    {
        return false;
    }

    __try
    {
        *reinterpret_cast<double *>(thisValue + posOffset) = snapshot.pos;
        *reinterpret_cast<double *>(thisValue + velOffset) = snapshot.vel;
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

static bool TryReadMountedFlightControlTargets(
    void *thisPtr,
    int *horizontalTargetOut,
    int *verticalTargetOut)
{
    if (horizontalTargetOut)
    {
        *horizontalTargetOut = 0;
    }
    if (verticalTargetOut)
    {
        *verticalTargetOut = 0;
    }

    if (!thisPtr)
    {
        return false;
    }

    const uintptr_t thisValue = reinterpret_cast<uintptr_t>(thisPtr);
    if (SafeIsBadReadPtr(reinterpret_cast<void *>(thisValue + 680), 8))
    {
        return false;
    }

    __try
    {
        if (horizontalTargetOut)
        {
            *horizontalTargetOut = *reinterpret_cast<int *>(thisValue + 680);
        }
        if (verticalTargetOut)
        {
            *verticalTargetOut = *reinterpret_cast<int *>(thisValue + 684);
        }
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        if (horizontalTargetOut)
        {
            *horizontalTargetOut = 0;
        }
        if (verticalTargetOut)
        {
            *verticalTargetOut = 0;
        }
        return false;
    }
}

enum MountedFlightNativeMotionBranch
{
    kMountedFlightNativeMotionBranchUnknown = 0,
    kMountedFlightNativeMotionBranchPrimary48 = 1,
    kMountedFlightNativeMotionBranchSecondary40 = 2,
    kMountedFlightNativeMotionBranchFallback = 3,
};

static bool TryReadMountedFlightBranchState(void *thisPtr, int *branchStateOut)
{
    if (branchStateOut)
    {
        *branchStateOut = 0;
    }

    if (!thisPtr)
    {
        return false;
    }

    const uintptr_t thisValue = reinterpret_cast<uintptr_t>(thisPtr);
    if (SafeIsBadReadPtr(reinterpret_cast<void *>(thisValue + 672), sizeof(DWORD)))
    {
        return false;
    }

    __try
    {
        if (branchStateOut)
        {
            *branchStateOut = *reinterpret_cast<int *>(thisValue + 672);
        }
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        if (branchStateOut)
        {
            *branchStateOut = 0;
        }
        return false;
    }
}

static bool TryResolveMountedFlightNativeMotionBranch(
    void *thisPtr,
    bool *mode48Out,
    bool *mode40Out,
    bool *mode44Out,
    int *branchOut)
{
    if (mode48Out)
    {
        *mode48Out = false;
    }
    if (mode40Out)
    {
        *mode40Out = false;
    }
    if (mode44Out)
    {
        *mode44Out = false;
    }
    if (branchOut)
    {
        *branchOut = kMountedFlightNativeMotionBranchUnknown;
    }

    if (!thisPtr)
    {
        return false;
    }

    const uintptr_t wrapperThis = reinterpret_cast<uintptr_t>(thisPtr) + 16;
    if (SafeIsBadReadPtr(reinterpret_cast<void *>(wrapperThis), sizeof(DWORD)))
    {
        return false;
    }

    __try
    {
        DWORD *vtbl = *reinterpret_cast<DWORD **>(wrapperThis);
        if (!vtbl)
        {
            return false;
        }

        typedef int(__thiscall *tVirtualIntNoArgFn)(void *thisPtr);
        tVirtualIntNoArgFn isPrimary48Fn =
            reinterpret_cast<tVirtualIntNoArgFn>(vtbl[48 / sizeof(DWORD)]);
        tVirtualIntNoArgFn isSecondary40Fn =
            reinterpret_cast<tVirtualIntNoArgFn>(vtbl[40 / sizeof(DWORD)]);
        tVirtualIntNoArgFn isFallback44Fn =
            reinterpret_cast<tVirtualIntNoArgFn>(vtbl[44 / sizeof(DWORD)]);

        const bool mode48 =
            isPrimary48Fn && isPrimary48Fn(reinterpret_cast<void *>(wrapperThis)) != 0;
        bool mode40 = false;
        bool mode44 = false;
        int branch = kMountedFlightNativeMotionBranchUnknown;
        if (mode48)
        {
            branch = kMountedFlightNativeMotionBranchPrimary48;
        }
        else
        {
            mode40 =
                isSecondary40Fn &&
                isSecondary40Fn(reinterpret_cast<void *>(wrapperThis)) != 0;
            if (mode40)
            {
                branch = kMountedFlightNativeMotionBranchSecondary40;
            }
            else
            {
                mode44 =
                    isFallback44Fn &&
                    isFallback44Fn(reinterpret_cast<void *>(wrapperThis)) != 0;
                branch = kMountedFlightNativeMotionBranchFallback;
            }
        }

        if (mode48Out)
        {
            *mode48Out = mode48;
        }
        if (mode40Out)
        {
            *mode40Out = mode40;
        }
        if (mode44Out)
        {
            *mode44Out = mode44;
        }
        if (branchOut)
        {
            *branchOut = branch;
        }
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        if (mode48Out)
        {
            *mode48Out = false;
        }
        if (mode40Out)
        {
            *mode40Out = false;
        }
        if (mode44Out)
        {
            *mode44Out = false;
        }
        if (branchOut)
        {
            *branchOut = kMountedFlightNativeMotionBranchUnknown;
        }
        return false;
    }
}

static void ObserveMountedFlightNativeMotionBranch(
    const char *label,
    void *thisPtr,
    int branchState,
    int horizontalTarget,
    int verticalTarget,
    bool hasMotionBranch,
    bool mode48,
    bool mode40,
    bool mode44,
    int motionBranch)
{
    static LONG s_mountedFlightNativeMotionBranchLogBudget = 64;
    if (InterlockedDecrement(&s_mountedFlightNativeMotionBranchLogBudget) < 0)
    {
        return;
    }

    WriteLogFmt(
        "[MountFlightBranch] %s this=0x%08X state=%d h=%d v=%d hasMotion=%d branch=%d f48=%d f40=%d f44=%d",
        label ? label : "unknown",
        (DWORD)(uintptr_t)thisPtr,
        branchState,
        horizontalTarget,
        verticalTarget,
        hasMotionBranch ? 1 : 0,
        motionBranch,
        mode48 ? 1 : 0,
        mode40 ? 1 : 0,
        mode44 ? 1 : 0);
}

static bool TryWriteMountedFlightPhysicsStateSnapshot(
    void *thisPtr,
    const MountedFlightPhysicsStateSnapshot &snapshot)
{
    if (!thisPtr ||
        !snapshot.valid ||
        !IsReasonableMountedFlightPhysicsValue(snapshot.posX) ||
        !IsReasonableMountedFlightPhysicsValue(snapshot.posY) ||
        !IsReasonableMountedFlightPhysicsValue(snapshot.velX) ||
        !IsReasonableMountedFlightPhysicsValue(snapshot.velY))
    {
        return false;
    }

    const uintptr_t thisValue = reinterpret_cast<uintptr_t>(thisPtr);
    if (SafeIsBadReadPtr(reinterpret_cast<void *>(thisValue + 40), 32))
    {
        return false;
    }

    __try
    {
        *reinterpret_cast<double *>(thisValue + 40) = snapshot.posX;
        *reinterpret_cast<double *>(thisValue + 48) = snapshot.posY;
        *reinterpret_cast<double *>(thisValue + 56) = snapshot.velX;
        *reinterpret_cast<double *>(thisValue + 64) = snapshot.velY;
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

static double ResolveMountedFlightPhysicsDeltaScaleFromBase(
    double scale,
    double extraScaleWeight,
    double maxDeltaScale)
{
    if (!(scale > 1.001))
    {
        return 1.0;
    }

    const double extraScale = scale > 1.0 ? (scale - 1.0) : 0.0;
    double deltaScale = 1.0 + extraScale * extraScaleWeight;
    if (maxDeltaScale > 1.001 && deltaScale > maxDeltaScale)
    {
        deltaScale = maxDeltaScale;
    }
    return deltaScale;
}

static double ResolveMountedFlightPhysicsEffectiveDeltaScale(double scale)
{
    if (!(scale > 1.001))
    {
        return 1.0;
    }

    const double kMaxEffectiveScale = 8.0;
    if (scale > kMaxEffectiveScale)
    {
        return kMaxEffectiveScale;
    }

    return scale;
}

static bool IsMountedFlightScaleSourceSwim(const char *scaleSource)
{
    return scaleSource && strcmp(scaleSource, "swim") == 0;
}

static double ResolveMountedFlightHorizontalDeltaExtraScaleWeight(
    int mountItemId,
    int dataKey,
    const char *scaleSource)
{
    // Native unlock-only mode stores a synthetic 100 -> N sample so mounts that
    // already use human-percent-like values (300 = 300%) can stay close to a
    // linear scale instead of reusing the old large-unit compensation weights.
    if (IsMountedFlightPhysicsHumanPercentRouteSample(mountItemId, dataKey))
    {
        return 1.0;
    }

    // Mounts whose sustained flight scale comes from `swim` start from a much
    // smaller native ratio (for example 300000/100000 = 3x on 1932063). The
    // old generic weight made those mounts feel almost unmodified.
    return IsMountedFlightScaleSourceSwim(scaleSource) ? 1.45 : 0.10;
}

static double ResolveMountedFlightHorizontalDeltaMaxScale(
    int mountItemId,
    int dataKey,
    const char *scaleSource)
{
    if (IsMountedFlightPhysicsHumanPercentRouteSample(mountItemId, dataKey))
    {
        return 8.0;
    }
    return IsMountedFlightScaleSourceSwim(scaleSource) ? 4.25 : 6.0;
}

static double ResolveMountedFlightHorizontalMinPosDelta(
    int mountItemId,
    int dataKey,
    const char *scaleSource)
{
    if (IsMountedFlightPhysicsHumanPercentRouteSample(mountItemId, dataKey))
    {
        return 0.10;
    }
    return IsMountedFlightScaleSourceSwim(scaleSource) ? 0.10 : 0.75;
}

static double ResolveMountedFlightHorizontalMinVelDelta(
    int mountItemId,
    int dataKey,
    const char *scaleSource)
{
    if (IsMountedFlightPhysicsHumanPercentRouteSample(mountItemId, dataKey))
    {
        return 1.0;
    }
    return IsMountedFlightScaleSourceSwim(scaleSource) ? 2.0 : 12.0;
}

static double ResolveMountedFlightVerticalDeltaExtraScaleWeight(
    int mountItemId,
    int dataKey,
    const char *scaleSource)
{
    if (IsMountedFlightPhysicsHumanPercentRouteSample(mountItemId, dataKey))
    {
        return 1.0;
    }
    return IsMountedFlightScaleSourceSwim(scaleSource) ? 1.35 : 0.08;
}

static double ResolveMountedFlightVerticalDeltaMaxScale(
    int mountItemId,
    int dataKey,
    const char *scaleSource)
{
    if (IsMountedFlightPhysicsHumanPercentRouteSample(mountItemId, dataKey))
    {
        return 4.5;
    }
    return IsMountedFlightScaleSourceSwim(scaleSource) ? 4.25 : 4.5;
}

static DWORD ResolveMountedFlightVerticalCruiseMinActiveMs(
    int mountItemId,
    int dataKey,
    const char *scaleSource)
{
    if (IsMountedFlightPhysicsHumanPercentRouteSample(mountItemId, dataKey))
    {
        return 120;
    }

    // Vertical cruise used to wait 2200ms to avoid stretching takeoff. That is
    // safe for high-ratio fs mounts, but swim-driven mounts never feel faster
    // with such a late gate. Start earlier once the flight is already in the
    // sustained cruise branch.
    return IsMountedFlightScaleSourceSwim(scaleSource) ? 500 : 2200;
}

static double ResolveMountedFlightVerticalMinPosDelta(
    int mountItemId,
    int dataKey,
    const char *scaleSource)
{
    if (IsMountedFlightPhysicsHumanPercentRouteSample(mountItemId, dataKey))
    {
        return 0.10;
    }
    return IsMountedFlightScaleSourceSwim(scaleSource) ? 0.18 : 1.5;
}

static double ResolveMountedFlightVerticalMinVelDelta(
    int mountItemId,
    int dataKey,
    const char *scaleSource)
{
    if (IsMountedFlightPhysicsHumanPercentRouteSample(mountItemId, dataKey))
    {
        return 1.0;
    }
    return IsMountedFlightScaleSourceSwim(scaleSource) ? 2.0 : 18.0;
}

static double ResolveMountedFlightVerticalMaxAbsRawVelDelta(
    int mountItemId,
    int dataKey,
    const char *scaleSource)
{
    if (IsMountedFlightPhysicsHumanPercentRouteSample(mountItemId, dataKey))
    {
        // Unlock-only / native-percent routes still hit a short vertical
        // impulse while entering cruise. Keeping the old 8.0 cap delays Y
        // scaling until roughly half a second into flight, which is why
        // "上下飞行起步" still feels sluggish even after key priming was
        // fixed. A slightly wider window keeps takeoff under control while
        // allowing vertical acceleration to begin earlier.
        return 14.0;
    }

    // The native "up+jump to enter flight" climb still flows through B844D0.
    // Those takeoff frames tend to carry a large velocity change spike, while
    // stable up/down cruise usually settles to near-constant velocity. Skip
    // scaling the impulse frames so mounted flight gets faster without turning
    // takeoff into a long upward launch.
    return IsMountedFlightScaleSourceSwim(scaleSource) ? 12.0 : 0.0;
}

static bool TryResolveMountedFlightScalarAdaptiveScale(
    const MountedFlightPhysicsScalarSnapshot &beforeSnapshot,
    const MountedFlightPhysicsScalarSnapshot &afterSnapshot,
    double baseScale,
    double minPosDelta,
    double minVelDelta,
    double *appliedScaleOut);

static DWORD ResolveMountedFlightCruiseMinActiveMsForMount(int mountItemId)
{
    if (mountItemId > 0 &&
        ResolveMountedRuntimeSkillIdForKind(
            MountedRuntimeSkillKind_DemonJump,
            mountItemId) == 30010110 &&
        HasRecentMountedDemonJumpIntent(mountItemId, 250))
    {
        return 40;
    }

    return kMountedFlightCruiseMinActiveMs;
}

static bool TryGetMountedSoaringFlightTiming(
    int mountItemId,
    DWORD *ageMsOut,
    DWORD *activeDurationMsOut)
{
    if (ageMsOut)
    {
        *ageMsOut = 0;
    }
    if (activeDurationMsOut)
    {
        *activeDurationMsOut = 0;
    }
    if (mountItemId <= 0)
    {
        return false;
    }

    const LONG activeMountItemId =
        InterlockedCompareExchange(&g_activeMountedSoaringFlightItemId, 0, 0);
    if (activeMountItemId <= 0 || activeMountItemId != mountItemId)
    {
        return false;
    }

    const LONG activeTick =
        InterlockedCompareExchange(&g_activeMountedSoaringFlightTick, 0, 0);
    if (activeTick <= 0)
    {
        return false;
    }

    const LONG activeStartTick =
        InterlockedCompareExchange(&g_activeMountedSoaringFlightStartTick, 0, 0);
    if (activeStartTick <= 0)
    {
        return false;
    }

    const DWORD nowTick = GetTickCount();
    const DWORD ageMs = nowTick - static_cast<DWORD>(activeTick);
    if (ageMs > kMountedSoaringFlightActiveTimeoutMs)
    {
        ClearMountedSoaringFlightActive();
        return false;
    }

    const DWORD activeDurationMs = nowTick - static_cast<DWORD>(activeStartTick);
    if (ageMsOut)
    {
        *ageMsOut = ageMs;
    }
    if (activeDurationMsOut)
    {
        *activeDurationMsOut = activeDurationMs;
    }
    return true;
}

static bool HasFreshMountedSoaringFlightCruiseTiming(
    int mountItemId,
    DWORD *ageMsOut,
    DWORD *activeDurationMsOut)
{
    DWORD ageMs = 0;
    DWORD activeDurationMs = 0;
    if (!TryGetMountedSoaringFlightTiming(
            mountItemId,
            &ageMs,
            &activeDurationMs))
    {
        return false;
    }

    const DWORD minActiveMs = ResolveMountedFlightCruiseMinActiveMsForMount(
        mountItemId);
    // Only boost sustained cruise frames. Skipping the first ~120ms avoids
    // stretching the takeoff / mounted jump arc. `ageMs` here is not a per-frame
    // flight heartbeat, so do not use it as a hard stop gate.
    if (activeDurationMs < minActiveMs)
    {
        return false;
    }

    if (ageMsOut)
    {
        *ageMsOut = ageMs;
    }
    if (activeDurationMsOut)
    {
        *activeDurationMsOut = activeDurationMs;
    }
    return true;
}

static void ApplyMountedFlightPhysicsDeltaScale(
    void *thisPtr,
    const char *label,
    const MountedFlightPhysicsStateSnapshot &beforeSnapshot,
    double scale,
    int mountItemId,
    int dataKey,
    const char *scaleSource,
    bool verticalOnly)
{
    if (!beforeSnapshot.valid || !(scale > 1.001))
    {
        return;
    }

    MountedFlightPhysicsStateSnapshot afterSnapshot = {};
    if (!TryReadMountedFlightPhysicsStateSnapshot(thisPtr, &afterSnapshot))
    {
        return;
    }

    const double effectiveScale = ResolveMountedFlightPhysicsEffectiveDeltaScale(scale);
    if (!(effectiveScale > 1.001))
    {
        return;
    }

    const double rawDeltaX = afterSnapshot.posX - beforeSnapshot.posX;
    const double rawDeltaY = afterSnapshot.posY - beforeSnapshot.posY;
    const double rawVelDeltaX = afterSnapshot.velX - beforeSnapshot.velX;
    const double rawVelDeltaY = afterSnapshot.velY - beforeSnapshot.velY;

    if (verticalOnly)
    {
        if (fabs(rawDeltaY) <= 0.000001 && fabs(rawVelDeltaY) <= 0.000001)
        {
            return;
        }
    }
    else if (fabs(rawDeltaX) <= 0.000001 &&
             fabs(rawDeltaY) <= 0.000001 &&
             fabs(rawVelDeltaX) <= 0.000001 &&
             fabs(rawVelDeltaY) <= 0.000001)
    {
        return;
    }

    MountedFlightPhysicsStateSnapshot boostedSnapshot = afterSnapshot;
    if (!verticalOnly)
    {
        boostedSnapshot.posX = beforeSnapshot.posX + rawDeltaX * effectiveScale;
        boostedSnapshot.velX = beforeSnapshot.velX + rawVelDeltaX * effectiveScale;
    }
    boostedSnapshot.posY = beforeSnapshot.posY + rawDeltaY * effectiveScale;
    boostedSnapshot.velY = beforeSnapshot.velY + rawVelDeltaY * effectiveScale;
    boostedSnapshot.valid = true;

    if (!TryWriteMountedFlightPhysicsStateSnapshot(thisPtr, boostedSnapshot))
    {
        return;
    }

    ObserveMountedFlightStateBoost(
        thisPtr,
        mountItemId,
        dataKey,
        boostedSnapshot);

    if (InterlockedDecrement(&g_MountedFlightPhysicsDeltaLogBudget) >= 0)
    {
        WriteLogFmt(
            "[MountFlightDelta] %s mount=%d key=%d scale=%.3f effective=%.3f source=%s rawDelta=(%.6f,%.6f) rawVelDelta=(%.6f,%.6f) boostedDelta=(%.6f,%.6f) boostedVelDelta=(%.6f,%.6f)",
            label ? label : "unknown",
            mountItemId,
            dataKey,
            scale,
            effectiveScale,
            scaleSource ? scaleSource : "none",
            rawDeltaX,
            rawDeltaY,
            rawVelDeltaX,
            rawVelDeltaY,
            boostedSnapshot.posX - beforeSnapshot.posX,
            boostedSnapshot.posY - beforeSnapshot.posY,
            boostedSnapshot.velX - beforeSnapshot.velX,
            boostedSnapshot.velY - beforeSnapshot.velY);
    }
}

static void ApplyMountedFlightPhysicsHorizontalDeltaScale(
    void *thisPtr,
    const char *label,
    const MountedFlightPhysicsStateSnapshot &beforeSnapshot,
    double scale,
    int mountItemId,
    int dataKey,
    const char *scaleSource,
    double minPosDelta,
    double minVelDelta)
{
    if (!beforeSnapshot.valid || !(scale > 1.001))
    {
        return;
    }

    MountedFlightPhysicsStateSnapshot afterSnapshot = {};
    if (!TryReadMountedFlightPhysicsStateSnapshot(thisPtr, &afterSnapshot))
    {
        return;
    }

    const MountedFlightPhysicsScalarSnapshot beforeHorizontal = {
        beforeSnapshot.posX,
        beforeSnapshot.velX,
        true,
    };
    const MountedFlightPhysicsScalarSnapshot afterHorizontal = {
        afterSnapshot.posX,
        afterSnapshot.velX,
        true,
    };

    const double effectiveScale = ResolveMountedFlightPhysicsEffectiveDeltaScale(scale);
    if (!(effectiveScale > 1.001))
    {
        return;
    }

    double adaptiveScale = 1.0;
    if (!TryResolveMountedFlightScalarAdaptiveScale(
            beforeHorizontal,
            afterHorizontal,
            effectiveScale,
            minPosDelta,
            minVelDelta,
            &adaptiveScale))
    {
        return;
    }

    const double rawDeltaX = afterSnapshot.posX - beforeSnapshot.posX;
    const double rawVelDeltaX = afterSnapshot.velX - beforeSnapshot.velX;
    if (fabs(rawDeltaX) <= 0.000001 && fabs(rawVelDeltaX) <= 0.000001)
    {
        return;
    }

    MountedFlightPhysicsStateSnapshot boostedSnapshot = afterSnapshot;
    boostedSnapshot.posX = beforeSnapshot.posX + rawDeltaX * adaptiveScale;
    boostedSnapshot.velX = beforeSnapshot.velX + rawVelDeltaX * adaptiveScale;
    boostedSnapshot.valid = true;

    if (!TryWriteMountedFlightPhysicsStateSnapshot(thisPtr, boostedSnapshot))
    {
        return;
    }

    ObserveMountedFlightStateBoost(
        thisPtr,
        mountItemId,
        dataKey,
        boostedSnapshot);

    if (InterlockedDecrement(&g_MountedFlightPhysicsDeltaLogBudget) >= 0)
    {
        WriteLogFmt(
            "[MountFlightDelta] %s mount=%d key=%d scale=%.3f effective=%.3f adaptive=%.3f source=%s rawDeltaX=%.6f rawVelDeltaX=%.6f boostedDeltaX=%.6f boostedVelDeltaX=%.6f",
            label ? label : "unknown",
            mountItemId,
            dataKey,
            scale,
            effectiveScale,
            adaptiveScale,
            scaleSource ? scaleSource : "none",
            rawDeltaX,
            rawVelDeltaX,
            boostedSnapshot.posX - beforeSnapshot.posX,
            boostedSnapshot.velX - beforeSnapshot.velX);
    }
}

static bool TryResolveMountedFlightScalarAdaptiveScale(
    const MountedFlightPhysicsScalarSnapshot &beforeSnapshot,
    const MountedFlightPhysicsScalarSnapshot &afterSnapshot,
    double baseScale,
    double minPosDelta,
    double minVelDelta,
    double *appliedScaleOut)
{
    if (appliedScaleOut)
    {
        *appliedScaleOut = 1.0;
    }
    if (!beforeSnapshot.valid || !afterSnapshot.valid)
    {
        return false;
    }

    if (!(baseScale > 1.001))
    {
        return false;
    }

    const double rawPosDelta = afterSnapshot.pos - beforeSnapshot.pos;
    const double rawVelDelta = afterSnapshot.vel - beforeSnapshot.vel;
    const double maxAbsVel =
        max(fabs(beforeSnapshot.vel), fabs(afterSnapshot.vel));
    if (fabs(rawPosDelta) <= 0.000001 && fabs(rawVelDelta) <= 0.000001)
    {
        return false;
    }

    double posStrength = 0.0;
    if (minPosDelta > 0.0)
    {
        posStrength = min(1.0, fabs(rawPosDelta) / minPosDelta);
    }
    else if (fabs(rawPosDelta) > 0.000001)
    {
        posStrength = 1.0;
    }

    double velStrength = 0.0;
    const double velMetric = max(fabs(rawVelDelta), maxAbsVel);
    if (minVelDelta > 0.0)
    {
        velStrength = min(1.0, velMetric / minVelDelta);
    }
    else if (velMetric > 0.000001)
    {
        velStrength = 1.0;
    }

    const double strength = max(posStrength, velStrength);
    if (!(strength > 0.01))
    {
        return false;
    }

    const double adaptiveScale = 1.0 + (baseScale - 1.0) * strength;
    if (!(adaptiveScale > 1.001))
    {
        return false;
    }

    if (appliedScaleOut)
    {
        *appliedScaleOut = adaptiveScale;
    }

    return true;
}

static void ApplyMountedFlightPhysicsScalarDeltaScale(
    void *thisPtr,
    const char *label,
    size_t posOffset,
    size_t velOffset,
    const MountedFlightPhysicsScalarSnapshot &beforeSnapshot,
    double scale,
    int mountItemId,
    int dataKey,
    const char *scaleSource,
    bool scalePosition,
    bool scaleVelocity,
    double minPosDelta,
    double minVelDelta,
    double maxAbsRawVelDelta)
{
    if (!beforeSnapshot.valid || !(scale > 1.001) || (!scalePosition && !scaleVelocity))
    {
        return;
    }

    MountedFlightPhysicsScalarSnapshot afterSnapshot = {};
    if (!TryReadMountedFlightPhysicsScalarSnapshot(
            thisPtr,
            posOffset,
            velOffset,
            &afterSnapshot))
    {
        return;
    }

    const double effectiveScale = ResolveMountedFlightPhysicsEffectiveDeltaScale(scale);
    if (!(effectiveScale > 1.001))
    {
        return;
    }

    const double rawPosDelta = afterSnapshot.pos - beforeSnapshot.pos;
    const double rawVelDelta = afterSnapshot.vel - beforeSnapshot.vel;
    if (maxAbsRawVelDelta > 0.0 && fabs(rawVelDelta) > maxAbsRawVelDelta)
    {
        return;
    }
    double adaptiveScale = 1.0;
    if (!TryResolveMountedFlightScalarAdaptiveScale(
            beforeSnapshot,
            afterSnapshot,
            effectiveScale,
            minPosDelta,
            minVelDelta,
            &adaptiveScale))
    {
        return;
    }

    MountedFlightPhysicsScalarSnapshot boostedSnapshot = afterSnapshot;
    if (scalePosition)
    {
        boostedSnapshot.pos = beforeSnapshot.pos + rawPosDelta * adaptiveScale;
    }
    if (scaleVelocity)
    {
        boostedSnapshot.vel = beforeSnapshot.vel + rawVelDelta * adaptiveScale;
    }
    boostedSnapshot.valid = true;
    if (!TryWriteMountedFlightPhysicsScalarSnapshot(
            thisPtr,
            posOffset,
            velOffset,
            boostedSnapshot))
    {
        return;
    }

    ObserveMountedFlightScalarBoost(
        thisPtr,
        mountItemId,
        dataKey,
        boostedSnapshot);

    if (InterlockedDecrement(&g_MountedFlightPhysicsDeltaLogBudget) >= 0)
    {
        WriteLogFmt(
            "[MountFlightScalarDelta] %s mount=%d key=%d scale=%.3f effective=%.3f adaptive=%.3f source=%s mode=%s rawPosDelta=%.6f rawVelDelta=%.6f boostedPosDelta=%.6f boostedVelDelta=%.6f",
            label ? label : "unknown",
            mountItemId,
            dataKey,
            scale,
            effectiveScale,
            adaptiveScale,
            scaleSource ? scaleSource : "none",
            scalePosition && scaleVelocity ? "pos+vel" : (scalePosition ? "pos" : "vel"),
            rawPosDelta,
            rawVelDelta,
            boostedSnapshot.pos - beforeSnapshot.pos,
            boostedSnapshot.vel - beforeSnapshot.vel);
    }
}

static bool TryResolveB8FE30ExternalFlightSlot(void *thisPtr, uintptr_t *slotAddrOut)
{
    if (slotAddrOut)
    {
        *slotAddrOut = 0;
    }
    if (!thisPtr || !slotAddrOut)
    {
        return false;
    }

    const uintptr_t thisValue = reinterpret_cast<uintptr_t>(thisPtr);
    const uintptr_t mountWrapper = thisValue + 16;
    if (SafeIsBadReadPtr(reinterpret_cast<void *>(mountWrapper), sizeof(DWORD)))
    {
        return false;
    }

    uintptr_t linkedStatePtr = 0;
    __try
    {
        DWORD *vtbl = *reinterpret_cast<DWORD **>(mountWrapper);
        if (!vtbl)
        {
            return false;
        }

        typedef int (__thiscall *tVirtualIntNoArgFn)(void *thisPtr);
        tVirtualIntNoArgFn getLinkedStateFn =
            reinterpret_cast<tVirtualIntNoArgFn>(vtbl[168 / sizeof(DWORD)]);
        if (!getLinkedStateFn)
        {
            return false;
        }

        linkedStatePtr = static_cast<uintptr_t>(getLinkedStateFn(reinterpret_cast<void *>(mountWrapper)));
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        linkedStatePtr = 0;
    }

    if (!linkedStatePtr)
    {
        return false;
    }

    uintptr_t ownerField = 0;
    if (!SafeIsBadReadPtr(reinterpret_cast<void *>(thisValue + 24), sizeof(DWORD)))
    {
        __try
        {
            ownerField = *reinterpret_cast<DWORD *>(thisValue + 24);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            ownerField = 0;
        }
    }
    if (!ownerField)
    {
        return false;
    }

    const uintptr_t ownerBase = ownerField - 4;
    if (SafeIsBadReadPtr(reinterpret_cast<void *>(ownerBase + 156), sizeof(DWORD)))
    {
        return false;
    }

    uintptr_t ownerObj = 0;
    uintptr_t flightObj = 0;
    __try
    {
        ownerObj = *reinterpret_cast<DWORD *>(ownerBase + 156);
        if (!ownerObj)
        {
            return false;
        }

        const uintptr_t ownerObjThis = ownerObj + 4;
        DWORD *vtbl = *reinterpret_cast<DWORD **>(ownerObjThis);
        if (!vtbl)
        {
            return false;
        }

        typedef int (__thiscall *tVirtualOwnerIntFn)(void *thisPtr);
        tVirtualOwnerIntFn getFlightObjFn =
            reinterpret_cast<tVirtualOwnerIntFn>(vtbl[32 / sizeof(DWORD)]);
        if (!getFlightObjFn)
        {
            return false;
        }

        flightObj = static_cast<uintptr_t>(getFlightObjFn(reinterpret_cast<void *>(ownerObjThis)));
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        flightObj = 0;
    }

    if (!flightObj)
    {
        return false;
    }

    *slotAddrOut = flightObj - 16 + 364;
    return !SafeIsBadReadPtr(reinterpret_cast<void *>(*slotAddrOut), 12);
}

static void __fastcall hkMountedFlightPhysicsStepB83C90(
    void *thisPtr,
    void * /*edxUnused*/,
    int deltaMs)
{
    ObserveMountedFlightPhysicsEntry("B83C90", thisPtr, 0);

    if (oMountedFlightPhysicsStepB83C90)
    {
        oMountedFlightPhysicsStepB83C90(thisPtr, deltaMs);
    }
}

static int __fastcall hkMountedFlightPhysicsDispatchB87E60(
    void *thisPtr,
    void * /*edxUnused*/,
    int deltaMs)
{
    int branchState = 0;
    const bool hasBranchState =
        TryReadMountedFlightBranchState(thisPtr, &branchState);

    int horizontalTarget = 0;
    int verticalTarget = 0;
    if (hasBranchState && branchState != 0)
    {
        TryReadMountedFlightControlTargets(
            thisPtr,
            &horizontalTarget,
            &verticalTarget);
        ObserveMountedFlightPhysicsEntry("B87E60", thisPtr, 0);
    }

    int mountItemId = 0;
    int dataKey = 0;
    double baseScale = 0.0;
    const char *scaleSource = nullptr;
    DWORD activeAgeMs = 0;
    DWORD activeDurationMs = 0;
    const bool hasScale =
        hasBranchState &&
        branchState != 0 &&
        TryResolveMountedFlightPhysicsScaleForAxis(
            true,
            &mountItemId,
            &dataKey,
            &baseScale,
            &scaleSource);
    const bool hasFreshCruiseTiming =
        hasScale &&
        HasFreshMountedSoaringFlightCruiseTiming(
            mountItemId,
            &activeAgeMs,
            &activeDurationMs);

    const double cruiseScale =
        hasFreshCruiseTiming
            ? ResolveMountedFlightPhysicsDeltaScaleFromBase(
                  baseScale,
                  ResolveMountedFlightHorizontalDeltaExtraScaleWeight(
                      mountItemId,
                      dataKey,
                      scaleSource),
                  ResolveMountedFlightHorizontalDeltaMaxScale(
                      mountItemId,
                      dataKey,
                      scaleSource))
            : 1.0;

    MountedFlightPhysicsStateSnapshot beforeSnapshot = {};
    const bool hasBeforeSnapshot =
        hasFreshCruiseTiming &&
        cruiseScale > 1.001 &&
        horizontalTarget != 0 &&
        TryReadMountedFlightPhysicsStateSnapshot(thisPtr, &beforeSnapshot);

    const int result = oMountedFlightPhysicsDispatchB87E60
                           ? oMountedFlightPhysicsDispatchB87E60(thisPtr, deltaMs)
                           : 0;

    if (hasBeforeSnapshot)
    {
        // B87E60 is the sustained mounted flight dispatcher. Only boost the
        // horizontal pair after the vtbl+0x48 cruise branch has run.
        ApplyMountedFlightPhysicsHorizontalDeltaScale(
            thisPtr,
            "B87E60:X",
            beforeSnapshot,
            cruiseScale,
            mountItemId,
            dataKey,
            scaleSource,
            ResolveMountedFlightHorizontalMinPosDelta(
                mountItemId,
                dataKey,
                scaleSource),
            ResolveMountedFlightHorizontalMinVelDelta(
                mountItemId,
                dataKey,
                scaleSource));
    }

    return result;
}

static void __fastcall hkMountedFlightPhysicsStepB844D0(
    void *thisPtr,
    void * /*edxUnused*/,
    int deltaMs)
{
    int horizontalMountItemId = 0;
    int horizontalDataKey = 0;
    double horizontalScale = 0.0;
    const char *horizontalScaleSource = nullptr;
    const bool hasHorizontalScale =
        TryResolveMountedFlightPhysicsScaleForAxis(
            true,
            &horizontalMountItemId,
            &horizontalDataKey,
            &horizontalScale,
            &horizontalScaleSource);

    int verticalMountItemId = 0;
    int verticalDataKey = 0;
    double verticalScale = 0.0;
    const char *verticalScaleSource = nullptr;
    const bool hasVerticalScale =
        TryResolveMountedFlightPhysicsScaleForAxis(
            false,
            &verticalMountItemId,
            &verticalDataKey,
            &verticalScale,
            &verticalScaleSource);
    const bool hasAnyScale = hasHorizontalScale || hasVerticalScale;
    const int activeMountItemId =
        hasHorizontalScale ? horizontalMountItemId : verticalMountItemId;

    int branchState = 0;
    int horizontalTarget = 0;
    int verticalTarget = 0;
    const bool hasBranchState =
        TryReadMountedFlightBranchState(thisPtr, &branchState);
    const bool hasControlTargets =
        hasBranchState &&
        TryReadMountedFlightControlTargets(
            thisPtr,
            &horizontalTarget,
            &verticalTarget);
    bool motionMode48 = false;
    bool motionMode40 = false;
    bool motionMode44 = false;
    int motionBranch = kMountedFlightNativeMotionBranchUnknown;
    const bool hasMotionBranch =
        TryResolveMountedFlightNativeMotionBranch(
            thisPtr,
            &motionMode48,
            &motionMode40,
            &motionMode44,
            &motionBranch);
    const bool hasRecentMountedDemonUpIntent =
        activeMountItemId > 0 &&
        HasRecentMountedDemonJumpIntent(activeMountItemId, 250);
    int recentGateProbeMountItemId = 0;
    const bool hasRecentMatchingMountedDemonUpProbe =
        activeMountItemId > 0 &&
        TryGetRecentMountedDemonJumpGateProbeMountItemId(
            &recentGateProbeMountItemId,
            250) &&
        recentGateProbeMountItemId == activeMountItemId;
    const bool hasRecentMountedDemonTerminalClear =
        activeMountItemId > 0 &&
        HasRecentMountedDemonJumpTerminalClear(
            activeMountItemId,
            kMountedDemonJumpTerminalClearSuppressMs);
    BYTE currentDownLatch = 0;
    bool hasCurrentDownLatch = false;
    void *currentUserLocal = nullptr;
    if (TryReadCurrentUserLocalPtr(&currentUserLocal) && currentUserLocal)
    {
        const uintptr_t userLocalAddr =
            reinterpret_cast<uintptr_t>(currentUserLocal);
        __try
        {
            currentDownLatch =
                *reinterpret_cast<BYTE *>(userLocalAddr + 24196);
            hasCurrentDownLatch = true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            currentDownLatch = 0;
            hasCurrentDownLatch = false;
        }
    }
    const bool shouldSeedMountedDemonUpProbeEarlyCandidate =
        activeMountItemId > 0 &&
        hasControlTargets &&
        hasMotionBranch &&
        branchState == 0 &&
        // Latest v23.37 logs show the first mounted up-branch reaches
        // B844D0 with h=1 before the current h=2 seed point. Keep the seed
        // narrow to the same mounted demon-jump motion branch, but allow that
        // earlier h=1 phase too so the first trigger does not wait for the
        // later frame.
        (horizontalTarget == 1 || horizontalTarget == 2) &&
        verticalTarget == 0 &&
        motionBranch == kMountedFlightNativeMotionBranchPrimary48 &&
        (!hasCurrentDownLatch || currentDownLatch == 0) &&
        ResolveMountedRuntimeSkillIdForKind(
            MountedRuntimeSkillKind_DemonJump,
            activeMountItemId) == 30010110 &&
        SkillOverlayBridgeCanUseMountedDemonJumpRuntimeSkill(
            activeMountItemId,
            30010183) &&
        !hasRecentMountedDemonUpIntent &&
        !hasRecentMatchingMountedDemonUpProbe;
    const bool shouldSeedMountedDemonUpProbeEarly =
        shouldSeedMountedDemonUpProbeEarlyCandidate &&
        !hasRecentMountedDemonTerminalClear;
    if (shouldSeedMountedDemonUpProbeEarlyCandidate &&
        hasRecentMountedDemonTerminalClear)
    {
        static LONG s_mountedFlightB844D0UpSeedSuppressLogBudget = 32;
        if (InterlockedDecrement(
                &s_mountedFlightB844D0UpSeedSuppressLogBudget) >= 0)
        {
            WriteLogFmt(
                "[MountDemonJumpProbe] B844D0 suppress terminal-clear mount=%d state=%d h=%d v=%d branch=%d",
                activeMountItemId,
                branchState,
                horizontalTarget,
                verticalTarget,
                motionBranch);
        }
    }
    if (shouldSeedMountedDemonUpProbeEarly)
    {
        DWORD staleFlightAgeMs = 0;
        DWORD staleFlightDurationMs = 0;
        const bool hasStaleMountedSoaringFlightActive =
            TryGetMountedSoaringFlightTiming(
                activeMountItemId,
                &staleFlightAgeMs,
                &staleFlightDurationMs) &&
            staleFlightAgeMs > kMountedDemonJumpIntentMaxAgeMs;
        if (hasStaleMountedSoaringFlightActive)
        {
            TryForceClearMountedSoaringFlightActiveForMount(
                activeMountItemId,
                "B844D0-up-seed-stale-active");
        }
    }
    const bool shouldRefreshActiveFlight =
        hasAnyScale &&
        activeMountItemId > 0 &&
        hasBranchState &&
        branchState != 0 &&
        hasMotionBranch &&
        motionBranch == kMountedFlightNativeMotionBranchSecondary40;
    if (shouldRefreshActiveFlight)
    {
        ObserveMountedSoaringFlightActive(activeMountItemId);
    }
    DWORD activeAgeMs = 0;
    DWORD activeDurationMs = 0;
    const bool hasFreshCruiseTiming =
        hasAnyScale &&
        HasFreshMountedSoaringFlightCruiseTiming(
            activeMountItemId,
            &activeAgeMs,
            &activeDurationMs);
    const bool shouldScaleHorizontal =
        hasHorizontalScale &&
        shouldRefreshActiveFlight &&
        hasFreshCruiseTiming;
    const bool shouldScaleVertical =
        hasVerticalScale &&
        shouldRefreshActiveFlight &&
        hasFreshCruiseTiming &&
        activeDurationMs >=
            ResolveMountedFlightVerticalCruiseMinActiveMs(
                verticalMountItemId,
                verticalDataKey,
                verticalScaleSource);
    const double horizontalDeltaScale =
        shouldScaleHorizontal
            ? ResolveMountedFlightPhysicsDeltaScaleFromBase(
                  horizontalScale,
                  ResolveMountedFlightHorizontalDeltaExtraScaleWeight(
                      horizontalMountItemId,
                      horizontalDataKey,
                      horizontalScaleSource),
                  ResolveMountedFlightHorizontalDeltaMaxScale(
                      horizontalMountItemId,
                      horizontalDataKey,
                      horizontalScaleSource))
            : 1.0;
    const double verticalDeltaScale =
        shouldScaleVertical
            ? ResolveMountedFlightPhysicsDeltaScaleFromBase(
                  verticalScale,
                  ResolveMountedFlightVerticalDeltaExtraScaleWeight(
                      verticalMountItemId,
                      verticalDataKey,
                      verticalScaleSource),
                  ResolveMountedFlightVerticalDeltaMaxScale(
                      verticalMountItemId,
                      verticalDataKey,
                      verticalScaleSource))
            : 1.0;

    MountedFlightPhysicsScalarSnapshot beforeHorizontalSnapshot = {};
    const bool hasBeforeHorizontalSnapshot =
        shouldScaleHorizontal &&
        horizontalDeltaScale > 1.001 &&
        TryReadMountedFlightPhysicsScalarSnapshot(
            thisPtr,
            40,
            56,
            &beforeHorizontalSnapshot);
    MountedFlightPhysicsScalarSnapshot beforeVerticalSnapshot = {};
    const bool hasBeforeVerticalSnapshot =
        shouldScaleVertical &&
        verticalDeltaScale > 1.001 &&
        TryReadMountedFlightPhysicsScalarSnapshot(
            thisPtr,
            48,
            64,
            &beforeVerticalSnapshot);

    ObserveMountedFlightPhysicsEntry("B844D0", thisPtr, 0);
    if (hasAnyScale || hasControlTargets || hasMotionBranch)
    {
        ObserveMountedFlightNativeMotionBranch(
            "B844D0",
            thisPtr,
            branchState,
            horizontalTarget,
            verticalTarget,
            hasMotionBranch,
            motionMode48,
            motionMode40,
            motionMode44,
            motionBranch);
    }

    if (shouldSeedMountedDemonUpProbeEarly)
    {
        const DWORD callerRet = (DWORD)(uintptr_t)_ReturnAddress();
        DWORD staleFlightAgeMs = 0;
        DWORD staleFlightDurationMs = 0;
        const bool hasStaleMountedSoaringFlightActive =
            TryGetMountedSoaringFlightTiming(
                activeMountItemId,
                &staleFlightAgeMs,
                &staleFlightDurationMs) &&
            staleFlightAgeMs > kMountedDemonJumpIntentMaxAgeMs;
        if (hasStaleMountedSoaringFlightActive)
        {
            TryForceClearMountedSoaringFlightActiveForMount(
                activeMountItemId,
                "B844D0-up-seed-stale-active");
        }
        RememberMountedDemonJumpNativeChildSkill(
            activeMountItemId,
            30010183,
            "B844D0-up-seed");
        ObserveMountedRuntimeSkillIntent(
            MountedRuntimeSkillKind_DemonJump,
            activeMountItemId);
        ObserveMountedDemonJumpGateProbe(
            activeMountItemId,
            "B844D0-up-seed",
            callerRet);

        int seededRootSkillId = 0;
        int seededCurrentSkillId = 0;
        const bool hasSeededUpChildContext =
            TryReadMountedDemonJumpEffectiveContextState(
                activeMountItemId,
                &seededRootSkillId,
                &seededCurrentSkillId) &&
            seededRootSkillId == 30010110 &&
            seededCurrentSkillId == 30010183;
        bool primedUpSeedContext = false;
        int primedUpSeedCurrentSkillId = 0;
        if (!hasSeededUpChildContext)
        {
            int primedUpSeedRootSkillId = 0;
            primedUpSeedContext =
                TryManualPrimeMountedDemonJumpContextResolveSafe(
                    activeMountItemId,
                    "B844D0-up-seed",
                    &primedUpSeedCurrentSkillId,
                    &primedUpSeedRootSkillId);
            if (primedUpSeedContext)
            {
                seededRootSkillId = primedUpSeedRootSkillId;
                seededCurrentSkillId = primedUpSeedCurrentSkillId;
            }
        }

        static LONG s_mountedFlightB844D0UpSeedLogBudget = 32;
        if (InterlockedDecrement(&s_mountedFlightB844D0UpSeedLogBudget) >= 0)
        {
            WriteLogFmt(
                "[MountDemonJumpProbe] B844D0 early-up seed caller=0x%08X mount=%d state=%d h=%d v=%d branch=%d recentIntent=%d recentProbe=%d staleActive=%d armIntent=1 prime=%d root=%d current=%d",
                callerRet,
                activeMountItemId,
                branchState,
                horizontalTarget,
                verticalTarget,
                motionBranch,
                hasRecentMountedDemonUpIntent ? 1 : 0,
                hasRecentMatchingMountedDemonUpProbe ? 1 : 0,
                hasStaleMountedSoaringFlightActive ? 1 : 0,
                primedUpSeedContext ? 1 : 0,
                seededRootSkillId,
                seededCurrentSkillId);
        }
    }

    if (oMountedFlightPhysicsStepB844D0)
    {
        oMountedFlightPhysicsStepB844D0(thisPtr, deltaMs);
    }

    if (hasBeforeHorizontalSnapshot)
    {
        // B844D0 also runs through mounted takeoff / jump states. The stable
        // sustained-flight path we observed lands on state!=0 + vtbl+0x28
        // (motion branch 2), so keep the boost there and leave the other
        // motion branches untouched to avoid stretching jump tails again.
        //
        // Only scale horizontal position here. Writing amplified velocity back
        // into the native state keeps cruise fast, but it also leaves residual
        // horizontal drift after the player releases flight input, which shows
        // up in game as the "停下来后抽搐慢慢挪动" tail. Per-frame position
        // amplification is enough to keep the mounted flight speed boost
        // visible while letting the native velocity settle cleanly on stop.
        ApplyMountedFlightPhysicsScalarDeltaScale(
            thisPtr,
            "B844D0:X",
            40,
            56,
            beforeHorizontalSnapshot,
            horizontalDeltaScale,
            horizontalMountItemId,
            horizontalDataKey,
            horizontalScaleSource,
            true,
            false,
            ResolveMountedFlightHorizontalMinPosDelta(
                horizontalMountItemId,
                horizontalDataKey,
                horizontalScaleSource),
            ResolveMountedFlightHorizontalMinVelDelta(
                horizontalMountItemId,
                horizontalDataKey,
                horizontalScaleSource),
            0.0);
    }
    if (hasBeforeVerticalSnapshot)
    {
        // Vertical flight needs a much later cruise gate than horizontal.
        // The "up+jump" takeoff climb keeps feeding into B844D0 well after the
        // horizontal cruise boost can safely start, so hold Y acceleration
        // until the flight has clearly settled. Keep it position-only;
        // amplifying vertical velocity here tends to leave the mount in a
        // sticky float state after releasing the fly key.
        ApplyMountedFlightPhysicsScalarDeltaScale(
            thisPtr,
            "B844D0:Y",
            48,
            64,
            beforeVerticalSnapshot,
            verticalDeltaScale,
            verticalMountItemId,
            verticalDataKey,
            verticalScaleSource,
            true,
            false,
            ResolveMountedFlightVerticalMinPosDelta(
                verticalMountItemId,
                verticalDataKey,
                verticalScaleSource),
            ResolveMountedFlightVerticalMinVelDelta(
                verticalMountItemId,
                verticalDataKey,
                verticalScaleSource),
            ResolveMountedFlightVerticalMaxAbsRawVelDelta(
                verticalMountItemId,
                verticalDataKey,
                verticalScaleSource));
    }
}

static void __fastcall hkMountedFlightPhysicsStepB88090(
    void *thisPtr,
    void * /*edxUnused*/,
    int deltaMs)
{
    ObserveMountedFlightPhysicsEntry("B88090", thisPtr, 0);

    if (oMountedFlightPhysicsStepB88090)
    {
        oMountedFlightPhysicsStepB88090(thisPtr, deltaMs);
    }
}

static int __fastcall hkMountedFlightPhysicsStateB84D70(
    void *thisPtr,
    void * /*edxUnused*/)
{
    // B84D70 mostly seeds takeoff / vertical velocity. Scaling it directly makes
    // mounted jump height explode long before horizontal flight feels faster, so
    // keep it observe-only and let B844D0 carry sustained flight acceleration.
    ObserveMountedFlightPhysicsEntry("B84D70", thisPtr, 0);

    const int result = oMountedFlightPhysicsStateB84D70
                           ? oMountedFlightPhysicsStateB84D70(thisPtr)
                           : 0;

    return result;
}

static int __fastcall hkMountedFlightPhysicsVerticalB8FE30(
    void *thisPtr,
    void * /*edxUnused*/)
{
    // B8FE30 is the vertical flight clamp / landing step. Touching it boosts
    // takeoff height and dive/fall response more than horizontal travel speed.
    ObserveMountedFlightPhysicsEntry("B8FE30", thisPtr, 0);
    const int result = oMountedFlightPhysicsVerticalB8FE30
                           ? oMountedFlightPhysicsVerticalB8FE30(thisPtr)
                           : 0;
    return result;
}

static int __fastcall hkMountedFlightPhysicsFinalizeB851F0(
    void *thisPtr,
    void * /*edxUnused*/,
    int deltaMs)
{
    const int result = oMountedFlightPhysicsFinalizeB851F0
                           ? oMountedFlightPhysicsFinalizeB851F0(thisPtr, deltaMs)
                           : 0;

    int mountItemId = 0;
    if (!TryReadCurrentUserMountItemId(&mountItemId) || mountItemId <= 0)
    {
        ClearMountedSoaringRuntimeFallbackState();
    }

    return result;
}

static bool ShouldSuppressMountedRuntimeSkillUseFailPrompt(
    MountedRuntimeSkillKind kind,
    void *thisPtr,
    int reason,
    DWORD callerRet,
    int *mountItemIdOut,
    int *configuredSkillIdOut)
{
    if (!IsMountedRuntimeSkillHooksEnabledForKind(kind) || !thisPtr)
    {
        return false;
    }

    int mountItemId = 0;
    const bool resolvedMount = kind == MountedRuntimeSkillKind_DemonJump
                                   ? TryResolveMountedDemonJumpMountItemIdWithFallback(
                                         thisPtr,
                                         &mountItemId,
                                         nullptr,
                                         1200)
                                   : TryResolveMountedDoubleJumpMountItemIdWithFallback(
                                         thisPtr,
                                         &mountItemId,
                                         nullptr,
                                         1200);
    if (!resolvedMount)
    {
        return false;
    }

    const bool hasRecentIntent = kind == MountedRuntimeSkillKind_DemonJump
                                     ? HasRecentMountedDemonJumpIntent(mountItemId, 1200)
                                     : HasRecentMountedDoubleJumpIntent(mountItemId, 1200);
    const bool isMountedJumpProbeCaller =
        reason == 0 && callerRet == 0x00B30785;

    const int configuredSkillId = ResolveMountedRuntimeSkillIdForKind(kind, mountItemId);
    if (configuredSkillId <= 0)
    {
        if (!hasRecentIntent && !isMountedJumpProbeCaller)
        {
            return false;
        }

        // Custom mounts without mountedDoubleJumpEnabled should still suppress
        // the native "cannot use while mounted" spam when the recent input was
        // only probing a double-jump path on that mount. 80001095 currently
        // hits AE6260 from 0x00B30785 without always arming recent intent, so
        // keep a narrow caller-based escape hatch for that mounted jump probe.
        MountedMovementOverride mountedOverride = {};
        if (!SkillOverlayBridgeResolveMountedMovementOverride(
                mountItemId,
                0,
                mountedOverride) ||
            !mountedOverride.matched)
        {
            return false;
        }

        if (mountItemIdOut)
        {
            *mountItemIdOut = mountItemId;
        }
        if (configuredSkillIdOut)
        {
            *configuredSkillIdOut = 0;
        }
        return true;
    }

    if (!hasRecentIntent)
    {
        return false;
    }

    if (mountItemIdOut)
    {
        *mountItemIdOut = mountItemId;
    }
    if (configuredSkillIdOut)
    {
        *configuredSkillIdOut = configuredSkillId;
    }
    return true;
}

static bool ShouldSuppressMountedConfiguredUseFailPrompt(
    void *thisPtr,
    int reason,
    DWORD callerRet,
    MountedRuntimeSkillKind *kindOut,
    int *mountItemIdOut,
    int *configuredSkillIdOut)
{
    if (kindOut)
    {
        *kindOut = MountedRuntimeSkillKind_DoubleJump;
    }

    if (ShouldSuppressMountedRuntimeSkillUseFailPrompt(
            MountedRuntimeSkillKind_DoubleJump,
            thisPtr,
            reason,
            callerRet,
            mountItemIdOut,
            configuredSkillIdOut))
    {
        if (kindOut)
        {
            *kindOut = MountedRuntimeSkillKind_DoubleJump;
        }
        return true;
    }

    if (ShouldSuppressMountedRuntimeSkillUseFailPrompt(
            MountedRuntimeSkillKind_DemonJump,
            thisPtr,
            reason,
            callerRet,
            mountItemIdOut,
            configuredSkillIdOut))
    {
        if (kindOut)
        {
            *kindOut = MountedRuntimeSkillKind_DemonJump;
        }
        return true;
    }

    return false;
}

#include "runtime_feature_mounted_double_jump_state_gate.inl"

