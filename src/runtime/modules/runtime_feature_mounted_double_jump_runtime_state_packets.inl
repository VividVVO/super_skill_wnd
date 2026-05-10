static void ArmMountedDemonJumpCrashTrace(int runtimeSkillId, int mountItemId)
{
    if (!IsMountedDemonJumpRelatedSkillId(runtimeSkillId) || mountItemId <= 0)
    {
        return;
    }

    InterlockedExchange(&g_mountedDemonJumpCrashTraceRuntimeSkillId, runtimeSkillId);
    InterlockedExchange(&g_mountedDemonJumpCrashTraceMountItemId, mountItemId);
    InterlockedExchange(&g_mountedDemonJumpCrashTraceTick, static_cast<LONG>(GetTickCount()));
}

static bool IsMountedDemonJumpCrashTraceFresh(
    int *runtimeSkillIdOut,
    int *mountItemIdOut,
    DWORD maxAgeMs)
{
    const LONG runtimeSkillId =
        InterlockedCompareExchange(&g_mountedDemonJumpCrashTraceRuntimeSkillId, 0, 0);
    const LONG mountItemId =
        InterlockedCompareExchange(&g_mountedDemonJumpCrashTraceMountItemId, 0, 0);
    const LONG tick =
        InterlockedCompareExchange(&g_mountedDemonJumpCrashTraceTick, 0, 0);
    if (!IsMountedDemonJumpRelatedSkillId(static_cast<int>(runtimeSkillId)) ||
        mountItemId <= 0 ||
        tick <= 0)
    {
        return false;
    }

    const DWORD nowTick = GetTickCount();
    if (nowTick - static_cast<DWORD>(tick) > maxAgeMs)
    {
        return false;
    }

    if (runtimeSkillIdOut)
    {
        *runtimeSkillIdOut = static_cast<int>(runtimeSkillId);
    }
    if (mountItemIdOut)
    {
        *mountItemIdOut = static_cast<int>(mountItemId);
    }
    return true;
}

static void RememberMountedDemonJumpNativeChildSkill(
    int mountItemId,
    int skillId,
    const char *source)
{
    if (mountItemId <= 0 || !IsMountedDemonJumpRuntimeChildSkillId(skillId))
    {
        return;
    }

    InterlockedExchange(&g_recentMountedDemonJumpNativeChildSkillId, skillId);
    InterlockedExchange(&g_recentMountedDemonJumpNativeChildMountItemId, mountItemId);
    InterlockedExchange(
        &g_recentMountedDemonJumpNativeChildTick,
        static_cast<LONG>(GetTickCount()));

    static LONG s_recentMountedDemonJumpNativeChildLogBudget = 48;
    if (InterlockedDecrement(&s_recentMountedDemonJumpNativeChildLogBudget) >= 0)
    {
        WriteLogFmt(
            "[MountDemonJumpChild] remember source=%s mount=%d child=%d",
            source ? source : "unknown",
            mountItemId,
            skillId);
    }
}

static bool TryGetRecentMountedDemonJumpNativeChildSkill(
    int mountItemId,
    int *skillIdOut,
    const char **sourceOut,
    DWORD maxAgeMs)
{
    if (skillIdOut)
    {
        *skillIdOut = 0;
    }
    if (sourceOut)
    {
        *sourceOut = nullptr;
    }

    const LONG recentSkillId =
        InterlockedCompareExchange(&g_recentMountedDemonJumpNativeChildSkillId, 0, 0);
    const LONG recentMountItemId =
        InterlockedCompareExchange(&g_recentMountedDemonJumpNativeChildMountItemId, 0, 0);
    const LONG recentTick =
        InterlockedCompareExchange(&g_recentMountedDemonJumpNativeChildTick, 0, 0);
    if (!IsMountedDemonJumpRuntimeChildSkillId(static_cast<int>(recentSkillId)) ||
        recentMountItemId <= 0 ||
        recentTick <= 0)
    {
        return false;
    }

    if (mountItemId > 0 && recentMountItemId != mountItemId)
    {
        return false;
    }

    const DWORD nowTick = GetTickCount();
    if (nowTick - static_cast<DWORD>(recentTick) > maxAgeMs)
    {
        return false;
    }

    if (skillIdOut)
    {
        *skillIdOut = static_cast<int>(recentSkillId);
    }
    if (sourceOut)
    {
        *sourceOut = "recent-child-cache";
    }
    return true;
}

