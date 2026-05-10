static int __cdecl hkMountNativeFlightSkillMap7CF370(int mountItemId)
{
    const int extendedSkillId = ResolveExtendedMountNativeFlightSkillId(mountItemId);
    if (extendedSkillId > 0)
    {
        ObserveExtendedMountContext(mountItemId);
        if (extendedSkillId == 80001089)
        {
            SkillOverlayBridgeObserveExtendedMountSoaringIntent(mountItemId, extendedSkillId);
        }
        static LONG s_mountNativeFlightSkillLogBudget = 8;
        const LONG budgetAfterDecrement = InterlockedDecrement(&s_mountNativeFlightSkillLogBudget);
        if (budgetAfterDecrement >= 0)
        {
            WriteLogFmt("[MountFlightMap] 7CF370 extend mount=%d -> skill=%d", mountItemId, extendedSkillId);
        }
        return extendedSkillId;
    }

    const int result = oMountNativeFlightSkillMap7CF370
                           ? oMountNativeFlightSkillMap7CF370(mountItemId)
                           : 0;

    if (IsExtendedMountServerValidatedSoaringMount(mountItemId))
    {
        static LONG s_mountNativeFlightObserveLogBudget = 12;
        const LONG budgetAfterDecrement = InterlockedDecrement(&s_mountNativeFlightObserveLogBudget);
        if (budgetAfterDecrement >= 0)
        {
            WriteLogFmt("[MountFlightMap] 7CF370 native mount=%d -> skill=%d", mountItemId, result);
        }
    }

    return result;
}

static int ResolveExtendedMountNativeSoaringShadowMountItemId(int mountItemId)
{
    if (IsExtendedMountServerValidatedSoaringMount(mountItemId))
    {
        // 只用于 B26290 开头的 199xxxx 家族门槛；真实发包 skillId 仍保持 80001089。
        return 1992018;
    }
    return 0;
}

static int __fastcall hkMountNativeSoaringReleaseB26290(
    void *thisPtr,
    void * /*edxUnused*/,
    int skillId)
{
    int mountItemId = 0;
    bool hasMountItemId = TryReadMountItemIdFromPlayerObject(thisPtr, &mountItemId);
    if (!hasMountItemId &&
        TryResolveCurrentUserMountItemIdWithFallback(&mountItemId, nullptr))
    {
        hasMountItemId = mountItemId > 0;
    }
    if (skillId == 80001089 && hasMountItemId)
    {
        SkillOverlayBridgeObserveExtendedMountSoaringIntent(mountItemId, skillId);
    }
    const int shadowMountItemId =
        skillId == 80001089 && hasMountItemId &&
                !HasRecentMountedDoubleJumpIntent(mountItemId)
            ? ResolveExtendedMountNativeSoaringShadowMountItemId(mountItemId)
            : 0;
    const uintptr_t mountItemIdAddr = reinterpret_cast<uintptr_t>(thisPtr) + 0x454;
    bool patchedMountFamilyGate = false;

    if (shadowMountItemId > 0 &&
        mountItemId != shadowMountItemId &&
        !SafeIsBadReadPtr(reinterpret_cast<void *>(mountItemIdAddr), sizeof(DWORD)))
    {
        __try
        {
            *reinterpret_cast<int *>(mountItemIdAddr) = shadowMountItemId;
            patchedMountFamilyGate = true;
            static LONG s_mountNativeSoaringPatchLogBudget = 24;
            const LONG budgetAfterDecrement = InterlockedDecrement(&s_mountNativeSoaringPatchLogBudget);
            if (budgetAfterDecrement >= 0)
            {
                WriteLogFmt("[MountSoaringNative] B26290 shadow mount=%d -> %d skill=%d",
                            mountItemId,
                            shadowMountItemId,
                            skillId);
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            patchedMountFamilyGate = false;
            static LONG s_mountNativeSoaringPatchExceptionLogBudget = 8;
            const LONG budgetAfterDecrement =
                InterlockedDecrement(&s_mountNativeSoaringPatchExceptionLogBudget);
            if (budgetAfterDecrement >= 0)
            {
                WriteLogFmt("[MountSoaringNative] B26290 shadow write exception code=0x%08X mount=%d shadow=%d skill=%d",
                            GetExceptionCode(),
                            mountItemId,
                            shadowMountItemId,
                            skillId);
            }
        }
    }

    int result = 0;
    DWORD callExceptionCode = 0;
    __try
    {
        result = oMountNativeSoaringReleaseB26290
                     ? oMountNativeSoaringReleaseB26290(thisPtr, skillId)
                     : 0;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        callExceptionCode = GetExceptionCode();
        result = 0;
    }

    if (patchedMountFamilyGate &&
        !SafeIsBadReadPtr(reinterpret_cast<void *>(mountItemIdAddr), sizeof(DWORD)))
    {
        __try
        {
            *reinterpret_cast<int *>(mountItemIdAddr) = mountItemId;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
        }
    }

    if (callExceptionCode != 0)
    {
        static LONG s_mountNativeSoaringCallExceptionLogBudget = 8;
        const LONG budgetAfterDecrement =
            InterlockedDecrement(&s_mountNativeSoaringCallExceptionLogBudget);
        if (budgetAfterDecrement >= 0)
        {
            WriteLogFmt("[MountSoaringNative] B26290 call exception code=0x%08X mount=%d skill=%d shadow=%d",
                        callExceptionCode,
                        mountItemId,
                        skillId,
                        patchedMountFamilyGate ? shadowMountItemId : 0);
        }
    }

    if (skillId == 80001089 && hasMountItemId)
    {
        SkillOverlayBridgeObserveExtendedMountSoaringIntent(mountItemId, skillId);
        if (result > 0)
        {
            ObserveMountedSoaringFlightActive(mountItemId);
        }

        static LONG s_mountNativeSoaringResultLogBudget = 48;
        const LONG budgetAfterDecrement = InterlockedDecrement(&s_mountNativeSoaringResultLogBudget);
        if (budgetAfterDecrement >= 0)
        {
            WriteLogFmt("[MountSoaringNative] B26290 final mount=%d skill=%d shadow=%d result=%d",
                        mountItemId,
                        skillId,
                        patchedMountFamilyGate ? shadowMountItemId : 0,
                        result);
        }
    }

    return result;
}
