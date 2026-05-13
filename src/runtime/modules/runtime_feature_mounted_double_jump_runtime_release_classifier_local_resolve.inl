static int ResolveMountedDemonJumpLatchedUpChildSkillId(
    int mountItemId,
    int rootSkillId,
    int currentSkillId)
{
    if (mountItemId <= 0 ||
        ResolveMountedRuntimeSkillIdForKind(
            MountedRuntimeSkillKind_DemonJump,
            mountItemId) != 30010110 ||
        !SkillOverlayBridgeCanUseMountedDemonJumpRuntimeSkill(
            mountItemId,
            30010183))
    {
        return 0;
    }

    // The failure-first / success-second samples do not always arrive with a
    // fully primed root/current pair. When the mounted gate itself is already
    // latched to the up branch, accept the "no effective context yet" shape as
    // long as we are not currently carrying some other demon-jump child.
    if ((rootSkillId > 0 && rootSkillId != 30010110) ||
        (currentSkillId > 0 && currentSkillId != 30010110))
    {
        return 0;
    }

    void *userLocal = nullptr;
    if (!TryReadCurrentUserLocalPtr(&userLocal) || !userLocal)
    {
        return 0;
    }

    const uintptr_t userLocalAddr = reinterpret_cast<uintptr_t>(userLocal);
    if (SafeIsBadReadPtr(
            reinterpret_cast<void *>(userLocalAddr + 24292),
            sizeof(BYTE)))
    {
        return 0;
    }

    BYTE gateMode = 0;
    BYTE upLatch = 0;
    __try
    {
        gateMode = *reinterpret_cast<BYTE *>(userLocalAddr + 24292);
        upLatch = *reinterpret_cast<BYTE *>(userLocalAddr + 24197);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        gateMode = 0;
        upLatch = 0;
    }

    // Latest job-100 mounted logs show the failed "retrigger after first up"
    // path entering the classifier as root/current=30010110 while the user gate
    // is already latched to the up branch (mode=3, upLatch=1). Mirror the
    // native child shape here so the next classifier pass can re-enter as
    // 30010183 instead of falling back to keep-root 30010110.
    if (gateMode == 3 && upLatch != 0)
    {
        return 30010183;
    }

    return 0;
}

