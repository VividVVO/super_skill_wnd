static void *oSendPacket = nullptr;
static DWORD g_SendPacketOriginalCallTarget = 0;
static void *oRecvPacket = nullptr;
static void *oExternalPotentialWritePatch = nullptr;
static void *oExternalPotentialClearPatch = nullptr;
static uintptr_t g_ExternalPotentialIncreaseAddressRuntime = 0;
static DWORD g_ExternalPotentialWriteContinue = 0;
static DWORD g_ExternalPotentialWriteLoopTarget = 0;
static DWORD g_ExternalPotentialClearContinue = 0;
static void *oLocalIndependentPotentialPrimaryFlatStats = nullptr;
static DWORD g_LocalIndependentPotentialPrimaryContinueNonZero = 0;
static DWORD g_LocalIndependentPotentialPrimaryContinueZero = 0;
static void *oLocalIndependentPotentialPrimaryPercentStats = nullptr;
static DWORD g_LocalIndependentPotentialPrimaryPercentContinueNonZero = 0;
static DWORD g_LocalIndependentPotentialPrimaryPercentContinueZero = 0;
static void *oLocalIndependentPotentialFlatStats = nullptr;
static uintptr_t g_LocalIndependentPotentialPreparedPtr = 0;
static DWORD g_LocalIndependentPotentialContinueNonZero = 0;
static DWORD g_LocalIndependentPotentialContinueZero = 0;
static void *oLocalIndependentPotentialSkillLevelDisplay = nullptr;
static void *oLocalIndependentPotentialDamageDisplay = nullptr;
static void *oAbilityRedHashLookupHook = nullptr;
static void *oAbilityRedHashInsertHook = nullptr;
typedef int (__fastcall *tAbilityRedDisplayCandidateFn)(void *thisPtr, void *edxUnused, DWORD arg1, DWORD arg2, DWORD arg3, DWORD arg4, DWORD arg5, DWORD arg6, DWORD arg7);
static tAbilityRedDisplayCandidateFn oAbilityRedDisplayCandidateFn = nullptr;
typedef int (__fastcall *tAbilityRedExtendedAggregateFn)(void *thisPtr, void *edxUnused, DWORD arg1, DWORD arg2, DWORD arg3);
static tAbilityRedExtendedAggregateFn oAbilityRedExtendedAggregateFn = nullptr;
typedef int (__fastcall *tAbilityRedMasterAggregateFn)(void *thisPtr, void *edxUnused, DWORD arg1, DWORD arg2, DWORD arg3, DWORD arg4, DWORD arg5, DWORD arg6, DWORD arg7);
static tAbilityRedMasterAggregateFn oAbilityRedMasterAggregateFn = nullptr;
typedef int (__cdecl *tAbilityRedMovementSpeedSourceFn)(int playerObj);
typedef int (__cdecl *tAbilityRedMovementSpeedCapBaseFn)(int playerObj);
typedef int (__thiscall *tAbilityRedMovementCapOverrideFn)(void *capObj);
typedef int (__thiscall *tAbilityRedMovementValueFn)(void *thisPtr);
typedef int (__fastcall *tAbilityRedMovementSetterFn)(void *thisPtr, int value);
typedef int (__thiscall *tMovementSpeedTransformFn)(void *thisPtr, int value);
typedef LONG (__cdecl *tMovementOutputClampComputeFn)(
    DWORD *a1,
    int a2,
    int a3,
    int a4,
    int a5,
    int a6,
    int a7,
    int *a8,
    int *a9,
    double *a10,
    DWORD *a11);
