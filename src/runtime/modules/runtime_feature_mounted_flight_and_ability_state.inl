static uintptr_t g_LocalIndependentPotentialSkillLevelLastTarget = 0;
static DWORD g_LocalIndependentPotentialSkillLevelLastTick = 0;
static uintptr_t g_LocalIndependentPotentialDamageLastKey = 0;
static DWORD g_LocalIndependentPotentialDamageLastTick = 0;
static uintptr_t g_LocalIndependentPotentialPercentQuadLastKey = 0;
static DWORD g_LocalIndependentPotentialPercentQuadLastTick = 0;
static uintptr_t g_LocalIndependentPotentialPercentFullLastKey = 0;
static DWORD g_LocalIndependentPotentialPercentFullLastTick = 0;
static uintptr_t g_LocalIndependentPotentialFlatBasicLastKey = 0;
static DWORD g_LocalIndependentPotentialFlatBasicLastTick = 0;
static uintptr_t g_LocalIndependentPotentialFlatExtendedLastKey = 0;
static DWORD g_LocalIndependentPotentialFlatExtendedLastTick = 0;
static const bool kLocalIndependentPotentialDisplayObserveOnly = true;
static uintptr_t g_AbilityRedDisplayCandidateLastThis = 0;
static DWORD g_AbilityRedDisplayCandidateLastTick = 0;
static DWORD g_AbilityRedDisplayCallsiteLastTick = 0;
static DWORD g_AbilityRedLevelReadLastTick = 0;
static DWORD g_AbilityRedSnapshotLastTick = 0;
static DWORD g_AbilityRedSkillWriteLastTick = 0;
static DWORD g_AbilityRedHashLookupLastCaller = 0;
static uintptr_t g_AbilityRedHashLookupLastThis = 0;
static DWORD g_AbilityRedHashLookupLastKey = 0;
static DWORD g_AbilityRedHashLookupLastTick = 0;
static DWORD g_AbilityRedHashInsertLastCaller = 0;
static uintptr_t g_AbilityRedHashInsertLastThis = 0;
static DWORD g_AbilityRedHashInsertLastKey = 0;
static DWORD g_AbilityRedHashInsertLastValue = 0;
static DWORD g_AbilityRedHashInsertLastTick = 0;
static DWORD g_AbilityRedExtendedAggregateLastCaller = 0;
static DWORD g_AbilityRedExtendedAggregateLastTick = 0;
static DWORD g_AbilityRedMasterAggregateLastCaller = 0;
static DWORD g_AbilityRedMasterAggregateLastTick = 0;
static DWORD g_AbilityRedSibling82F780LastCaller = 0;
static uintptr_t g_AbilityRedSibling82F780LastThis = 0;
static DWORD g_AbilityRedSibling82F780LastTick = 0;
static int g_AbilityRedSibling82F780LastActive = -1;
static DWORD g_AbilityRedSibling82F870LastCaller = 0;
static uintptr_t g_AbilityRedSibling82F870LastThis = 0;
static DWORD g_AbilityRedSibling82F870LastTick = 0;
static int g_AbilityRedSibling82F870LastActive = -1;
static DWORD g_AbilityRedSibling82F960LastCaller = 0;
static uintptr_t g_AbilityRedSibling82F960LastThis = 0;
static DWORD g_AbilityRedSibling82F960LastTick = 0;
static int g_AbilityRedSibling82F960LastActive = -1;
static DWORD g_AbilityRedSibling82FA50LastCaller = 0;
static uintptr_t g_AbilityRedSibling82FA50LastThis = 0;
static DWORD g_AbilityRedSibling82FA50LastTick = 0;
static int g_AbilityRedSibling82FA50LastActive = -1;
static LONG g_MountMovementObserveLogBudget = 32;
static LONG g_MountMovementOverrideMissLogBudget = 16;
static volatile LONG g_RecentMountedMovementRawMountItemId = 0;
static volatile LONG g_RecentMountedMovementRawDataKey = 0;
static volatile LONG g_RecentMountedMovementRawSpeed = 0;
static volatile LONG g_RecentMountedMovementRawJump = 0;
static volatile LONG g_RecentMountedMovementRawTick = 0;
static LONG g_MountedFlightPhysicsScaleLogBudget = 96;
static LONG g_MountedFlightPhysicsEntryLogBudget = 96;
static LONG g_MountedFlightPhysicsDeltaLogBudget = 24;
static LONG g_MountMovementNativeCacheRebuildLogBudget = 24;
static volatile LONG g_MountMovementNativeCacheRebuildInProgress = 0;
static volatile LONG g_LastMountMovementNativeCacheRebuildDataKey = 0;
static volatile LONG g_LastMountMovementNativeCacheRebuildTick = 0;
static DWORD g_AbilityRedDiff84C470LastCaller = 0;
static uintptr_t g_AbilityRedDiff84C470LastThis = 0;
static DWORD g_AbilityRedDiff84C470LastTick = 0;
static int g_AbilityRedDiff84C470LastActive = -1;
static DWORD g_AbilityRedBaseSumInactive9F7241 = 0;
static DWORD g_AbilityRedBaseSumInactive9F7546 = 0;
static DWORD g_AbilityRedBaseSumInactive9F7893 = 0;
static DWORD g_AbilityRedBaseSumInactive9F7C7F = 0;
static DWORD g_AbilityRedBaseSumInactive9F8048 = 0;
static DWORD g_AbilityRedBaseSumInactive9F82A8 = 0;
static DWORD g_AbilityRedBake857BB6LastSig = 0;
static DWORD g_AbilityRedBake857BB6LastTick = 0;
static DWORD g_AbilityRedBake857C29LastSig = 0;
static DWORD g_AbilityRedBake857C29LastTick = 0;
static DWORD g_AbilityRedBake857C9CLastSig = 0;
static DWORD g_AbilityRedBake857C9CLastTick = 0;
static DWORD g_AbilityRedBake857D0FLastSig = 0;
static DWORD g_AbilityRedBake857D0FLastTick = 0;
static DWORD g_AbilityRedBake1988569C3LastSig = 0;
static DWORD g_AbilityRedBake1988569C3LastTick = 0;
static DWORD g_AbilityRedBake198856D57LastSig = 0;
static DWORD g_AbilityRedBake198856D57LastTick = 0;
static DWORD g_AbilityRedBake19885725FLastSig = 0;
static DWORD g_AbilityRedBake19885725FLastTick = 0;
static DWORD g_AbilityRedBake198857C3BLastSig = 0;
static DWORD g_AbilityRedBake198857C3BLastTick = 0;
static DWORD g_AbilityRedBake198858AEDLastSig = 0;
static DWORD g_AbilityRedBake198858AEDLastTick = 0;
static DWORD g_AbilityRedBake198831A50LastSig = 0;
static DWORD g_AbilityRedBake198831A50LastTick = 0;
static volatile LONG g_activeMountedSoaringFlightItemId = 0;
static volatile LONG g_activeMountedSoaringFlightTick = 0;
static volatile LONG g_activeMountedSoaringFlightStartTick = 0;
static const DWORD kMountedSoaringFallbackGraceMs = 1500;
static bool HasMeaningfulMountedFlightPhysicsScaleBaseline(
    double nativeFs,
    double nativeSwim,
    double overrideFs,
    double overrideSwim);
