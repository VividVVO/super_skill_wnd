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