static tAbilityRedMovementSetterFn oAbilityRedMovementSpeedSetter831F00Fn = nullptr;
static tAbilityRedMovementSetterFn oAbilityRedMovementJumpSetter832000Fn = nullptr;
static tMovementOutputClampComputeFn oMovementOutputClampComputeB93B80Fn = nullptr;
typedef int (__fastcall *tAbilityRedSiblingCalcFn)(void *thisPtr, void *edxUnused);
static tAbilityRedSiblingCalcFn oAbilityRedSiblingCalc82F780Fn = nullptr;
static tAbilityRedSiblingCalcFn oAbilityRedSiblingCalc82F870Fn = nullptr;
static tAbilityRedSiblingCalcFn oAbilityRedSiblingCalc82F960Fn = nullptr;
static tAbilityRedSiblingCalcFn oAbilityRedSiblingCalc82FA50Fn = nullptr;
typedef int (__fastcall *tAbilityRedFinalCalc7Fn)(void *thisPtr, void *edxUnused, DWORD arg1, DWORD arg2, DWORD arg3, DWORD arg4, DWORD arg5, DWORD arg6, DWORD arg7);
typedef int (__fastcall *tAbilityRedFinalCalc6Fn)(void *thisPtr, void *edxUnused, DWORD arg1, DWORD arg2, DWORD arg3, DWORD arg4, DWORD arg5, DWORD arg6);
typedef int (__fastcall *tAbilityRedFinalCalc5Fn)(void *thisPtr, void *edxUnused, DWORD arg1, DWORD arg2, DWORD arg3, DWORD arg4, DWORD arg5);
static tAbilityRedFinalCalc7Fn oAbilityRedFinalCalc84BE40Fn = nullptr;
static tAbilityRedFinalCalc6Fn oAbilityRedFinalCalc84C470Fn = nullptr;
static tAbilityRedFinalCalc5Fn oAbilityRedFinalCalc84CA90Fn = nullptr;
static tAbilityRedFinalCalc5Fn oAbilityRedFinalCalc84CBD0Fn = nullptr;
static DWORD g_AbilityRedDisplayCallsiteOriginalTarget = 0;
static DWORD g_AbilityRedDiff84C470PreSubContinue = 0;
static DWORD g_AbilityRedDiff84BE40PreSubContinue = 0;
static DWORD g_AbilityRedDiff84CA90AccPreSubContinue = 0;
static DWORD g_AbilityRedDiff84CA90MagicAccPreSubContinue = 0;
static DWORD g_AbilityRedDiff84CBD0AvoidPreSubContinue = 0;
static DWORD g_AbilityRedDiff84CBD0MagicAvoidPreSubContinue = 0;
static DWORD g_AbilityRedAttackRangeStyleContinue = 0;
static DWORD g_AbilityRedCriticalRateStyleContinue = 0;
static DWORD g_AbilityRedSpeedStyleContinue = 0;
static DWORD g_AbilityRedJumpStyleContinue = 0;
static void *oAbilityRedBake857BB6Hook = nullptr;
static void *oAbilityRedBake857C29Hook = nullptr;
static void *oAbilityRedBake857C9CHook = nullptr;
static void *oAbilityRedBake857D0FHook = nullptr;
static void *oAbilityRedBake1988569C3Hook = nullptr;
static void *oAbilityRedBake198856D57Hook = nullptr;
static void *oAbilityRedBake19885725FHook = nullptr;
static void *oAbilityRedBake198857C3BHook = nullptr;
static void *oAbilityRedBake198858AEDHook = nullptr;
static void *oAbilityRedBake198831A50Hook = nullptr;
static void *oAbilityRedBake19883AF02Hook = nullptr;
static void *oAbilityRedLevelReadHook = nullptr;
static void *oAbilityRedSkillWrite52FE14Hook = nullptr;
static void *oAbilityRedSkillWrite6226CEHook = nullptr;
static void *oAbilityRedSkillWrite49CA01Hook = nullptr;
typedef int (__stdcall *tLocalIndependentPotentialSkillLevelDisplayFn)(int a1, int a2, DWORD *a3);
typedef LONG (__cdecl *tLocalIndependentPotentialPercentQuadDisplayFn)(int a1, int a2, DWORD *a3, DWORD *a4, DWORD *a5, DWORD *a6);
typedef LONG (__thiscall *tLocalIndependentPotentialPercentFullDisplayFn)(DWORD *thisPtr, int pExceptionObject, int a3, DWORD *a4);
typedef LONG (__thiscall *tLocalIndependentPotentialFlatBasicDisplayFn)(DWORD *thisPtr, int pExceptionObject, int a3);
typedef LONG (__thiscall *tLocalIndependentPotentialFlatExtendedDisplayFn)(DWORD *thisPtr, int pExceptionObject, int a3);
static tLocalIndependentPotentialSkillLevelDisplayFn oLocalIndependentPotentialSkillLevelDisplayFn = nullptr;
static tLocalIndependentPotentialPercentQuadDisplayFn oLocalIndependentPotentialPercentQuadDisplayFn = nullptr;
static tLocalIndependentPotentialPercentFullDisplayFn oLocalIndependentPotentialPercentFullDisplayFn = nullptr;
static tLocalIndependentPotentialFlatBasicDisplayFn oLocalIndependentPotentialFlatBasicDisplayFn = nullptr;
static tLocalIndependentPotentialFlatExtendedDisplayFn oLocalIndependentPotentialFlatExtendedDisplayFn = nullptr;
typedef BYTE* (__stdcall *tPotentialTextFormatFn)(int* src, BYTE* statsPtr);
static tPotentialTextFormatFn oPotentialTextFormat = nullptr;
static void *oSkillReleaseClassifierRoot = nullptr;
static void *oSkillReleaseClassifier = nullptr;
static void *oSkillReleaseClassifierB2F370 = nullptr;
static void *oEchoOfHeroPostReleaseContextGuardB3355C = nullptr;
static DWORD g_EchoOfHeroPostReleaseContinueB33563 = ADDR_B33563;
static DWORD g_EchoOfHeroPostReleaseSkipB33587 = ADDR_B33587;
typedef BOOL(__cdecl *tSkillNativeIdGateFn)(int skillId);
static tSkillNativeIdGateFn oSkillNativeIdGate7CE790 = nullptr;
static tSkillNativeIdGateFn oSkillNativeIdGate7D0000 = nullptr;
static tSkillNativeIdGateFn oMountedSkillWhitelist7CF270 = nullptr;
typedef int(__thiscall *tMountedStateGateFn)(void *thisPtr);
static tMountedStateGateFn oMountedStateGate42DE20 = nullptr;
typedef int(__thiscall *tMountedUseFailPromptFn)(void *thisPtr, int a2);
static tMountedUseFailPromptFn oMountedUseFailPromptAE6260 = nullptr;
typedef int(__thiscall *tMountedDemonJumpContextClearFn)(void *contextPtr);
static tMountedDemonJumpContextClearFn oMountedDemonJumpContextClear433380 = nullptr;
typedef void(__thiscall *tMountedSkillPacketDispatchFn)(void *thisPtr, int skillId, char level);
static tMountedSkillPacketDispatchFn oMountedSkillPacketDispatchB26760 = nullptr;
typedef int(__thiscall *tMountedSkillAttackPacketFn)(
    void *thisPtr,
    int *skillIdPtr,
    int a3,
    int a4,
    int a5,
    int a6,
    unsigned int a7,
    int a8);
static tMountedSkillAttackPacketFn oMountedSkillAttackPacketB28A00 = nullptr;
typedef int(__cdecl *tMountActionGateFn)(int mountItemId);
static tMountActionGateFn oMountActionGate4069E0 = nullptr;
static tMountActionGateFn oMountActionGate406AB0 = nullptr;
typedef int(__cdecl *tMountNativeFlightSkillMapFn)(int mountItemId);
static tMountNativeFlightSkillMapFn oMountNativeFlightSkillMap7CF370 = nullptr;
typedef int(__thiscall *tMountedSkillContextGateFn)(void *mountContext);
static tMountedSkillContextGateFn oMountedSkillContextGateA9BF40 = nullptr;
static DWORD g_MountedSkillContextGateCallsiteOriginalTarget = 0;
static DWORD g_MountedUnknownSkillReleaseBranchOriginalTarget = 0;
typedef void(__thiscall *tMountedDemonJumpContextPrimeFn)(
    void *userLocal,
    unsigned int **skillEntryOrNull);