static double NormalizeMountedFlightPhysicsHumanScalar(double rawValue);
static void RememberMountedFlightPhysicsScaleSample(
    int mountItemId,
    int dataKey,
    double nativeFs,
    double nativeSwim,
    const MountedMovementOverride &appliedOverride);
struct MountedFlightPhysicsScaleSample
{
    int mountItemId;
    int dataKey;
    double nativeFs;
    double nativeSwim;
    double overrideFs;
    double overrideSwim;
    DWORD tick;
};
static MountedFlightPhysicsScaleSample g_MountedFlightPhysicsScaleSample = {};
static const size_t kMountedFlightPhysicsScaleSampleHistoryCapacity = 8;
static MountedFlightPhysicsScaleSample
    g_MountedFlightPhysicsScaleSampleHistory[kMountedFlightPhysicsScaleSampleHistoryCapacity] = {};

static bool IsMountedFlightPhysicsScaleSampleUseful(
    const MountedFlightPhysicsScaleSample &sample)
{
    return sample.mountItemId > 0 &&
           sample.dataKey > 0 &&
           sample.tick != 0 &&
           HasMeaningfulMountedFlightPhysicsScaleBaseline(
               sample.nativeFs,
               sample.nativeSwim,
               sample.overrideFs,
               sample.overrideSwim);
}

