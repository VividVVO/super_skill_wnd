static bool SetupMountMovementAbilityFeatureHooks()
{
    bool anyOk = false;

    if (kEnableMountMovementAbilityRedHooks)
    {
        if (SetupAbilityRedMasterAggregateHook())
            anyOk = true;
    }
    else
    {
        WriteLog("[AbilityRedMaster] 856C60 hook disabled for mount movement rollback");
    }

    if (kEnableGlobalMovementSetterProtectionHooks)
    {
        if (SetupAbilityRedMovementSetterHooks())
            anyOk = true;
    }
    else
    {
        WriteLog("[MoveSetter] movement setter protection disabled");
    }

    if (kEnableGlobalMovementOutputClampHook)
    {
        if (SetupMovementOutputClampHook())
            anyOk = true;
    }
    else
    {
        WriteLog("[MoveClamp] movement output clamp hook disabled");
    }

    return anyOk;
}