typedef int(__thiscall *tMountedDemonJumpContextCurrentSkillFn)(void *contextPtr);
typedef signed int(__stdcall *tMountedDemonJumpSkillEntryLookupFn)(
    int skillId,
    unsigned int **skillEntryOut);
typedef void(__thiscall *tMountedDemonJumpContextSeedFn)(
    void *userLocal,
    int a2,
    int a3,
    int a4,
    int a5,
    int a6,
    int a7,
    int a8,
    int a9,
    int a10,
    int a11,
    int a12);
static const DWORD ADDR_MountedDemonJumpContextPrimeB00AD0 = 0x00B00AD0;
static const DWORD ADDR_MountedDemonJumpContextCurrentSkill4300A0 = 0x004300A0;
static const DWORD ADDR_MountedDemonJumpSkillEntryLookupAE0420 = 0x00AE0420;
static const DWORD ADDR_MountedDemonJumpContextSeedAC6B00 = 0x00AC6B00;
static const DWORD ADDR_MountedDemonJumpContextClear433380 = 0x00433380;
static const DWORD ADDR_MountedDemonJumpContextMountedClearReturn433F48 = 0x00433F48;
typedef int(__thiscall *tMountedDemonJumpLateRouteFn)(
    void *thisPtr,
    int a2,
    int a3);
typedef void(__thiscall *tMountedDemonJumpLateVoidRouteFn)(
    void *thisPtr,
    int a2,
    int a3);
typedef void(__thiscall *tMountedDemonJumpLateNoArgFn)(void *thisPtr);
typedef int(__thiscall *tMountedDemonJumpLateBranchFn)(
    void *thisPtr,
    int a2);
typedef UINT(__thiscall *tMountedDemonJumpContextInputFn)(
    void *thisPtr,
    UINT a2,
    int a3);
typedef BOOL(__thiscall *tMountedDemonJumpLateFilterFn)(
    void *thisPtr,
    int a2,
    int a3);
typedef void(__stdcall *tMountedDemonJumpLateTickFn)(
    unsigned int a1,
    int a2);
typedef void(__stdcall *tMountedDemonJumpLatePrimeFn)(void *a1);
typedef int(__thiscall *tMountedDemonJumpLateKeyStateFn)(
    void *thisPtr,
    unsigned int keyCode,
    unsigned int active);
typedef int(__thiscall *tMountedDemonJumpLateStateGetterFn)(void *thisPtr);
typedef void *(__thiscall *tMountedDemonJumpLateLookupFn)(
    void *thisPtr,
    int a2,
    int a3,
    int a4);
typedef unsigned int *(__thiscall *tMountedDemonJumpNativeActionResetFn)(
    char *thisPtr,
    int a2);
typedef int(__thiscall *tMountedDemonJumpCarrierResetFn)(
    int *thisPtr);
static tMountedDemonJumpLateRouteFn oMountedDemonJumpLateRoute575D60 = nullptr;
static tMountedDemonJumpLateTickFn oMountedDemonJumpLateTick576020 = nullptr;
static tMountedDemonJumpContextInputFn oMountedDemonJumpContextInputB22630 = nullptr;
static tMountedDemonJumpLateVoidRouteFn oMountedDemonJumpMoveB1DB10 = nullptr;
static tMountedDemonJumpLateVoidRouteFn oMountedDemonJumpMoveB1C9E0 = nullptr;
static tMountedDemonJumpLatePrimeFn oMountedDemonJumpPrimeAE8F70 = nullptr;
static tMountedDemonJumpLateNoArgFn oMountedDemonJumpUpActionAFB710 = nullptr;
static tMountedDemonJumpLateStateGetterFn oMountedDemonJumpAfbState42E170 = nullptr;
static tMountedDemonJumpLateStateGetterFn oMountedDemonJumpAfbGateADB240 = nullptr;
static tMountedDemonJumpLateStateGetterFn oMountedDemonJumpAfbGateAD9500 = nullptr;
static tMountedDemonJumpLateLookupFn oMountedDemonJumpAfbLookup773500 = nullptr;
static tMountedDemonJumpLateBranchFn oMountedDemonJumpBranchADEDA0 = nullptr;
static tMountedDemonJumpLateFilterFn oMountedDemonJumpFilterBDBFD0 = nullptr;
static tMountedDemonJumpLateKeyStateFn oMountedDemonJumpKeyState7BECF0 = nullptr;
static const DWORD ADDR_MountedDemonJumpLateRoute575D60 = 0x00575D60;
static const DWORD ADDR_MountedDemonJumpLateTick576020 = 0x00576020;
static const DWORD ADDR_MountedDemonJumpContextInputB22630 = 0x00B22630;
static const DWORD ADDR_MountedDemonJumpMoveB1DB10 = 0x00B1DB10;
static const DWORD ADDR_MountedDemonJumpMoveB1C9E0 = 0x00B1C9E0;
static const DWORD ADDR_MountedDemonJumpPrimeAE8F70 = 0x00AE8F70;
static const DWORD ADDR_MountedDemonJumpUpActionAFB710 = 0x00AFB710;
static const DWORD ADDR_MountedDemonJumpAfbState42E170 = 0x0042E170;
static const DWORD ADDR_MountedDemonJumpNativeActionReset47F1C0 = 0x0047F1C0;
static const DWORD ADDR_MountedDemonJumpCarrierResetB22AE0 = 0x00B22AE0;
static const DWORD ADDR_MountedDemonJumpAfbGateADB240 = 0x00ADB240;
static const DWORD ADDR_MountedDemonJumpAfbGateAD9500 = 0x00AD9500;
static const DWORD ADDR_MountedDemonJumpAfbLookup773500 = 0x00773500;
static const DWORD ADDR_MountedDemonJumpBranchADEDA0 = 0x00ADEDA0;
static const DWORD ADDR_MountedDemonJumpFilterBDBFD0 = 0x00BDBFD0;
static const DWORD ADDR_MountedDemonJumpKeyState7BECF0 = 0x007BECF0;
static const DWORD ADDR_MountedSkillPacketDispatchB26760 = 0x00B26760;
static const DWORD ADDR_MountedSkillAttackPacketB28A00 = 0x00B28A00;
static const size_t kMountedDemonJumpContextOffset = 24200;
static const size_t kMountedDemonJumpContextRootSkillOffset = 24204;
static const size_t kMountedDemonJumpReadyFlagOffset = 24197;
typedef int(__thiscall *tMountedCrashTrace2ArgFn)(void *thisPtr, DWORD arg1, DWORD arg2);
typedef int(__thiscall *tMountedCrashTraceNoArgFn)(void *thisPtr);
typedef int(__thiscall *tMountedCrashTrace1ArgFn)(void *thisPtr, int arg1);
typedef int(__thiscall *tMountedCrashTrace3ArgFn)(void *thisPtr, DWORD arg1, DWORD arg2, DWORD arg3);
typedef int(__thiscall *tMountedCrashTrace5ArgFn)(
    void *thisPtr,
    DWORD arg1,
    DWORD arg2,
    DWORD arg3,
    DWORD arg4,
    DWORD arg5);
