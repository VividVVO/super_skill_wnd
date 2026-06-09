#pragma once
//
// Common.h — 公共头文件、工具函数、日志
//
#include <windows.h>
#include <d3d9.h>
#include <cstdio>
#include <cstdarg>
#include <cstdint>
#include <cstring>

#pragma comment(lib, "d3d9.lib")

// ============================================================================
// 安全内存检查
// ============================================================================
inline bool SafeIsBadReadPtr(const void* ptr, size_t size)
{
    if (!ptr) return true;
    MEMORY_BASIC_INFORMATION mbi = {};
    if (VirtualQuery(ptr, &mbi, sizeof(mbi)) == 0) return true;
    if (mbi.State != MEM_COMMIT) return true;
    if (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD)) return true;
    uintptr_t regionEnd = (uintptr_t)mbi.BaseAddress + mbi.RegionSize;
    if ((uintptr_t)ptr + size > regionEnd) return true;
    return false;
}

inline bool SafeIsBadWritePtr(void* ptr, size_t size)
{
    if (!ptr) return true;
    MEMORY_BASIC_INFORMATION mbi = {};
    if (VirtualQuery(ptr, &mbi, sizeof(mbi)) == 0) return true;
    if (mbi.State != MEM_COMMIT) return true;
    if (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD)) return true;
    const DWORD writableMask =
        PAGE_READWRITE | PAGE_WRITECOPY |
        PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY;
    if ((mbi.Protect & writableMask) == 0) return true;
    uintptr_t regionEnd = (uintptr_t)mbi.BaseAddress + mbi.RegionSize;
    if ((uintptr_t)ptr + size > regionEnd) return true;
    return false;
}

// ============================================================================
// 日志
// ============================================================================
#if defined(SSW_ENABLE_SECOND_CHILD_CARRIER_PROBE_RUNTIME)
#define LOG_FILE "C:\\SuperSkillWnd_probe.log"
#else
#define LOG_FILE "C:\\SuperSkillWnd.log"
#endif

#ifndef SSW_ENABLE_RUNTIME_LOGS
#if defined(SSW_ENABLE_SECOND_CHILD_CARRIER_PROBE_RUNTIME)
#define SSW_ENABLE_RUNTIME_LOGS 1
#else
#define SSW_ENABLE_RUNTIME_LOGS 0
#endif
#endif

#ifndef SSW_ENABLE_DIAGNOSTIC_LOGS
#define SSW_ENABLE_DIAGNOSTIC_LOGS 0
#endif

#ifndef SSW_ENABLE_INDEPENDENT_BUFF_DIAGNOSTIC_LOGS
#define SSW_ENABLE_INDEPENDENT_BUFF_DIAGNOSTIC_LOGS 0
#endif

#ifndef SSW_ENABLE_BUFF_PACKET_PROBE_LOGS
#define SSW_ENABLE_BUFF_PACKET_PROBE_LOGS 0
#endif

#ifndef SSW_ENABLE_ABILITYRED_DIAGNOSTIC_LOGS
#define SSW_ENABLE_ABILITYRED_DIAGNOSTIC_LOGS 0
#endif

#ifndef SSW_ENABLE_ABILITYRED_OBSERVATION_HOOKS
#define SSW_ENABLE_ABILITYRED_OBSERVATION_HOOKS 0
#endif

#ifndef SSW_ENABLE_SUPER_SKILL_SYNC_DIAGNOSTIC_LOGS
#define SSW_ENABLE_SUPER_SKILL_SYNC_DIAGNOSTIC_LOGS 0
#endif

#ifndef SSW_ENABLE_UI_OBSERVATION_DIAGNOSTIC_LOGS
#define SSW_ENABLE_UI_OBSERVATION_DIAGNOSTIC_LOGS 0
#endif

#ifndef SSW_ENABLE_MOUNT_MOVEMENT_DIAGNOSTIC_LOGS
#define SSW_ENABLE_MOUNT_MOVEMENT_DIAGNOSTIC_LOGS 0
#endif

