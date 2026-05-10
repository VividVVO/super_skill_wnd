static void __fastcall hkMountedSkillPacketDispatchB26760(
    void *thisPtr,
    void * /*edxUnused*/,
    int skillId,
    int level)
{
    int mountItemId = 0;
    const char *mountSource = nullptr;
    const bool resolvedMount =
        TryResolveMountedDemonJumpMountItemIdWithFallback(
            thisPtr,
            &mountItemId,
            &mountSource,
            1200) ||
        TryReadMountItemIdFromPlayerObject(thisPtr, &mountItemId);
    const bool hasRecentIntent =
        resolvedMount && mountItemId > 0 &&
        HasRecentMountedDemonJumpIntent(mountItemId, 1200);
    int recentGlideProxyMountItemId = 0;
    const bool hasRecentGlideProxyNativeRelease =
        resolvedMount &&
        mountItemId > 0 &&
        ((TryResolveRecentMountedRuntimeSkillNativeRelease(
              MountedRuntimeSkillKind_DemonJump,
              23001002,
              &recentGlideProxyMountItemId,
              450) &&
          recentGlideProxyMountItemId == mountItemId) ||
         HasFreshMountedRuntimeSkillNativeReleaseRaw(
             MountedRuntimeSkillKind_DemonJump,
             23001002,
             mountItemId,
             450));
    int rewrittenSkillId = skillId;
    int rewrittenLevel = level;
    const char *rewriteSource = nullptr;
    int packet93SkillId = skillId;
    int packet93Level = level;
    bool packet93RewriteArmed = false;
    DWORD packet93RewriteTick = 0;
    const bool canUseMountedUpChildPacketRewrite =
        resolvedMount &&
        mountItemId > 0 &&
        hasRecentIntent &&
        skillId == 30010183 &&
        ResolveMountedRuntimeSkillIdForKind(
            MountedRuntimeSkillKind_DemonJump,
            mountItemId) == 30010110 &&
        SkillOverlayBridgeCanUseMountedDemonJumpRuntimeSkill(
            mountItemId,
            skillId);
    if (resolvedMount &&
        mountItemId > 0 &&
        skillId == 30010110 &&
        hasRecentIntent)
    {
        packet93SkillId = 30010110;
        packet93Level = 1;
        int rootSkillId = 0;
        int currentSkillId = 0;
        if (TryReadMountedDemonJumpContextState(
                &rootSkillId,
                &currentSkillId,
                nullptr) &&
            rootSkillId == 30010110 &&
            IsMountedDemonJumpRuntimeChildSkillId(currentSkillId))
        {
            if (!(hasRecentGlideProxyNativeRelease &&
                  currentSkillId == 30010183))
            {
                rewrittenSkillId = currentSkillId;
                rewrittenLevel = 1;
                rewriteSource = "native-context";
            }
        }
        else
        {
            int recentChildSkillId = 0;
            const char *recentChildSource = nullptr;
            if (TryGetRecentMountedDemonJumpNativeChildSkill(
                    mountItemId,
                    &recentChildSkillId,
                    &recentChildSource,
                    1500) &&
                IsMountedDemonJumpRuntimeChildSkillId(recentChildSkillId))
            {
                if (!(hasRecentGlideProxyNativeRelease &&
                      recentChildSkillId == 30010183))
                {
                    rewrittenSkillId = recentChildSkillId;
                    rewrittenLevel = 1;
                    rewriteSource = recentChildSource;
                }
            }
        }
        if (hasRecentGlideProxyNativeRelease &&
            rewrittenSkillId == 30010110)
        {
            static LONG s_mountedDemonJumpPacketDispatchDeferGlideUpRewriteLogBudget =
                24;
            if (InterlockedDecrement(
                    &s_mountedDemonJumpPacketDispatchDeferGlideUpRewriteLogBudget) >=
                0)
            {
                WriteLogFmt(
                    "[MountDemonJump93Rewrite] defer glide proxy stale up-child mount=%d skill=%d source=%s",
                    mountItemId,
                    skillId,
                    rewriteSource ? rewriteSource : "none");
            }
        }
        packet93RewriteArmed =
            ArmMountedDemonJumpPendingSpecialMoveRewrite(
                mountItemId,
                skillId,
                packet93SkillId,
                packet93Level,
                IsMountedDemonJumpRuntimeChildSkillId(rewrittenSkillId)
                    ? rewrittenSkillId
                    : 0,
                rewriteSource,
                &packet93RewriteTick);
    }
    else if (canUseMountedUpChildPacketRewrite)
    {
        rewrittenSkillId = 30010183;
        rewrittenLevel = 1;
        rewriteSource = "native-up-child-packet";
        packet93SkillId = 30010110;
        packet93Level = 1;
        packet93RewriteArmed =
            ArmMountedDemonJumpPendingSpecialMoveRewrite(
                mountItemId,
                skillId,
                packet93SkillId,
                packet93Level,
                rewrittenSkillId,
                rewriteSource,
                &packet93RewriteTick);
    }
    const bool shouldLog =
        IsMountedDemonJumpRelatedSkillId(skillId) || hasRecentIntent;
    if (shouldLog)
    {
        int rootSkillId = 0;
        int currentSkillId = 0;
        const bool hasContext = TryReadMountedDemonJumpContextState(
            &rootSkillId,
            &currentSkillId,
            nullptr);
        static LONG s_mountedDemonJumpPacketDispatchLogBudget = 64;
        if (InterlockedDecrement(&s_mountedDemonJumpPacketDispatchLogBudget) >= 0)
        {
            WriteLogFmt(
                "[MountDemonJumpPacket] B26760 caller=0x%08X player=0x%08X skill=%d level=%d child=%d childLevel=%d packet93Skill=%d packet93Level=%d mount=%d source=%s recentIntent=%d root=%d current=%d context=%d rewrite=%s pending93=%d tick=%u",
                (DWORD)(uintptr_t)_ReturnAddress(),
                (DWORD)(uintptr_t)thisPtr,
                skillId,
                level & 0xFF,
                rewrittenSkillId,
                rewrittenLevel & 0xFF,
                packet93SkillId,
                packet93Level & 0xFF,
                mountItemId,
                mountSource ? mountSource : (resolvedMount ? "player" : "none"),
                hasRecentIntent ? 1 : 0,
                hasContext ? rootSkillId : 0,
                hasContext ? currentSkillId : 0,
                hasContext ? 1 : 0,
                rewriteSource ? rewriteSource : "none",
                packet93RewriteArmed ? 1 : 0,
                packet93RewriteTick);
        }
    }

    if (oMountedSkillPacketDispatchB26760)
    {
        oMountedSkillPacketDispatchB26760(
            thisPtr,
            skillId,
            static_cast<char>(level));
    }
}

