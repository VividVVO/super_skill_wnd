static bool IsMountedDemonJumpRelatedSkillId(int skillId)
{
    return skillId == 30010110 ||
           skillId == 30010183 ||
           skillId == 30010184 ||
           skillId == 30010186 ||
           skillId == 20021181 ||
           skillId == 23001002 ||
           skillId == 33001002;
}

static bool IsMountedDemonJumpRuntimeChildSkillId(int skillId)
{
    return skillId == 30010183 ||
           skillId == 30010184 ||
           skillId == 30010186;
}

static bool IsMountedDemonJumpGateProbeLevelSeedCaller(DWORD callerRet)
{
    return callerRet == 0x79AEB002 ||
           callerRet == 0x00A04B7A;
}

static bool TryResolveMountedDemonJumpActiveChildSkill(
    int observedSkillId,
    DWORD maxAgeMs,
    int *mountItemIdOut,
    int *rootSkillIdOut,
    int *currentSkillIdOut,
    int *childSkillIdOut)
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
    if (childSkillIdOut)
    {
        *childSkillIdOut = 0;
    }

    if (IsMountedDemonJumpRuntimeChildSkillId(observedSkillId))
    {
        int visualGraceMountItemId = 0;
        int visualGraceChildSkillId = 0;
        int currentMountItemId = 0;
        if (TryGetRecentMountedDemonJumpPostPacketVisualChildSkill(
                &visualGraceMountItemId,
                &visualGraceChildSkillId,
                maxAgeMs) &&
            visualGraceChildSkillId == observedSkillId &&
            TryReadCurrentUserMountItemId(&currentMountItemId) &&
            currentMountItemId == visualGraceMountItemId &&
            ResolveMountedRuntimeSkillIdForKind(
                MountedRuntimeSkillKind_DemonJump,
                visualGraceMountItemId) == 30010110 &&
            SkillOverlayBridgeCanUseMountedDemonJumpRuntimeSkill(
                visualGraceMountItemId,
                visualGraceChildSkillId))
        {
            if (mountItemIdOut)
            {
                *mountItemIdOut = visualGraceMountItemId;
            }
            if (rootSkillIdOut)
            {
                *rootSkillIdOut = 30010110;
            }
            if (currentSkillIdOut)
            {
                *currentSkillIdOut = visualGraceChildSkillId;
            }
            if (childSkillIdOut)
            {
                *childSkillIdOut = visualGraceChildSkillId;
            }
            return true;
        }
    }

    int mountItemId = 0;
    if ((!TryResolveMountedDemonJumpMountItemIdWithFallback(
             nullptr,
             &mountItemId,
             nullptr,
             maxAgeMs) &&
         !TryGetRecentMountedDemonJumpIntentItemId(&mountItemId, maxAgeMs)) ||
        mountItemId <= 0 ||
        ResolveMountedRuntimeSkillIdForKind(
            MountedRuntimeSkillKind_DemonJump,
            mountItemId) != 30010110 ||
        !HasRecentMountedDemonJumpIntent(mountItemId, maxAgeMs))
    {
        return false;
    }

    int rootSkillId = 0;
    int currentSkillId = 0;
    if (!TryReadMountedDemonJumpContextState(
            &rootSkillId,
            &currentSkillId,
            nullptr))
    {
        return false;
    }

    const bool rootIsChild =
        IsMountedDemonJumpRuntimeChildSkillId(rootSkillId);
    const bool currentIsChild =
        IsMountedDemonJumpRuntimeChildSkillId(currentSkillId);
    if (rootSkillId != 30010110 &&
        !rootIsChild &&
        !currentIsChild)
    {
        return false;
    }

    if (!currentIsChild && rootIsChild)
    {
        currentSkillId = rootSkillId;
    }

    const int effectiveRootSkillId =
        (rootSkillId == 30010110 || rootIsChild || currentIsChild)
            ? 30010110
            : rootSkillId;

    int childSkillId = 0;
    if (IsMountedDemonJumpRuntimeChildSkillId(observedSkillId))
    {
        childSkillId = observedSkillId;
    }
    else if (IsMountedDemonJumpRuntimeChildSkillId(currentSkillId))
    {
        childSkillId = currentSkillId;
    }
    else
    {
        int recentChildSkillId = 0;
        if (TryGetRecentMountedDemonJumpNativeChildSkill(
                mountItemId,
                &recentChildSkillId,
                nullptr,
                maxAgeMs) &&
            IsMountedDemonJumpRuntimeChildSkillId(recentChildSkillId))
        {
            childSkillId = recentChildSkillId;
        }
    }

    if (!IsMountedDemonJumpRuntimeChildSkillId(childSkillId) ||
        !SkillOverlayBridgeCanUseMountedDemonJumpRuntimeSkill(
            mountItemId,
            childSkillId))
    {
        int recentChildSkillId = 0;
        const bool hasRecentChild =
            TryGetRecentMountedDemonJumpNativeChildSkill(
                mountItemId,
                &recentChildSkillId,
                nullptr,
                maxAgeMs) &&
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

        static LONG s_mountedDemonJumpActiveChildUnresolvedLogBudget = 48;
        if (InterlockedDecrement(
                &s_mountedDemonJumpActiveChildUnresolvedLogBudget) >= 0)
        {
            WriteLogFmt(
                "[MountDemonJumpDiag] active child unresolved observed=%d mount=%d root=%d current=%d recentChild=%d gate=%u/%u/%u local=%u/%u/%u",
                observedSkillId,
                mountItemId,
                rootSkillId,
                currentSkillId,
                hasRecentChild ? recentChildSkillId : 0,
                static_cast<unsigned int>(gateMode),
                static_cast<unsigned int>(downLatch),
                static_cast<unsigned int>(upLatch),
                static_cast<unsigned int>(local5E84),
                static_cast<unsigned int>(local5E85),
                static_cast<unsigned int>(local5EE4));
        }
        return false;
    }

    if (mountItemIdOut)
    {
        *mountItemIdOut = mountItemId;
    }
    if (rootSkillIdOut)
    {
        *rootSkillIdOut = effectiveRootSkillId;
    }
    if (currentSkillIdOut)
    {
        *currentSkillIdOut = currentSkillId;
    }
    if (childSkillIdOut)
    {
        *childSkillIdOut = childSkillId;
    }
    return true;
}

