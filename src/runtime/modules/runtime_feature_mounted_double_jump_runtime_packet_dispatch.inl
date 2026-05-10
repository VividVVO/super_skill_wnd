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