static bool TryFindMountedFlightPhysicsScaleHistorySample(
    int mountItemId,
    int dataKey,
    MountedFlightPhysicsScaleSample *sampleOut)
{
    if (sampleOut)
    {
        ZeroMemory(sampleOut, sizeof(*sampleOut));
    }
    if (mountItemId <= 0 || dataKey <= 0 || !sampleOut)
    {
        return false;
    }

    int bestIndex = -1;
    DWORD bestTick = 0;
    for (size_t i = 0; i < kMountedFlightPhysicsScaleSampleHistoryCapacity; ++i)
    {
        const MountedFlightPhysicsScaleSample &candidate =
            g_MountedFlightPhysicsScaleSampleHistory[i];
        if (candidate.mountItemId != mountItemId ||
            candidate.dataKey != dataKey ||
            !IsMountedFlightPhysicsScaleSampleUseful(candidate))
        {
            continue;
        }

        if (bestIndex < 0 || candidate.tick >= bestTick)
        {
            bestIndex = static_cast<int>(i);
            bestTick = candidate.tick;
        }
    }

    if (bestIndex < 0)
    {
        return false;
    }

    *sampleOut = g_MountedFlightPhysicsScaleSampleHistory[bestIndex];
    return true;
}

static bool TryGetMountedFlightPhysicsScaleSampleForRoute(
    int mountItemId,
    int dataKey,
    MountedFlightPhysicsScaleSample *sampleOut)
{
    if (sampleOut)
    {
        ZeroMemory(sampleOut, sizeof(*sampleOut));
    }
    if (mountItemId <= 0 || dataKey <= 0 || !sampleOut)
    {
        return false;
    }

    const MountedFlightPhysicsScaleSample liveSample =
        g_MountedFlightPhysicsScaleSample;
    if (liveSample.mountItemId == mountItemId &&
        liveSample.dataKey == dataKey &&
        liveSample.tick != 0)
    {
        *sampleOut = liveSample;
        return true;
    }

    return TryFindMountedFlightPhysicsScaleHistorySample(
        mountItemId,
        dataKey,
        sampleOut);
}

static bool TryPrimeMountedFlightPhysicsScaleSampleForMount(
    int mountItemId,
    MountedFlightPhysicsScaleSample *sampleOut)
{
    if (sampleOut)
    {
        ZeroMemory(sampleOut, sizeof(*sampleOut));
    }
    if (mountItemId <= 0 || !sampleOut || !oMountMovementDataLookup888B30)
    {
        return false;
    }

    int recentMountItemId = 0;
    int dataKey = 0;
    const bool hasRecentDataKey =
        TryGetRecentMountedMovementRawSample(
            &recentMountItemId,
            &dataKey,
            nullptr,
            nullptr,
            30000) &&
        recentMountItemId == mountItemId &&
        dataKey > 0;
    if (!hasRecentDataKey &&
        !TryResolveMountedMovementDataKeyFromMountItemId(
            mountItemId,
            &dataKey))
    {
        return false;
    }

    if (TryGetMountedFlightPhysicsScaleSampleForRoute(
            mountItemId,
            dataKey,
            sampleOut))
    {
        return true;
    }

    MountedMovementOverride mountedOverride = {};
    if (!SkillOverlayBridgeResolveMountedSoaringOverride(
            mountItemId,
            dataKey,
            mountedOverride) &&
        !SkillOverlayBridgeResolveMountedMovementOverride(
            mountItemId,
            dataKey,
            mountedOverride))
    {
        return false;
    }

    const int dataPtr = oMountMovementDataLookup888B30(dataKey);
    if (dataPtr <= 0 ||
        SafeIsBadReadPtr(reinterpret_cast<void *>(static_cast<uintptr_t>(dataPtr)), 0x2C))
    {
        return false;
    }

    double rawFs = 0.0;
    DWORD rawSwim = 0;
    __try
    {
        rawFs = *reinterpret_cast<double *>(static_cast<uintptr_t>(dataPtr) + 0x20);
        rawSwim = *reinterpret_cast<DWORD *>(static_cast<uintptr_t>(dataPtr) + 0x28);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }

    MountedMovementOverride primedOverride = mountedOverride;
    // Flight priming must follow the runtime lookup key actually observed by
    // 888B30. The config-side mountTamingMobId is only a selector hint and can
    // differ from the real mounted movement table key (for example 1932064 can
    // resolve to runtime key 18). Priming with the config key poisons the
    // sample cache and blocks the later real flight sample from taking over.
    if (mountedOverride.useNativeMovement)
    {
        const double normalizedFs =
            NormalizeMountedFlightPhysicsHumanScalar(rawFs);
        const double normalizedSwim =
            NormalizeMountedFlightPhysicsHumanScalar(static_cast<double>(rawSwim));
        primedOverride.hasFs = normalizedFs > 0.0;
        primedOverride.fs = normalizedFs;
        primedOverride.hasSwim = normalizedSwim > 0.0;
        primedOverride.swim = normalizedSwim;
        RememberMountedFlightPhysicsScaleSample(
            mountItemId,
            dataKey,
            100.0,
            100.0,
            primedOverride);
    }
    else
    {
        RememberMountedFlightPhysicsScaleSample(
            mountItemId,
            dataKey,
            rawFs,
            static_cast<double>(rawSwim),
            primedOverride);
    }

    const bool primed =
        TryGetMountedFlightPhysicsScaleSampleForRoute(
            mountItemId,
            dataKey,
            sampleOut);
    if (primed)
    {
        static LONG s_mountFlightPrimeLogBudget = 24;
        if (InterlockedDecrement(&s_mountFlightPrimeLogBudget) >= 0)
        {
            WriteLogFmt(
                "[MountFlightPrime] mount=%d key=%d raw=[fs=%.3f swim=%u] sample=[nativeFs=%.3f nativeSwim=%.3f overrideFs=%.3f overrideSwim=%.3f] nativeMode=%d",
                mountItemId,
                dataKey,
                rawFs,
                rawSwim,
                sampleOut->nativeFs,
                sampleOut->nativeSwim,
                sampleOut->overrideFs,
                sampleOut->overrideSwim,
                mountedOverride.useNativeMovement ? 1 : 0);
        }
    }
    return primed;
}

