static bool TryResolveMountedDemonJumpRequirementBypassContext(
    DWORD callerRet,
    int skillId,
    int *mountItemIdOut,
    int *rootSkillIdOut,
    int *currentSkillIdOut)
{
    if (mountItemIdOut)
    {
        *mountItemIdOut = 0;
    }
    if (rootSkillIdOut)
    {
        *rootSkillIdOut = 0;
    }
    if (currentSkillIdOut)
    {
        *currentSkillIdOut = 0;
    }

    if (callerRet != 0x00B29F1E ||
        !IsMountedDemonJumpRuntimeChildSkillId(skillId))
    {
        return false;
    }

    int mountItemId = 0;
    int rootSkillId = 0;
    int currentSkillId = 0;
    int childSkillId = 0;
    if (!TryResolveMountedDemonJumpActiveChildSkill(
            skillId,
            1200,
            &mountItemId,
            &rootSkillId,
            &currentSkillId,
            &childSkillId) ||
        mountItemId <= 0 ||
        rootSkillId != 30010110 ||
        childSkillId != skillId)
    {
        return false;
    }

    if (mountItemIdOut)
    {
        *mountItemIdOut = mountItemId;
    }
    if (rootSkillIdOut)
    {
        *rootSkillIdOut = rootSkillId;
    }
    if (currentSkillIdOut)
    {
        *currentSkillIdOut = currentSkillId;
    }
    return true;
}
