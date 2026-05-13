static BOOL ResolveMountedRuntimeSkillNativeReleaseAllowBySkill(
    MountedRuntimeSkillKind kind,
    int skillId,
    int *resolvedMountItemIdOut)
{
    if (!IsMountedRuntimeSkillHooksEnabledForKind(kind))
    {
        return FALSE;
    }

    int mountItemId = 0;
    const bool resolvedRecentRuntimeSkill =
        kind == MountedRuntimeSkillKind_DemonJump
            ? TryResolveRecentMountedDemonJumpNativeReleaseRuntimeSkill(
                  skillId,
                  &mountItemId,
                  nullptr)
            : TryResolveRecentMountedDoubleJumpNativeReleaseRuntimeSkill(
                  skillId,
                  &mountItemId,
                  nullptr);
    if (!resolvedRecentRuntimeSkill)
    {
        return FALSE;
    }

    int currentMountItemId = 0;
    if (TryResolveCurrentUserMountItemIdWithFallback(&currentMountItemId, nullptr) &&
        currentMountItemId > 0 &&
        currentMountItemId != mountItemId)
    {
        return FALSE;
    }

    if (!CanUseMountedRuntimeSkillRuntimeForKind(kind, mountItemId, skillId))
    {
        return FALSE;
    }

    if (resolvedMountItemIdOut)
    {
        *resolvedMountItemIdOut = mountItemId;
    }
    return TRUE;
}

static BOOL ResolveMountedRuntimeSkillNativeReleaseAllowByMountContext(
    MountedRuntimeSkillKind kind,
    void *mountContext,
    int *resolvedMountItemIdOut)
{
    if (!IsMountedRuntimeSkillHooksEnabledForKind(kind))
    {
        return FALSE;
    }

    const LONG recentSkillId =
        InterlockedCompareExchange(&g_recentMountedRuntimeSkillNativeReleaseSkillId[kind], 0, 0);
    if (recentSkillId <= 0)
    {
        if (kind != MountedRuntimeSkillKind_DemonJump)
        {
            return FALSE;
        }

        int contextMountItemId = 0;
        if (!mountContext ||
            !TryResolveMountItemIdFromContextPointer(mountContext, &contextMountItemId) ||
            contextMountItemId <= 0 ||
            !HasRecentMountedDemonJumpIntent(contextMountItemId, 1200))
        {
            return FALSE;
        }

        if (!CanUseMountedRuntimeSkillRuntimeForKind(
                kind,
                contextMountItemId,
                ResolveMountedRuntimeSkillIdForKind(kind, contextMountItemId)))
        {
            return FALSE;
        }

        if (resolvedMountItemIdOut)
        {
            *resolvedMountItemIdOut = contextMountItemId;
        }
        return TRUE;
    }

    int mountItemId = 0;
    const bool resolvedRecentMount =
        kind == MountedRuntimeSkillKind_DemonJump
            ? TryResolveRecentMountedDemonJumpNativeRelease(
                  static_cast<int>(recentSkillId),
                  &mountItemId)
            : TryResolveRecentMountedDoubleJumpNativeRelease(
                  static_cast<int>(recentSkillId),
                  &mountItemId);
    if (!resolvedRecentMount)
    {
        return FALSE;
    }

    int contextMountItemId = 0;
    if (!mountContext ||
        !TryResolveMountItemIdFromContextPointer(mountContext, &contextMountItemId) ||
        contextMountItemId <= 0 ||
        contextMountItemId != mountItemId)
    {
        return FALSE;
    }

    if (!CanUseMountedRuntimeSkillRuntimeForKind(
            kind,
            mountItemId,
            static_cast<int>(recentSkillId)))
    {
        return FALSE;
    }

    if (resolvedMountItemIdOut)
    {
        *resolvedMountItemIdOut = mountItemId;
    }
    return TRUE;
}

static BOOL ResolveMountedConfiguredNativeReleaseAllowBySkill(
    int skillId,
    MountedRuntimeSkillKind *kindOut,
    int *resolvedMountItemIdOut)
{
    if (kindOut)
    {
        *kindOut = MountedRuntimeSkillKind_DoubleJump;
    }

    if (ResolveMountedRuntimeSkillNativeReleaseAllowBySkill(
            MountedRuntimeSkillKind_DoubleJump,
            skillId,
            resolvedMountItemIdOut))
    {
        if (kindOut)
        {
            *kindOut = MountedRuntimeSkillKind_DoubleJump;
        }
        return TRUE;
    }

    if (ResolveMountedRuntimeSkillNativeReleaseAllowBySkill(
            MountedRuntimeSkillKind_DemonJump,
            skillId,
            resolvedMountItemIdOut))
    {
        if (kindOut)
        {
            *kindOut = MountedRuntimeSkillKind_DemonJump;
        }
        return TRUE;
    }

    return FALSE;
}