static int __fastcall hkMountedSkillAttackPacketB28A00(
    void *thisPtr,
    void * /*edxUnused*/,
    int *skillIdPtr,
    int a3,
    int a4,
    int a5,
    int a6,
    unsigned int a7,
    int a8)
{
    const int skillId = skillIdPtr ? *skillIdPtr : 0;
    int mountItemId = 0;
    const char *mountSource = nullptr;
    const bool resolvedMount =
        TryResolveMountedDemonJumpMountItemIdWithFallback(
            thisPtr,
            &mountItemId,
            &mountSource,
            1200) ||
        TryReadMountItemIdFromPlayerObject(thisPtr, &mountItemId);
    const bool hasRecentIntent =
        resolvedMount && mountItemId > 0 &&
        HasRecentMountedDemonJumpIntent(mountItemId, 1200);
    const bool shouldLog =
        IsMountedDemonJumpRelatedSkillId(skillId) || hasRecentIntent;
    int contextRootSkillId = 0;
    int contextCurrentSkillId = 0;
    const bool hasContextBeforeCall =
        (shouldLog ||
         (skillIdPtr &&
          resolvedMount &&
          mountItemId > 0 &&
          hasRecentIntent)) &&
        TryReadMountedDemonJumpContextState(
            &contextRootSkillId,
            &contextCurrentSkillId,
            nullptr);
    int rewrittenSkillId = skillId;
    if (skillIdPtr &&
        resolvedMount &&
        mountItemId > 0 &&
        hasRecentIntent)
    {
        if (skillId == 30010110)
        {
            if (hasContextBeforeCall &&
                contextRootSkillId == 30010110 &&
                IsMountedDemonJumpRuntimeChildSkillId(
                    contextCurrentSkillId))
            {
                rewrittenSkillId = contextCurrentSkillId;
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
                    rewrittenSkillId = recentChildSkillId;
                }
            }

            if (IsMountedDemonJumpRuntimeChildSkillId(rewrittenSkillId))
            {
                static LONG s_mountedDemonJumpAttackPacketKeepRootLogBudget = 32;
                const LONG budgetAfterDecrement =
                    InterlockedDecrement(
                        &s_mountedDemonJumpAttackPacketKeepRootLogBudget);
                if (budgetAfterDecrement >= 0)
                {
                    WriteLogFmt(
                        "[MountDemonJumpPacket] B28A00 keep root=%d localChild=%d mount=%d source=%s recentIntent=%d",
                        skillId,
                        rewrittenSkillId,
                        mountItemId,
                        mountSource ? mountSource : (resolvedMount ? "player" : "none"),
                        hasRecentIntent ? 1 : 0);
                }
            }
        }
    }

    const int originalSkillId = skillId;
    const int effectiveSkillId =
        rewrittenSkillId > 0 ? rewrittenSkillId : originalSkillId;
    if (skillIdPtr &&
        resolvedMount &&
        mountItemId > 0 &&
        hasRecentIntent &&
        hasContextBeforeCall &&
        contextRootSkillId == 30010110 &&
        effectiveSkillId == 30010186)
    {
        DWORD repeatedAge = 0;
        if (ShouldSuppressMountedDemonJumpRepeatedGlidePacket(
                mountItemId,
                kMountedDemonJumpRepeatedGlidePacketSuppressMaxAgeMs,
                &repeatedAge))
        {
            SkillOverlayBridgeCompleteMountedNativeReleaseContext(
                30010110,
                30010186);
            static LONG s_mountedDemonJumpRepeatedGlideSuppressLogBudget = 32;
            if (InterlockedDecrement(
                    &s_mountedDemonJumpRepeatedGlideSuppressLogBudget) >= 0)
            {
                WriteLogFmt(
                    "[MountDemonJumpPacket] B28A00 suppress repeated glide mount=%d skill=%d age=%u root=%d current=%d source=%s",
                    mountItemId,
                    effectiveSkillId,
                    repeatedAge,
                    contextRootSkillId,
                    contextCurrentSkillId,
                    mountSource ? mountSource
                                : (resolvedMount ? "player" : "none"));
            }
            return 1;
        }
    }

    bool rewroteSkillIdPtr = false;
    if (skillIdPtr &&
        rewrittenSkillId > 0 &&
        rewrittenSkillId != originalSkillId)
    {
        __try
        {
            *skillIdPtr = rewrittenSkillId;
            rewroteSkillIdPtr = true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            rewroteSkillIdPtr = false;
        }
    }

    DWORD preWndMan = 0;
    DWORD prePacketBusy = 0;
    DWORD prePacketLastTick = 0;
    DWORD prePacketResourcePtr = 0;
    DWORD prePacketAge = 0;
    int prePacketResource = 0;
    DWORD bypassSavedPacketBusy = 0;
    DWORD bypassSavedPacketLastTick = 0;
    bool bypassedInitialPacketGuard = false;
    if (resolvedMount &&
        mountItemId > 0 &&
        hasRecentIntent &&
        hasContextBeforeCall &&
        contextRootSkillId == 30010110 &&
        IsMountedDemonJumpRuntimeChildSkillId(rewrittenSkillId))
    {
        typedef DWORD(__cdecl *tGameTickFn)();
        __try
        {
            preWndMan = *reinterpret_cast<DWORD *>(ADDR_CWndMan);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            preWndMan = 0;
        }
        if (preWndMan)
        {
            __try
            {
                prePacketBusy =
                    *reinterpret_cast<DWORD *>(preWndMan + 8372);
                prePacketLastTick =
                    *reinterpret_cast<DWORD *>(preWndMan + 8376);
                prePacketResourcePtr =
                    *reinterpret_cast<DWORD *>(preWndMan + 8392);
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                prePacketBusy = 0;
                prePacketLastTick = 0;
                prePacketResourcePtr = 0;
            }
        }
        if (prePacketResourcePtr &&
            oMountedDemonJumpTrace4C1720)
        {
            __try
            {
                prePacketResource = oMountedDemonJumpTrace4C1720(
                    reinterpret_cast<void *>(prePacketResourcePtr));
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                prePacketResource = 0;
            }
        }
        if (prePacketLastTick != 0)
        {
            DWORD nowTick = GetTickCount();
            tGameTickFn gameTickFn =
                reinterpret_cast<tGameTickFn>(ADDR_B4C450);
            __try
            {
                nowTick = gameTickFn ? gameTickFn() : GetTickCount();
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                nowTick = GetTickCount();
            }
            prePacketAge = nowTick - prePacketLastTick;
        }
        if (preWndMan &&
            prePacketResource > 0 &&
            (prePacketBusy != 0 || prePacketAge < 300))
        {
            DWORD syntheticLastTick = 0;
            if (prePacketLastTick != 0)
            {
                DWORD nowTick = GetTickCount();
                tGameTickFn gameTickFn =
                    reinterpret_cast<tGameTickFn>(ADDR_B4C450);
                __try
                {
                    nowTick = gameTickFn ? gameTickFn() : GetTickCount();
                }
                __except (EXCEPTION_EXECUTE_HANDLER)
                {
                    nowTick = GetTickCount();
                }
                syntheticLastTick = nowTick > 300 ? (nowTick - 300) : 0;
            }
            __try
            {
                bypassSavedPacketBusy =
                    *reinterpret_cast<DWORD *>(preWndMan + 8372);
                bypassSavedPacketLastTick =
                    *reinterpret_cast<DWORD *>(preWndMan + 8376);
                *reinterpret_cast<DWORD *>(preWndMan + 8372) = 0;
                *reinterpret_cast<DWORD *>(preWndMan + 8376) =
                    syntheticLastTick;
                bypassedInitialPacketGuard = true;
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                bypassSavedPacketBusy = 0;
                bypassSavedPacketLastTick = 0;
                bypassedInitialPacketGuard = false;
            }
        }
    }

    DWORD nativePostPacketBusy = 0;
    DWORD nativePostPacketLastTick = 0;
    DWORD nativePostPacketAge = 0;
    bool releasedPostPacketGuard = false;
    const int effectiveReleasedSkillId =
        rewrittenSkillId > 0 ? rewrittenSkillId : originalSkillId;
    const bool isMountedDemonJumpChildRelease =
        resolvedMount &&
        mountItemId > 0 &&
        ResolveMountedRuntimeSkillIdForKind(
            MountedRuntimeSkillKind_DemonJump,
            mountItemId) == 30010110 &&
        IsMountedDemonJumpRuntimeChildSkillId(
            effectiveReleasedSkillId);

    const int result = oMountedSkillAttackPacketB28A00
                           ? oMountedSkillAttackPacketB28A00(
                                 thisPtr,
                                 skillIdPtr,
                                 a3,
                                 a4,
                                 a5,
                                 a6,
                                 a7,
                                 a8)
                           : 0;
    if (result != 0 &&
        isMountedDemonJumpChildRelease)
    {
        SkillOverlayBridgeCompleteMountedNativeReleaseContext(
            30010110,
            effectiveReleasedSkillId);
        if (effectiveReleasedSkillId == 30010186)
        {
            RememberMountedDemonJumpGlidePacket(mountItemId);
            // Evidence from v22.06 logs:
            // glide keeps rearming current=30010186 with finalGuard busy=1 and
            // 433380 suppress-clear for roughly the same 400ms as
            // kMountedDemonJumpIntentMaxAgeMs. Expire only the synthetic recent
            // intent here so native clear can recover naturally, without
            // forcibly dropping the live glide context itself.
            ExpireMountedDemonJumpGlideRecentIntent(
                mountItemId,
                "B28A00-glide-postpacket");
        }
    }
    if (result != 0 &&
        resolvedMount &&
        mountItemId > 0)
    {
        TryForceFinalizeMountedDemonJumpUpStateAfterPacket(
            thisPtr,
            mountItemId,
            effectiveReleasedSkillId,
            "B28A00-up-postpacket",
            (DWORD)(uintptr_t)_ReturnAddress());
    }
    if (result != 0 &&
        preWndMan &&
        isMountedDemonJumpChildRelease &&
        (effectiveReleasedSkillId == 30010183 ||
         effectiveReleasedSkillId == 30010184 ||
         effectiveReleasedSkillId == 30010186))
    {
        typedef DWORD(__cdecl *tGameTickFn)();
        DWORD nowTick = GetTickCount();
        tGameTickFn gameTickFn =
            reinterpret_cast<tGameTickFn>(ADDR_B4C450);
        __try
        {
            nowTick = gameTickFn ? gameTickFn() : GetTickCount();
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            nowTick = GetTickCount();
        }

        __try
        {
            DWORD releasedGuardAgeMs = 450;
            if (effectiveReleasedSkillId == 30010183 ||
                effectiveReleasedSkillId == 30010184)
            {
                releasedGuardAgeMs =
                    kMountedDemonJumpPacketGuardReleaseAgeMs;
            }

            nativePostPacketBusy =
                *reinterpret_cast<DWORD *>(preWndMan + 8372);
            nativePostPacketLastTick =
                *reinterpret_cast<DWORD *>(preWndMan + 8376);
            if (nativePostPacketLastTick != 0)
            {
                nativePostPacketAge = nowTick - nativePostPacketLastTick;
            }

            // Evidence from v22.05 logs:
            // once mounted demon child B28A00 succeeds, CWndMan immediately
            // re-arms busy=1 age=0. Up/side are already fully finalized by this
            // point; glide still keeps its native context, but the packet guard
            // tail itself should not continue to block landing movement or the
            // next trigger window. v22.36 logs then showed finalGuard age=450
            // alongside the remaining ~0.5s landing lock. For up/side, release
            // this guard to an already-old age instead of refreshing it back
            // into the same block window; if the prepacket guard was even older,
            // preserve that older timestamp.
            DWORD releasedGuardLastTick =
                nowTick > releasedGuardAgeMs
                    ? (nowTick - releasedGuardAgeMs)
                    : 0;
            if (prePacketLastTick != 0 &&
                prePacketAge > releasedGuardAgeMs &&
                (releasedGuardLastTick == 0 ||
                 prePacketLastTick < releasedGuardLastTick))
            {
                releasedGuardLastTick = prePacketLastTick;
            }
            *reinterpret_cast<DWORD *>(preWndMan + 8372) = 0;
            *reinterpret_cast<DWORD *>(preWndMan + 8376) =
                releasedGuardLastTick;
            releasedPostPacketGuard = true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            nativePostPacketBusy = 0;
            nativePostPacketLastTick = 0;
            nativePostPacketAge = 0;
            releasedPostPacketGuard = false;
        }
    }
    if (bypassedInitialPacketGuard &&
        result == 0 &&
        preWndMan)
    {
        __try
        {
            *reinterpret_cast<DWORD *>(preWndMan + 8372) =
                bypassSavedPacketBusy;
            *reinterpret_cast<DWORD *>(preWndMan + 8376) =
                bypassSavedPacketLastTick;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
        }
    }
    if (rewroteSkillIdPtr)
    {
        __try
        {
            *skillIdPtr = originalSkillId;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
        }
    }
    if (shouldLog)
    {
        int rootSkillId = 0;
        int currentSkillId = 0;
        const bool hasContext = TryReadMountedDemonJumpContextState(
            &rootSkillId,
            &currentSkillId,
            nullptr);
        DWORD wndMan = 0;
        DWORD packetBusy = 0;
        DWORD packetLastTick = 0;
        DWORD packetResourcePtr = 0;
        DWORD packetAge = 0;
        int packetResource = 0;
        BOOL nativeWhitelist = FALSE;
        typedef DWORD(__cdecl *tGameTickFn)();
        __try
        {
            wndMan = *reinterpret_cast<DWORD *>(ADDR_CWndMan);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            wndMan = 0;
        }
        if (wndMan)
        {
            __try
            {
                packetBusy = *reinterpret_cast<DWORD *>(wndMan + 8372);
                packetLastTick = *reinterpret_cast<DWORD *>(wndMan + 8376);
                packetResourcePtr = *reinterpret_cast<DWORD *>(wndMan + 8392);
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                packetBusy = 0;
                packetLastTick = 0;
                packetResourcePtr = 0;
            }
        }
        if (packetResourcePtr &&
            oMountedDemonJumpTrace4C1720)
        {
            __try
            {
                packetResource = oMountedDemonJumpTrace4C1720(
                    reinterpret_cast<void *>(packetResourcePtr));
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                packetResource = 0;
            }
        }
        if (packetLastTick != 0)
        {
            DWORD nowTick = GetTickCount();
            tGameTickFn gameTickFn =
                reinterpret_cast<tGameTickFn>(ADDR_B4C450);
            __try
            {
                nowTick = gameTickFn ? gameTickFn() : GetTickCount();
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                nowTick = GetTickCount();
            }
            packetAge = nowTick - packetLastTick;
        }
        if (oMountedSkillWhitelist7CF270)
        {
            __try
            {
                nativeWhitelist = oMountedSkillWhitelist7CF270(
                    rewrittenSkillId > 0 ? rewrittenSkillId : originalSkillId);
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                nativeWhitelist = FALSE;
            }
        }
        static LONG s_mountedDemonJumpAttackPacketLogBudget = 64;
        if (InterlockedDecrement(&s_mountedDemonJumpAttackPacketLogBudget) >= 0)
        {
            WriteLogFmt(
                "[MountDemonJumpPacket] B28A00 caller=0x%08X player=0x%08X skill=%d sent=%d mount=%d source=%s recentIntent=%d root=%d current=%d context=%d result=%d count=%d arg4=%d opcode147Tail=%u nativeWhitelist=%d preGuard[busy=%u resource=%d age=%u] nativePostGuard[busy=%u age=%u] finalGuard[busy=%u resource=%d age=%u] bypass=%d postRelease=%d",
                (DWORD)(uintptr_t)_ReturnAddress(),
                (DWORD)(uintptr_t)thisPtr,
                originalSkillId,
                rewrittenSkillId,
                mountItemId,
                mountSource ? mountSource : (resolvedMount ? "player" : "none"),
                hasRecentIntent ? 1 : 0,
                hasContext ? rootSkillId : 0,
                hasContext ? currentSkillId : 0,
                hasContext ? 1 : 0,
                result,
                a5,
                a4,
                a7,
                nativeWhitelist ? 1 : 0,
                prePacketBusy,
                prePacketResource,
                prePacketAge,
                nativePostPacketBusy,
                nativePostPacketAge,
                packetBusy,
                packetResource,
                packetAge,
                bypassedInitialPacketGuard ? 1 : 0,
                releasedPostPacketGuard ? 1 : 0);
        }
    }

    return result;
}

