// 技能释放与展示模块：负责 release classifier、presentation override 和被动效果桥接。
static int __fastcall hkSkillEffect800260(void *thisPtr, void * /*edxUnused*/, int level)
{
    int result = 0;
    if (oSkillEffect800260)
    {
        __try
        {
            result = oSkillEffect800260(thisPtr, level);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            static DWORD s_lastSkillEffect800260ExceptionLogTick = 0;
            const DWORD nowTick = GetTickCount();
            if (nowTick - s_lastSkillEffect800260ExceptionLogTick > 1000)
            {
                s_lastSkillEffect800260ExceptionLogTick = nowTick;
                WriteLogFmt("[SuperPassiveEffectHook] 800260 EXCEPTION entry=0x%08X level=%d code=0x%08X",
                    (DWORD)(uintptr_t)thisPtr,
                    level,
                    GetExceptionCode());
            }
            result = 0;
        }
    }

    if (result > 0)
        SkillOverlayBridgeApplyConfiguredPassiveEffectBonuses((uintptr_t)thisPtr, level, (uintptr_t)result, "800260");
    return result;
}

static int __fastcall hkSkillEffect800580(void *thisPtr, void * /*edxUnused*/, int level)
{
    int result = 0;
    if (oSkillEffect800580)
    {
        __try
        {
            result = oSkillEffect800580(thisPtr, level);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            static DWORD s_lastSkillEffect800580ExceptionLogTick = 0;
            const DWORD nowTick = GetTickCount();
            if (nowTick - s_lastSkillEffect800580ExceptionLogTick > 1000)
            {
                s_lastSkillEffect800580ExceptionLogTick = nowTick;
                WriteLogFmt("[SuperPassiveEffectHook] 800580 EXCEPTION entry=0x%08X level=%d code=0x%08X",
                    (DWORD)(uintptr_t)thisPtr,
                    level,
                    GetExceptionCode());
            }
            result = 0;
        }
    }

    if (result > 0)
        SkillOverlayBridgeApplyConfiguredPassiveEffectBonuses((uintptr_t)thisPtr, level, (uintptr_t)result, "800580");
    return result;
}

static int __fastcall hkPassiveEffectDamage43DE00(void *thisPtr, void * /*edxUnused*/)
{
    int result = 0;
    if (oPassiveEffectDamage43DE00)
        result = oPassiveEffectDamage43DE00(thisPtr);
    return SkillOverlayBridgeOverridePassiveEffectGetterValue((uintptr_t)thisPtr, result, "damage", "43DE00");
}

static int __fastcall hkPassiveEffectDamage43DE50(void *thisPtr, void * /*edxUnused*/)
{
    int result = 0;
    if (oPassiveEffectDamage43DE50)
        result = oPassiveEffectDamage43DE50(thisPtr);
    return SkillOverlayBridgeOverridePassiveEffectGetterValue((uintptr_t)thisPtr, result, "damage", "43DE50");
}

static int __fastcall hkPassiveEffectAttackCount5E9EE0(void *thisPtr, void * /*edxUnused*/)
{
    int result = 0;
    if (oPassiveEffectAttackCount5E9EE0)
        result = oPassiveEffectAttackCount5E9EE0(thisPtr);
    return SkillOverlayBridgeOverridePassiveEffectGetterValue((uintptr_t)thisPtr, result, "attackCount", "5E9EE0");
}

static int __fastcall hkPassiveEffectMobCount7D1990(void *thisPtr, void * /*edxUnused*/)
{
    int result = 0;
    if (oPassiveEffectMobCount7D1990)
        result = oPassiveEffectMobCount7D1990(thisPtr);
    return SkillOverlayBridgeOverridePassiveEffectGetterValue((uintptr_t)thisPtr, result, "mobCount", "7D1990");
}

static int __fastcall hkPassiveEffectAttackCount7D19E0(void *thisPtr, void * /*edxUnused*/)
{
    int result = 0;
    if (oPassiveEffectAttackCount7D19E0)
        result = oPassiveEffectAttackCount7D19E0(thisPtr);
    return SkillOverlayBridgeOverridePassiveEffectGetterValue((uintptr_t)thisPtr, result, "attackCount", "7D19E0");
}

static int __fastcall hkPassiveEffectIgnore7D28E0(void *thisPtr, void * /*edxUnused*/)
{
    int result = 0;
    if (oPassiveEffectIgnore7D28E0)
        result = oPassiveEffectIgnore7D28E0(thisPtr);
    return SkillOverlayBridgeOverridePassiveEffectGetterValue((uintptr_t)thisPtr, result, "ignoreMobpdpR", "7D28E0");
}

