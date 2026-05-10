static int __fastcall hkMountedDemonJumpTrace8057F0(
    void *thisPtr,
    void * /*edxUnused*/,
    DWORD arg1,
    DWORD arg2)
{
    const DWORD callerRet = (DWORD)(uintptr_t)_ReturnAddress();
    int runtimeSkillId = 0;
    int mountItemId = 0;
    const bool trace =
        callerRet == 0x00B30CE7 &&
        IsMountedDemonJumpCrashTraceFresh(&runtimeSkillId, &mountItemId);
    if (trace)
    {
        WriteLogFmt("[MountDemonJumpTrace] 8057F0 enter caller=0x%08X skill=%d mount=%d this=0x%08X args=[0x%08X,0x%08X]",
                    callerRet,
                    runtimeSkillId,
                    mountItemId,
                    (DWORD)(uintptr_t)thisPtr,
                    arg1,
                    arg2);
    }

    const int result = oMountedDemonJumpTrace8057F0
                           ? oMountedDemonJumpTrace8057F0(thisPtr, arg1, arg2)
                           : 0;
    if (trace)
    {
        WriteLogFmt("[MountDemonJumpTrace] 8057F0 leave result=0x%08X", result);
    }
    return result;
}

static int __cdecl hkMountedDemonJumpTrace550FF0(int skillId)
{
    const DWORD callerRet = (DWORD)(uintptr_t)_ReturnAddress();
    int runtimeSkillId = 0;
    int mountItemId = 0;
    const bool trace =
        callerRet == 0x00B30CF1 &&
        IsMountedDemonJumpCrashTraceFresh(&runtimeSkillId, &mountItemId);
    if (trace)
    {
        WriteLogFmt("[MountDemonJumpTrace] 550FF0 enter caller=0x%08X skill=%d mount=%d argSkill=%d",
                    callerRet,
                    runtimeSkillId,
                    mountItemId,
                    skillId);
    }

    const int result = oMountedDemonJumpTrace550FF0
                           ? oMountedDemonJumpTrace550FF0(skillId)
                           : 0;
    if (trace &&
        result == 0 &&
        mountItemId > 0 &&
        IsMountedDemonJumpRuntimeChildSkillId(runtimeSkillId) &&
        HasRecentMountedDemonJumpIntent(mountItemId) &&
        SkillOverlayBridgeCanUseMountedDemonJumpRuntimeSkill(
            mountItemId,
            runtimeSkillId))
    {
        // B2F370 reuses this return value twice: forcing it to 1 suppresses the
        // early fail prompt, but it also trips the later `|| v78` branch at
        // 00B31123 and skips sub_B273B0, which is the local action setup path.
        // Keep the native 0 here and let 551170/A01BF0 own the prompt bypass.
        WriteLogFmt(
            "[MountDemonJumpTrace] 550FF0 keep original zero skill=%d mount=%d argSkill=%d original=%d",
            runtimeSkillId,
            mountItemId,
            skillId,
            result);
    }
    if (trace)
    {
        WriteLogFmt("[MountDemonJumpTrace] 550FF0 leave result=%d", result);
    }
    return result;
}

static int __fastcall hkMountedDemonJumpTrace829EC0(void *thisPtr, void * /*edxUnused*/)
{
    const DWORD callerRet = (DWORD)(uintptr_t)_ReturnAddress();
    int runtimeSkillId = 0;
    int mountItemId = 0;
    const bool trace =
        callerRet == 0x00B30D07 &&
        IsMountedDemonJumpCrashTraceFresh(&runtimeSkillId, &mountItemId);
    if (trace)
    {
        WriteLogFmt("[MountDemonJumpTrace] 829EC0 enter caller=0x%08X skill=%d mount=%d this=0x%08X",
                    callerRet,
                    runtimeSkillId,
                    mountItemId,
                    (DWORD)(uintptr_t)thisPtr);
    }

    const int result = oMountedDemonJumpTrace829EC0
                           ? oMountedDemonJumpTrace829EC0(thisPtr)
                           : 0;
    if (trace &&
        result == 0 &&
        mountItemId > 0 &&
        IsMountedDemonJumpRuntimeChildSkillId(runtimeSkillId) &&
        HasRecentMountedDemonJumpIntent(mountItemId) &&
        SkillOverlayBridgeCanUseMountedDemonJumpRuntimeSkill(
            mountItemId,
            runtimeSkillId))
    {
        WriteLogFmt(
            "[MountDemonJumpTrace] 829EC0 force allow skill=%d mount=%d this=0x%08X original=%d -> 1",
            runtimeSkillId,
            mountItemId,
            (DWORD)(uintptr_t)thisPtr,
            result);
        return 1;
    }
    if (trace)
    {
        WriteLogFmt("[MountDemonJumpTrace] 829EC0 leave result=%d", result);
    }
    return result;
}