inline const char* GetRuntimeLogFileName()
{
#if defined(SSW_ENABLE_SECOND_CHILD_CARRIER_PROBE_RUNTIME)
    return "SuperSkillWnd_probe.log";
#else
    return "SuperSkillWnd.log";
#endif
}

inline bool GetHookModuleDirectoryA(char* outDir, size_t outDirCount)
{
    if (!outDir || outDirCount == 0)
        return false;

    outDir[0] = '\0';

    HMODULE module = nullptr;
    if (!GetModuleHandleExA(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCSTR>(&GetHookModuleDirectoryA),
            &module))
    {
        return false;
    }

    char modulePath[MAX_PATH] = {};
    const DWORD length = GetModuleFileNameA(module, modulePath, MAX_PATH);
    if (length == 0 || length >= MAX_PATH)
        return false;

    for (DWORD i = length; i > 0; --i)
    {
        if (modulePath[i - 1] == '\\' || modulePath[i - 1] == '/')
        {
            modulePath[i - 1] = '\0';
            break;
        }
    }

    strncpy_s(outDir, outDirCount, modulePath, _TRUNCATE);
    return outDir[0] != '\0';
}

inline bool BuildRuntimeLogPathFromDirectoryA(const char* dir, char* outPath, size_t outPathCount)
{
    if (!dir || !dir[0] || !outPath || outPathCount == 0)
        return false;

    const size_t dirLength = strlen(dir);
    const char* separator =
        (dirLength > 0 && (dir[dirLength - 1] == '\\' || dir[dirLength - 1] == '/')) ? "" : "\\";
    const int written = _snprintf_s(
        outPath,
        outPathCount,
        _TRUNCATE,
        "%s%s%s",
        dir,
        separator,
        GetRuntimeLogFileName());
    return written >= 0 && outPath[0] != '\0';
}

inline char* RuntimeLogPathCacheA()
{
#if SSW_ENABLE_RUNTIME_LOGS
    static char s_cachedPath[MAX_PATH] = {};
    return s_cachedPath;
#else
    return nullptr;
#endif
}

inline FILE* OpenRuntimeLogFile(const char* mode)
{
#if SSW_ENABLE_RUNTIME_LOGS
    if (!mode || !mode[0])
        return nullptr;

    char* cachedPath = RuntimeLogPathCacheA();
    if (cachedPath && cachedPath[0])
    {
        FILE* cachedFile = fopen(cachedPath, mode);
        if (cachedFile)
            return cachedFile;
        cachedPath[0] = '\0';
    }

    char candidatePath[MAX_PATH] = {};
    char dir[MAX_PATH] = {};
    if (GetHookModuleDirectoryA(dir, sizeof(dir)) &&
        BuildRuntimeLogPathFromDirectoryA(dir, candidatePath, sizeof(candidatePath)))
    {
        FILE* hookDirFile = fopen(candidatePath, mode);
        if (hookDirFile)
        {
            if (cachedPath)
                strncpy_s(cachedPath, MAX_PATH, candidatePath, _TRUNCATE);
            return hookDirFile;
        }
    }

    char tempDir[MAX_PATH] = {};
    const DWORD tempDirLength = GetTempPathA(MAX_PATH, tempDir);
    if (tempDirLength > 0 &&
        tempDirLength < MAX_PATH &&
        BuildRuntimeLogPathFromDirectoryA(tempDir, candidatePath, sizeof(candidatePath)))
    {
        FILE* tempFile = fopen(candidatePath, mode);
        if (tempFile)
        {
            if (cachedPath)
                strncpy_s(cachedPath, MAX_PATH, candidatePath, _TRUNCATE);
            return tempFile;
        }
    }

    FILE* localFile = fopen(GetRuntimeLogFileName(), mode);
    if (localFile && cachedPath)
        strncpy_s(cachedPath, MAX_PATH, GetRuntimeLogFileName(), _TRUNCATE);
    return localFile;
#else
    (void)mode;
    return nullptr;
#endif
}