static void RememberMountedDemonJumpPostPacketVisualChildSkill(
    int mountItemId,
    int skillId,
    const char *source)
{
    if (mountItemId <= 0 || !IsMountedDemonJumpRuntimeChildSkillId(skillId))
    {
        return;
    }

    InterlockedExchange(
        &g_recentMountedDemonJumpPostPacketVisualChildSkillId,
        skillId);
    InterlockedExchange(
        &g_recentMountedDemonJumpPostPacketVisualMountItemId,
        mountItemId);
    InterlockedExchange(
        &g_recentMountedDemonJumpPostPacketVisualTick,
        static_cast<LONG>(GetTickCount()));

    static LONG s_recentMountedDemonJumpPostPacketVisualLogBudget = 48;
    if (InterlockedDecrement(
            &s_recentMountedDemonJumpPostPacketVisualLogBudget) >= 0)
    {
        WriteLogFmt(
            "[MountDemonJumpVisual] remember source=%s mount=%d child=%d",
            source ? source : "unknown",
            mountItemId,
            skillId);
    }
}

static bool TryGetRecentMountedDemonJumpPostPacketVisualChildSkill(
    int *mountItemIdOut,
    int *skillIdOut,
    DWORD maxAgeMs)
{
    if (mountItemIdOut)
    {
        *mountItemIdOut = 0;
    }
    if (skillIdOut)
    {
        *skillIdOut = 0;
    }

    const LONG recentSkillId = InterlockedCompareExchange(
        &g_recentMountedDemonJumpPostPacketVisualChildSkillId,
        0,
        0);
    const LONG recentMountItemId = InterlockedCompareExchange(
        &g_recentMountedDemonJumpPostPacketVisualMountItemId,
        0,
        0);
    const LONG recentTick = InterlockedCompareExchange(
        &g_recentMountedDemonJumpPostPacketVisualTick,
        0,
        0);
    if (!IsMountedDemonJumpRuntimeChildSkillId(
            static_cast<int>(recentSkillId)) ||
        recentMountItemId <= 0 ||
        recentTick <= 0)
    {
        return false;
    }

    const DWORD nowTick = GetTickCount();
    if (nowTick - static_cast<DWORD>(recentTick) > maxAgeMs)
    {
        return false;
    }

    if (mountItemIdOut)
    {
        *mountItemIdOut = static_cast<int>(recentMountItemId);
    }
    if (skillIdOut)
    {
        *skillIdOut = static_cast<int>(recentSkillId);
    }
    return true;
}