static int __fastcall hkMountedDemonJumpTrace829F10(void *thisPtr, void * /*edxUnused*/)
{
    const DWORD callerRet = (DWORD)(uintptr_t)_ReturnAddress();
    int runtimeSkillId = 0;
    int mountItemId = 0;
    const bool trace =
        callerRet == 0x00B30D30 &&
        IsMountedDemonJumpCrashTraceFresh(&runtimeSkillId, &mountItemId);
    if (trace)
    {
        WriteLogFmt("[MountDemonJumpTrace] 829F10 enter caller=0x%08X skill=%d mount=%d this=0x%08X",
                    callerRet,
                    runtimeSkillId,
                    mountItemId,
                    (DWORD)(uintptr_t)thisPtr);
    }

    const int result = oMountedDemonJumpTrace829F10
                           ? oMountedDemonJumpTrace829F10(thisPtr)
                           : 0;
    const int expectedMatchValue =
        runtimeSkillId == 5311002 ? 5310008 : runtimeSkillId;
    if (trace &&
        result != expectedMatchValue &&
        mountItemId > 0 &&
        IsMountedDemonJumpRuntimeChildSkillId(runtimeSkillId) &&
        HasRecentMountedDemonJumpIntent(mountItemId) &&
        SkillOverlayBridgeCanUseMountedDemonJumpRuntimeSkill(
            mountItemId,
            runtimeSkillId))
    {
        WriteLogFmt(
            "[MountDemonJumpTrace] 829F10 force match skill=%d mount=%d this=0x%08X original=0x%08X -> 0x%08X",
            runtimeSkillId,
            mountItemId,
            (DWORD)(uintptr_t)thisPtr,
            result,
            expectedMatchValue);
        return expectedMatchValue;
    }
    if (trace)
    {
        WriteLogFmt("[MountDemonJumpTrace] 829F10 leave result=0x%08X", result);
    }
    return result;
}

static int __fastcall hkMountedDemonJumpTrace551170(void *thisPtr, void * /*edxUnused*/)
{
    const DWORD callerRet = (DWORD)(uintptr_t)_ReturnAddress();
    int runtimeSkillId = 0;
    int mountItemId = 0;
    const bool trace =
        callerRet == 0x00B30D41 &&
        IsMountedDemonJumpCrashTraceFresh(&runtimeSkillId, &mountItemId);
    if (trace)
    {
        WriteLogFmt("[MountDemonJumpTrace] 551170 enter caller=0x%08X skill=%d mount=%d this=0x%08X",
                    callerRet,
                    runtimeSkillId,
                    mountItemId,
                    (DWORD)(uintptr_t)thisPtr);
    }

    const int result = oMountedDemonJumpTrace551170
                           ? oMountedDemonJumpTrace551170(thisPtr)
                           : 0;
    if (trace &&
        result != 0 &&
        mountItemId > 0 &&
        IsMountedDemonJumpRuntimeChildSkillId(runtimeSkillId) &&
        HasRecentMountedDemonJumpIntent(mountItemId) &&
        SkillOverlayBridgeCanUseMountedDemonJumpRuntimeSkill(
            mountItemId,
            runtimeSkillId))
    {
        WriteLogFmt(
            "[MountDemonJumpTrace] 551170 force continue skill=%d mount=%d this=0x%08X original=%d -> 0",
            runtimeSkillId,
            mountItemId,
            (DWORD)(uintptr_t)thisPtr,
            result);
        return 0;
    }
    if (trace)
    {
        WriteLogFmt("[MountDemonJumpTrace] 551170 leave result=%d", result);
    }
    return result;
}

static int __fastcall hkMountedDemonJumpTraceA01BF0(
    void *thisPtr,
    void * /*edxUnused*/,
    int skillId)
{
    const DWORD callerRet = (DWORD)(uintptr_t)_ReturnAddress();
    int runtimeSkillId = 0;
    int mountItemId = 0;
    const bool trace =
        callerRet == 0x00B30D4F &&
        IsMountedDemonJumpCrashTraceFresh(&runtimeSkillId, &mountItemId);
    if (trace)
    {
        WriteLogFmt("[MountDemonJumpTrace] A01BF0 enter caller=0x%08X skill=%d mount=%d this=0x%08X argSkill=%d",
                    callerRet,
                    runtimeSkillId,
                    mountItemId,
                    (DWORD)(uintptr_t)thisPtr,
                    skillId);
    }

    const int result = oMountedDemonJumpTraceA01BF0
                           ? oMountedDemonJumpTraceA01BF0(thisPtr, skillId)
                           : 0;
    if (trace &&
        result != 0 &&
        mountItemId > 0 &&
        IsMountedDemonJumpRuntimeChildSkillId(runtimeSkillId) &&
        HasRecentMountedDemonJumpIntent(mountItemId) &&
        SkillOverlayBridgeCanUseMountedDemonJumpRuntimeSkill(
            mountItemId,
            runtimeSkillId))
    {
        WriteLogFmt(
            "[MountDemonJumpTrace] A01BF0 force continue skill=%d mount=%d this=0x%08X argSkill=%d original=%d -> 0",
            runtimeSkillId,
            mountItemId,
            (DWORD)(uintptr_t)thisPtr,
            skillId,
            result);
        return 0;
    }
    if (trace)
    {
        WriteLogFmt("[MountDemonJumpTrace] A01BF0 leave result=%d", result);
    }
    return result;
}