__declspec(naked) static void hkSkillReleaseClassifierRootNaked()
{
    __asm {
        pushad
        push esi
        call hkSkillReleaseClassifierRootDispatch
        add esp, 4
        popad

        mov eax, dword ptr [g_ClassifierOverrideSkillId]
        test eax, eax
        je continue_original
        mov esi, eax
        mov dword ptr [g_ClassifierOverrideSkillId], 0

    continue_original:
        jmp [oSkillReleaseClassifierRoot]
    }
}

__declspec(naked) static void hkSkillReleaseClassifierNaked()
{
    __asm {
        pushad
        push esi
        call hkSkillReleaseClassifierDispatch
        add esp, 4
        popad

        mov eax, dword ptr [g_ForcedNativeReleaseJump]
        test eax, eax
        jne force_jump
        jmp [oSkillReleaseClassifier]

    force_jump:
        mov dword ptr [g_ForcedNativeReleaseJump], 0
        jmp eax
    }
}

// sub_B2F370 是独立函数入口：这里只改它的 arg0(skillId)，不做“跨函数 jump”。
__declspec(naked) static void hkSkillReleaseClassifierB2F370Naked()
{
    __asm {
        pushad
        mov ecx, [esp + 12] // pushad 保存的原始 ESP
        mov ecx, [ecx + 4] // arg0 = skillId
        push ecx
        call hkSkillReleaseClassifierB2F370Dispatch
        add esp, 4
        popad

        mov eax, dword ptr [g_BlockSkillReleaseB2F370]
        test eax, eax
        jne block_release

        mov eax, dword ptr [g_ClassifierOverrideSkillId]
        test eax, eax
        je continue_original
        mov dword ptr [esp + 4], eax
        mov dword ptr [g_ClassifierOverrideSkillId], 0

    continue_original:
        jmp [oSkillReleaseClassifierB2F370]

    block_release:
        mov dword ptr [g_BlockSkillReleaseB2F370], 0
        mov dword ptr [g_ClassifierOverrideSkillId], 0
        xor eax, eax
        ret 10h
    }
}

static void __cdecl LogEchoOfHeroPostReleaseNullContextGuard(int skillId)
{
    static LONG s_echoPostReleaseNullContextLogBudget = 24;
    if (InterlockedDecrement(&s_echoPostReleaseNullContextLogBudget) >= 0)
    {
        WriteLogFmt(
            "[SkillReleaseHook] B3356D echo null-context skip skillId=%d",
            skillId);
    }
}

__declspec(naked) static void hkEchoOfHeroPostReleaseContextGuardB3355CNaked()
{
    __asm {
        mov ecx, [esp + 64h]
        add esp, 10h

        cmp esi, 3EDh
        jne continue_original
        test ecx, ecx
        jne continue_original

        mov dword ptr [ebx + 4DE0h], 1
        pushad
        push esi
        call LogEchoOfHeroPostReleaseNullContextGuard
        add esp, 4
        popad
        jmp dword ptr [g_EchoOfHeroPostReleaseSkipB33587]

    continue_original:
        jmp dword ptr [g_EchoOfHeroPostReleaseContinueB33563]
    }
}