inline const char* GetRuntimeLogPathA()
{
#if SSW_ENABLE_RUNTIME_LOGS
    char* cachedPath = RuntimeLogPathCacheA();
    if (!cachedPath)
        return "";

    if (!cachedPath[0])
    {
        FILE* file = OpenRuntimeLogFile("a");
        if (file)
            fclose(file);
        if (!cachedPath[0])
        {
            char dir[MAX_PATH] = {};
            if (GetHookModuleDirectoryA(dir, sizeof(dir)))
                BuildRuntimeLogPathFromDirectoryA(dir, cachedPath, MAX_PATH);
        }
        if (!cachedPath[0])
            strncpy_s(cachedPath, MAX_PATH, GetRuntimeLogFileName(), _TRUNCATE);
    }
    return cachedPath;
#else
    return "";
#endif
}

inline void WriteLog(const char* msg)
{
#if SSW_ENABLE_RUNTIME_LOGS
    FILE* f = OpenRuntimeLogFile("a");
    if (f) { fprintf(f, "%s\n", msg); fclose(f); }
#else
    (void)msg;
#endif
}

inline void WriteLogFmt(const char* fmt, ...)
{
#if SSW_ENABLE_RUNTIME_LOGS
    char buf[512];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    WriteLog(buf);
#else
    (void)fmt;
#endif
}

inline bool EnableIndependentBuffOverlayDiagnosticLogs()
{
    return SSW_ENABLE_INDEPENDENT_BUFF_DIAGNOSTIC_LOGS != 0;
}

inline bool EnableBuffPacketProbeLogs()
{
    return SSW_ENABLE_BUFF_PACKET_PROBE_LOGS != 0;
}

inline bool EnableAbilityRedDiagnosticLogs()
{
    return SSW_ENABLE_ABILITYRED_DIAGNOSTIC_LOGS != 0;
}

inline bool EnableAbilityRedObservationHooks()
{
    return SSW_ENABLE_ABILITYRED_OBSERVATION_HOOKS != 0;
}

inline bool EnableSceneFadeObservationHooks()
{
    return false;
}

inline bool EnableSuperSkillSyncStateDiagnosticLogs()
{
    return SSW_ENABLE_SUPER_SKILL_SYNC_DIAGNOSTIC_LOGS != 0;
}

inline bool EnableUiObservationDiagnosticLogs()
{
    return SSW_ENABLE_UI_OBSERVATION_DIAGNOSTIC_LOGS != 0;
}

inline bool EnableMountMovementDiagnosticLogs()
{
    return SSW_ENABLE_MOUNT_MOVEMENT_DIAGNOSTIC_LOGS != 0;
}

inline void HardenPixelArtAlphaEdgesRgba(
    unsigned char* rgba,
    int width,
    int height,
    unsigned char transparentCutoff = 64,
    unsigned char alphaBoost = 48)
{
    if (!rgba || width <= 0 || height <= 0)
        return;

    const size_t pixelCount = (size_t)width * (size_t)height;
    for (size_t i = 0; i < pixelCount; ++i)
    {
        unsigned char& alpha = rgba[i * 4 + 3];
        if (alpha == 0 || alpha == 255)
            continue;

        if (alpha < transparentCutoff)
        {
            alpha = 0;
            continue;
        }

        const unsigned int boosted = (unsigned int)alpha + (unsigned int)alphaBoost;
        alpha = (unsigned char)(boosted > 255 ? 255 : boosted);
    }
}

// ============================================================================
// CWnd坐标读取
// ============================================================================
inline int CWnd_GetX(uintptr_t cwnd)
{
    if (!cwnd || SafeIsBadReadPtr((void*)(cwnd + 0x18), 4)) return 0;
    uintptr_t com = *(uintptr_t*)(cwnd + 0x18);
    if (!com || SafeIsBadReadPtr((void*)(com + 0x54), 4)) return 0;
    return *(int*)(com + 0x54);
}

