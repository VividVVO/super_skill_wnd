static bool HasMountedDemonJumpLateSustainIntent(
    int mountItemId,
    int rootSkillId,
    int currentSkillId,
    DWORD maxAgeMs)
{
    if (mountItemId <= 0 ||
        ResolveMountedRuntimeSkillIdForKind(
            MountedRuntimeSkillKind_DemonJump,
            mountItemId) != 30010110)
    {
        return false;
    }

    if (HasRecentMountedDemonJumpIntent(mountItemId, maxAgeMs))
    {
        return true;
    }

    // v23.36 latest mounted-up logs:
    //   575D60/B22630 a2=18 a3=0x00380300 recent=1
    //   575D60/B22630 a2=18 a3=0x80380000 recent=1
    //   575D60/B22630 a2=38 a3=0x01480000 recent=0 while root/current are still
    //   30010110/30010110 and the native up gate is already latched (mode=3,
    //   upLatch=1).
    //
    // IDA-derived helper ResolveMountedDemonJumpLatchedUpChildSkillId already
    // recognizes that exact gate shape as the native transition into child
    // 30010183. Reuse it here, but keep the sustain extremely narrow:
    // - only mounted demon-jump root context
    // - only before current has become a child
    // - only while the same mount still has a fresh gate probe
    if (rootSkillId == 30010110 &&
        currentSkillId == 30010110)
    {
        int gateProbeMountItemId = 0;
        const bool hasRecentMatchingGateProbe =
            TryGetRecentMountedDemonJumpGateProbeMountItemId(
                &gateProbeMountItemId,
                maxAgeMs + 250) &&
            gateProbeMountItemId == mountItemId;
        if (hasRecentMatchingGateProbe &&
            ResolveMountedDemonJumpLatchedUpChildSkillId(
                mountItemId,
                rootSkillId,
                currentSkillId) == 30010183)
        {
            return true;
        }
    }

    // Evidence captured in the knowledge base (v22.53) already narrowed the
    // only safe late-sustain shapes:
    // - side child 30010184 may reuse the fresh child cache briefly;
    // - glide child 30010186 may reuse only a real recent glide packet while
    //   mounted soaring is still actively alive.
    // Do not revive children purely because current still says 30010184/30010186.
    if (rootSkillId != 30010110)
    {
        return false;
    }

    if (currentSkillId == 30010184)
    {
        int recentChildSkillId = 0;
        return TryGetRecentMountedDemonJumpNativeChildSkill(
                   mountItemId,
                   &recentChildSkillId,
                   nullptr,
                   kMountedDemonJumpLateChildCacheMatchMaxAgeMs) &&
               recentChildSkillId == 30010184;
    }

    if (currentSkillId == 30010186)
    {
        DWORD glidePacketAgeMs = 0;
        const bool hasRecentGlidePacket =
            ShouldSuppressMountedDemonJumpRepeatedGlidePacket(
                mountItemId,
                kMountedDemonJumpGlidePacketSustainMaxAgeMs,
                &glidePacketAgeMs);
        DWORD activeAgeMs = 0;
        DWORD activeDurationMs = 0;
        const bool hasActiveSoaringFlight =
            TryGetMountedSoaringFlightTiming(
                mountItemId,
                &activeAgeMs,
                &activeDurationMs);
        return hasRecentGlidePacket && hasActiveSoaringFlight;
    }

    return false;
}

static bool ShouldObserveMountedDemonJumpLatePath(
    int *mountItemIdOut,
    int *rootSkillIdOut,
    int *currentSkillIdOut,
    bool *recentIntentOut)
{
    if (mountItemIdOut)
    {
        *mountItemIdOut = 0;
    }
    if (rootSkillIdOut)
    {
        *rootSkillIdOut = 0;
    }
    if (currentSkillIdOut)
    {
        *currentSkillIdOut = 0;
    }
    if (recentIntentOut)
    {
        *recentIntentOut = false;
    }

    int mountItemId = 0;
    const bool hasMount =
        TryResolveCurrentUserMountItemIdWithFallback(&mountItemId, nullptr) &&
        mountItemId > 0;
    int rootSkillId = 0;
    int currentSkillId = 0;
    const bool hasContext = TryReadMountedDemonJumpContextState(
        &rootSkillId,
        &currentSkillId,
        nullptr);
    const bool hasRecentIntent =
        hasMount &&
        HasMountedDemonJumpLateSustainIntent(
            mountItemId,
            rootSkillId,
            currentSkillId,
            kMountedDemonJumpIntentMaxAgeMs);
    const bool demonContextRelevant =
        hasContext &&
        (rootSkillId == 30010110 ||
         currentSkillId == 30010110 ||
         currentSkillId == 30010183 ||
         currentSkillId == 30010184 ||
         currentSkillId == 30010186);
    if (!hasRecentIntent && !demonContextRelevant)
    {
        return false;
    }

    if (mountItemIdOut)
    {
        *mountItemIdOut = mountItemId;
    }
    if (rootSkillIdOut)
    {
        *rootSkillIdOut = rootSkillId;
    }
    if (currentSkillIdOut)
    {
        *currentSkillIdOut = currentSkillId;
    }
    if (recentIntentOut)
    {
        *recentIntentOut = hasRecentIntent;
    }
    return true;
}

