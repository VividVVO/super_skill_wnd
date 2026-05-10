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