typedef int(__stdcall *tMountedCrashTracePromptReasonFn)(int arg1, int arg2);
typedef int(__thiscall *tMountedCrashTrace6ArgFn)(
    void *thisPtr,
    DWORD arg1,
    DWORD arg2,
    DWORD arg3,
    DWORD arg4,
    DWORD arg5,
    DWORD arg6);
typedef int(__cdecl *tMountedCrashTraceCdecl1ArgFn)(int arg1);
typedef int(__thiscall *tMountedDemonJumpRequirementFn)(void *thisPtr, int skillId);
static tMountedCrashTrace2ArgFn oMountedDemonJumpTrace8057F0 = nullptr;
static tMountedCrashTraceCdecl1ArgFn oMountedDemonJumpTrace550FF0 = nullptr;
static tMountedCrashTraceNoArgFn oMountedDemonJumpTrace829EC0 = nullptr;
static tMountedCrashTraceNoArgFn oMountedDemonJumpTrace829F10 = nullptr;
static tMountedCrashTraceNoArgFn oMountedDemonJumpTrace551170 = nullptr;
static tMountedCrashTrace1ArgFn oMountedDemonJumpTraceA01BF0 = nullptr;
static tMountedCrashTraceNoArgFn oMountedDemonJumpTrace4C1720 = nullptr;
static tMountedCrashTraceNoArgFn oMountedDemonJumpTrace52BCB0 = nullptr;
static tMountedCrashTrace6ArgFn oMountedDemonJumpTrace805850 = nullptr;
static tMountedCrashTraceCdecl1ArgFn oMountedDemonJumpActionGate7DC870 = nullptr;
static tMountedCrashTraceCdecl1ArgFn oMountedDemonJumpActionGate7DC810 = nullptr;
static tMountedCrashTraceCdecl1ArgFn oMountedDemonJumpActionGate7DC7B0 = nullptr;
static tMountedCrashTraceCdecl1ArgFn oMountedDemonJumpActionGate7DC8D0 = nullptr;
static tMountedCrashTraceCdecl1ArgFn oMountedDemonJumpActionGate7DC710 = nullptr;
static tMountedCrashTraceCdecl1ArgFn oMountedDemonJumpActionGate7DC900 = nullptr;
static tMountedCrashTraceCdecl1ArgFn oMountedDemonJumpActionGate7CF840 = nullptr;
static tMountedCrashTraceCdecl1ArgFn oMountedDemonJumpActionGate7DC750 = nullptr;
static tMountedCrashTraceCdecl1ArgFn oMountedDemonJumpActionGate7DC8A0 = nullptr;
static tMountedCrashTraceCdecl1ArgFn oMountedDemonJumpActionKind7CE210 = nullptr;
static tMountedCrashTraceCdecl1ArgFn oMountedDemonJumpActionUsable7DAAF0 = nullptr;
static tMountedCrashTrace2ArgFn oMountedDemonJumpActionJobGate7D7D20 = nullptr;
static tMountedCrashTraceCdecl1ArgFn oMountedDemonJumpActionBattlegroundSkillGate4E1D30 = nullptr;
static tMountedCrashTraceNoArgFn oMountedDemonJumpActionAbilityGateAE5870 = nullptr;
static tMountedCrashTracePromptReasonFn oMountedDemonJumpActionPromptReason42DAF0 = nullptr;
static tMountedCrashTrace5ArgFn oMountedDemonJumpActionPrepareB273B0 = nullptr;
static tMountedCrashTraceNoArgFn oMountedDemonJumpActionGateA9B710 = nullptr;
static tMountedCrashTraceCdecl1ArgFn oMountedDemonJumpActionKind52BAD0 = nullptr;
static tMountedCrashTrace3ArgFn oMountedDemonJumpActionRouteB29C70 = nullptr;
static tMountedCrashTrace2ArgFn oMountedDemonJumpActionRouteB24010 = nullptr;
static tMountedCrashTrace2ArgFn oMountedDemonJumpActionRouteB24EA0 = nullptr;
static tMountedCrashTrace1ArgFn oMountedDemonJumpActionRouteB26550 = nullptr;
static tMountedCrashTrace1ArgFn oMountedDemonJumpActionRouteB26050 = nullptr;
static tMountedDemonJumpRequirementFn oMountedDemonJumpRequirementBF65C0 = nullptr;
static const DWORD ADDR_MountedDemonJumpActionGate7DC870 = 0x007DC870;
static const DWORD ADDR_MountedDemonJumpActionGate7DC810 = 0x007DC810;
static const DWORD ADDR_MountedDemonJumpActionGate7DC7B0 = 0x007DC7B0;
static const DWORD ADDR_MountedDemonJumpActionGate7DC8D0 = 0x007DC8D0;
static const DWORD ADDR_MountedDemonJumpActionGate7DC710 = 0x007DC710;
static const DWORD ADDR_MountedDemonJumpActionGate7DC900 = 0x007DC900;
static const DWORD ADDR_MountedDemonJumpActionGate7CF840 = 0x007CF840;
static const DWORD ADDR_MountedDemonJumpActionGate7DC750 = 0x007DC750;
static const DWORD ADDR_MountedDemonJumpActionGate7DC8A0 = 0x007DC8A0;
static const DWORD ADDR_MountedDemonJumpActionKind7CE210 = 0x007CE210;
static const DWORD ADDR_MountedDemonJumpActionUsable7DAAF0 = 0x007DAAF0;
static const DWORD ADDR_MountedDemonJumpActionJobGate7D7D20 = 0x007D7D20;
static const DWORD ADDR_MountedDemonJumpActionBattlegroundSkillGate4E1D30 = 0x004E1D30;
static const DWORD ADDR_MountedDemonJumpActionAbilityGateAE5870 = 0x00AE5870;
static const DWORD ADDR_MountedDemonJumpActionPromptReason42DAF0 = 0x0042DAF0;
static const DWORD ADDR_MountedDemonJumpActionPrepareB273B0 = 0x00B273B0;
static const DWORD ADDR_MountedDemonJumpActionGateA9B710 = 0x00A9B710;
static const DWORD ADDR_MountedDemonJumpActionKind52BAD0 = 0x0052BAD0;
static const DWORD ADDR_MountedDemonJumpActionRouteB29C70 = 0x00B29C70;
static const DWORD ADDR_MountedDemonJumpActionRouteB24010 = 0x00B24010;
static const DWORD ADDR_MountedDemonJumpActionRouteB24EA0 = 0x00B24EA0;
static const DWORD ADDR_MountedDemonJumpActionRouteB26550 = 0x00B26550;
static const DWORD ADDR_MountedDemonJumpActionRouteB26050 = 0x00B26050;
static const DWORD ADDR_MountedDemonJumpRequirementBF65C0 = 0x00BF65C0;
static volatile LONG g_MountedDemonJumpActionPrepareTraceDepth = 0;
typedef int(__thiscall *tMountSoaringGateFn)(void *thisPtr, int levelContext, void *mountContext, int skillId, unsigned int **skillEntryOut);
static tMountSoaringGateFn oMountSoaringGate7DC1B0 = nullptr;
typedef int(__thiscall *tMountNativeSoaringReleaseFn)(void *thisPtr, int skillId);
static tMountNativeSoaringReleaseFn oMountNativeSoaringReleaseB26290 = nullptr;
typedef int(__thiscall *tMountFamilyGateFn)(void *mountContext);
static tMountFamilyGateFn oMountFamilyGateA9AAA0 = nullptr;
typedef int(__thiscall *tMountContextGetItemIdFn)(void *mountContext);
typedef BOOL(__thiscall *tMountContextIsFlyingFamilyFn)(void *mountContext);
static tMountContextIsFlyingFamilyFn oMountContextIsFlyingFamily7D4CD0 = nullptr;
typedef void *(__thiscall *tMountItemInfoLookupFn)(void *thisPtr, int mountItemId);
typedef int(__thiscall *tMountItemInfoDataKeyFn)(void *thisPtr);
typedef int(__cdecl *tMountMovementDataLookupFn)(int dataKey);
static tMountMovementDataLookupFn oMountMovementDataLookup888B30 = nullptr;
typedef __int16 (__thiscall *tEncodedDoubleWriteFn)(void *slot, int lowDword, int highDword);
typedef double (__thiscall *tEncodedDoubleReadFn)(void *slot);
typedef int (__thiscall *tMountedFlightPhysicsDispatchFn)(void *thisPtr, int deltaMs);
typedef void (__thiscall *tMountedFlightPhysicsStepFn)(void *thisPtr, int deltaMs);
typedef int (__thiscall *tMountedFlightPhysicsStateFn)(void *thisPtr);
typedef int (__thiscall *tMountedFlightPhysicsVerticalFn)(void *thisPtr);
typedef int (__thiscall *tMountedFlightPhysicsFinalizeFn)(void *thisPtr, int deltaMs);
static tMountedFlightPhysicsDispatchFn oMountedFlightPhysicsDispatchB87E60 = nullptr;
static tMountedFlightPhysicsStepFn oMountedFlightPhysicsStepB83C90 = nullptr;
static tMountedFlightPhysicsStepFn oMountedFlightPhysicsStepB844D0 = nullptr;
static tMountedFlightPhysicsStepFn oMountedFlightPhysicsStepB88090 = nullptr;
static tMountedFlightPhysicsStateFn oMountedFlightPhysicsStateB84D70 = nullptr;
static tMountedFlightPhysicsVerticalFn oMountedFlightPhysicsVerticalB8FE30 = nullptr;
static tMountedFlightPhysicsVerticalFn oMountedFlightPhysicsVerticalB92990 = nullptr;
static tMountedFlightPhysicsFinalizeFn oMountedFlightPhysicsFinalizeB851F0 = nullptr;
typedef int(__thiscall *tNativeGlyphLookupFn)(void *fontCache, unsigned int codepoint, RECT *outRectOrNull);
static tNativeGlyphLookupFn oNativeGlyphLookup = nullptr;
typedef int(__thiscall *tSkillLevelBaseFn)(void *thisPtr, DWORD playerObj, int skillId, void *cachePtr);
typedef int(__thiscall *tSkillLevelCurrentFn)(void *thisPtr, DWORD playerObj, int skillId, void *cachePtr, int flags);
static tSkillLevelBaseFn oSkillLevelBase = nullptr;
static tSkillLevelCurrentFn oSkillLevelCurrent = nullptr;
typedef int(__thiscall *tSkillEffectFn)(void *thisPtr, int level);
static tSkillEffectFn oSkillEffect800260 = nullptr;
static tSkillEffectFn oSkillEffect800580 = nullptr;
typedef int(__thiscall *tPassiveEffectGetterFn)(void *effectPtr);
static tPassiveEffectGetterFn oPassiveEffectDamage43DE00 = nullptr;
static tPassiveEffectGetterFn oPassiveEffectDamage43DE50 = nullptr;
static tPassiveEffectGetterFn oPassiveEffectAttackCount5E9EE0 = nullptr;
static tPassiveEffectGetterFn oPassiveEffectMobCount7D1990 = nullptr;
static tPassiveEffectGetterFn oPassiveEffectAttackCount7D19E0 = nullptr;
static tPassiveEffectGetterFn oPassiveEffectIgnore7D28E0 = nullptr;
typedef void(__thiscall *tSkillPresentationDispatch)(void *thisPtr, int *skillData, int a3, int a4, int a5, int a6, int a7);
static tSkillPresentationDispatch oSkillPresentationDispatch = nullptr;
typedef void(__thiscall *tStatusBarInternalRefreshFn)(uintptr_t thisPtr);
static tStatusBarInternalRefreshFn oStatusBarRefreshSlotsPrimary = nullptr;
static tStatusBarInternalRefreshFn oStatusBarRefreshSlotsSecondary = nullptr;
static tStatusBarInternalRefreshFn oStatusBarRefreshInternal = nullptr;
static tStatusBarInternalRefreshFn oStatusBarCleanupTransient = nullptr;
typedef int(__thiscall *tSurfaceDrawImageFn)(void *surface, int x, int y, int imageObj, DWORD *variantLikeAlpha);
static tSurfaceDrawImageFn oSurfaceDrawImageFn = nullptr;
typedef char(__thiscall *tNativeCursorStateSetFn)(uintptr_t thisPtr, unsigned int state);
static tNativeCursorStateSetFn oNativeCursorStateSetFn = nullptr;
typedef int(__thiscall *tStatusBarTransientRefreshFn)(uintptr_t thisPtr, int a2);
typedef void(__thiscall *tStatusBarTransientDispatchFn)(uintptr_t thisPtr, int a2);
typedef LONG* (__stdcall *tStatusBarTransientToggleFn)(int a1);
static tStatusBarTransientRefreshFn oStatusBarTransientRefresh = nullptr;
static tStatusBarTransientDispatchFn oStatusBarTransientDispatch = nullptr;
static tStatusBarTransientToggleFn oStatusBarTransientToggle = nullptr;
struct StatusBarObservedBuffSlot
{
    uintptr_t wrapper = 0;
    uintptr_t child = 0;
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
    int renderX = 0;
    int renderY = 0;
};
static StatusBarObservedBuffSlot g_StatusBarObservedBuffSlots[9] = {};
static bool g_StatusBarBuffSlotHooksInstalled = false;
static volatile DWORD g_ClassifierOverrideSkillId = 0;
static volatile DWORD g_ForcedNativeReleaseJump = 0;
static void hkExternalPotentialWriteNaked();
static void hkExternalPotentialClearNaked();
static bool PatchExternalPotentialIncreaseStub(BYTE* stubTarget);
static bool SetupLocalIndependentPotentialPrimaryFlatStatHook();
static bool SetupLocalIndependentPotentialPrimaryPercentStatHook();
static bool SetupLocalIndependentPotentialFlatStatHook();
static bool SetupLocalIndependentPotentialDisplayFunctionHooks();
static bool SetupAbilityRedHashContainerHooks();
static bool SetupAbilityRedDisplayCandidateHook();
static bool SetupAbilityRedDisplayCallsiteHook();
static bool SetupAbilityRedLevelReadHook();
static bool SetupAbilityRedSkillWriteHooks();
static bool SetupAbilityRedExtendedAggregateHook();
static bool SetupAbilityRedMasterAggregateHook();
static bool SetupAbilityRedMovementSetterHooks();
static bool SetupMovementOutputClampHook();
static bool SetupAbilityRedSiblingCalcHooks();
static bool SetupAbilityRedDiff84C470PreSubHook();
static bool SetupAbilityRedAdditionalDiffHooks();
static bool SetupAbilityRedPositiveStyleHooks();
static bool SetupAbilityRedBakeWriteHooks();
static bool SetupAbilityRedBake198Hooks();
static bool SetupAbilityRedFinalValueHooks();
static bool SetupLocalIndependentPotentialSkillLevelDisplayHook();
static bool SetupLocalIndependentPotentialDamageDisplayHook();
static bool SetupPotentialTextDisplayHook();
static bool SetupStatusBarBuffSlotHooks();
static bool SetupSurfaceDrawImageObservationHook();
static bool SetupNativeCursorStateHook();
static bool SetupSkillEffectPassiveBonusHooks();
static bool SetupIndependentBuffLocalRuntimeHooks();
static bool SetupUiObservationRuntimeHooks();
static bool SetupMovementAbilityFeatureHooks();
static bool SetupPassiveEffectFeatureHooks();
static bool SetupMountedUnknownSkillReleaseBranchHook();
static bool SetupMountedUseFailPromptSuppressHook();
static bool SetupMountMovementObservationHooks();
static bool SetupMountedFlightPhysicsSpeedHooks();
static bool SetupMountedDemonJumpCrashTraceHooks();
static bool SetupMountedDemonJumpPacketObserveHooks();
static bool SetupMountedDemonJumpLatePathHooks();
static bool SetupMountedDemonJumpActionTraceHooks();
static bool SetupMountedDemonJumpRequirementBypassHook();
static bool SetupMountMovementAbilityFeatureHooks();
static bool SetupMountedRuntimeFeatureHooks();
static bool SetupMountedDoubleJumpRuntimeFeatureHooks();
static bool SetupMountedDemonJumpRuntimeFeatureHooks();
static bool SetupMountClimbGateFeatureHooks();
static bool SetupMountFlightMappingFeatureHooks();
static bool SetupMountMovementRuntimeFeatureHooks();
enum MountedRuntimeSkillKind
{
    MountedRuntimeSkillKind_DoubleJump = 0,
    MountedRuntimeSkillKind_DemonJump = 1,
    MountedRuntimeSkillKind_Count = 2
};
static bool HasRecentMountedDoubleJumpIntent(int mountItemId, DWORD maxAgeMs = 400);
static bool HasRecentMountedDemonJumpIntent(int mountItemId, DWORD maxAgeMs = 400);
static bool IsMountedDemonJumpRelatedSkillId(int skillId);
static void ObserveMountedDemonJumpIntent(
    int mountItemId,
    const char *reasonTag = nullptr);