static int __fastcall hkMountedDemonJumpLateRoute575D60(
    void *thisPtr,
    void * /*edxUnused*/,
    int a2,
    int a3)
{
    int mountItemId = 0;
    int rootSkillId = 0;
    int currentSkillId = 0;
    bool hasRecentIntent = false;
    const bool shouldLog = ShouldObserveMountedDemonJumpLatePath(
        &mountItemId,
        &rootSkillId,
        &currentSkillId,
        &hasRecentIntent);
    const bool isObservedUpLatchStepSignature =
        a2 == 18 &&
        static_cast<unsigned int>(a3) == 0x80380000u;
    const bool isObservedUpCommitSignature =
        a2 == 38 &&
        (static_cast<unsigned int>(a3) == 0x01480000u ||
         static_cast<unsigned int>(a3) == 0x81480000u);
    const bool shouldTryLateRoutePreseed =
        mountItemId > 0 &&
        hasRecentIntent &&
        (isObservedUpLatchStepSignature || isObservedUpCommitSignature) &&
        ((rootSkillId == 0 && currentSkillId == 0) ||
         (rootSkillId == 30010110 && currentSkillId == 30010110));
    bool primedLateRoutePreseed = false;
    bool forcedRawLateRoutePreseed = false;
    int primedLateRouteRootSkillId = 0;
    int primedLateRouteCurrentSkillId = 0;
    if (shouldTryLateRoutePreseed)
    {
        int gateProbeChildSkillId = 0;
        const bool hasFreshUpGateProbe =
            TryGetRecentMountedDemonJumpGateProbePreferredChildSkillId(
                mountItemId,
                &gateProbeChildSkillId,
                kMountedDemonJumpIntentMaxAgeMs) &&
            gateProbeChildSkillId == 30010183;
        if (hasFreshUpGateProbe)
        {
            int primedCurrentSkillId = 0;
            if (rootSkillId == 30010110 &&
                currentSkillId == 30010110)
            {
                forcedRawLateRoutePreseed =
                    TryManualPrimeMountedDemonJumpContextResolveSafe(
                        mountItemId,
                        "575D60-preseed-root-only-up",
                        &primedCurrentSkillId,
                        &primedLateRouteRootSkillId);
                if (forcedRawLateRoutePreseed)
                {
                    primedLateRoutePreseed = true;
                    primedLateRouteCurrentSkillId = primedCurrentSkillId;
                }
            }
            if (!primedLateRoutePreseed)
            {
                primedLateRoutePreseed =
                    PrimeMountedDemonJumpContextIfNeeded(
                        mountItemId,
                        "575D60-preseed",
                        &primedCurrentSkillId);
            }
            if (primedLateRoutePreseed)
            {
                // v23.35 logs show that preseed can restore root/current for the
                // first mounted up-jump while the original short recent-intent
                // window has already expired before the first B22630 commit runs.
                // Refresh only the narrow demon intent timestamp here so the
                // first late-route commit keeps recent=1 without broadening the
                // global up/side/glide intent window.
                ObserveMountedRuntimeSkillIntent(
                    MountedRuntimeSkillKind_DemonJump,
                    mountItemId);
                if (!TryReadMountedDemonJumpEffectiveContextState(
                        mountItemId,
                        &primedLateRouteRootSkillId,
                        &primedLateRouteCurrentSkillId))
                {
                    primedLateRouteRootSkillId = 30010110;
                    if (!IsMountedDemonJumpRuntimeChildSkillId(
                            primedLateRouteCurrentSkillId))
                    {
                        primedLateRouteCurrentSkillId =
                            primedCurrentSkillId > 0
                                ? primedCurrentSkillId
                                : 30010183;
                    }
                }
                rootSkillId = primedLateRouteRootSkillId;
                currentSkillId = primedLateRouteCurrentSkillId;
            }

            static LONG s_mountedDemonJumpLateRoutePreseedLogBudget = 32;
            if (InterlockedDecrement(
                    &s_mountedDemonJumpLateRoutePreseedLogBudget) >= 0)
            {
                WriteLogFmt(
                    "[MountDemonJumpLate] 575D60 preseed a2=%d a3=0x%08X mount=%d recent=%d probeChild=%d prime=%d forceRaw=%d root=%d current=%d",
                    a2,
                    a3,
                    mountItemId,
                    hasRecentIntent ? 1 : 0,
                    gateProbeChildSkillId,
                    primedLateRoutePreseed ? 1 : 0,
                    forcedRawLateRoutePreseed ? 1 : 0,
                    primedLateRouteRootSkillId,
                    primedLateRouteCurrentSkillId);
            }
        }
    }
    if (shouldLog)
    {
        static LONG s_mountedDemonJumpLateRouteEnterLogBudget = 64;
        if (InterlockedDecrement(&s_mountedDemonJumpLateRouteEnterLogBudget) >= 0)
        {
            WriteLogFmt(
                "[MountDemonJumpLate] 575D60 enter this=0x%08X a2=%d a3=0x%08X mount=%d recent=%d root=%d current=%d",
                (DWORD)(uintptr_t)thisPtr,
                a2,
                a3,
                mountItemId,
                hasRecentIntent ? 1 : 0,
                rootSkillId,
                currentSkillId);
        }
    }

    const int result = oMountedDemonJumpLateRoute575D60
                           ? oMountedDemonJumpLateRoute575D60(thisPtr, a2, a3)
                           : 0;
    if (shouldLog)
    {
        static LONG s_mountedDemonJumpLateRouteLeaveLogBudget = 64;
        if (InterlockedDecrement(&s_mountedDemonJumpLateRouteLeaveLogBudget) >= 0)
        {
            int afterRootSkillId = 0;
            int afterCurrentSkillId = 0;
            TryReadMountedDemonJumpContextState(
                &afterRootSkillId,
                &afterCurrentSkillId,
                nullptr);
            WriteLogFmt(
                "[MountDemonJumpLate] 575D60 leave result=%d mount=%d root=%d current=%d",
                result,
                mountItemId,
                afterRootSkillId,
                afterCurrentSkillId);
        }
    }
    return result;
}