static BOOL ResolveMountedConfiguredNativeReleaseAllowByMountContext(
    void *mountContext,
    MountedRuntimeSkillKind *kindOut,
    int *resolvedMountItemIdOut)
{
    if (kindOut)
    {
        *kindOut = MountedRuntimeSkillKind_DoubleJump;
    }

    if (ResolveMountedRuntimeSkillNativeReleaseAllowByMountContext(
            MountedRuntimeSkillKind_DoubleJump,
            mountContext,
            resolvedMountItemIdOut))
    {
        if (kindOut)
        {
            *kindOut = MountedRuntimeSkillKind_DoubleJump;
        }
        return TRUE;
    }

    if (ResolveMountedRuntimeSkillNativeReleaseAllowByMountContext(
            MountedRuntimeSkillKind_DemonJump,
            mountContext,
            resolvedMountItemIdOut))
    {
        if (kindOut)
        {
            *kindOut = MountedRuntimeSkillKind_DemonJump;
        }
        return TRUE;
    }

    return FALSE;
}

static BOOL __cdecl hkMountedSkillWhitelist7CF270(int skillId)
{
    BOOL result = oMountedSkillWhitelist7CF270
                      ? oMountedSkillWhitelist7CF270(skillId)
                      : FALSE;
    const bool shouldObserve =
        IsMountedDemonJumpRelatedSkillId(skillId) ||
        skillId == 80001096;
    if (shouldObserve)
    {
        static LONG s_mountedSkillWhitelistObserveLogBudget = 96;
        const LONG budgetAfterDecrement =
            InterlockedDecrement(&s_mountedSkillWhitelistObserveLogBudget);
        if (budgetAfterDecrement >= 0)
        {
            int currentMountItemId = 0;
            const bool hasCurrentMount =
                TryResolveCurrentUserMountItemIdWithFallback(
                    &currentMountItemId,
                    nullptr) &&
                currentMountItemId > 0;
            WriteLogFmt(
                "[MountWhitelist] 7CF270 observe skill=%d native=%d mount=%d hasDoubleIntent=%d hasDemonIntent=%d routeDouble=%d routeDemon=%d",
                skillId,
                result ? 1 : 0,
                hasCurrentMount ? currentMountItemId : 0,
                hasCurrentMount &&
                        HasRecentMountedDoubleJumpIntent(currentMountItemId, 1200)
                    ? 1
                    : 0,
                hasCurrentMount &&
                        HasRecentMountedDemonJumpIntent(currentMountItemId, 1200)
                    ? 1
                    : 0,
                hasCurrentMount &&
                        SkillOverlayBridgeHasRecentMountedDoubleJumpRouteArm(
                            currentMountItemId,
                            1200)
                    ? 1
                    : 0,
                hasCurrentMount &&
                        SkillOverlayBridgeHasRecentMountedDemonJumpRouteArm(
                            currentMountItemId,
                            1200)
                    ? 1
                    : 0);
        }
    }
    if (result)
    {
        return result;
    }

    int resolvedMountItemId = 0;
    MountedRuntimeSkillKind kind = MountedRuntimeSkillKind_DoubleJump;
    if (!ResolveMountedConfiguredNativeReleaseAllowBySkill(
            skillId,
            &kind,
            &resolvedMountItemId))
    {
        return result;
    }

    if (kind == MountedRuntimeSkillKind_DemonJump &&
        IsMountedDemonJumpRuntimeChildSkillId(skillId))
    {
        int rootSkillId = 0;
        int currentSkillId = 0;
        const bool hasContext =
            TryReadMountedDemonJumpContextState(
                &rootSkillId,
                &currentSkillId,
                nullptr);
        static LONG s_mountedSkillWhitelistForceAllowChildLogBudget = 24;
        if (InterlockedDecrement(
                &s_mountedSkillWhitelistForceAllowChildLogBudget) >= 0)
        {
            WriteLogFmt(
                "[MountDemonJump] 7CF270 force allow child skill=%d mount=%d native=%d recent=%d root=%d current=%d",
                skillId,
                resolvedMountItemId,
                result ? 1 : 0,
                HasRecentMountedDemonJumpIntent(resolvedMountItemId, 1200) ? 1 : 0,
                hasContext ? rootSkillId : 0,
                hasContext ? currentSkillId : 0);
        }
        return TRUE;
    }

    static LONG s_mountedSkillWhitelistForceAllowLogBudget = 24;
    const LONG budgetAfterDecrement =
        InterlockedDecrement(&s_mountedSkillWhitelistForceAllowLogBudget);
    if (budgetAfterDecrement >= 0)
    {
        WriteLogFmt("[%s] 7CF270 native release force allow skill=%d mount=%d",
                    GetMountedRuntimeSkillLogTag(kind),
                    skillId,
                    resolvedMountItemId);
    }
    return TRUE;
}