static void ClearMountedDemonJumpTransientRuntimeState(
    int mountItemId,
    const char *reasonTag)
{
    if (mountItemId <= 0)
    {
        return;
    }

    ClearMountedRuntimeSkillTransientStateIfMatching(
        MountedRuntimeSkillKind_DoubleJump,
        mountItemId,
        reasonTag ? reasonTag : "mounted-demon-clear",
        "MountDoubleJump");

    const LONG recentGateProbeMountItemId = InterlockedCompareExchange(
        &g_recentMountedDemonJumpGateProbeItemId,
        0,
        0);
    const LONG recentGateProbeChildSkillId = InterlockedCompareExchange(
        &g_recentMountedDemonJumpGateProbeChildSkillId,
        0,
        0);
    const LONG recentGateProbeTick = InterlockedCompareExchange(
        &g_recentMountedDemonJumpGateProbeTick,
        0,
        0);
    const bool hadGateProbe = recentGateProbeMountItemId == mountItemId;
    DWORD gateProbeAgeMs = 0;
    if (hadGateProbe && recentGateProbeTick > 0)
    {
        gateProbeAgeMs =
            GetTickCount() - static_cast<DWORD>(recentGateProbeTick);
    }

    InterlockedExchange(
        &g_recentMountedRuntimeSkillIntentItemId[MountedRuntimeSkillKind_DemonJump],
        0);
    InterlockedExchange(
        &g_recentMountedRuntimeSkillIntentTick[MountedRuntimeSkillKind_DemonJump],
        0);
    InterlockedExchange(
        &g_recentMountedRuntimeSkillNativeReleaseItemId[MountedRuntimeSkillKind_DemonJump],
        0);
    InterlockedExchange(
        &g_recentMountedRuntimeSkillNativeReleaseSkillId[MountedRuntimeSkillKind_DemonJump],
        0);
    InterlockedExchange(
        &g_recentMountedRuntimeSkillNativeReleaseTick[MountedRuntimeSkillKind_DemonJump],
        0);
    InterlockedExchange(&g_recentMountedDemonJumpNativeChildSkillId, 0);
    InterlockedExchange(&g_recentMountedDemonJumpNativeChildMountItemId, 0);
    InterlockedExchange(&g_recentMountedDemonJumpNativeChildTick, 0);
    InterlockedExchange(&g_recentMountedDemonJumpPostPacketVisualChildSkillId, 0);
    InterlockedExchange(&g_recentMountedDemonJumpPostPacketVisualMountItemId, 0);
    InterlockedExchange(&g_recentMountedDemonJumpPostPacketVisualTick, 0);
    InterlockedExchange(&g_recentMountedDemonJumpGlidePacketMountItemId, 0);
    InterlockedExchange(&g_recentMountedDemonJumpGlidePacketTick, 0);
    InterlockedExchange(&g_mountedDemonJumpCrashTraceRuntimeSkillId, 0);
    InterlockedExchange(&g_mountedDemonJumpCrashTraceMountItemId, 0);
    InterlockedExchange(&g_mountedDemonJumpCrashTraceTick, 0);
    InterlockedExchange(&g_mountedUnknownSkillReleaseBranchRuntimeSkillOverride, 0);
    InterlockedExchange(&g_pendingMountedDemonJumpRewriteActive, 0);
    InterlockedExchange(&g_pendingMountedDemonJumpRewriteMountItemId, 0);
    InterlockedExchange(&g_pendingMountedDemonJumpRewriteExpectedSkillId, 0);
    InterlockedExchange(&g_pendingMountedDemonJumpRewritePacketSkillId, 0);
    InterlockedExchange(&g_pendingMountedDemonJumpRewritePacketLevel, 0);
    InterlockedExchange(&g_pendingMountedDemonJumpRewriteRuntimeChildSkillId, 0);
    InterlockedExchange(&g_pendingMountedDemonJumpRewriteArmTick, 0);
    InterlockedExchange(&g_recentMountedDemonJumpGateProbeItemId, 0);
    InterlockedExchange(&g_recentMountedDemonJumpGateProbeChildSkillId, 0);
    InterlockedExchange(&g_recentMountedDemonJumpGateProbeTick, 0);
    ZeroMemory(
        g_pendingMountedDemonJumpRewritePacket,
        sizeof(g_pendingMountedDemonJumpRewritePacket));
    SkillOverlayBridgeClearMountedDemonJumpTransientState(
        mountItemId,
        reasonTag);

    static LONG s_clearMountedDemonJumpTransientRuntimeStateLogBudget = 64;
    if (InterlockedDecrement(
            &s_clearMountedDemonJumpTransientRuntimeStateLogBudget) >= 0)
    {
        WriteLogFmt(
            "[MountDemonJump] clear runtime transient state mount=%d reason=%s hadProbe=%d probeChild=%d probeAge=%u",
            mountItemId,
            reasonTag ? reasonTag : "unknown",
            hadGateProbe ? 1 : 0,
            hadGateProbe
                ? static_cast<int>(recentGateProbeChildSkillId)
                : 0,
            gateProbeAgeMs);
    }
}

static void RememberMountedDemonJumpGlidePacket(int mountItemId)
{
    if (mountItemId <= 0)
    {
        return;
    }

    InterlockedExchange(
        &g_recentMountedDemonJumpGlidePacketMountItemId,
        mountItemId);
    InterlockedExchange(
        &g_recentMountedDemonJumpGlidePacketTick,
        static_cast<LONG>(GetTickCount()));
}

static void ExpireMountedDemonJumpGlideRecentIntent(
    int mountItemId,
    const char *reasonTag)
{
    if (mountItemId <= 0)
    {
        return;
    }

    const LONG recentMountItemId = InterlockedCompareExchange(
        &g_recentMountedRuntimeSkillIntentItemId[MountedRuntimeSkillKind_DemonJump],
        0,
        0);
    const LONG recentTick = InterlockedCompareExchange(
        &g_recentMountedRuntimeSkillIntentTick[MountedRuntimeSkillKind_DemonJump],
        0,
        0);
    if (recentMountItemId != mountItemId)
    {
        return;
    }

    DWORD age = 0;
    if (recentTick > 0)
    {
        age = GetTickCount() - static_cast<DWORD>(recentTick);
    }

    InterlockedExchange(
        &g_recentMountedRuntimeSkillIntentItemId[MountedRuntimeSkillKind_DemonJump],
        0);
    InterlockedExchange(
        &g_recentMountedRuntimeSkillIntentTick[MountedRuntimeSkillKind_DemonJump],
        0);

    static LONG s_mountedDemonJumpGlideRecentIntentExpireLogBudget = 32;
    if (InterlockedDecrement(
            &s_mountedDemonJumpGlideRecentIntentExpireLogBudget) >= 0)
    {
        WriteLogFmt(
            "[MountDemonJumpGlide] expire recent intent mount=%d age=%u reason=%s",
            mountItemId,
            age,
            reasonTag ? reasonTag : "unknown");
    }
}

