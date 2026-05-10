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
