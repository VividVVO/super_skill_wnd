static bool IsMountedRuntimePlayerObjectCurrentUserLocal(void *playerObj)
{
    if (!playerObj)
    {
        return false;
    }

    void *currentUserLocal = nullptr;
    return TryReadCurrentUserLocalPtr(&currentUserLocal) &&
           currentUserLocal == playerObj;
}

static bool TryResolveMountedRuntimeSkillMountItemIdWithFallback(
    MountedRuntimeSkillKind kind,
    void *playerObj,
    int *mountItemIdOut,
    const char **sourceOut,
    DWORD maxAgeMs)
{
    if (mountItemIdOut)
    {
        *mountItemIdOut = 0;
    }
    if (sourceOut)
    {
        *sourceOut = nullptr;
    }

    int mountItemId = 0;
    const char *source = nullptr;
    bool playerObjIsCurrentUser = false;
    if (playerObj)
    {
        playerObjIsCurrentUser =
            IsMountedRuntimePlayerObjectCurrentUserLocal(playerObj);

        int playerMountItemId = 0;
        if (TryReadMountItemIdFromPlayerObjectRaw(playerObj, &playerMountItemId) &&
            playerMountItemId > 0)
        {
            mountItemId = playerMountItemId;
            source = "player";
        }
        else if (!playerObjIsCurrentUser)
        {
            // A non-local player object is authoritative for that actor. Do not
            // fall back to local/recent mount caches or party members can leak
            // their mount runtime into another character's skill release path.
            return false;
        }
    }

    if (!source)
    {
        if (TryReadCurrentUserMountItemId(&mountItemId))
        {
            source = "user";
        }
        else if (TryGetRecentMountedRuntimeRouteArmMountItemIdForKind(
                     kind,
                     &mountItemId,
                     maxAgeMs) &&
                 ResolveMountedRuntimeSkillIdForKind(kind, mountItemId) > 0)
        {
            source = "route-arm";
        }
        else if (TryGetRecentMountedRuntimeSkillIntentItemId(kind, &mountItemId, maxAgeMs) &&
                 ResolveMountedRuntimeSkillIdForKind(kind, mountItemId) > 0)
        {
            source = "intent";
        }
        else if (TryGetRecentMountedRuntimeSkillNativeReleaseItemId(kind, &mountItemId, maxAgeMs) &&
                 ResolveMountedRuntimeSkillIdForKind(kind, mountItemId) > 0)
        {
            source = "native-release";
        }
        else if (TryResolveCurrentUserMountItemIdWithFallback(&mountItemId, &source) &&
                 ResolveMountedRuntimeSkillIdForKind(kind, mountItemId) > 0)
        {
        }
        else
        {
            return false;
        }
    }

    if (source &&
        (!strcmp(source, "player") || !strcmp(source, "user")) &&
        IsExtendedMountSoaringContextMount(mountItemId))
    {
        ObserveExtendedMountContext(mountItemId);
    }

    if (mountItemIdOut)
    {
        *mountItemIdOut = mountItemId;
    }
    if (sourceOut)
    {
        *sourceOut = source;
    }
    return true;
}

static bool TryResolveMountedDoubleJumpMountItemIdWithFallback(
    void *playerObj,
    int *mountItemIdOut,
    const char **sourceOut,
    DWORD maxAgeMs)
{
    return TryResolveMountedRuntimeSkillMountItemIdWithFallback(
        MountedRuntimeSkillKind_DoubleJump,
        playerObj,
        mountItemIdOut,
        sourceOut,
        maxAgeMs);
}

static bool TryResolveMountedDemonJumpMountItemIdWithFallback(
    void *playerObj,
    int *mountItemIdOut,
    const char **sourceOut,
    DWORD maxAgeMs)
{
    return TryResolveMountedRuntimeSkillMountItemIdWithFallback(
        MountedRuntimeSkillKind_DemonJump,
        playerObj,
        mountItemIdOut,
        sourceOut,
        maxAgeMs);
}