static bool ShouldSuppressMountedDemonJumpRepeatedGlidePacket(
    int mountItemId,
    DWORD maxAgeMs,
    DWORD *ageOut)
{
    if (ageOut)
    {
        *ageOut = 0;
    }

    if (mountItemId <= 0)
    {
        return false;
    }

    const LONG recentMountItemId = InterlockedCompareExchange(
        &g_recentMountedDemonJumpGlidePacketMountItemId,
        0,
        0);
    const LONG recentTick = InterlockedCompareExchange(
        &g_recentMountedDemonJumpGlidePacketTick,
        0,
        0);
    if (recentMountItemId != mountItemId || recentTick <= 0)
    {
        return false;
    }

    const DWORD nowTick = GetTickCount();
    const DWORD age = nowTick - static_cast<DWORD>(recentTick);
    if (age > maxAgeMs)
    {
        return false;
    }

    if (ageOut)
    {
        *ageOut = age;
    }
    return true;
}

static bool SendMountedDemonJumpSyntheticSpecialMovePacket(
    int skillId,
    int level,
    DWORD *tickOut)
{
    if (tickOut)
    {
        *tickOut = 0;
    }

    if (skillId <= 0)
    {
        return false;
    }

    typedef void(__thiscall *tOutPacketInitFn)(void *thisPtr, unsigned short opcode);
    typedef void(__thiscall *tOutPacketEncode4Fn)(void *thisPtr, int value);
    typedef void(__thiscall *tOutPacketEncode1Fn)(void *thisPtr, int value);
    typedef void(__thiscall *tNetSendPacketFn)(void *thisPtr, void *packet);
    typedef void(__thiscall *tGameFreeFn)(void *heapPtr, void *allocBase);
    typedef DWORD(__cdecl *tGameTickFn)();

    tOutPacketInitFn outPacketInitFn =
        reinterpret_cast<tOutPacketInitFn>(ADDR_750C20);
    tOutPacketEncode4Fn outPacketEncode4Fn =
        reinterpret_cast<tOutPacketEncode4Fn>(ADDR_417240);
    tOutPacketEncode1Fn outPacketEncode1Fn =
        reinterpret_cast<tOutPacketEncode1Fn>(0x004171F0);
    tNetSendPacketFn netSendPacketFn =
        reinterpret_cast<tNetSendPacketFn>(ADDR_4D63A0);
    tGameFreeFn gameFreeFn =
        reinterpret_cast<tGameFreeFn>(ADDR_4020B0);
    tGameTickFn gameTickFn =
        reinterpret_cast<tGameTickFn>(ADDR_B4C450);
    if (!outPacketInitFn ||
        !outPacketEncode4Fn ||
        !outPacketEncode1Fn ||
        !netSendPacketFn ||
        !gameFreeFn ||
        !gameTickFn)
    {
        return false;
    }

    DWORD netClient = 0;
    __try
    {
        netClient = *reinterpret_cast<DWORD *>(ADDR_NetClient);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        netClient = 0;
    }
    if (!netClient)
    {
        return false;
    }

    alignas(4) BYTE packetStorage[0x20] = {};
    DWORD tick = 0;
    __try
    {
        tick = gameTickFn();
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        tick = GetTickCount();
    }

    bool sent = false;
    __try
    {
        outPacketInitFn(packetStorage, 0x93);
        outPacketEncode4Fn(packetStorage, static_cast<int>(tick));
        outPacketEncode4Fn(packetStorage, skillId);
        outPacketEncode1Fn(packetStorage, level);
        netSendPacketFn(reinterpret_cast<void *>(netClient), packetStorage);
        sent = true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        sent = false;
    }

    DWORD allocPtr = 0;
    __try
    {
        allocPtr = *reinterpret_cast<DWORD *>(packetStorage + 4);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        allocPtr = 0;
    }

    if (allocPtr)
    {
        __try
        {
            gameFreeFn(
                reinterpret_cast<void *>(ADDR_GameHeap),
                reinterpret_cast<void *>(allocPtr - 4));
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
        }
    }

    if (sent && tickOut)
    {
        *tickOut = tick;
    }
    return sent;
}