static void RememberMountedDemonJumpGlidePacket(int mountItemId);
static bool ShouldSuppressMountedDemonJumpRepeatedGlidePacket(
    int mountItemId,
    DWORD maxAgeMs,
    DWORD *ageOut = nullptr);
static void ArmMountedDemonJumpCrashTrace(int runtimeSkillId, int mountItemId);
static bool IsMountedDemonJumpCrashTraceFresh(
    int *runtimeSkillIdOut = nullptr,
    int *mountItemIdOut = nullptr,
    DWORD maxAgeMs = 2500);
static bool IsExtendedMountSoaringContextMount(int mountItemId);
static bool TryGetMountedSoaringFlightTiming(
    int mountItemId,
    DWORD *ageMsOut,
    DWORD *activeDurationMsOut);
// 骑宠二段跳总闸门：
// 关闭后，下面这一整组 mounted double jump / late path / trace / packet
// 逻辑都会短路，只保留编译期存在，不再安装运行时 hook。
static bool IsMountedDoubleJumpRuntimeHooksEnabled()
{
    return ssw::runtime::IsFeatureEnabled(ssw::runtime::FeatureSwitchId::MountedDoubleJumpRuntimeHooks);
}
#define kEnableMountedDoubleJumpRuntimeHooks (IsMountedDoubleJumpRuntimeHooksEnabled())
// 独立的恶魔跳跃分支开关。它只管 demon 专属链，避免再复用 doubleJump 这个总名。
static bool IsMountedDemonJumpRuntimeHooksEnabled()
{
    return ssw::runtime::IsFeatureEnabled(ssw::runtime::FeatureSwitchId::MountedDemonJumpRuntimeHooks);
}
#define kEnableMountedDemonJumpRuntimeHooks (IsMountedDemonJumpRuntimeHooksEnabled())
#define kEnableMountedDoubleJumpBaseGateHooks (ssw::runtime::IsFeatureEnabled(ssw::runtime::FeatureSwitchId::MountedDoubleJumpBaseGateHooks))
#define kEnableMountedDemonJumpCrashTraceHooks (ssw::runtime::IsFeatureEnabled(ssw::runtime::FeatureSwitchId::MountedDemonJumpCrashTraceHooks))
#define kEnableMountedDemonJumpPacketObserveHooks (ssw::runtime::IsFeatureEnabled(ssw::runtime::FeatureSwitchId::MountedDemonJumpPacketObserveHooks))
#define kEnableMountedDemonJumpLatePathHooks (ssw::runtime::IsFeatureEnabled(ssw::runtime::FeatureSwitchId::MountedDemonJumpLatePathHooks))
#define kEnableMountedDemonJumpActionTraceHooks (ssw::runtime::IsFeatureEnabled(ssw::runtime::FeatureSwitchId::MountedDemonJumpActionTraceHooks))
#define kEnableMountedDemonJumpRequirementBypassHook (ssw::runtime::IsFeatureEnabled(ssw::runtime::FeatureSwitchId::MountedDemonJumpRequirementBypassHook))
#define kEnableMountedDemonJumpContextClearHook (ssw::runtime::IsFeatureEnabled(ssw::runtime::FeatureSwitchId::MountedDemonJumpContextClearHook))
// 这三个开关是二段跳家族的从属门闸，通常与总闸门一起看。
#define kEnableMountMovementAbilityRedHooks (ssw::runtime::IsFeatureEnabled(ssw::runtime::FeatureSwitchId::MountMovementAbilityRedHooks))
#define kEnableGlobalMovementSetterProtectionHooks (ssw::runtime::IsFeatureEnabled(ssw::runtime::FeatureSwitchId::GlobalMovementSetterProtectionHooks))
static const DWORD kMountedDemonJumpIntentMaxAgeMs = 400;
static const DWORD kMountedDemonJumpContextClearProtectMs = 300;
static const DWORD kMountedDemonJumpLateChildPrimeWindowMs = 180;
static const DWORD kMountedDemonJumpLateChildCacheMatchMaxAgeMs = 250;
static const DWORD kMountedDemonJumpTerminalClearSuppressMs = 1500;
static const DWORD kMountedDemonJumpPacketGuardReleaseAgeMs = 2000;
static const DWORD kMountedDemonJumpGlidePacketSustainMaxAgeMs = 600;
static const DWORD kMountedDemonJumpRepeatedGlidePacketSuppressMaxAgeMs = 80;
// 这组开关分别控制：移动 cap patch、移动观测、飞行物理速度。
// historical 名称仍保留，但语义已扩展到后续共享链。
#define kEnableMountMovementCapPatches (ssw::runtime::IsFeatureEnabled(ssw::runtime::FeatureSwitchId::MountMovementCapPatches))
#define kEnableMountMovementObservationHooks (ssw::runtime::IsFeatureEnabled(ssw::runtime::FeatureSwitchId::MountMovementObservationHooks))
#define kEnableMountedFlightPhysicsSpeedHooks (ssw::runtime::IsFeatureEnabled(ssw::runtime::FeatureSwitchId::MountedFlightPhysicsSpeedHooks))
static bool TryReadCurrentUserLocalPtr(void **userLocalOut);
static bool TryReadMountedDemonJumpContextState(
    int *rootSkillIdOut,
    int *currentSkillIdOut = nullptr,
    DWORD *userLocalOut = nullptr);