static int __fastcall hkMountedDemonJumpTrace4C1720(void *thisPtr, void * /*edxUnused*/)
{
    const DWORD callerRet = (DWORD)(uintptr_t)_ReturnAddress();
    int runtimeSkillId = 0;
    int mountItemId = 0;
    const bool trace =
        callerRet == 0x00B30D7F &&
        IsMountedDemonJumpCrashTraceFresh(&runtimeSkillId, &mountItemId);
    if (trace)
    {
        WriteLogFmt("[MountDemonJumpTrace] 4C1720 enter caller=0x%08X skill=%d mount=%d this=0x%08X",
                    callerRet,
                    runtimeSkillId,
                    mountItemId,
                    (DWORD)(uintptr_t)thisPtr);
        LogMountedDemonJumpTraceRowTriplets("4C1720", thisPtr);
    }

    const int result = oMountedDemonJumpTrace4C1720
                           ? oMountedDemonJumpTrace4C1720(thisPtr)
                           : 0;
    if (trace)
    {
        WriteLogFmt("[MountDemonJumpTrace] 4C1720 leave result=%d", result);
    }
    return result;
}

static int __fastcall hkMountedDemonJumpTrace52BCB0(void *thisPtr, void * /*edxUnused*/)
{
    const DWORD callerRet = (DWORD)(uintptr_t)_ReturnAddress();
    int runtimeSkillId = 0;
    int mountItemId = 0;
    const bool trace =
        callerRet == 0x00B30D8C &&
        IsMountedDemonJumpCrashTraceFresh(&runtimeSkillId, &mountItemId);
    if (trace)
    {
        WriteLogFmt("[MountDemonJumpTrace] 52BCB0 enter caller=0x%08X skill=%d mount=%d this=0x%08X",
                    callerRet,
                    runtimeSkillId,
                    mountItemId,
                    (DWORD)(uintptr_t)thisPtr);
        LogMountedDemonJumpTraceRowTriplets("52BCB0", thisPtr);
    }

    const int result = oMountedDemonJumpTrace52BCB0
                           ? oMountedDemonJumpTrace52BCB0(thisPtr)
                           : 0;
    if (trace)
    {
        WriteLogFmt("[MountDemonJumpTrace] 52BCB0 leave result=%d", result);
    }
    return result;
}

static int __fastcall hkMountedDemonJumpTrace805850(
    void *thisPtr,
    void * /*edxUnused*/,
    DWORD arg1,
    DWORD arg2,
    DWORD arg3,
    DWORD arg4,
    DWORD arg5,
    DWORD arg6)
{
    const DWORD callerRet = (DWORD)(uintptr_t)_ReturnAddress();
    int runtimeSkillId = 0;
    int mountItemId = 0;
    const bool trace =
        callerRet == 0x00B30DD8 &&
        IsMountedDemonJumpCrashTraceFresh(&runtimeSkillId, &mountItemId);
    if (trace)
    {
        WriteLogFmt("[MountDemonJumpTrace] 805850 enter caller=0x%08X skill=%d mount=%d this=0x%08X args=[0x%08X,0x%08X,0x%08X,0x%08X,0x%08X,0x%08X]",
                    callerRet,
                    runtimeSkillId,
                    mountItemId,
                    (DWORD)(uintptr_t)thisPtr,
                    arg1,
                    arg2,
                    arg3,
                    arg4,
                    arg5,
                    arg6);
    }

    const int result = oMountedDemonJumpTrace805850
                           ? oMountedDemonJumpTrace805850(
                                 thisPtr,
                                 arg1,
                                 arg2,
                                 arg3,
                                 arg4,
                                 arg5,
                                 arg6)
                           : 0;
    if (trace)
    {
        WriteLogFmt("[MountDemonJumpTrace] 805850 leave result=%d", result);
    }
    return result;
}