static int __fastcall hkMountedSkillContextGateA9BF40(
    void *thisPtr,
    void * /*edxUnused*/)
{
    int result = oMountedSkillContextGateA9BF40
                     ? oMountedSkillContextGateA9BF40(thisPtr)
                     : 0;
    {
        int debugMountItemId = 0;
        const bool shouldObserve =
            TryResolveMountItemIdFromContextPointer(thisPtr, &debugMountItemId) &&
            debugMountItemId > 0 &&
            (SkillOverlayBridgeResolveMountedDemonJumpSkillId(debugMountItemId) > 0 ||
             SkillOverlayBridgeResolveMountedDoubleJumpSkillId(debugMountItemId) > 0);
        if (shouldObserve)
        {
            static LONG s_mountedSkillContextGateObserveLogBudget = 96;
            const LONG budgetAfterDecrement =
                InterlockedDecrement(&s_mountedSkillContextGateObserveLogBudget);
            if (budgetAfterDecrement >= 0)
            {
                WriteLogFmt(
                    "[MountContextGate] A9BF40 observe native=%d mount=%d doubleIntent=%d demonIntent=%d doubleRecent=%d demonRecent=%d",
                    result,
                    debugMountItemId,
                    HasRecentMountedDoubleJumpIntent(debugMountItemId, 1200) ? 1 : 0,
                    HasRecentMountedDemonJumpIntent(debugMountItemId, 1200) ? 1 : 0,
                    SkillOverlayBridgeResolveMountedDoubleJumpSkillId(debugMountItemId),
                    SkillOverlayBridgeResolveMountedDemonJumpSkillId(debugMountItemId));
            }
        }
    }
    if (result > 0)
    {
        return result;
    }

    int resolvedMountItemId = 0;
    MountedRuntimeSkillKind kind = MountedRuntimeSkillKind_DoubleJump;
    if (!ResolveMountedConfiguredNativeReleaseAllowByMountContext(
            thisPtr,
            &kind,
            &resolvedMountItemId))
    {
        return result;
    }

    static LONG s_mountedSkillContextGateForceAllowLogBudget = 24;
    const LONG budgetAfterDecrement =
        InterlockedDecrement(&s_mountedSkillContextGateForceAllowLogBudget);
    if (budgetAfterDecrement >= 0)
    {
        WriteLogFmt("[%s] A9BF40 native release force allow mount=%d",
                    GetMountedRuntimeSkillLogTag(kind),
                    resolvedMountItemId);
    }
    return 1;
}

static int __fastcall hkMountedSkillContextGateCallsiteB3009F(
    void *mountContext,
    void * /*edxUnused*/)
{
    int result = 0;
    const DWORD originalTarget = g_MountedSkillContextGateCallsiteOriginalTarget
                                     ? g_MountedSkillContextGateCallsiteOriginalTarget
                                     : ADDR_A9BF40;
    if (originalTarget)
    {
        result =
            ((tMountedSkillContextGateFn)(uintptr_t)originalTarget)(mountContext);
    }
    int resolvedMountItemId = 0;
    MountedRuntimeSkillKind kind = MountedRuntimeSkillKind_DoubleJump;
    const bool hasMountedRuntimeSkillContext =
        ResolveMountedConfiguredNativeReleaseAllowByMountContext(
            mountContext,
            &kind,
            &resolvedMountItemId) != FALSE;
    if (result > 0)
    {
        static LONG s_mountedSkillContextGateCallsiteNativeAllowLogBudget = 24;
        const LONG budgetAfterDecrement =
            InterlockedDecrement(&s_mountedSkillContextGateCallsiteNativeAllowLogBudget);
        if (hasMountedRuntimeSkillContext && budgetAfterDecrement >= 0)
        {
            const LONG recentSkillId =
                InterlockedCompareExchange(&g_recentMountedRuntimeSkillNativeReleaseSkillId[kind], 0, 0);
            WriteLogFmt("[%s] B3009F callsite native allow mount=%d skill=%d result=%d",
                        GetMountedRuntimeSkillLogTag(kind),
                        resolvedMountItemId,
                        static_cast<int>(recentSkillId),
                        result);
        }
        return result;
    }

    if (!hasMountedRuntimeSkillContext)
    {
        return result;
    }

    static LONG s_mountedSkillContextGateCallsiteForceAllowLogBudget = 24;
    const LONG budgetAfterDecrement =
        InterlockedDecrement(&s_mountedSkillContextGateCallsiteForceAllowLogBudget);
    if (budgetAfterDecrement >= 0)
    {
        const LONG recentSkillId =
            InterlockedCompareExchange(&g_recentMountedRuntimeSkillNativeReleaseSkillId[kind], 0, 0);
        WriteLogFmt("[%s] B3009F callsite force allow mount=%d skill=%d",
                    GetMountedRuntimeSkillLogTag(kind),
                    resolvedMountItemId,
                    static_cast<int>(recentSkillId));
    }

    return 1;
}

