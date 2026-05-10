static DWORD RewriteMountedDemonJumpActionRouteB29C70SkillEntry(
    DWORD callerRet,
    int runtimeSkillId,
    int mountItemId,
    DWORD originalEntryPtr,
    int *originalSkillIdOut,
    int *rewrittenSkillIdOut)
{
    if (originalSkillIdOut)
    {
        *originalSkillIdOut = 0;
    }
    if (rewrittenSkillIdOut)
    {
        *rewrittenSkillIdOut = 0;
    }

    if (callerRet != 0x00B32235 ||
        originalEntryPtr == 0 ||
        !IsMountedDemonJumpRuntimeChildSkillId(runtimeSkillId) ||
        mountItemId <= 0 ||
        ResolveMountedRuntimeSkillIdForKind(
            MountedRuntimeSkillKind_DemonJump,
            mountItemId) != 30010110 ||
        SafeIsBadReadPtr(reinterpret_cast<void *>(originalEntryPtr), 0x20))
    {
        return originalEntryPtr;
    }

    int originalSkillId = 0;
    __try
    {
        originalSkillId = *reinterpret_cast<int *>(originalEntryPtr);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return originalEntryPtr;
    }

    if (originalSkillIdOut)
    {
        *originalSkillIdOut = originalSkillId;
    }

    if (originalSkillId != 30010110)
    {
        if (rewrittenSkillIdOut)
        {
            *rewrittenSkillIdOut = originalSkillId;
        }
        return originalEntryPtr;
    }

    int rootSkillId = 0;
    int currentSkillId = 0;
    if (!TryReadMountedDemonJumpEffectiveContextState(
            mountItemId,
            &rootSkillId,
            &currentSkillId) ||
        rootSkillId != 30010110)
    {
        return originalEntryPtr;
    }

    int childSkillId = 0;
    if (IsMountedDemonJumpRuntimeChildSkillId(currentSkillId))
    {
        childSkillId = currentSkillId;
    }
    else if (IsMountedDemonJumpRuntimeChildSkillId(runtimeSkillId))
    {
        childSkillId = runtimeSkillId;
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
            childSkillId = recentChildSkillId;
        }
    }

    if (!IsMountedDemonJumpRuntimeChildSkillId(childSkillId))
    {
        return originalEntryPtr;
    }

    const uintptr_t childEntryPtr =
        SkillOverlayBridgeLookupSkillEntryPointer(childSkillId);
    if (!childEntryPtr ||
        SafeIsBadReadPtr(reinterpret_cast<void *>(childEntryPtr), 0x20))
    {
        return originalEntryPtr;
    }

    int verifiedChildSkillId = 0;
    __try
    {
        verifiedChildSkillId = *reinterpret_cast<int *>(childEntryPtr);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return originalEntryPtr;
    }

    if (verifiedChildSkillId != childSkillId)
    {
        return originalEntryPtr;
    }

    if (rewrittenSkillIdOut)
    {
        *rewrittenSkillIdOut = childSkillId;
    }
    return static_cast<DWORD>(childEntryPtr);
}

static int __fastcall hkMountedDemonJumpActionRouteB29C70(
    void *thisPtr,
    void * /*edxUnused*/,
    DWORD arg1,
    DWORD arg2,
    DWORD arg3)
{
    const DWORD callerRet = (DWORD)(uintptr_t)_ReturnAddress();
    int runtimeSkillId = 0;
    int mountItemId = 0;
    int originalEntrySkillId = 0;
    int rewrittenEntrySkillId = 0;
    DWORD routeArg1 = arg1;
    const bool trace =
        IsMountedDemonJumpCrashTraceFresh(&runtimeSkillId, &mountItemId);
    if (trace)
    {
        routeArg1 = RewriteMountedDemonJumpActionRouteB29C70SkillEntry(
            callerRet,
            runtimeSkillId,
            mountItemId,
            arg1,
            &originalEntrySkillId,
            &rewrittenEntrySkillId);
    }
    if (trace)
    {
        WriteLogFmt(
            "[MountDemonJumpAction] B29C70 enter caller=0x%08X runtime=%d mount=%d this=0x%08X args=[0x%08X,0x%08X,0x%08X]",
            callerRet,
            runtimeSkillId,
            mountItemId,
            (DWORD)(uintptr_t)thisPtr,
            arg1,
            arg2,
            arg3);
        if (routeArg1 != arg1 &&
            rewrittenEntrySkillId > 0)
        {
            WriteLogFmt(
                "[MountDemonJumpAction] B29C70 rewrite entry mount=%d runtime=%d skill=%d -> child=%d ptr=0x%08X->0x%08X",
                mountItemId,
                runtimeSkillId,
                originalEntrySkillId,
                rewrittenEntrySkillId,
                arg1,
                routeArg1);
        }
    }

    const int result = oMountedDemonJumpActionRouteB29C70
                           ? oMountedDemonJumpActionRouteB29C70(
                                 thisPtr,
                                 routeArg1,
                                 arg2,
                                 arg3)
                           : 0;
    if (trace)
    {
        WriteLogFmt(
            "[MountDemonJumpAction] B29C70 leave caller=0x%08X result=%d mount=%d",
            callerRet,
            result,
            mountItemId);
    }
    return result;
}