static bool PrimeMountedDemonJumpContextIfNeeded(
    int mountItemId,
    const char *reason = nullptr,
    int *currentSkillIdOut = nullptr);
static bool HasMountedDemonJumpContextPrimedForMount(
    int mountItemId,
    int *currentSkillIdOut = nullptr,
    int *rootSkillIdOut = nullptr);
static void ObserveMountedDemonJumpTerminalClear(
    int mountItemId,
    const char *reasonTag);
static void ClearMountedDemonJumpTerminalClear(
    int mountItemId,
    const char *reasonTag);
static bool ShouldReleaseMountedDemonJumpTerminalClearForReason(
    const char *reasonTag);
static bool HasRecentMountedDemonJumpTerminalClear(
    int mountItemId,
    DWORD maxAgeMs);
static bool TryReadMountedDemonJumpEffectiveContextState(
    int mountItemId,
    int *rootSkillIdOut,
    int *currentSkillIdOut);
static bool TryReadCurrentUserMountItemId(int *mountItemIdOut);
static bool IsMountedDemonJumpRuntimeChildSkillId(int skillId);
static bool TryReadMountedDemonJumpPrimePlayerObject(void **playerObjOut);
static bool TryClearMountedDemonJumpLateLocalLockState(
    void *thisPtr,
    int mountItemId,
    int rootSkillId,
    int currentSkillId,
    const char *reasonTag,
    DWORD callerRet);