static void RememberMountedFlightPhysicsScaleHistorySample(
    const MountedFlightPhysicsScaleSample &sample)
{
    if (!IsMountedFlightPhysicsScaleSampleUseful(sample))
    {
        return;
    }

    int replaceIndex = -1;
    DWORD oldestTick = 0xFFFFFFFFu;
    for (size_t i = 0; i < kMountedFlightPhysicsScaleSampleHistoryCapacity; ++i)
    {
        MountedFlightPhysicsScaleSample &entry =
            g_MountedFlightPhysicsScaleSampleHistory[i];
        if (entry.mountItemId == sample.mountItemId &&
            entry.dataKey == sample.dataKey)
        {
            replaceIndex = static_cast<int>(i);
            break;
        }
        if (!IsMountedFlightPhysicsScaleSampleUseful(entry))
        {
            replaceIndex = static_cast<int>(i);
            break;
        }
        if (entry.tick < oldestTick)
        {
            oldestTick = entry.tick;
            replaceIndex = static_cast<int>(i);
        }
    }

    if (replaceIndex < 0)
    {
        replaceIndex = 0;
    }
    g_MountedFlightPhysicsScaleSampleHistory[replaceIndex] = sample;
}

static void ClearMountedFlightPhysicsScaleSample()
{
    ZeroMemory(&g_MountedFlightPhysicsScaleSample, sizeof(g_MountedFlightPhysicsScaleSample));
    ZeroMemory(
        g_MountedFlightPhysicsScaleSampleHistory,
        sizeof(g_MountedFlightPhysicsScaleSampleHistory));
}

static bool HasMeaningfulMountedFlightPhysicsScaleBaseline(
    double nativeFs,
    double nativeSwim,
    double overrideFs,
    double overrideSwim)
{
    const bool hasFsBaseline =
        nativeFs > 0.0 &&
        overrideFs > 0.0 &&
        fabs(nativeFs - overrideFs) > 0.001;
    const bool hasSwimBaseline =
        nativeSwim > 0.0 &&
        overrideSwim > 0.0 &&
        fabs(nativeSwim - overrideSwim) > 0.5;
    return hasFsBaseline || hasSwimBaseline;
}

static double NormalizeMountedFlightPhysicsHumanScalar(double rawValue)
{
    if (!(rawValue > 0.0))
    {
        return 0.0;
    }

    double normalizedValue = rawValue;
    while (normalizedValue > 999.0)
    {
        normalizedValue /= 10.0;
    }
    return normalizedValue;
}

