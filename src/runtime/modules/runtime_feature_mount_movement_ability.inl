// 骑宠移动能力模块：负责 AbilityRed 移动保护、setter 防护和 output clamp 装配。
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
        WriteLog("[AbilityRedMaster] 856C60 hook disabled for player/mount movement rollback");
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
