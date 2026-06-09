static bool TryResolveExtendedMountContextForSoaring(int *mountItemIdOut, bool *fromUserLocalOut)
{
    int mountItemId = 0;
    const char *mountSource = nullptr;
    if (!TryResolveCurrentUserMountItemIdWithFallback(&mountItemId, &mountSource) ||
        !IsExtendedMountSoaringContextMount(mountItemId))
    {
        return false;
    }

    const bool fromUserLocal = mountSource && strcmp(mountSource, "user") == 0;
    if (!fromUserLocal)
    {
        static LONG s_rejectFallbackContextLogBudget = 16;
        if (InterlockedDecrement(&s_rejectFallbackContextLogBudget) >= 0)
        {
            WriteLogFmt(
                "[MountFamilyGate] reject fallback soaring context mount=%d source=%s",
                mountItemId,
                mountSource ? mountSource : "unknown");
        }
        return false;
    }

    if (fromUserLocal)
    {
        ObserveExtendedMountContext(mountItemId);
    }
    if (mountItemIdOut)
    {
        *mountItemIdOut = mountItemId;
    }
    if (fromUserLocalOut)
    {
        *fromUserLocalOut = fromUserLocal;
    }
    return true;
}

static int __cdecl hkMountActionGate4069E0(int mountItemId)
{
    if (IsExtendedMountActionGateMount(mountItemId))
    {
        ObserveExtendedMountContext(mountItemId);
        static LONG s_mountActionGateLogBudget = 8;
        const LONG budgetAfterDecrement = InterlockedDecrement(&s_mountActionGateLogBudget);
        if (budgetAfterDecrement >= 0)
        {
            WriteLogFmt("[MountGate] 4069E0 extend mount=%d -> allow", mountItemId);
        }
        return 1;
    }

    return oMountActionGate4069E0
               ? oMountActionGate4069E0(mountItemId)
               : 0;
}

static int __cdecl hkMountActionGate406AB0(int mountItemId)
{
    const int result = oMountActionGate406AB0
                           ? oMountActionGate406AB0(mountItemId)
                           : 0;

    if (result <= 0 && IsExtendedMountActionGateMount(mountItemId))
    {
        ObserveExtendedMountContext(mountItemId);
        static LONG s_mountActionGate406AB0ForceAllowLogBudget = 24;
        const LONG budgetAfterDecrement = InterlockedDecrement(&s_mountActionGate406AB0ForceAllowLogBudget);
        if (budgetAfterDecrement >= 0)
        {
            WriteLogFmt("[MountGate] 406AB0 extend mount=%d native=%d -> allow",
                        mountItemId,
                        result);
        }
        return 1;
    }

    if (IsExtendedMountServerValidatedSoaringMount(mountItemId))
    {
        static LONG s_mountActionGate406AB0LogBudget = 12;
        const LONG budgetAfterDecrement = InterlockedDecrement(&s_mountActionGate406AB0LogBudget);
        if (budgetAfterDecrement >= 0)
        {
            WriteLogFmt("[MountGate] 406AB0 mount=%d -> %d", mountItemId, result);
        }
    }

    return result;
}