static void __fastcall hkSkillPresentationDispatch(void *thisPtr, void * /*edxUnused*/, int *skillData, int a3, int a4, int a5, int a6, int a7)
{
    int originalSkillId = 0;
    int desiredSkillId = 0;
    int mountedDemonJumpPresentationMountItemId = 0;
    int mountedDemonJumpPresentationChildSkillId = 0;
    int patchedFromSkillId = 0;
    bool patchedSkillId = false;
    bool patchedSkillDataPtr = false;
    bool keepOverrideAfterDispatch = false;
    bool keepMountedDemonJumpChildVisual = false;
    static const int kSkillDataScanSlots = 12;
    int patchedSlots[kSkillDataScanSlots] = {};
    int patchedSlotCount = 0;
    int *originalSkillDataPtr = skillData;

    if (skillData)
    {
        __try
        {
            originalSkillId = *skillData;
            if (IsMountedDemonJumpRelatedSkillId(originalSkillId))
            {
                static LONG s_mountedDemonJumpPresentationEnterLogBudget = 24;
                const LONG budgetAfterDecrement =
                    InterlockedDecrement(&s_mountedDemonJumpPresentationEnterLogBudget);
                if (budgetAfterDecrement >= 0)
                {
                    WriteLogFmt("[MountDemonJump] ABAF70 enter skill=%d ptr=0x%08X",
                                originalSkillId,
                                (DWORD)(uintptr_t)skillData);
                }
            }
            keepMountedDemonJumpChildVisual =
                IsMountedDemonJumpRuntimeChildSkillId(originalSkillId) &&
                TryResolveMountedDemonJumpActiveChildSkill(
                    originalSkillId,
                    2500,
                    &mountedDemonJumpPresentationMountItemId,
                    nullptr,
                    nullptr,
                    &mountedDemonJumpPresentationChildSkillId) &&
                mountedDemonJumpPresentationChildSkillId == originalSkillId;
            // WZ evidence from Skill.wz/3001.img.xml shows 30010186 already has
            // native effect frames and demonFly action. Keeping the child visual
            // is the correct path for mounted glide as well.
            if (keepMountedDemonJumpChildVisual)
            {
                static LONG s_mountedDemonJumpPresentationKeepChildLogBudget = 24;
                if (InterlockedDecrement(
                        &s_mountedDemonJumpPresentationKeepChildLogBudget) >= 0)
                {
                    WriteLogFmt(
                        "[MountDemonJump] ABAF70 keep child visual mount=%d skill=%d",
                        mountedDemonJumpPresentationMountItemId,
                        mountedDemonJumpPresentationChildSkillId);
                }
            }
            else if (desiredSkillId <= 0)
            {
                desiredSkillId =
                    SkillOverlayBridgeResolveNativePresentationDesiredSkillId(
                        originalSkillId);
            }
            if (desiredSkillId > 0)
            {
                const uintptr_t desiredSkillDataPtr = SkillOverlayBridgeLookupSkillEntryPointer(desiredSkillId);
                if (desiredSkillDataPtr && desiredSkillDataPtr != (uintptr_t)skillData)
                {
                    skillData = (int *)desiredSkillDataPtr;
                    patchedSkillDataPtr = true;
                    WriteLogFmt("[SkillVisual] swap skillData observed=%d desired=%d ptr=0x%08X->0x%08X via ABAF70",
                                originalSkillId,
                                desiredSkillId,
                                (DWORD)(uintptr_t)originalSkillDataPtr,
                                (DWORD)desiredSkillDataPtr);
                }
                else if (!desiredSkillDataPtr)
                {
                    static DWORD s_lastMissingSkillEntryLogTick = 0;
                    const DWORD nowTick = GetTickCount();
                    if (nowTick - s_lastMissingSkillEntryLogTick > 1000)
                    {
                        s_lastMissingSkillEntryLogTick = nowTick;
                        WriteLogFmt("[SkillVisual] WARN: desired skillEntry missing skillId=%d observed=%d via ABAF70",
                                    desiredSkillId,
                                    originalSkillId);
                    }
                }
            }

            if (skillData && desiredSkillId > 0)
            {
                patchedFromSkillId = *skillData;
                if (patchedFromSkillId != desiredSkillId)
                {
                    // 一些技能分支会在同一 skillData 结构中多处读取 skillId；统一替换可减少 donor 残留。
                    for (int i = 0; i < kSkillDataScanSlots; ++i)
                    {
                        if (skillData[i] == patchedFromSkillId)
                        {
                            skillData[i] = desiredSkillId;
                            patchedSlots[patchedSlotCount++] = i;
                        }
                    }

                    if (patchedSlotCount <= 0)
                    {
                        *skillData = desiredSkillId;
                        patchedSlots[patchedSlotCount++] = 0;
                    }

                    patchedSkillId = true;
                    keepOverrideAfterDispatch = SkillOverlayBridgeShouldKeepPresentationOverrideAfterDispatch(
                        originalSkillId,
                        desiredSkillId);
                    WriteLogFmt("[SkillVisual] override visual observed=%d visual=%d via ABAF70 patchedSlots=%d keep=%d ptrSwap=%d",
                                patchedFromSkillId,
                                desiredSkillId,
                                patchedSlotCount,
                                keepOverrideAfterDispatch ? 1 : 0,
                                patchedSkillDataPtr ? 1 : 0);
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            patchedSkillId = false;
        }
    }

    if (oSkillPresentationDispatch)
        oSkillPresentationDispatch(thisPtr, skillData, a3, a4, a5, a6, a7);

    // 始终恢复：skillData 指向的是游戏内部共享结构体，
    // 修改后不恢复会污染后续所有使用该 entry 的代码（包括原生技能释放的特效）。
    if (patchedSkillId && skillData)
    {
        __try
        {
            for (int i = 0; i < patchedSlotCount; ++i)
                skillData[patchedSlots[i]] = patchedFromSkillId;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
        }
    }
}

// ============================================================================
// 每帧刷新面板锚点（v8.0：只维护 SkillWnd 原生扩展层的屏幕坐标）
// ============================================================================