static bool IsMountedDemonJumpRuntimeProxySkillId(int skillId)
{
    return skillId == 20021181 ||
           skillId == 23001002 ||
           skillId == 33001002;
}

static bool IsMountedDemonJumpBridgeEntrySkillId(int skillId)
{
    return skillId == 30010110 ||
           IsMountedDemonJumpRuntimeProxySkillId(skillId);
}

static bool TryResolveMountedDemonJumpProxyFallbackChildSkill(
    int proxySkillId,
    int demonContextSkillId,
    int *childSkillIdOut,
    const char **sourceOut)
{
    if (childSkillIdOut)
    {
        *childSkillIdOut = 0;
    }
    if (sourceOut)
    {
        *sourceOut = nullptr;
    }

    // Evidence from v22.51/v22.90 logs:
    // when context is only root/current=30010110, forcing proxy-23001002 to
    // glide child 30010186 hijacks fresh side-jump attempts. Keep this fallback
    // only while the active demon-jump context is already the real glide child.
    if (proxySkillId == 23001002)
    {
        if (demonContextSkillId != 30010186)
        {
            return false;
        }
        if (childSkillIdOut)
        {
            *childSkillIdOut = 30010186;
        }
        if (sourceOut)
        {
            *sourceOut = "proxy-23001002";
        }
        return true;
    }

    return false;
}

static bool TryResolveMountedDemonJumpRequirementBypassContext(
    DWORD callerRet,
    int skillId,
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

    if (callerRet != 0x00B29F1E ||
        !IsMountedDemonJumpRuntimeChildSkillId(skillId))
    {
        return false;
    }

    int mountItemId = 0;
    int rootSkillId = 0;
    int currentSkillId = 0;
    int childSkillId = 0;
    if (!TryResolveMountedDemonJumpActiveChildSkill(
            skillId,
            1200,
            &mountItemId,
            &rootSkillId,
            &currentSkillId,
            &childSkillId) ||
        mountItemId <= 0 ||
        rootSkillId != 30010110 ||
        childSkillId != skillId)
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
    return true;
}

static volatile LONG g_mountedDemonJumpCrashTraceRuntimeSkillId = 0;
static volatile LONG g_mountedDemonJumpCrashTraceMountItemId = 0;
static volatile LONG g_mountedDemonJumpCrashTraceTick = 0;
static volatile LONG g_mountedUnknownSkillReleaseBranchRuntimeSkillOverride = 0;
static volatile LONG g_recentMountedDemonJumpNativeChildSkillId = 0;
static volatile LONG g_recentMountedDemonJumpNativeChildMountItemId = 0;
static volatile LONG g_recentMountedDemonJumpNativeChildTick = 0;
static volatile LONG g_recentMountedDemonJumpPostPacketVisualChildSkillId = 0;
static volatile LONG g_recentMountedDemonJumpPostPacketVisualMountItemId = 0;
static volatile LONG g_recentMountedDemonJumpPostPacketVisualTick = 0;
static volatile LONG g_recentMountedDemonJumpGlidePacketMountItemId = 0;
static volatile LONG g_recentMountedDemonJumpGlidePacketTick = 0;
static volatile LONG g_pendingMountedDemonJumpRewriteActive = 0;
static volatile LONG g_pendingMountedDemonJumpRewriteMountItemId = 0;
static volatile LONG g_pendingMountedDemonJumpRewriteExpectedSkillId = 0;
static volatile LONG g_pendingMountedDemonJumpRewritePacketSkillId = 0;
static volatile LONG g_pendingMountedDemonJumpRewritePacketLevel = 0;
static volatile LONG g_pendingMountedDemonJumpRewriteRuntimeChildSkillId = 0;
static volatile LONG g_pendingMountedDemonJumpRewriteArmTick = 0;
static BYTE g_pendingMountedDemonJumpRewritePacket[16] = {};