static int __fastcall hkMountedDemonJumpActionRouteB24010(
    void *thisPtr,
    void * /*edxUnused*/,
    DWORD arg1,
    DWORD arg2)
{
    const DWORD callerRet = (DWORD)(uintptr_t)_ReturnAddress();
    int runtimeSkillId = 0;
    int mountItemId = 0;
    const bool trace =
        callerRet == 0x00B31320 &&
        IsMountedDemonJumpCrashTraceFresh(&runtimeSkillId, &mountItemId);
    if (trace)
    {
        WriteLogFmt(
            "[MountDemonJumpAction] B24010 enter caller=0x%08X runtime=%d mount=%d this=0x%08X args=[0x%08X,0x%08X]",
            callerRet,
            runtimeSkillId,
            mountItemId,
            (DWORD)(uintptr_t)thisPtr,
            arg1,
            arg2);
    }

    const int result = oMountedDemonJumpActionRouteB24010
                           ? oMountedDemonJumpActionRouteB24010(thisPtr, arg1, arg2)
                           : 0;
    if (trace)
    {
        WriteLogFmt(
            "[MountDemonJumpAction] B24010 leave caller=0x%08X result=%d mount=%d",
            callerRet,
            result,
            mountItemId);
    }
    return result;
}

static int __fastcall hkMountedDemonJumpActionRouteB24EA0(
    void *thisPtr,
    void * /*edxUnused*/,
    DWORD arg1,
    DWORD arg2)
{
    const DWORD callerRet = (DWORD)(uintptr_t)_ReturnAddress();
    int runtimeSkillId = 0;
    int mountItemId = 0;
    const bool trace =
        callerRet == 0x00B31337 &&
        IsMountedDemonJumpCrashTraceFresh(&runtimeSkillId, &mountItemId);
    if (trace)
    {
        WriteLogFmt(
            "[MountDemonJumpAction] B24EA0 enter caller=0x%08X runtime=%d mount=%d this=0x%08X args=[0x%08X,0x%08X]",
            callerRet,
            runtimeSkillId,
            mountItemId,
            (DWORD)(uintptr_t)thisPtr,
            arg1,
            arg2);
    }

    const int result = oMountedDemonJumpActionRouteB24EA0
                           ? oMountedDemonJumpActionRouteB24EA0(thisPtr, arg1, arg2)
                           : 0;
    if (trace)
    {
        WriteLogFmt(
            "[MountDemonJumpAction] B24EA0 leave caller=0x%08X result=%d mount=%d",
            callerRet,
            result,
            mountItemId);
    }
    return result;
}

static int __fastcall hkMountedDemonJumpActionRouteB26550(
    void *thisPtr,
    void * /*edxUnused*/,
    int skillId)
{
    const DWORD callerRet = (DWORD)(uintptr_t)_ReturnAddress();
    int runtimeSkillId = 0;
    int mountItemId = 0;
    const bool trace =
        callerRet == 0x00B31345 &&
        IsMountedDemonJumpCrashTraceFresh(&runtimeSkillId, &mountItemId);
    if (trace)
    {
        WriteLogFmt(
            "[MountDemonJumpAction] B26550 enter caller=0x%08X runtime=%d mount=%d this=0x%08X argSkill=%d",
            callerRet,
            runtimeSkillId,
            mountItemId,
            (DWORD)(uintptr_t)thisPtr,
            skillId);
    }

    const int result = oMountedDemonJumpActionRouteB26550
                           ? oMountedDemonJumpActionRouteB26550(thisPtr, skillId)
                           : 0;
    if (trace)
    {
        WriteLogFmt(
            "[MountDemonJumpAction] B26550 leave caller=0x%08X result=%d mount=%d",
            callerRet,
            result,
            mountItemId);
    }
    return result;
}

static int __fastcall hkMountedDemonJumpActionRouteB26050(
    void *thisPtr,
    void * /*edxUnused*/,
    int skillId)
{
    const DWORD callerRet = (DWORD)(uintptr_t)_ReturnAddress();
    int runtimeSkillId = 0;
    int mountItemId = 0;
    const bool trace =
        callerRet == 0x00B313F6 &&
        IsMountedDemonJumpCrashTraceFresh(&runtimeSkillId, &mountItemId);
    if (trace)
    {
        WriteLogFmt(
            "[MountDemonJumpAction] B26050 enter caller=0x%08X runtime=%d mount=%d this=0x%08X argSkill=%d",
            callerRet,
            runtimeSkillId,
            mountItemId,
            (DWORD)(uintptr_t)thisPtr,
            skillId);
    }

    const int result = oMountedDemonJumpActionRouteB26050
                           ? oMountedDemonJumpActionRouteB26050(thisPtr, skillId)
                           : 0;
    if (trace)
    {
        WriteLogFmt(
            "[MountDemonJumpAction] B26050 leave caller=0x%08X result=%d mount=%d",
            callerRet,
            result,
            mountItemId);
    }
    return result;
}

