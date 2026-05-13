// 骑宠移动运行时模块：负责移动观测、飞行速度 hook 和 movement cap patch 装配。
static bool SetupMountMovementRuntimeFeatureHooks()
{
    bool anyOk = false;

    if (kEnableMountMovementObservationHooks)
    {
        if (SetupMountMovementObservationHooks())
            anyOk = true;
    }
    else
    {
        WriteLog("[MountMoveObserve] observation hook disabled");
    }

    if (kEnableMountedFlightPhysicsSpeedHooks)
    {
        if (SetupMountedFlightPhysicsSpeedHooks())
            anyOk = true;
    }
    else
    {
        WriteLog("[MountFlightSpeed] physics speed hooks disabled");
    }

    if (kEnableMountMovementCapPatches)
    {
        if (ApplyMountMovementCapPatches())
            anyOk = true;
    }
    else
    {
        WriteLog("[RuntimePatch] mount movement cap patches disabled for rollback");
    }

    return anyOk;
}