inline int CWnd_GetY(uintptr_t cwnd)
{
    if (!cwnd || SafeIsBadReadPtr((void*)(cwnd + 0x18), 4)) return 0;
    uintptr_t com = *(uintptr_t*)(cwnd + 0x18);
    if (!com || SafeIsBadReadPtr((void*)(com + 0x58), 4)) return 0;
    return *(int*)(com + 0x58);
}

inline void CWnd_SetHomePos(uintptr_t cwnd, int x, int y)
{
    // 仅适用于已确认拥有 +2756/+2760 字段的大对象（如 A996B0-family）。
    // 不要用于 SkillWnd official second-child：该对象只有 0x84 字节。
    if (!cwnd || SafeIsBadReadPtr((void*)(cwnd + 2756), 4)) return;
    *(int*)(cwnd + 2756) = x;
    *(int*)(cwnd + 2760) = y;
}

// 直接写COM surface的屏幕坐标（游戏通常从home自动计算，但自建CWnd可能不走那条路）
inline void CWnd_SetComPos(uintptr_t cwnd, int x, int y)
{
    if (!cwnd || SafeIsBadReadPtr((void*)(cwnd + 0x18), 4)) return;
    uintptr_t com = *(uintptr_t*)(cwnd + 0x18);
    if (!com || SafeIsBadReadPtr((void*)(com + 0x54), 4)) return;
    *(int*)(com + 0x54) = x;
    *(int*)(com + 0x58) = y;
}

// 原生绘制管线使用的渲染坐标字段，证据来自 sub_B9B800 / sub_B9DF60
inline int CWnd_GetRenderX(uintptr_t cwnd)
{
    if (!cwnd || SafeIsBadReadPtr((void*)(cwnd + 0x44), 4)) return 0;
    return *(int*)(cwnd + 0x44);
}

inline int CWnd_GetRenderY(uintptr_t cwnd)
{
    if (!cwnd || SafeIsBadReadPtr((void*)(cwnd + 0x48), 4)) return 0;
    return *(int*)(cwnd + 0x48);
}

inline int CWnd_GetWidth(uintptr_t cwnd)
{
    if (!cwnd || SafeIsBadReadPtr((void*)(cwnd + 0x28), 4)) return 0;
    return *(int*)(cwnd + 0x28);
}

inline int CWnd_GetHeight(uintptr_t cwnd)
{
    if (!cwnd || SafeIsBadReadPtr((void*)(cwnd + 0x2C), 4)) return 0;
    return *(int*)(cwnd + 0x2C);
}

inline void CWnd_SetRenderPos(uintptr_t cwnd, int x, int y)
{
    if (!cwnd || SafeIsBadReadPtr((void*)(cwnd + 0x44), 8)) return;
    *(int*)(cwnd + 0x44) = x;
    *(int*)(cwnd + 0x48) = y;
}

inline int CWnd_GetHomeX(uintptr_t cwnd)
{
    // 仅适用于已确认拥有 +2756/+2760 字段的大对象。
    if (!cwnd || SafeIsBadReadPtr((void*)(cwnd + 2756), 4)) return 0;
    return *(int*)(cwnd + 2756);
}

inline int CWnd_GetHomeY(uintptr_t cwnd)
{
    // 仅适用于已确认拥有 +2756/+2760 字段的大对象。
    if (!cwnd || SafeIsBadReadPtr((void*)(cwnd + 2760), 4)) return 0;
    return *(int*)(cwnd + 2760);
}

// ============================================================================
// D3D9纹理绘制
// ============================================================================
struct TexturedVertex { float x, y, z, rhw; DWORD color; float u, v; };
#define D3DFVF_TLVERTEX (D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1)
struct SolidVertex { float x, y, z, rhw; DWORD color; };
#define D3DFVF_SOLIDVERTEX (D3DFVF_XYZRHW | D3DFVF_DIFFUSE)