static int __fastcall hkMountedUseFailPromptAE6260(
    void *thisPtr,
    void * /*edxUnused*/,
    int a2)
{
    int mountItemId = 0;
    int configuredSkillId = 0;
    MountedRuntimeSkillKind kind = MountedRuntimeSkillKind_DoubleJump;
    const DWORD callerRet = (DWORD)(uintptr_t)_ReturnAddress();
    if (ShouldSuppressMountedConfiguredUseFailPrompt(
            thisPtr,
            a2,
            callerRet,
            &kind,
            &mountItemId,
            &configuredSkillId))
    {
        static LONG s_mountUseFailPromptSuppressLogBudget = 24;
        if (InterlockedDecrement(&s_mountUseFailPromptSuppressLogBudget) >= 0)
        {
            WriteLogFmt(
                "[%s] AE6260 suppress prompt mount=%d skill=%d reason=%d caller=0x%08X",
                GetMountedRuntimeSkillLogTag(kind),
                mountItemId,
                configuredSkillId,
                a2,
                callerRet);
        }
        return 1;
    }

    int debugMountItemId = 0;
    if (TryResolveCurrentUserMountItemIdWithFallback(&debugMountItemId, nullptr))
    {
        MountedMovementOverride mountedOverride = {};
        if (debugMountItemId > 0 &&
            SkillOverlayBridgeResolveMountedMovementOverride(
                debugMountItemId,
                0,
                mountedOverride) &&
            mountedOverride.matched)
        {
            static LONG s_mountUseFailPromptObserveLogBudget = 24;
            if (InterlockedDecrement(&s_mountUseFailPromptObserveLogBudget) >= 0)
            {
                WriteLogFmt(
                    "[MountUseFail] AE6260 passthrough mount=%d reason=%d caller=0x%08X doubleIntent=%d doubleSkill=%d demonIntent=%d demonSkill=%d",
                    debugMountItemId,
                    a2,
                    callerRet,
                    HasRecentMountedDoubleJumpIntent(debugMountItemId, 1200) ? 1 : 0,
                    SkillOverlayBridgeResolveMountedDoubleJumpSkillId(debugMountItemId),
                    HasRecentMountedDemonJumpIntent(debugMountItemId, 1200) ? 1 : 0,
                    SkillOverlayBridgeResolveMountedDemonJumpSkillId(debugMountItemId));
            }
        }
    }

    return oMountedUseFailPromptAE6260
               ? oMountedUseFailPromptAE6260(thisPtr, a2)
               : 0;
}