static int ResolveMountedDemonJumpClassifierLocalSkillId(
    int observedSkillId,
    int *mountItemIdOut,
    int *rootSkillIdOut,
    int *currentSkillIdOut,
    const char **sourceOut)
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
    if (sourceOut)
    {
        *sourceOut = nullptr;
    }

    if (observedSkillId <= 0)
    {
        return 0;
    }
    if (!kEnableMountedDemonJumpRuntimeHooks)
    {
        return 0;
    }

    int mountItemId = 0;
    if (!TryResolveMountedDemonJumpMountItemIdWithFallback(
            nullptr,
            &mountItemId,
            nullptr,
            1200) ||
        mountItemId <= 0)
    {
        return 0;
    }

    if (ResolveMountedRuntimeSkillIdForKind(
            MountedRuntimeSkillKind_DemonJump,
            mountItemId) != 30010110)
    {
        return 0;
    }

    int rootSkillId = 0;
    int currentSkillId = 0;
    const bool hasContext = TryReadMountedDemonJumpEffectiveContextState(
        mountItemId,
        &rootSkillId,
        &currentSkillId);
    const bool hasRecentIntent =
        HasRecentMountedDemonJumpIntent(mountItemId, 1200);

    if (hasContext && rootSkillId != 30010110)
    {
        return 0;
    }

    int localSkillId = 0;
    const char *localSkillSource = nullptr;
    const bool isMountedDemonGlideProxySkill = observedSkillId == 23001002;
    if (observedSkillId == 30010110)
    {
        if (hasContext &&
            IsMountedDemonJumpRuntimeChildSkillId(currentSkillId))
        {
            localSkillId = currentSkillId;
            localSkillSource = "context-child";
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
                localSkillId = recentChildSkillId;
                localSkillSource = "recent-child-cache";
            }
            else
            {
                const int latchedUpChildSkillId =
                    ResolveMountedDemonJumpLatchedUpChildSkillId(
                        mountItemId,
                        rootSkillId,
                        currentSkillId);
                if (IsMountedDemonJumpRuntimeChildSkillId(
                        latchedUpChildSkillId))
                {
                    localSkillId = latchedUpChildSkillId;
                    localSkillSource = "up-latch";
                }
            }
        }
    }
    else if (IsMountedDemonJumpRuntimeChildSkillId(observedSkillId) &&
             SkillOverlayBridgeCanUseMountedDemonJumpRuntimeSkill(
                 mountItemId,
                 observedSkillId) &&
             ((hasContext &&
               (currentSkillId == observedSkillId ||
                currentSkillId == 30010110 ||
                IsMountedDemonJumpRuntimeChildSkillId(currentSkillId))) ||
              hasRecentIntent))
    {
        localSkillId = observedSkillId;
        localSkillSource = "keep-child";
    }

    if (localSkillId <= 0)
    {
        if ((observedSkillId == 20021181 ||
             observedSkillId == 23001002 ||
             observedSkillId == 33001002) &&
            hasRecentIntent)
        {
            if (hasContext &&
                IsMountedDemonJumpRuntimeChildSkillId(currentSkillId) &&
                SkillOverlayBridgeCanUseMountedDemonJumpRuntimeSkill(
                    mountItemId,
                    currentSkillId))
            {
                // For glide proxy 23001002, stale up-child 30010183 must not
                // win over the real glide child. Only keep the live child shape
                // when it is already 30010186; otherwise let the native glide
                // branch continue and resolve its own child.
                if (!isMountedDemonGlideProxySkill ||
                    currentSkillId == 30010186)
                {
                    localSkillId = currentSkillId;
                    localSkillSource = "context-child-proxy";
                }
            }
            else
            {
                int freshRecentChildSkillId = 0;
                if (TryGetRecentMountedDemonJumpNativeChildSkill(
                        mountItemId,
                        &freshRecentChildSkillId,
                        nullptr,
                        kMountedDemonJumpLateChildCacheMatchMaxAgeMs) &&
                        IsMountedDemonJumpRuntimeChildSkillId(
                            freshRecentChildSkillId) &&
                    SkillOverlayBridgeCanUseMountedDemonJumpRuntimeSkill(
                        mountItemId,
                        freshRecentChildSkillId))
                {
                    if (!isMountedDemonGlideProxySkill ||
                        freshRecentChildSkillId == 30010186)
                    {
                        localSkillId = freshRecentChildSkillId;
                        localSkillSource = "recent-child-proxy";
                    }
                }
            }
        }
    }

    if (localSkillId <= 0)
    {
        if ((observedSkillId == 20021181 ||
             observedSkillId == 23001002 ||
             observedSkillId == 33001002) &&
            hasContext)
        {
            const int latchedUpChildSkillId =
                ResolveMountedDemonJumpLatchedUpChildSkillId(
                    mountItemId,
                    rootSkillId,
                    currentSkillId);
            if (IsMountedDemonJumpRuntimeChildSkillId(
                    latchedUpChildSkillId))
            {
                if (!isMountedDemonGlideProxySkill ||
                    latchedUpChildSkillId == 30010186)
                {
                    localSkillId = latchedUpChildSkillId;
                    localSkillSource = "up-latch-proxy";
                }
            }
        }
    }

    if (localSkillId <= 0)
    {
        if ((observedSkillId == 20021181 ||
             observedSkillId == 23001002 ||
             observedSkillId == 33001002) &&
            mountItemId > 0 &&
            hasRecentIntent)
        {
            int recentChildSkillId = 0;
            const bool hasRecentChild =
                TryGetRecentMountedDemonJumpNativeChildSkill(
                    mountItemId,
                    &recentChildSkillId,
                    nullptr,
                    1500) &&
                IsMountedDemonJumpRuntimeChildSkillId(recentChildSkillId);
            BYTE gateMode = 0;
            BYTE downLatch = 0;
            BYTE upLatch = 0;
            BYTE local5E84 = 0;
            BYTE local5E85 = 0;
            BYTE local5EE4 = 0;
            void *userLocal = nullptr;
            if (TryReadCurrentUserLocalPtr(&userLocal) && userLocal)
            {
                const uintptr_t userLocalAddr =
                    reinterpret_cast<uintptr_t>(userLocal);
                __try
                {
                    gateMode =
                        *reinterpret_cast<BYTE *>(userLocalAddr + 24292);
                    downLatch =
                        *reinterpret_cast<BYTE *>(userLocalAddr + 24196);
                    upLatch =
                        *reinterpret_cast<BYTE *>(userLocalAddr + 24197);
                    local5E84 =
                        *reinterpret_cast<BYTE *>(userLocalAddr + 0x5E84);
                    local5E85 =
                        *reinterpret_cast<BYTE *>(userLocalAddr + 0x5E85);
                    local5EE4 =
                        *reinterpret_cast<BYTE *>(userLocalAddr + 0x5EE4);
                }
                __except (EXCEPTION_EXECUTE_HANDLER)
                {
                    gateMode = 0;
                    downLatch = 0;
                    upLatch = 0;
                    local5E84 = 0;
                    local5E85 = 0;
                    local5EE4 = 0;
                }
            }

            static LONG s_mountedDemonJumpClassifierProxyUnresolvedLogBudget = 32;
            if (InterlockedDecrement(
                    &s_mountedDemonJumpClassifierProxyUnresolvedLogBudget) >= 0)
            {
                WriteLogFmt(
                    "[MountDemonJumpDiag] classifier unresolved observed=%d mount=%d root=%d current=%d recentIntent=%d recentChild=%d gate=%u/%u/%u local=%u/%u/%u",
                    observedSkillId,
                    mountItemId,
                    rootSkillId,
                    currentSkillId,
                    hasRecentIntent ? 1 : 0,
                    hasRecentChild ? recentChildSkillId : 0,
                    static_cast<unsigned int>(gateMode),
                    static_cast<unsigned int>(downLatch),
                    static_cast<unsigned int>(upLatch),
                    static_cast<unsigned int>(local5E84),
                    static_cast<unsigned int>(local5E85),
                    static_cast<unsigned int>(local5EE4));
            }
        }
        return 0;
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
    if (sourceOut)
    {
        *sourceOut = localSkillSource;
    }
    return localSkillId;
}

static int ResolveMountedDemonJumpContextFallbackOverrideSkillId(
    int observedSkillId,
    int *mountItemIdOut,
    int *rootSkillIdOut,
    int *currentSkillIdOut)
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
    if (!kEnableMountedDemonJumpRuntimeHooks)
    {
        return 0;
    }

    // Mounted demon jump child skills must stay as child inside the native
    // classifier tree. Only repair the rare root=30010110 case back to the
    // locally resolved child, mirroring the B28A00 packet-side keep-child fix.
    if (observedSkillId != 30010110)
    {
        return 0;
    }

    const int localSkillId = ResolveMountedDemonJumpClassifierLocalSkillId(
        observedSkillId,
        mountItemIdOut,
        rootSkillIdOut,
        currentSkillIdOut,
        nullptr);
    return IsMountedDemonJumpRuntimeChildSkillId(localSkillId)
               ? localSkillId
               : 0;
}