static bool TryClearMountedDemonJumpTailLocalGateState(
    void *thisPtr,
    int mountItemId,
    const char *reasonTag,
    DWORD callerRet);
static bool TryForceFinalizeMountedDemonJumpNativeActionTailState(
    void *thisPtr,
    int mountItemId,
    const char *reasonTag,
    DWORD callerRet);
static bool TryClearMountedDemonJump35121005CarrierTailState(
    void *thisPtr,
    int mountItemId,
    const char *reasonTag,
    DWORD callerRet);
static bool TryReassertMountedDemonJumpChildPacketAfterOverlay(
    void **packetDataSlot,
    int *packetLenSlot,
    uintptr_t callerRetAddr);
static void RememberMountedDemonJumpNativeChildSkill(
    int mountItemId,
    int skillId,
    const char *source = nullptr);
static bool TryGetRecentMountedDemonJumpNativeChildSkill(
    int mountItemId,
    int *skillIdOut,
    const char **sourceOut = nullptr,
    DWORD maxAgeMs = 0);
static void RememberMountedDemonJumpPostPacketVisualChildSkill(
    int mountItemId,
    int skillId,
    const char *source = nullptr);
static bool TryGetRecentMountedDemonJumpPostPacketVisualChildSkill(
    int *mountItemIdOut,
    int *skillIdOut,
    DWORD maxAgeMs);