static DWORD __cdecl hkMountedUnknownSkillReleaseBranchTargetB300AC(int skillId)
{
    InterlockedExchange(
        &g_mountedUnknownSkillReleaseBranchRuntimeSkillOverride,
        0);
    int resolvedMountItemId = 0;
    int recentNativeSkillId = 0;
    MountedRuntimeSkillKind kind = MountedRuntimeSkillKind_DoubleJump;
    if ((TryResolveRecentMountedDoubleJumpNativeReleaseRuntimeSkill(
             skillId,
             &resolvedMountItemId,
             &recentNativeSkillId) &&
        (kind = MountedRuntimeSkillKind_DoubleJump, true)) ||
        (TryResolveRecentMountedDemonJumpNativeReleaseRuntimeSkill(
             skillId,
             &resolvedMountItemId,
             &recentNativeSkillId) &&
         (kind = MountedRuntimeSkillKind_DemonJump, true)))
    {
        const int configuredSkillId =
            resolvedMountItemId > 0
                ? ResolveMountedRuntimeSkillIdForKind(kind, resolvedMountItemId)
                : 0;
        if (kind == MountedRuntimeSkillKind_DemonJump)
        {
            int demonContextSkillId = 0;
            int demonContextRootSkillId = 0;
            PrimeMountedDemonJumpContextIfNeeded(
                resolvedMountItemId,
                "B300AC",
                &demonContextSkillId);
            const bool hasMountedDemonContext =
                HasMountedDemonJumpContextPrimedForMount(
                    resolvedMountItemId,
                    &demonContextSkillId,
                    &demonContextRootSkillId);
            const bool isDemonRootSkill =
                configuredSkillId > 0 && skillId == configuredSkillId;
            const bool isDemonProxySkill =
                IsMountedDemonJumpRuntimeProxySkillId(skillId);
            const bool isDemonChildRuntimeSkill =
                !isDemonRootSkill &&
                (skillId == 30010183 ||
                 skillId == 30010184 ||
                 skillId == 30010186);
            int rerouteDemonChildSkillId = 0;
            const char *rerouteDemonChildSource = nullptr;
            if (isDemonRootSkill && !isDemonProxySkill)
            {
                if (IsMountedDemonJumpRuntimeChildSkillId(recentNativeSkillId))
                {
                    rerouteDemonChildSkillId = recentNativeSkillId;
                    rerouteDemonChildSource = "recent-runtime";
                }
                else if ((IsMountedDemonJumpRuntimeProxySkillId(
                              recentNativeSkillId) ||
                          recentNativeSkillId == configuredSkillId) &&
                         TryGetRecentMountedDemonJumpNativeChildSkill(
                             resolvedMountItemId,
                             &rerouteDemonChildSkillId,
                             &rerouteDemonChildSource,
                             1500) &&
                         IsMountedDemonJumpRuntimeChildSkillId(
                             rerouteDemonChildSkillId))
                {
                }
                else if (IsMountedDemonJumpRuntimeProxySkillId(
                             recentNativeSkillId) &&
                         TryResolveMountedDemonJumpProxyFallbackChildSkill(
                             recentNativeSkillId,
                             hasMountedDemonContext ? demonContextSkillId : 0,
                             &rerouteDemonChildSkillId,
                             &rerouteDemonChildSource) &&
                         IsMountedDemonJumpRuntimeChildSkillId(
                             rerouteDemonChildSkillId))
                {
                }
                else if (IsMountedDemonJumpRuntimeChildSkillId(
                             demonContextSkillId))
                {
                    rerouteDemonChildSkillId = demonContextSkillId;
                    rerouteDemonChildSource = "context-child";
                }
            }
            // Latest v21.88 evidence tightened the root cause again:
            // 1. Keeping ESI=root(30010110) into B300E3 still reaches the late
            //    key path, but 805850 receives a5=30010110 and returns 2
            //    immediately because 7D3E30 only whitelists the child skills
            //    30010183/30010184/30010186.
            // 2. Natural demon jump success enters B2F370 with ESI already set
            //    to the child runtime skill (see normal-job x64dbg trace:
            //    00B2F370 start -> ESI=0x01C9EB47).
            //
            // So for mounted root=30010110 we must now mirror the native shape:
            // once we have a fresh child skill (83/84/86), switch ESI to that
            // child and continue through the full-release B300E3 path.
            if (isDemonRootSkill &&
                !isDemonProxySkill &&
                IsMountedDemonJumpRuntimeChildSkillId(
                    rerouteDemonChildSkillId))
            {
                const bool isGlideProxyRecentChildHijack =
                    recentNativeSkillId == 23001002 &&
                    rerouteDemonChildSkillId == 30010183 &&
                    rerouteDemonChildSource &&
                    strcmp(rerouteDemonChildSource, "recent-child-cache") == 0;
                if (isGlideProxyRecentChildHijack)
                {
                    static LONG s_mountedUnknownSkillReleaseBranchDeferGlideRecentChildLogBudget =
                        24;
                    const LONG budgetAfterDecrement =
                        InterlockedDecrement(
                            &s_mountedUnknownSkillReleaseBranchDeferGlideRecentChildLogBudget);
                    if (budgetAfterDecrement >= 0)
                    {
                        WriteLogFmt(
                            "[MountDemonJump] B300AC defer glide recent-child root=%d recent=%d child=%d source=%s configured=%d mount=%d rootCtx=%d current=%d context=%d -> original-target(0x%08X)",
                            skillId,
                            recentNativeSkillId,
                            rerouteDemonChildSkillId,
                            rerouteDemonChildSource
                                ? rerouteDemonChildSource
                                : "unknown",
                            configuredSkillId,
                            resolvedMountItemId,
                            demonContextRootSkillId,
                            demonContextSkillId,
                            hasMountedDemonContext ? 1 : 0,
                            g_MountedUnknownSkillReleaseBranchOriginalTarget
                                ? g_MountedUnknownSkillReleaseBranchOriginalTarget
                                : ADDR_B30240);
                    }
                    return g_MountedUnknownSkillReleaseBranchOriginalTarget
                               ? g_MountedUnknownSkillReleaseBranchOriginalTarget
                               : ADDR_B30240;
                }
                RememberMountedDemonJumpNativeChildSkill(
                    resolvedMountItemId,
                    rerouteDemonChildSkillId,
                    "B300AC-root-reroute");
                ArmMountedDemonJumpCrashTrace(
                    rerouteDemonChildSkillId,
                    resolvedMountItemId);
                InterlockedExchange(
                    &g_mountedUnknownSkillReleaseBranchRuntimeSkillOverride,
                    static_cast<LONG>(rerouteDemonChildSkillId));
                static LONG s_mountedUnknownSkillReleaseBranchRootToChildRerouteLogBudget = 24;
                const LONG budgetAfterDecrement =
                    InterlockedDecrement(
                        &s_mountedUnknownSkillReleaseBranchRootToChildRerouteLogBudget);
                if (budgetAfterDecrement >= 0)
                {
                    WriteLogFmt(
                        "[MountDemonJump] B300AC root->child reroute root=%d recent=%d child=%d source=%s configured=%d mount=%d rootCtx=%d current=%d context=%d -> full-release skill=%d(0x%08X)",
                        skillId,
                        recentNativeSkillId,
                        rerouteDemonChildSkillId,
                        rerouteDemonChildSource
                            ? rerouteDemonChildSource
                            : "unknown",
                        configuredSkillId,
                        resolvedMountItemId,
                        demonContextRootSkillId,
                        demonContextSkillId,
                        hasMountedDemonContext ? 1 : 0,
                        rerouteDemonChildSkillId,
                        ADDR_B300E3);
                }
                return ADDR_B300E3;
            }

            if (isDemonRootSkill && !isDemonProxySkill)
            {
                static LONG s_mountedUnknownSkillReleaseBranchKeepRootNativeLogBudget = 24;
                const LONG budgetAfterDecrement =
                    InterlockedDecrement(
                        &s_mountedUnknownSkillReleaseBranchKeepRootNativeLogBudget);
                if (budgetAfterDecrement >= 0)
                {
                    WriteLogFmt(
                        "[MountDemonJump] B300AC keep root native skill=%d recent=%d configured=%d mount=%d root=%d current=%d context=%d currentProxy=%d target=0x%08X",
                        skillId,
                        recentNativeSkillId,
                        configuredSkillId,
                        resolvedMountItemId,
                        demonContextRootSkillId,
                        demonContextSkillId,
                        hasMountedDemonContext ? 1 : 0,
                        isDemonProxySkill ? 1 : 0,
                        g_MountedUnknownSkillReleaseBranchOriginalTarget
                            ? g_MountedUnknownSkillReleaseBranchOriginalTarget
                            : ADDR_B30240);
                }
                return g_MountedUnknownSkillReleaseBranchOriginalTarget
                           ? g_MountedUnknownSkillReleaseBranchOriginalTarget
                           : ADDR_B30240;
            }

            if (isDemonChildRuntimeSkill)
            {
                // v23.09 latest logs still show a failing early path:
                // - B2F370 first observes proxy=23001002
                // - B300AC immediately enters with skill=30010183 but recent=23001002
                // - 52BCB0 returns 995xx and A9B710 re-reads current=30010110
                //
                // In the normal chain, the late child path (B22630/B1DB10) finishes
                // first, then B300AC sees recent=30010183 and only then full-release
                // succeeds. So keep this deferral extremely narrow: only mounted
                // up-child 30010183, only while the "recent native release" is still
                // the proxy 23001002. Let the original branch continue so the late
                // path can arm the real child release inside the same jump.
                if (skillId == 30010183 &&
                    recentNativeSkillId == 23001002)
                {
                    static LONG s_mountedUnknownSkillReleaseBranchDeferProxyUpChildLogBudget =
                        24;
                    const LONG budgetAfterDecrement =
                        InterlockedDecrement(
                            &s_mountedUnknownSkillReleaseBranchDeferProxyUpChildLogBudget);
                    if (budgetAfterDecrement >= 0)
                    {
                        WriteLogFmt(
                            "[MountDemonJump] B300AC defer proxy-up child skill=%d recent=%d configured=%d mount=%d root=%d current=%d context=%d -> original-target(0x%08X)",
                            skillId,
                            recentNativeSkillId,
                            configuredSkillId,
                            resolvedMountItemId,
                            demonContextRootSkillId,
                            demonContextSkillId,
                            hasMountedDemonContext ? 1 : 0,
                            g_MountedUnknownSkillReleaseBranchOriginalTarget
                                ? g_MountedUnknownSkillReleaseBranchOriginalTarget
                                : ADDR_B30240);
                    }
                    return g_MountedUnknownSkillReleaseBranchOriginalTarget
                               ? g_MountedUnknownSkillReleaseBranchOriginalTarget
                               : ADDR_B30240;
                }

                RememberMountedDemonJumpNativeChildSkill(
                    resolvedMountItemId,
                    skillId,
                    "B300AC");
            }
            ArmMountedDemonJumpCrashTrace(skillId, resolvedMountItemId);
            static LONG s_mountedUnknownSkillReleaseBranchDemonRerouteLogBudget = 24;
            const LONG budgetAfterDecrement =
                InterlockedDecrement(
                    &s_mountedUnknownSkillReleaseBranchDemonRerouteLogBudget);
            if (budgetAfterDecrement >= 0)
            {
                WriteLogFmt(
                    "[MountDemonJump] B300AC reroute demon skill=%d recent=%d configured=%d mount=%d root=%d current=%d context=%d child=%d proxy=%d -> full-release(0x%08X)",
                    skillId,
                    recentNativeSkillId,
                    configuredSkillId,
                    resolvedMountItemId,
                    demonContextRootSkillId,
                    demonContextSkillId,
                    hasMountedDemonContext ? 1 : 0,
                    isDemonChildRuntimeSkill ? 1 : 0,
                    isDemonProxySkill ? 1 : 0,
                    ADDR_B300E3);
            }
            return ADDR_B300E3;
        }

        static LONG s_mountedUnknownSkillReleaseBranchRerouteLogBudget = 24;
        const LONG budgetAfterDecrement =
            InterlockedDecrement(&s_mountedUnknownSkillReleaseBranchRerouteLogBudget);
        if (budgetAfterDecrement >= 0)
        {
            WriteLogFmt("[%s] B300AC reroute skill=%d recent=%d configured=%d mount=%d keep-runtime=%d -> full-release(0x%08X)",
                        GetMountedRuntimeSkillLogTag(kind),
                        skillId,
                        recentNativeSkillId,
                        configuredSkillId,
                        resolvedMountItemId,
                        skillId,
                        ADDR_B300E3);
        }
        return ADDR_B300E3;
    }

    return g_MountedUnknownSkillReleaseBranchOriginalTarget
               ? g_MountedUnknownSkillReleaseBranchOriginalTarget
               : ADDR_B30240;
}