static bool ArmMountedDemonJumpPendingSpecialMoveRewrite(
    int mountItemId,
    int expectedSkillId,
    int packetSkillId,
    int packetLevel,
    int runtimeChildSkillId,
    const char *source,
    DWORD *tickOut)
{
    if (tickOut)
    {
        *tickOut = 0;
    }

    if (mountItemId <= 0 ||
        expectedSkillId <= 0 ||
        packetSkillId <= 0 ||
        packetLevel <= 0 ||
        packetLevel > 255)
    {
        return false;
    }

    const DWORD armTick = GetTickCount();
    InterlockedExchange(&g_pendingMountedDemonJumpRewriteMountItemId, mountItemId);
    InterlockedExchange(&g_pendingMountedDemonJumpRewriteExpectedSkillId, expectedSkillId);
    InterlockedExchange(&g_pendingMountedDemonJumpRewritePacketSkillId, packetSkillId);
    InterlockedExchange(&g_pendingMountedDemonJumpRewritePacketLevel, packetLevel);
    InterlockedExchange(
        &g_pendingMountedDemonJumpRewriteRuntimeChildSkillId,
        runtimeChildSkillId);
    InterlockedExchange(
        &g_pendingMountedDemonJumpRewriteArmTick,
        static_cast<LONG>(armTick));
    InterlockedExchange(&g_pendingMountedDemonJumpRewriteActive, 1);

    static LONG s_mountDemonJumpRewriteArmLogBudget = 64;
    if (InterlockedDecrement(&s_mountDemonJumpRewriteArmLogBudget) >= 0)
    {
        WriteLogFmt(
            "[MountDemonJump93Rewrite] arm mount=%d expectSkill=%d packetSkill=%d packetLevel=%d child=%d source=%s",
            mountItemId,
            expectedSkillId,
            packetSkillId,
            packetLevel,
            runtimeChildSkillId,
            source ? source : "none");
    }

    if (tickOut)
    {
        *tickOut = armTick;
    }
    return true;
}

static int __cdecl ConsumeMountedUnknownSkillReleaseBranchRuntimeSkillOverride()
{
    return static_cast<int>(
        InterlockedExchange(
            &g_mountedUnknownSkillReleaseBranchRuntimeSkillOverride,
            0));
}