static void ExpireMountedDemonJumpGlideRecentIntent(
    int mountItemId,
    const char *reasonTag);
static void ClearMountedDemonJumpTransientRuntimeState(
    int mountItemId,
    const char *reasonTag);
static int ResolveMountedRuntimeSkillIdForKind(
    MountedRuntimeSkillKind kind,
    int mountItemId);
static bool SendMountedDemonJumpSyntheticSpecialMovePacket(
    int skillId,
    int level,
    DWORD *tickOut = nullptr);
static bool ArmMountedDemonJumpPendingSpecialMoveRewrite(
    int mountItemId,
    int expectedSkillId,
    int packetSkillId,
    int packetLevel,
    int runtimeChildSkillId,
    const char *source = nullptr,
    DWORD *tickOut = nullptr);
static bool TryRewriteMountedDemonJumpOutgoingPacket(
    void **packetDataSlot,
    int *packetLenSlot,
    uintptr_t callerRetAddr);
static bool ShouldSuppressMountedDemonJumpMountedContextClear(
    void *contextPtr,
    DWORD callerRet,
    int *mountItemIdOut = nullptr,
    int *rootSkillIdOut = nullptr,
    int *currentSkillIdOut = nullptr);
static bool IsAddressInCurrentModule(DWORD address);
static bool TryResolveMountedMovementDataKeyFromMountItemId(int mountItemId, int *dataKeyOut);
static bool TryReadMountItemIdFromPlayerObjectRaw(void *playerObj, int *mountItemIdOut);
static bool TryReadMountItemIdFromPlayerObject(void *playerObj, int *mountItemIdOut);
static bool TryGetRecentMountedMovementRawSample(
    int *mountItemIdOut,
    int *dataKeyOut,
    int *speedOut,
    int *jumpOut,
    DWORD maxAgeMs);
static void ClearRecentMountedMovementRawSample();
static bool TryResolveCurrentUserMountItemIdWithFallback(
    int *mountItemIdOut,
    const char **sourceOut = nullptr);
static bool TryResolveMountedDoubleJumpMountItemIdWithFallback(
    void *playerObj,
    int *mountItemIdOut,
    const char **sourceOut = nullptr,
    DWORD maxAgeMs = 1200);
static void ObserveMountedDoubleJumpNativeRelease(int mountItemId, int skillId);
static bool TryResolveMountedDemonJumpMountItemIdWithFallback(
    void *playerObj,
    int *mountItemIdOut,
    const char **sourceOut = nullptr,
    DWORD maxAgeMs = 1200);
static bool TryResolveMountedDemonJumpRequirementBypassContext(
    DWORD callerRet,
    int skillId,
    int *mountItemIdOut = nullptr,
    int *rootSkillIdOut = nullptr,
    int *currentSkillIdOut = nullptr);
static bool TryGetRecentMountedDemonJumpIntentItemId(
    int *mountItemIdOut,
    DWORD maxAgeMs = 400);
static void ObserveMountedDemonJumpNativeRelease(int mountItemId, int skillId);