static bool IsMountedFlightPhysicsHumanPercentBaselineValue(
    double nativeValue,
    double overrideValue)
{
    return nativeValue >= 95.0 &&
           nativeValue <= 105.0 &&
           overrideValue > nativeValue + 0.001 &&
           overrideValue <= 999.0;
}

static bool IsMountedFlightPhysicsHumanPercentScaleSample(
    const MountedFlightPhysicsScaleSample &sample)
{
    if (sample.mountItemId <= 0 || sample.dataKey <= 0 || sample.tick == 0)
    {
        return false;
    }

    return IsMountedFlightPhysicsHumanPercentBaselineValue(
               sample.nativeFs,
               sample.overrideFs) ||
           IsMountedFlightPhysicsHumanPercentBaselineValue(
               sample.nativeSwim,
               sample.overrideSwim);
}

static bool IsMountedFlightPhysicsHumanPercentRouteSample(
    int mountItemId,
    int dataKey)
{
    MountedFlightPhysicsScaleSample sample = {};
    return TryGetMountedFlightPhysicsScaleSampleForRoute(
               mountItemId,
               dataKey,
               &sample) &&
           IsMountedFlightPhysicsHumanPercentScaleSample(sample);
}

static void RememberMountedFlightPhysicsScaleSample(
    int mountItemId,
    int dataKey,
    double nativeFs,
    double nativeSwim,
    const MountedMovementOverride &appliedOverride)
{
    const double overrideFs = appliedOverride.hasFs ? appliedOverride.fs : 0.0;
    const double overrideSwim = appliedOverride.hasSwim ? appliedOverride.swim : 0.0;
    const DWORD nowTick = GetTickCount();
    const MountedFlightPhysicsScaleSample existingSample =
        g_MountedFlightPhysicsScaleSample;
    const bool hasFreshBaseline =
        HasMeaningfulMountedFlightPhysicsScaleBaseline(
            nativeFs,
            nativeSwim,
            overrideFs,
            overrideSwim);
    const bool hasExistingUsefulBaseline =
        HasMeaningfulMountedFlightPhysicsScaleBaseline(
            existingSample.nativeFs,
            existingSample.nativeSwim,
            existingSample.overrideFs,
            existingSample.overrideSwim);
    const bool canReuseExistingBaseline =
        existingSample.mountItemId == mountItemId &&
        existingSample.dataKey == dataKey &&
        existingSample.tick != 0;
    MountedFlightPhysicsScaleSample historicalSample = {};
    const bool hasHistoricalUsefulBaseline =
        TryFindMountedFlightPhysicsScaleHistorySample(
            mountItemId,
            dataKey,
            &historicalSample);

    if (!hasFreshBaseline && canReuseExistingBaseline && hasExistingUsefulBaseline)
    {
        g_MountedFlightPhysicsScaleSample.overrideFs =
            overrideFs > existingSample.overrideFs ? overrideFs : existingSample.overrideFs;
        g_MountedFlightPhysicsScaleSample.overrideSwim =
            overrideSwim > existingSample.overrideSwim ? overrideSwim : existingSample.overrideSwim;
        g_MountedFlightPhysicsScaleSample.tick = nowTick;
        RememberMountedFlightPhysicsScaleHistorySample(g_MountedFlightPhysicsScaleSample);
        return;
    }

    if (!hasFreshBaseline && hasHistoricalUsefulBaseline)
    {
        g_MountedFlightPhysicsScaleSample = historicalSample;
        if (overrideFs > g_MountedFlightPhysicsScaleSample.overrideFs)
        {
            g_MountedFlightPhysicsScaleSample.overrideFs = overrideFs;
        }
        if (overrideSwim > g_MountedFlightPhysicsScaleSample.overrideSwim)
        {
            g_MountedFlightPhysicsScaleSample.overrideSwim = overrideSwim;
        }
        g_MountedFlightPhysicsScaleSample.tick = nowTick;
        RememberMountedFlightPhysicsScaleHistorySample(g_MountedFlightPhysicsScaleSample);
        return;
    }

    g_MountedFlightPhysicsScaleSample.mountItemId = mountItemId;
    g_MountedFlightPhysicsScaleSample.dataKey = dataKey;
    g_MountedFlightPhysicsScaleSample.nativeFs = nativeFs;
    g_MountedFlightPhysicsScaleSample.nativeSwim = nativeSwim;
    g_MountedFlightPhysicsScaleSample.overrideFs = overrideFs;
    g_MountedFlightPhysicsScaleSample.overrideSwim = overrideSwim;
    g_MountedFlightPhysicsScaleSample.tick = nowTick;
    RememberMountedFlightPhysicsScaleHistorySample(g_MountedFlightPhysicsScaleSample);
}
static DWORD g_AbilityRedBake19883AF02LastSig = 0;
static DWORD g_AbilityRedBake19883AF02LastTick = 0;
static DWORD g_AbilityRedFinal84BE40LastCaller = 0;
static uintptr_t g_AbilityRedFinal84BE40LastThis = 0;
static DWORD g_AbilityRedFinal84BE40LastTick = 0;
static int g_AbilityRedFinal84BE40LastActive = -1;
static DWORD g_AbilityRedFinal84C470LastCaller = 0;
static uintptr_t g_AbilityRedFinal84C470LastThis = 0;
static DWORD g_AbilityRedFinal84C470LastTick = 0;
static int g_AbilityRedFinal84C470LastActive = -1;
static DWORD g_AbilityRedFinal84CA90LastCaller = 0;
static uintptr_t g_AbilityRedFinal84CA90LastThis = 0;
static DWORD g_AbilityRedFinal84CA90LastTick = 0;
static int g_AbilityRedFinal84CA90LastActive = -1;
static DWORD g_AbilityRedFinal84CBD0LastCaller = 0;
static uintptr_t g_AbilityRedFinal84CBD0LastThis = 0;
static DWORD g_AbilityRedFinal84CBD0LastTick = 0;
static int g_AbilityRedFinal84CBD0LastActive = -1;