inline void DrawTexturedQuadUV(IDirect3DDevice9* dev, IDirect3DTexture9* tex,
                               float x, float y, float w, float h,
                               float u0, float v0, float u1, float v1,
                               DWORD color = 0xFFFFFFFF)
{
    if (!dev || !tex) return;

    TexturedVertex verts[4] = {
        { x,     y,     0.5f, 1.0f, color, u0, v0 },
        { x + w, y,     0.5f, 1.0f, color, u1, v0 },
        { x,     y + h, 0.5f, 1.0f, color, u0, v1 },
        { x + w, y + h, 0.5f, 1.0f, color, u1, v1 },
    };

    IDirect3DStateBlock9* pSB = nullptr;
    if (FAILED(dev->CreateStateBlock(D3DSBT_ALL, &pSB))) return;
    pSB->Capture();

    dev->SetVertexShader(nullptr);
    dev->SetPixelShader(nullptr);
    dev->SetTexture(0, tex);
    dev->SetTexture(1, nullptr);
    dev->SetFVF(D3DFVF_TLVERTEX);
    dev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    dev->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, FALSE);
    dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    dev->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
    dev->SetRenderState(D3DRS_COLORWRITEENABLE, 0x0000000F);
    dev->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
    dev->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
    dev->SetRenderState(D3DRS_LIGHTING, FALSE);
    dev->SetRenderState(D3DRS_ZENABLE, FALSE);
    dev->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
    dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    dev->SetRenderState(D3DRS_FOGENABLE, FALSE);
    dev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    dev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    dev->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    dev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
    dev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    dev->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
    dev->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
    dev->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
    dev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
    dev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
    dev->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
    dev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    dev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

    dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, verts, sizeof(TexturedVertex));

    pSB->Apply();
    pSB->Release();
    dev->SetTexture(0, nullptr);
}

inline void DrawTexturedQuad(IDirect3DDevice9* dev, IDirect3DTexture9* tex,
                             float x, float y, float w, float h, DWORD color = 0xFFFFFFFF)
{
    DrawTexturedQuadUV(dev, tex, x, y, w, h, 0.0f, 0.0f, 1.0f, 1.0f, color);
}

inline void DrawSolidQuad(IDirect3DDevice9* dev, float x, float y, float w, float h, DWORD color)
{
    if (!dev || w <= 0.0f || h <= 0.0f) return;

    SolidVertex verts[4] = {
        { x,     y,     0.5f, 1.0f, color },
        { x + w, y,     0.5f, 1.0f, color },
        { x,     y + h, 0.5f, 1.0f, color },
        { x + w, y + h, 0.5f, 1.0f, color },
    };

    IDirect3DStateBlock9* pSB = nullptr;
    if (FAILED(dev->CreateStateBlock(D3DSBT_ALL, &pSB))) return;
    pSB->Capture();

    dev->SetVertexShader(nullptr);
    dev->SetPixelShader(nullptr);
    dev->SetTexture(0, nullptr);
    dev->SetTexture(1, nullptr);
    dev->SetFVF(D3DFVF_SOLIDVERTEX);
    dev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    dev->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, FALSE);
    dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    dev->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
    dev->SetRenderState(D3DRS_COLORWRITEENABLE, 0x0000000F);
    dev->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
    dev->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
    dev->SetRenderState(D3DRS_LIGHTING, FALSE);
    dev->SetRenderState(D3DRS_ZENABLE, FALSE);
    dev->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
    dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    dev->SetRenderState(D3DRS_FOGENABLE, FALSE);
    dev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    dev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
    dev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
    dev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
    dev->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
    dev->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);

    dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, verts, sizeof(SolidVertex));

    pSB->Apply();
    pSB->Release();
}