__declspec(naked) static void hkMountedUnknownSkillReleaseBranchB300AC()
{
    __asm
    {
        push esi
        call hkMountedUnknownSkillReleaseBranchTargetB300AC
        add esp, 4
        push eax
        call ConsumeMountedUnknownSkillReleaseBranchRuntimeSkillOverride
        mov ecx, eax
        pop eax
        test ecx, ecx
        jz no_runtime_override
        mov esi, ecx
    no_runtime_override:
        jmp eax
    }
}

static int __fastcall hkSkillLevelBase(void *thisPtr, void * /*edxUnused*/, DWORD playerObj, int skillId, void *cachePtr)
{
    const DWORD callerRet = (DWORD)(uintptr_t)_ReturnAddress();
    SkillOverlayBridgeObserveLevelQueryContext(thisPtr, playerObj);
    int lookupSkillId = SkillOverlayBridgeResolveNativeLevelLookupSkillId(skillId);
    if (lookupSkillId > 0)
    {
        const uintptr_t remappedEntry = SkillOverlayBridgeLookupSkillEntryPointer(lookupSkillId);
        if (!remappedEntry && lookupSkillId != skillId)
        {
            static DWORD s_lastBaseFallbackLogTick = 0;
            const DWORD nowTick = GetTickCount();
            if (nowTick - s_lastBaseFallbackLogTick > 1000)
            {
                s_lastBaseFallbackLogTick = nowTick;
                WriteLogFmt("[SkillLevelHook] 7DA7D0 fallback remap=%d -> donor=%d (entry missing)",
                            lookupSkillId, skillId);
            }
            lookupSkillId = skillId;
        }
    }
    TryPreseedMountedDemonJumpLevelCache(
        "7DA7D0",
        callerRet,
        skillId,
        lookupSkillId,
        cachePtr);
    ObserveMountedDemonJumpGateProbeLevelLookup(
        "7DA7D0",
        "pre",
        callerRet,
        skillId,
        lookupSkillId,
        cachePtr,
        -1,
        0,
        0);
    ObserveMountedDemonJumpFreshChildLevelLookup(
        "7DA7D0",
        callerRet,
        skillId,
        lookupSkillId,
        cachePtr);

    int result = 0;
    int rawResult = 0;
    if (oSkillLevelBase)
    {
        __try
        {
            result = oSkillLevelBase(thisPtr, playerObj, lookupSkillId, cachePtr);
            rawResult = result;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            static DWORD s_lastBaseExceptionLogTick = 0;
            const DWORD nowTick = GetTickCount();
            if (nowTick - s_lastBaseExceptionLogTick > 1000)
            {
                s_lastBaseExceptionLogTick = nowTick;
                WriteLogFmt("[SkillLevelHook] 7DA7D0 EXCEPTION query=%d lookup=%d player=0x%08X cache=0x%08X code=0x%08X",
                            skillId,
                            lookupSkillId,
                            playerObj,
                            (DWORD)(uintptr_t)cachePtr,
                            GetExceptionCode());
            }
            result = 0;
        }
    }
    if (lookupSkillId != skillId)
    {
        WriteLogFmt("[SkillLevelHook] 7DA7D0 query=%d -> %d result=%d",
                    skillId, lookupSkillId, result);
    }
    ObserveMountedDemonJumpFreshChildLevelLookup(
        "7DA7D0",
        callerRet,
        skillId,
        lookupSkillId,
        cachePtr);
    ObserveMountedDemonJumpGateProbeLevelLookup(
        "7DA7D0",
        "post",
        callerRet,
        skillId,
        lookupSkillId,
        cachePtr,
        -1,
        rawResult,
        result);
    if (result <= 0)
    {
        int resolvedMountItemId = 0;
        MountedRuntimeSkillKind kind = MountedRuntimeSkillKind_DoubleJump;
        if (ResolveMountedConfiguredSkillGateAllow(
                skillId,
                lookupSkillId,
                &kind,
                &resolvedMountItemId))
        {
            result = 1;
            if (kind == MountedRuntimeSkillKind_DemonJump)
            {
                TryBackfillMountedDemonJumpLevelCache(
                    "7DA7D0",
                    skillId,
                    lookupSkillId,
                    cachePtr,
                    resolvedMountItemId);
            }
            static LONG s_skillLevelBaseMountedDoubleJumpFallbackLogBudget = 24;
            const LONG budgetAfterDecrement =
                InterlockedDecrement(&s_skillLevelBaseMountedDoubleJumpFallbackLogBudget);
            if (budgetAfterDecrement >= 0)
            {
                WriteLogFmt("[%s] 7DA7D0 fallback query=%d lookup=%d mount=%d -> result=1",
                            GetMountedRuntimeSkillLogTag(kind),
                            skillId,
                            lookupSkillId,
                            resolvedMountItemId);
            }
        }
    }
    if (result <= 0 && (skillId == 80001089 || lookupSkillId == 80001089))
    {
        int resolvedMountItemId = 0;
        bool fromUserLocal = false;
        if (TryResolveExtendedMountContextForSoaring(&resolvedMountItemId, &fromUserLocal))
        {
            result = 1;
            static LONG s_skillLevelBaseRecentMountFallbackLogBudget = 24;
            const LONG budgetAfterDecrement = InterlockedDecrement(&s_skillLevelBaseRecentMountFallbackLogBudget);
            if (budgetAfterDecrement >= 0)
            {
                WriteLogFmt("[SkillLevelHook] 7DA7D0 mount fallback query=%d lookup=%d mount=%d source=%s -> result=1",
                            skillId,
                            lookupSkillId,
                            resolvedMountItemId,
                            fromUserLocal ? "userlocal" : "recent");
            }
        }
    }
    if (skillId == 80001089 || lookupSkillId == 80001089)
    {
        static LONG s_skillLevelBaseFinalSoaringLogBudget = 48;
        const LONG budgetAfterDecrement = InterlockedDecrement(&s_skillLevelBaseFinalSoaringLogBudget);
        if (budgetAfterDecrement >= 0)
        {
            WriteLogFmt("[SkillLevelHook] 7DA7D0 final query=%d lookup=%d result=%d",
                        skillId, lookupSkillId, result);
        }
    }
    ObserveMountedDemonJumpLevelQueryCaller(
        "7DA7D0",
        callerRet,
        skillId,
        lookupSkillId,
        -1,
        rawResult,
        result);
    SkillOverlayBridgeObserveLevelResult(lookupSkillId, result, true);
    return result;
}