static DWORD SeedAbilityRedInactiveBaselineFromPrimary(DWORD siteId, DWORD currentSum);

static bool ShouldApplyLocalIndependentPotentialBurst(uintptr_t key, uintptr_t* lastKey, DWORD* lastTick)
{
    if (!key || !lastKey || !lastTick)
        return false;

    const DWORD now = GetTickCount();
    if (*lastKey == key && now - *lastTick <= 15)
        return false;

    *lastKey = key;
    *lastTick = now;
    return true;
}

static bool TryGetObservedDrawObjectSize(int imageObj, int* outW, int* outH)
{
    if (outW)
        *outW = 0;
    if (outH)
        *outH = 0;
    if (imageObj <= 0 || SafeIsBadReadPtr(reinterpret_cast<void*>(imageObj), 4))
        return false;

    __try
    {
        DWORD vtable = *reinterpret_cast<DWORD*>(imageObj);
        if (!vtable ||
            SafeIsBadReadPtr(reinterpret_cast<void*>(vtable + 64), 4) ||
            SafeIsBadReadPtr(reinterpret_cast<void*>(vtable + 72), 4))
        {
            return false;
        }

        typedef int (__stdcall *tGetDrawObjMetric)(int obj, LONG* outValue);
        tGetDrawObjMetric fnGetWidth = *reinterpret_cast<tGetDrawObjMetric*>(vtable + 64);
        tGetDrawObjMetric fnGetHeight = *reinterpret_cast<tGetDrawObjMetric*>(vtable + 72);
        if (!fnGetWidth || !fnGetHeight)
            return false;

        LONG w = 0;
        LONG h = 0;
        if (fnGetWidth(imageObj, &w) < 0 || fnGetHeight(imageObj, &h) < 0)
            return false;
        if (w <= 0 || h <= 0 || w > 4096 || h > 4096)
            return false;

        if (outW)
            *outW = (int)w;
        if (outH)
            *outH = (int)h;
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

static int ExtractObservedDrawAlpha(const DWORD* variantLikeAlpha, WORD* outVariantType)
{
    if (outVariantType)
        *outVariantType = VT_EMPTY;
    if (!variantLikeAlpha)
        return 255;

    const WORD variantType = static_cast<WORD>(variantLikeAlpha[0] & 0xFFFFu);
    if (outVariantType)
        *outVariantType = variantType;

    switch (variantType)
    {
    case VT_I4:
    case VT_INT:
    case VT_UI4:
    case VT_UINT:
        return static_cast<int>(variantLikeAlpha[2]);
    case VT_I2:
    case VT_UI2:
        return static_cast<short>(variantLikeAlpha[2] & 0xFFFFu);
    case VT_EMPTY:
    default:
        return 255;
    }
}