static bool TryRewriteMountedDemonJumpOutgoingPacket(
    void **packetDataSlot,
    int *packetLenSlot,
    uintptr_t callerRetAddr)
{
    if (!packetDataSlot || !packetLenSlot || !*packetDataSlot || *packetLenSlot < 7)
    {
        return false;
    }

    if (InterlockedCompareExchange(&g_pendingMountedDemonJumpRewriteActive, 0, 0) == 0)
    {
        return false;
    }

    const LONG armTick =
        InterlockedCompareExchange(&g_pendingMountedDemonJumpRewriteArmTick, 0, 0);
    const DWORD nowTick = GetTickCount();
    if (armTick <= 0 || nowTick - static_cast<DWORD>(armTick) > 1500)
    {
        InterlockedExchange(&g_pendingMountedDemonJumpRewriteActive, 0);
        static LONG s_mountDemonJumpRewriteExpireLogBudget = 16;
        if (InterlockedDecrement(&s_mountDemonJumpRewriteExpireLogBudget) >= 0)
        {
            WriteLogFmt(
                "[MountDemonJump93Rewrite] expire age=%u caller=0x%08X",
                armTick > 0 ? (nowTick - static_cast<DWORD>(armTick)) : 0,
                (DWORD)callerRetAddr);
        }
        return false;
    }

    BYTE *packet = static_cast<BYTE *>(*packetDataSlot);
    unsigned short opcode = 0;
    int observedSkillId = 0;
    BYTE observedLevel = packet[6];
    memcpy(&opcode, packet, sizeof(opcode));
    memcpy(&observedSkillId, packet + 2, sizeof(observedSkillId));
    if (opcode != 0x0094)
    {
        return false;
    }

    const LONG expectedSkillId =
        InterlockedCompareExchange(&g_pendingMountedDemonJumpRewriteExpectedSkillId, 0, 0);
    if (expectedSkillId <= 0 || observedSkillId != expectedSkillId)
    {
        return false;
    }

    const LONG packetSkillId =
        InterlockedCompareExchange(&g_pendingMountedDemonJumpRewritePacketSkillId, 0, 0);
    LONG packetLevel =
        InterlockedCompareExchange(&g_pendingMountedDemonJumpRewritePacketLevel, 0, 0);
    const LONG mountItemId =
        InterlockedCompareExchange(&g_pendingMountedDemonJumpRewriteMountItemId, 0, 0);
    const LONG runtimeChildSkillId =
        InterlockedCompareExchange(&g_pendingMountedDemonJumpRewriteRuntimeChildSkillId, 0, 0);
    if (packetSkillId <= 0)
    {
        return false;
    }
    if (packetLevel <= 0)
    {
        packetLevel = 1;
    }
    if (packetLevel > 255)
    {
        packetLevel = 255;
    }

    DWORD packetTick = 0;
    __try
    {
        typedef DWORD(__cdecl *tGameTickFn)();
        tGameTickFn gameTickFn = reinterpret_cast<tGameTickFn>(ADDR_B4C450);
        packetTick = gameTickFn ? gameTickFn() : GetTickCount();
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        packetTick = GetTickCount();
    }

    const unsigned short newOpcode = 0x0093;
    memcpy(g_pendingMountedDemonJumpRewritePacket, &newOpcode, sizeof(newOpcode));
    memcpy(g_pendingMountedDemonJumpRewritePacket + 2, &packetTick, sizeof(packetTick));
    memcpy(
        g_pendingMountedDemonJumpRewritePacket + 6,
        &packetSkillId,
        sizeof(packetSkillId));
    g_pendingMountedDemonJumpRewritePacket[10] =
        static_cast<BYTE>(packetLevel & 0xFF);

    *packetDataSlot = g_pendingMountedDemonJumpRewritePacket;
    *packetLenSlot = 11;
    InterlockedExchange(&g_pendingMountedDemonJumpRewriteActive, 0);

    static LONG s_mountDemonJumpRewriteApplyLogBudget = 64;
    if (InterlockedDecrement(&s_mountDemonJumpRewriteApplyLogBudget) >= 0)
    {
        WriteLogFmt(
            "[MountDemonJump93Rewrite] apply mount=%d oldSkill=%d oldLevel=%d newSkill=%d newLevel=%d child=%d tick=%u len=%d->%d caller=0x%08X",
            mountItemId,
            observedSkillId,
            observedLevel,
            packetSkillId,
            packetLevel,
            runtimeChildSkillId,
            packetTick,
            7,
            *packetLenSlot,
            (DWORD)callerRetAddr);
    }
    return true;
}

static bool TryReassertMountedDemonJumpChildPacketAfterOverlay(
    void **packetDataSlot,
    int *packetLenSlot,
    uintptr_t callerRetAddr)
{
    if (!packetDataSlot || !packetLenSlot || !*packetDataSlot || *packetLenSlot < 11)
    {
        return false;
    }

    BYTE *packet = static_cast<BYTE *>(*packetDataSlot);
    unsigned short opcode = 0;
    int observedSkillId = 0;
    memcpy(&opcode, packet, sizeof(opcode));
    if (opcode != 0x0093)
    {
        return false;
    }

    memcpy(&observedSkillId, packet + 6, sizeof(observedSkillId));
    if (observedSkillId != 30010110)
    {
        return false;
    }

    int mountItemId = 0;
    int childSkillId = 0;
    if (!TryResolveMountedDemonJumpActiveChildSkill(
            observedSkillId,
            1200,
            &mountItemId,
            nullptr,
            nullptr,
            &childSkillId) ||
        !IsMountedDemonJumpRuntimeChildSkillId(childSkillId))
    {
        return false;
    }

    __try
    {
        memcpy(packet + 6, &childSkillId, sizeof(childSkillId));
        packet[10] = 1;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }

    static LONG s_mountedDemonJumpPacketPostOverlayRewriteLogBudget = 48;
    if (InterlockedDecrement(&s_mountedDemonJumpPacketPostOverlayRewriteLogBudget) >= 0)
    {
        WriteLogFmt(
            "[MountDemonJump93Rewrite] post-overlay caller=0x%08X mount=%d skill=%d->%d len=%d",
            (DWORD)callerRetAddr,
            mountItemId,
            observedSkillId,
            childSkillId,
            *packetLenSlot);
    }
    return true;
}