static int __fastcall hkSkillLevelCurrent(void *thisPtr, void * /*edxUnused*/, DWORD playerObj, int skillId, void *cachePtr, int flags)
{
    const DWORD callerRet = (DWORD)(uintptr_t)_ReturnAddress();
    SkillOverlayBridgeObserveLevelQueryContext(thisPtr, playerObj);
    int lookupSkillId = SkillOverlayBridgeResolveNativeLevelLookupSkillId(skillId);
    if (lookupSkillId > 0)
    {
        const uintptr_t remappedEntry = SkillOverlayBridgeLookupSkillEntryPointer(lookupSkillId);
        if (!remappedEntry && lookupSkillId != skillId)
        {
            static DWORD s_lastCurrentFallbackLogTick = 0;
            const DWORD nowTick = GetTickCount();
            if (nowTick - s_lastCurrentFallbackLogTick > 1000)
            {
                s_lastCurrentFallbackLogTick = nowTick;
                WriteLogFmt("[SkillLevelHook] 7DBC50 fallback remap=%d -> donor=%d (entry missing)",
                            lookupSkillId, skillId);
            }
            lookupSkillId = skillId;
        }
    }
    TryPreseedMountedDemonJumpLevelCache(
        "7DBC50",
        callerRet,
        skillId,
        lookupSkillId,
        cachePtr);
    ObserveMountedDemonJumpGateProbeLevelLookup(
        "7DBC50",
        "pre",
        callerRet,
        skillId,
        lookupSkillId,
        cachePtr,
        flags,
        0,
        0);
    ObserveMountedDemonJumpFreshChildLevelLookup(
        "7DBC50",
        callerRet,
        skillId,
        lookupSkillId,
        cachePtr);

    int result = 0;
    int rawResult = 0;
    if (oSkillLevelCurrent)
    {
        __try
        {
            result = oSkillLevelCurrent(thisPtr, playerObj, lookupSkillId, cachePtr, flags);
            rawResult = result;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            static DWORD s_lastCurrentExceptionLogTick = 0;
            const DWORD nowTick = GetTickCount();
            if (nowTick - s_lastCurrentExceptionLogTick > 1000)
            {
                s_lastCurrentExceptionLogTick = nowTick;
                WriteLogFmt("[SkillLevelHook] 7DBC50 EXCEPTION query=%d lookup=%d player=0x%08X cache=0x%08X flags=%d code=0x%08X",
                            skillId,
                            lookupSkillId,
                            playerObj,
                            (DWORD)(uintptr_t)cachePtr,
                            flags,
                            GetExceptionCode());
            }
            result = 0;
        }
    }
    if (lookupSkillId != skillId)
    {
        static DWORD s_lastCurrentRemapLogTick = 0;
        const DWORD nowTick = GetTickCount();
        if (ENABLE_HOTPATH_DIAGNOSTIC_LOGS && nowTick - s_lastCurrentRemapLogTick > 1000)
        {
            s_lastCurrentRemapLogTick = nowTick;
            WriteLogFmt("[SkillLevelHook] 7DBC50 query=%d -> %d flags=%d result=%d",
                        skillId, lookupSkillId, flags, result);
        }
    }
    ObserveMountedDemonJumpFreshChildLevelLookup(
        "7DBC50",
        callerRet,
        skillId,
        lookupSkillId,
        cachePtr);
    ObserveMountedDemonJumpGateProbeLevelLookup(
        "7DBC50",
        "post",
        callerRet,
        skillId,
        lookupSkillId,
        cachePtr,
        flags,
        rawResult,
        result);
    if (result <= 0)
    {
        int resolvedMountItemId = 0;
        MountedRuntimeSkillKind kind = MountedRuntimeSkillKind_DoubleJump;
        if (ResolveMountedConfiguredSkillGateAllow(
                skillId,
                lookupSkillId,
                &kind,
                &resolvedMountItemId))
        {
            result = 1;
            if (kind == MountedRuntimeSkillKind_DemonJump)
            {
                TryBackfillMountedDemonJumpLevelCache(
                    "7DBC50",
                    skillId,
                    lookupSkillId,
                    cachePtr,
                    resolvedMountItemId);
            }
            static LONG s_skillLevelCurrentMountedDoubleJumpFallbackLogBudget = 24;
            const LONG budgetAfterDecrement =
                InterlockedDecrement(&s_skillLevelCurrentMountedDoubleJumpFallbackLogBudget);
            if (budgetAfterDecrement >= 0)
            {
                WriteLogFmt("[%s] 7DBC50 fallback query=%d lookup=%d flags=%d mount=%d -> result=1",
                            GetMountedRuntimeSkillLogTag(kind),
                            skillId,
                            lookupSkillId,
                            flags,
                            resolvedMountItemId);
            }
        }
    }
    if (result <= 0 && (skillId == 80001089 || lookupSkillId == 80001089))
    {
        int resolvedMountItemId = 0;
        bool fromUserLocal = false;
        if (TryResolveExtendedMountContextForSoaring(&resolvedMountItemId, &fromUserLocal))
        {
            result = 1;
            static LONG s_skillLevelCurrentRecentMountFallbackLogBudget = 24;
            const LONG budgetAfterDecrement = InterlockedDecrement(&s_skillLevelCurrentRecentMountFallbackLogBudget);
            if (budgetAfterDecrement >= 0)
            {
                WriteLogFmt("[SkillLevelHook] 7DBC50 mount fallback query=%d lookup=%d flags=%d mount=%d source=%s -> result=1",
                            skillId,
                            lookupSkillId,
                            flags,
                            resolvedMountItemId,
                            fromUserLocal ? "userlocal" : "recent");
            }
        }
    }
    if (skillId == 80001089 || lookupSkillId == 80001089)
    {
        static DWORD s_lastCurrentFinalSoaringLogTick = 0;
        const DWORD nowTick = GetTickCount();
        if (ENABLE_HOTPATH_DIAGNOSTIC_LOGS && nowTick - s_lastCurrentFinalSoaringLogTick > 1000)
        {
            s_lastCurrentFinalSoaringLogTick = nowTick;
            WriteLogFmt("[SkillLevelHook] 7DBC50 final query=%d lookup=%d flags=%d result=%d",
                        skillId, lookupSkillId, flags, result);
        }
    }
    ObserveMountedDemonJumpLevelQueryCaller(
        "7DBC50",
        callerRet,
        skillId,
        lookupSkillId,
        flags,
        rawResult,
        result);
    SkillOverlayBridgeObserveLevelResult(lookupSkillId, result, false);
    return result;
}

