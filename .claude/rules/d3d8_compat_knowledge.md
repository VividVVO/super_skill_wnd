---
name: D3D8 兼容性完整知识库
description: D3D8 模式检测、hook 安装、overlay 渲染方案、已知问题、vtable 索引、结构体布局、失败路线记录、黑屏修复
type: project
originSessionId: a67572ac-6f53-4e9f-b686-12f30fc2a464
---
# D3D8 兼容性完整知识库

更新时间：2026-04-11（v2 — 含黑屏诊断与修复）

## 1. 背景与需求

用户有两台电脑：
- **电脑 A**：游戏使用 D3D9（正常工作）
- **电脑 B**：游戏使用 D3D8（d3d8.dll + d3d8thk.dll 已加载），所有 D3D9 hook 失效，`g_pDevice` 永远为 null，整个 overlay UI 不工作

目标：运行时自动检测 D3D8/D3D9，D3D8 模式下 overlay 照常显示。

**Why:** 不同部署环境的游戏客户端使用不同 D3D 版本，必须两种都兼容才能让 overlay 在所有用户机器上工作。

**How to apply:** DLL 加载时检测 d3d8.dll 是否已被加载，据此走不同的 hook 路径。D3D9 路径零修改。

---

## 2. 运行时检测机制

**检测方式**（dllmain.cpp 初始化流程）：
```cpp
if (::GetModuleHandleA("d3d8.dll")) {
    g_IsD3D8Mode = true;
    SetupD3D8Hook();
} else {
    SetupD3D9Hook();  // 现有逻辑不变
}
```

**关键全局变量**（dllmain.cpp）：
- `g_IsD3D8Mode` (line ~45) — 运行时标记
- `g_D3D8GameHwnd` — 游戏窗口句柄
- `oD3D8Present` / `oD3D8Reset` — 原始 D3D8 函数指针

---

## 3. D3D8 IDirect3DDevice8 VTable 索引（已修正 2026-04-11，二次验证）

**来源**: d3d8to9 项目 d3d8.hpp 头文件，逐一与 d3d8.h STDMETHOD 声明顺序对比。

**验证方法**: d3d8to9 列表中的编号从 1 开始（IUnknown 之后），加上 IUnknown 的 3 个方法（0=QI, 1=AddRef, 2=Release），得到真实 vtable 索引 = listing_number + 2。

**重大踩坑历史**:
1. 初版多个索引错误（BeginScene=33→34, EndScene=34→35, GetRenderState=49→51, GetTextureStageState=55→62, SetVertexShader=58→76, SetTextureStageState=62→63）
2. 旧版 GetRenderState=49 实际是 GetClipPlane → 保存的 render state 值全是垃圾
3. 旧版 SetTextureStageState=62 实际是 GetTextureStageState → 把 value 当 DWORD* 写，访问 0x4 地址崩溃

**完整索引表**（已验证）：

| Index | Method | 用途 |
|-------|--------|------|
| 0 | QueryInterface | COM 标准 |
| 1 | AddRef | COM 标准 |
| 2 | Release | 释放对象 |
| 3 | TestCooperativeLevel | 设备状态检测 |
| 4 | GetAvailableTextureMem | |
| 5 | ResourceManagerDiscardBytes | |
| 6 | GetDirect3D | |
| 7 | GetDeviceCaps | |
| 8 | GetDisplayMode | |
| 9 | GetCreationParameters | |
| 10 | SetCursorProperties | |
| 11 | SetCursorPosition | |
| 12 | ShowCursor | |
| 13 | CreateAdditionalSwapChain | |
| 14 | Reset | hook 目标：设备重置 |
| 15 | Present | hook 目标：每帧渲染 |
| 16 | GetBackBuffer | |
| 17 | GetRasterStatus | |
| 18 | SetGammaRamp | |
| 19 | GetGammaRamp | |
| 20 | CreateTexture | 纹理创建 |
| 21 | CreateVolumeTexture | |
| 22 | CreateCubeTexture | |
| 23 | CreateVertexBuffer | |
| 24 | CreateIndexBuffer | |
| 25 | CreateRenderTarget | |
| 26 | CreateDepthStencilSurface | |
| 27 | CreateImageSurface | |
| 28 | CopyRects | |
| 29 | UpdateTexture | |
| 30 | GetFrontBuffer | |
| 31 | SetRenderTarget | |
| 32 | GetRenderTarget | |
| 33 | GetDepthStencilSurface | |
| 34 | BeginScene | 开始场景 |
| 35 | EndScene | 结束场景 |
| 36 | Clear | 清除 |
| 37 | SetTransform | 设置变换矩阵 |
| 38 | GetTransform | 获取变换矩阵 |
| 39 | MultiplyTransform | |
| 40 | SetViewport | 设置视口 |
| 41 | GetViewport | 获取视口 |
| 42 | SetMaterial | |
| 43 | GetMaterial | |
| 44 | SetLight | |
| 45 | GetLight | |
| 46 | LightEnable | |
| 47 | GetLightEnable | |
| 48 | SetClipPlane | |
| 49 | GetClipPlane | ⚠ 旧版误当 GetRenderState |
| 50 | SetRenderState | 设置渲染状态 |
| 51 | GetRenderState | 获取渲染状态 |
| 52 | BeginStateBlock | |
| 53 | EndStateBlock | |
| 54 | ApplyStateBlock | |
| 55 | CaptureStateBlock | ⚠ 旧版误当 GetTextureStageState |
| 56 | DeleteStateBlock | |
| 57 | CreateStateBlock | |
| 58 | SetClipStatus | ⚠ 旧版误当 SetVertexShader |
| 59 | GetClipStatus | |
| 60 | GetTexture | 获取纹理 |
| 61 | SetTexture | 设置纹理 |
| 62 | GetTextureStageState | 获取TSS ⚠ 旧版误当 SetTSS |
| 63 | SetTextureStageState | 设置TSS |
| 64 | ValidateDevice | |
| 65 | GetInfo | |
| 66 | SetPaletteEntries | |
| 67 | GetPaletteEntries | |
| 68 | SetCurrentTexturePalette | |
| 69 | GetCurrentTexturePalette | |
| 70 | DrawPrimitive | |
| 71 | DrawIndexedPrimitive | |
| 72 | DrawPrimitiveUP | 用户指针绘制 |
| 73 | DrawIndexedPrimitiveUP | |
| 74 | ProcessVertices | |
| 75 | CreateVertexShader | |
| 76 | SetVertexShader | 设置FVF（D3D8用此传FVF常量） |
| 77 | GetVertexShader | 获取FVF/shader handle |

## 4. D3D8 IDirect3D8 VTable 索引

| Index | Method | 用途 |
|-------|--------|------|
| 0 | QueryInterface | COM 标准 |
| 1 | AddRef | COM 标准 |
| 2 | Release | 释放对象 |
| 8 | GetAdapterDisplayMode | 查询显示格式（用于 BackBufferFormat） |
| 15 | CreateDevice | 创建设备 |

## 5. D3D8 IDirect3DTexture8 VTable 索引

| Index | Method | 用途 |
|-------|--------|------|
| 0 | QueryInterface | |
| 1 | AddRef | |
| 2 | Release | 释放纹理 |
| 16 | LockRect | 锁定纹理写入 |
| 17 | UnlockRect | 解锁纹理 |

---

## 6. D3D8 D3DPRESENT_PARAMETERS 结构体布局

**关键区别**：D3D8 没有 `MultiSampleQuality` 字段（D3D9 在 MultiSampleType 之后新增了它），导致后续字段偏移全不同。

```
Offset  Field                          类型
0x00    BackBufferWidth                DWORD
0x04    BackBufferHeight               DWORD
0x08    BackBufferFormat               D3DFORMAT  ← D3D8 不支持 D3DFMT_UNKNOWN(0)！
0x0C    BackBufferCount                DWORD
0x10    MultiSampleType                D3DMULTISAMPLE_TYPE
0x14    SwapEffect                     D3DSWAPEFFECT (1=DISCARD)
0x18    hDeviceWindow                  HWND
0x1C    Windowed                       BOOL (1=窗口模式)
0x20    EnableAutoDepthStencil         BOOL
0x24    AutoDepthStencilFormat         D3DFORMAT
0x28    Flags                          DWORD
0x2C    FullScreen_RefreshRateInHz     UINT
0x30    FullScreen_PresentationInterval UINT
```

**已踩坑**：`BackBufferFormat=0 (D3DFMT_UNKNOWN)` 在 D3D9 窗口模式下有效，但 **D3D8 直接拒绝**，返回 `D3DERR_INVALIDCALL (0x8876086C)`。

**正确做法**：先调 `IDirect3D8::GetAdapterDisplayMode` (vtable[8]) 获取当前显示格式，或 fallback 到 `D3DFMT_X8R8G8B8 (22)`。

---

## 7. SetupD3D8Hook 流程

1. 获取 `d3d8.dll` 模块和 `Direct3DCreate8` 函数
2. 创建 dummy 窗口 "SSWDummyD3D8"
3. `Direct3DCreate8(220)` 获取 IDirect3D8 对象（SDK version = 220）
4. 查询显示模式格式（GetAdapterDisplayMode vtable[8]），fallback `D3DFMT_X8R8G8B8`
5. 构造 D3D8 D3DPRESENT_PARAMETERS（手工填充 BYTE[64] 缓冲区）
6. `CreateDevice` (vtable[15]) 创建 dummy D3D8 设备（先 HAL，失败试 REF）
7. 从 dummy device vtable 读取 Present(15) 和 Reset(14) 的真实地址
8. Inline hook Present 和 Reset（用 InlineHook.h 的 `GenericInlineHook5`）
9. 如果 inline hook 失败，fallback 到 vtable patch
10. 释放 dummy 设备和 IDirect3D8，销毁 dummy 窗口

**注意**：
- D3D8 没有 `D3DDEVTYPE_NULLREF`，只有 HAL(1) 和 REF(2)
- vtable 类型是 `DWORD*`（x86 32位），不是 `DWORD**`（曾因此编译错误 C2440）

---

## 8. 渲染方案演进

### 8.1 方案 A：共享 HWND（v19.1）— 已失败

D3D9 overlay 设备和游戏 D3D8 设备绑定到**同一个 HWND**。

**结果**：疯狂闪屏。

**根因**：两个设备各自拥有独立 swap chain，各自 `Present` 都会把自己的 backbuffer 翻到窗口前台，互相覆盖。

**结论**：**D3D8 和 D3D9 不能共享同一个 HWND 做 Present**。

### 8.2 方案 B：透明覆盖窗口 + Color Key（v19.2）— 已放弃

创建 `WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_NOACTIVATE` 的覆盖窗口。

**已知问题**：抗锯齿打孔、全屏不可用、DWM 开销、焦点问题。

### 8.3 方案 C：直接在 D3D8 设备上画（v20.x）— 当前版本

在 hkD3D8Present 中，直接用游戏的 D3D8 设备做所有 overlay 渲染。

**关键设计**：
- 不创建 D3D9 设备
- 不用 ImGui
- 用 `DrawPrimitiveUP` + XYZRHW 顶点直接画
- 用 DWrite（实际是 GDI TextOutW）渲染文字到内存 bitmap → D3D8 纹理 → DrawPrimitiveUP
- 纹理从 DLL 嵌入资源加载（stb_image 解码 PNG → RGBA → D3D8 CreateTexture）

**每帧流程（v20.1 — 含 BeginScene/EndScene 修复）**：
```
游戏调用 D3D8 Present
    ↓
hkD3D8Present 拦截
    ↓
首次调用? → 查找游戏 HWND
    ↓
纹理加载（一次性）
    ↓
面板展开时：
  1. D3D8_SaveRenderState() — 保存 RS/TSS/Texture/VS/Viewport/Transforms
  2. BeginScene() — 重新开启场景（游戏已经调过 EndScene）
  3. D3D8_SetOverlayRenderState() — 设置 alpha blend 等
  4. 同步面板数据 (SkillOverlayBridgeSyncRetroState)
  5. D3D8_RenderOverlayPanel() — 画面板
  6. EndScene()
  7. D3D8_RestoreRenderState() — 恢复所有游戏状态
    ↓
调用原始 D3D8 Present
```

---

## 9. 已踩坑记录

### 9.1 BackBufferFormat = D3DFMT_UNKNOWN (0)
- **现象**：D3D8 dummy device 创建失败，`hr=0x8876086C (D3DERR_INVALIDCALL)`
- **根因**：D3D8 不接受 `D3DFMT_UNKNOWN`，D3D9 接受
- **修复**：查询 GetAdapterDisplayMode 或 fallback `D3DFMT_X8R8G8B8 (22)`

### 9.2 vtable 类型 DWORD** vs DWORD*
- **现象**：编译错误 C2440，DWORD* 到 DWORD 的转换
- **根因**：x86 下 vtable 条目是 `DWORD`（4 字节指针），应声明为 `DWORD*`，不是 `DWORD**`
- **修复**：`DWORD* vtable = *(DWORD**)pDevice`

### 9.3 共享 HWND 闪屏
- **现象**：一黑一黑疯狂闪烁
- **根因**：两个 D3D 设备各自 Present 互相覆盖 swap chain
- **修复**：放弃方案 A，改用方案 C（直接在 D3D8 设备上画）

### 9.4 VTable 索引全错（2026-04-11 修复）
- **现象**：点击按钮后崩溃于 d3d8.514152CB
- **根因**：6 个 vtable 索引全错（来自错误的 D3D9 映射或猜测）
  - BeginScene 33→34, EndScene 34→35
  - GetRenderState 49→51 (49=GetClipPlane!)
  - GetTextureStageState 55→62 (55=CaptureStateBlock!)
  - SetVertexShader 58→76 (58=SetClipStatus!)
  - SetTextureStageState 62→63 (62=GetTextureStageState!)
- **崩溃根因详析**：调 SetTextureStageState(dev, 0, TSS_COLOROP, TOP_MODULATE) 时，实际调到了 GetTextureStageState(dev, 0, TSS_COLOROP, &outValue)。因为 TOP_MODULATE=4，GetTSS 把 4 当指针写入 → 访问 0x00000004 → 崩溃
- **修复**：d3d8_renderer.h D3D8VT namespace 全部索引修正

### 9.5 黑屏 + 控件错位（2026-04-11 诊断修复）
- **现象**：vtable 修复后不再崩溃，但点击按钮后游戏黑屏，左下角技能窗消失，控件错位
- **根因分析（多因）**：
  1. **GetVertexShader 未保存**：savedVS 始终为 0，恢复时把游戏 FVF/shader 清零
  2. **Viewport 未保存/恢复**：overlay 可能改变内部 viewport 状态
  3. **Transform 未保存/恢复**：虽然 XYZRHW 绕过 transform pipeline，但 DrawPrimitiveUP 可能修改内部状态
  4. **没有 BeginScene/EndScene**：游戏已经调过 EndScene，我们在 EndScene 和 Present 之间画，D3D8 可能忽略所有 draw calls
- **修复（v20.1）**：
  1. 添加 GetVertexShader (vtable[77]) 保存/恢复
  2. 添加 GetViewport/SetViewport (vtable[41/40]) 保存/恢复
  3. 添加 GetTransform/SetTransform (vtable[38/37]) 保存/恢复 VIEW/PROJECTION/WORLD
  4. 在 overlay 渲染前后加 BeginScene/EndScene 包装

---

## 10. D3D8 Render State 保存/恢复完整清单（v20.1）

### 保存的 Render States (11 个)：
| RS 枚举 | 值 | 说明 |
|---------|-----|------|
| RS_ZENABLE | 7 | Z 缓冲 |
| RS_ZWRITEENABLE | 14 | Z 写入 |
| RS_ALPHATESTENABLE | 15 | Alpha 测试 |
| RS_SRCBLEND | 19 | 源混合 |
| RS_DESTBLEND | 20 | 目标混合 |
| RS_CULLMODE | 22 | 裁剪模式 |
| RS_ALPHABLENDENABLE | 27 | Alpha 混合 |
| RS_FOGENABLE | 28 | 雾 |
| RS_LIGHTING | 137 | 光照 |
| RS_COLORWRITEENABLE | 168 | 颜色通道写入 |
| RS_BLENDOP | 171 | 混合操作 |

### 保存的 TSS Stage 0 (11 个)：
TSS_COLOROP, TSS_COLORARG1, TSS_COLORARG2, TSS_ALPHAOP, TSS_ALPHAARG1, TSS_ALPHAARG2, TSS_MINFILTER, TSS_MAGFILTER, TSS_MIPFILTER, TSS_ADDRESSU, TSS_ADDRESSV

### 保存的 TSS Stage 1 (2 个)：
TSS_COLOROP, TSS_ALPHAOP

### 其他保存项：
- Texture stage 0 (GetTexture/SetTexture)
- Vertex Shader / FVF (GetVertexShader/SetVertexShader)
- Viewport (GetViewport/SetViewport)
- Transform D3DTS_VIEW=2
- Transform D3DTS_PROJECTION=3
- Transform D3DTS_WORLD=256

---

## 11. Hook 引擎技术细节

### InlineHook.h
- `FollowJmpChain()` — 跟随最多 16 层 JMP 链找到真实入口
- `CalcMinCopyLen()` — 用 hde32 计算安全拷贝长度（≥5 字节）
- `GenericInlineHook5()` — 5 字节 inline hook：复制原始指令到 trampoline，覆写入口为 `JMP hookFunc`
- `InstallInlineHook()` — 完整版：自动 FollowJmpChain + CalculateRelocatedByteCount + hook

### hde32.h
- 表驱动 x86-32 指令长度解码器
- 支持所有标准指令前缀、ModR/M、SIB、displacement、immediate
- 返回 0 表示未知指令

---

## 12. D3D8 纹理管理

### 创建流程
```
stb_image_load_from_memory(PNG) → RGBA buffer
    ↓
IDirect3DDevice8::CreateTexture(w, h, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED)
    ↓
IDirect3DTexture8::LockRect(0, &locked, NULL, 0)
    ↓
RGBA → BGRA 通道交换（D3DFMT_A8R8G8B8 内存布局是 BGRA）
    ↓
IDirect3DTexture8::UnlockRect(0)
```

### 当前加载的纹理（9 个面板资源 + 动态技能 icon + 文字缓存）
- `g_d3d8PanelBg` — 面板背景
- `g_d3d8BtnNormal/Hover/Pressed/Disabled` — 4 个按钮状态
- `g_d3d8CursorNormal/1/2/3` — 4 个鼠标帧
- 技能 icon 纹理 — 按需从 DLL 资源加载
- 文字纹理缓存 — GDI TextOutW → D3D8 纹理，用 std::map<string, D3D8Texture> 缓存

---

## 13. D3D8 文字渲染实现

**命名为 DWrite 但实际是 GDI 方案**：

```
CreateCompatibleDC → CreateDIBSection(ARGB32)
    ↓
SelectObject(hFont) → SetBkMode(TRANSPARENT) → SetTextColor(white)
    ↓
TextOutW(hdc, 0, 0, text, len) → GDI 渲染到 DIB
    ↓
读取 DIB 像素 → 应用 color tint → D3D8_CreateTextureFromRGBA
    ↓
缓存到 D3D8TextCache::cache[key]
```

**文字测量**：`GetTextExtentPoint32W` 获取精确像素宽高。

---

## 14. 关键常量速查

| 常量 | 值 | 说明 |
|------|-----|------|
| D3D8 SDK Version | 220 | Direct3DCreate8 参数 |
| D3DFMT_X8R8G8B8 | 22 | BackBufferFormat fallback |
| D3DFMT_A8R8G8B8 | 21 | 纹理格式 |
| D3DFMT_UNKNOWN | 0 | D3D9 支持，D3D8 不支持 |
| D3DSWAPEFFECT_DISCARD | 1 | SwapEffect |
| D3DDEVTYPE_HAL | 1 | 硬件加速 |
| D3DDEVTYPE_REF | 2 | 软件参考 |
| D3DCREATE_SOFTWARE_VERTEXPROCESSING | 0x20 | 创建标志 |
| D3DPOOL_MANAGED | 1 | 纹理池 |
| D3DERR_INVALIDCALL | 0x8876086C | 参数错误 |
| D3DTS_VIEW | 2 | 视图变换 |
| D3DTS_PROJECTION | 3 | 投影变换 |
| D3DTS_WORLD | 256 | 世界变换 |
| FVF_TLVERTEX | 0x144 | XYZRHW\|DIFFUSE\|TEX1 |
| FVF_SOLIDVERTEX | 0x044 | XYZRHW\|DIFFUSE |

---

## 15. ToggleSuperWnd D3D8 路径

在 dllmain.cpp 的 `ToggleSuperWnd()` 中，D3D8 模式有独立旁路（lines 4481-4483）：
```cpp
if (g_IsD3D8Mode) {
    g_SuperExpanded = !g_SuperExpanded;
    return;  // D3D8 模式不走 ImGui/CreateSuperWnd
}
```

D3D8 模式下面板的 show/hide 纯靠 `g_SuperExpanded` 标志，由 `hkD3D8Present` 每帧检查。

---

## 16. 版本历史

| 版本 | 日期 | 改动 |
|------|------|------|
| v19.1 | 2026-04-10 | 方案 A：共享 HWND → 闪屏，放弃 |
| v19.2 | 2026-04-10 | 方案 B：透明覆盖窗口 → 有硬伤，放弃 |
| v20.0 | 2026-04-11 | 方案 C：D3D8 直接渲染，vtable 索引错误导致崩溃 |
| v20.0-fix | 2026-04-11 | 修正 6 个 vtable 索引，不再崩溃，但黑屏 |
| v20.1 | 2026-04-11 | 修复黑屏：添加 VS/Viewport/Transform 保存恢复 + BeginScene/EndScene |

---

## 17. 待解决问题清单

1. **v20.1 黑屏修复是否生效** — 用户尚未部署验证
2. **面板控件是否正确显示** — 纹理加载日志显示成功，但用户报告 "按钮没显示" + "控件错位"
3. **D3D8 电脑是窗口模式还是全屏** — 决定某些方案是否可行
4. **文字渲染质量** — GDI TextOutW 在小字号下的清晰度
5. **鼠标输入** — D3D8 模式下的按钮点击和鼠标命中检测
6. **性能** — 每帧 save/restore 大量状态的开销

### 17.1 2026-06-05 独立 BUFF D3D8 常驻卡顿结论

客户日志 `SuperSkillWnd (38).log` 证明现场为 D3D8：

- `marker=v23.85-2026-06-05-pending-package-runtime-hooks-fail-closed`
- `routeB_hooks_ready=0 imgui_overlay=1 d3d8=1`
- `[D3D8] display mode: 3440x1440 fmt=22`
- 独立 BUFF 出现后有 `IndependentBuffVirtual activate skillId=1111098/1111097/1111096`

该版本中只要 `SkillOverlayBridgeHasIndependentBuffOverlayEntries()` 为真，即使超级技能面板没有展开，也会在每帧 Present 里走完整 `SuperD3D8OverlayRender()`：

- `SkillOverlayBridgeGetIndependentBuffOverlayEntries`
- `ImGui_ImplD3D8_NewFrame`
- `ImGui::NewFrame`
- `ImGui::Render`
- `ImGui_ImplD3D8_RenderDrawData`

并且 entries 查询会每帧填充名称、等级和三段 tooltip 格式化文本。D3D8 backend 还会做 CPU clip / draw data 提交，因此客户机器上“2 个 BUFF 开始掉帧”是合理的；本地若不是同一 D3D8/分辨率/显卡路径，不能用体感否定。

v23.87 修复方向：

- 新增 `SkillOverlayBridgeGetIndependentBuffOverlayEntriesLite(...)`，D3D8 高频路径只取绘制必需字段，不再每帧格式化 tooltip。
- 面板未展开、无超级按钮、鼠标未悬停 BUFF 时，D3D8 独立 BUFF 图标改走 `RenderIndependentBuffOverlayBarDirectD3D8(...)` 轻量直绘。
- 只有面板展开、超级按钮可见或悬停 tooltip 时，才回到完整 ImGui 路径。
- 新日志标记：`[D3D8BuffFastPath] direct count=...`。

### 17.2 2026-06-05 v23.89 补充：开着超级技能面板释放多 BUFF 的卡顿

客户日志 `SuperSkillWnd (39).log` 的 SHA256：

```text
2BF326415228720EE754A560656D137706023BBD4E1E9A66BCE624C4B5C65569
```

该日志证明 v23.88 已加载且 D3D8 生效：

- `marker=v23.88-2026-06-05-d3d8-buff-fastpath-perf`
- `[D3D8] display mode: 3440x1440 fmt=22`
- 9 个独立 BUFF 被激活：`1111098/1111095/1111092/1111091/1111090/1111085/1111084/1111083/1111082`

关键结论：

- 44 条性能采样全部是 `full-imgui`，没有任何 `direct`。
- 所有采样都是 `panel=1 btn=1 hover=0`，说明客户测试时超级技能面板一直展开。
- 平均 `3.827 ms`，最大 `5.391 ms`，最高 active BUFF 数量为 9。
- 旧 v23.87/v23.88 快路径只覆盖“面板关闭 + 无按钮 + 非 hover”的常驻战斗帧，不能覆盖“打开面板继续点多个 BUFF”的路径。

v23.89 自动收起路线已判定为失败路线并在 v23.90 移除：

- 客户明确拒绝“释放 BUFF 后自动关闭超级技能栏”，该方案只规避症状，不能作为正式修复。
- 客户日志 `SuperSkillWnd (40).log` 的 SHA256：

```text
EE8ED3769BE5D94258035348CB66B548DA75BFED64BA04301F6C9E692B380E48
```

- 该日志证明自动收起后仍有大量 `full-imgui panel=0 btn=1`，原因是超级技能按钮可见时仍强制走 ImGui，BUFF 图标直绘本身很便宜，典型 `direct` 约 `0.24~0.33 ms`。
- 该日志还证明 `BuffPacketProbe` 每个 GIVE/CANCEL BUFF 包打印 `head160` 长 hex，释放多个 BUFF 时会制造额外 IO 抖动。

v23.90 修复方向：

- 删除 `SkillOverlayBridgeConsumeIndependentBuffD3D8PanelCollapseRequest(...)`、`CollapseD3D8SuperPanelForBuffPerf(...)` 和所有 `d3d8_buff_perf / panel-collapse` 调用点。
- 面板关闭时，D3D8 超级技能按钮改走 `RenderOverlaySuperButtonDirectD3D8(...)` 直绘；WndProc 鼠标命中仍沿用 `HandleOverlaySuperButtonMouseEvent(...)`，不改变点击开关语义。
- D3D8 面板关闭且无拖拽/捕获/tooltip 时，`SuperD3D8OverlayRender(...)` 直接画 BUFF 图标和超级按钮，不再因为 `btn=1` 进入 `ImGui_ImplD3D8_NewFrame / ImGui::Render`。
- `BuffPacketProbe` 只在 `SSW_ENABLE_BUFF_PACKET_PROBE_LOGS=1` 时打印，ReleaseLogs 默认关闭，避免 BUFF 包热路径写长日志。
- `SkillLocalDataInitialize()` 在包/缓存加载失败后 fail-closed，不再每 3 秒由技能栏/图标热路径反复重试；运行时密码就绪时仍通过 `InvalidateSkillConfigPackage()` + `SkillLocalDataInvalidate()` 显式重载。
- 新日志关键字：
  - `[D3D8OverlayFastPath] direct panel=0 btn=... buff=...`
  - `[D3D8ButtonFastPath] direct ...`
  - `[D3D8BuffFastPath] direct count=...`

后续验证标准：

- 运行日志 marker 必须是 `v23.90-2026-06-05-d3d8-button-direct-no-autoclose`。
- 面板关闭后不应再反复出现 `full-imgui panel=0 btn=1`；应看到 `D3D8OverlayFastPath direct panel=0 btn=1`。
- 释放多个 BUFF 时默认不应再出现连续的 `[BuffPacketProbe] pre/post ... head160=[...]`。
- 超级技能栏不能因释放 BUFF 自动关闭；如果仍自动关闭，优先判断旧 DLL 没替换。
- 面板展开期间仍会走 ImGui，因为当前 D3D8 面板本体还是 ImGui 渲染；若客户要求“长期展开面板也完全不卡”，下一步需要做 D3D8 面板本体直绘，而不是恢复自动收起。

### 17.3 2026-06-06 v23.91/v23.93：D3D8 整面板 direct 化判定为失败路线

用户反馈 v23.91/v23.93 的 D3D8 direct panel 虽然试图绕开 ImGui，但破坏了旧 UI 视觉契约：

- 自绘鼠标消失或状态不完整。
- 字体、数字、边框、文本间隔、tooltip 格式不再等同旧版。
- tooltip 层级、超级按钮遮挡、初始化确认窗等旧功能出现回归。
- 用户明确表示不接受逐像素重新指导，要求做不到就退回。

结论：

- 卡顿根因不能简单归因成“D3D8ImGuiOverlay 本身一定卡”。
- 客户日志指向的是“超级技能独立 BUFF 激活后，把 BUFF 常驻渲染 / 数据构造放进 D3D8 ImGui 每帧热路径”。
- 正确修复方向是保留旧 ImGui/Retro 面板视觉，只优化 BUFF 热路径；不要再整面板 direct 化。

### 17.4 2026-06-06 v23.95：旧 UI 回退 + 超级 BUFF 快路径

v23.95 当前方案：

- D3D8 面板本体恢复 `D3D8ImGuiOverlay`，继续使用旧 `RenderRetroSkillPanel(...)`、旧字体、旧 tooltip、旧鼠标/拖拽/初始化确认逻辑。
- `SuperD3D8OverlayRender(...)` 常驻帧先调用 `SkillOverlayBridgeGetIndependentBuffOverlayEntriesLite(...)`，只取图标、slot、剩余时间等轻量字段。
- 只有鼠标悬停 BUFF 图标需要 tooltip 时，才通过 `SkillOverlayBridgeGetIndependentBuffOverlayEntries(...)` 补全 name/description/detail。
- 面板未展开、鼠标未悬停 BUFF/超级按钮、无拖拽/鼠标捕获时，进入 `RenderD3D8OverlayFastPath(...)`：
  - BUFF 图标走 `RenderIndependentBuffOverlayBarDirectD3D8(...)`
  - 超级按钮走 `RenderOverlaySuperButtonDirectD3D8(...)`
  - 不进入 `ImGui_ImplD3D8_NewFrame / ImGui::Render / ImGui_ImplD3D8_RenderDrawData`
- 一旦面板展开、hover、tooltip、拖拽或捕获发生，立即回到旧 ImGui UI 路径，优先保证视觉和交互一致。

v23.95 构建/部署证据：

```text
marker = v23.95-2026-06-06-d3d8-imgui-ui-buff-fastpath
ReleaseLogs SHA256 = 1766E56660DBA7FD9B6313FE974E0CADA03B9240C93AFD4952417E260B5188E3
Release     SHA256 = A13013B0D1E8B5105261D7664F7C16111A249CC5829D1D819A300088CDEDD73B
runtime     SHA256 = 1766E56660DBA7FD9B6313FE974E0CADA03B9240C93AFD4952417E260B5188E3
runtime path = G:\code\mxd\Data\Plugins\SS\SS.dll
```

二进制字符串验证：

- `v23.95-2026-06-06-d3d8-imgui-ui-buff-fastpath` 存在。
- `d3d8_ui_rollback=1` 存在。
- `d3d8_buff_fastpath=1` 存在。
- `D3D8ImGuiOverlay` 存在，这是预期：面板 UI 回退旧路径。
- `D3D8OverlayFastPath` 存在，这是预期：BUFF 常驻帧有快路径。
- `D3D8DirectOverlay` / `D3D8OverlayDirectPerf` / `d3d8_direct_no_imgui_backend` 不应存在。

后续客户验证标准：

- 日志 marker 必须是 `v23.95-2026-06-06-d3d8-imgui-ui-buff-fastpath`。
- 打开超级技能栏后，字体/数字/边框/tooltip/鼠标/初始化确认窗应恢复旧版表现。
- 面板关闭、鼠标不悬停时，开多个超级 BUFF 后日志应周期性出现 `[D3D8OverlayFastPath] direct panel=0 ... buff=1 count=...`。
- 鼠标悬停 BUFF 看 tooltip 或展开超级技能栏时，允许短暂回到 ImGui，因为这时优先保证旧 UI。
- 如果客户仍反馈“不开面板只开 BUFF 也爆卡”，优先看是否命中 fast path、BUFF 数量、运行 DLL marker/hash、是否有其它热路径日志/解包重试。

### 17.5 2026-06-07 v23.96：覆盖“双击 BUFF 后面板仍打开”的持续卡顿场景

用户纠正：BUFF 的实际使用方式是双击技能项，使用时一定在面板/技能项上；如果 BUFF 生效后全局持续卡，不能要求用户“不悬停/关面板”。

v23.96 对 v23.95 的补充：

- v23.95 的 `D3D8OverlayFastPath` 只覆盖“面板关闭 + 无 hover/拖拽/捕获”的常驻帧。
- v23.96 保留旧 `D3D8ImGuiOverlay` 面板 UI，但即使面板仍展开，只要不是鼠标悬停 BUFF 图标需要显示 BUFF tooltip，就不再调用 ImGui 版 `RenderIndependentBuffOverlayBar(...)`。
- 面板本体先按旧 ImGui/Retro 画完，随后在同一个 D3D8 scene 内用 `RenderIndependentBuffOverlayBarDirectD3D8(...)` 直绘 BUFF 图标。
- 只有 `hoveredIndependentBuff == true` 时，才走 ImGui BUFF 窗口，以保留旧 tooltip。
- 新日志关键字：`[D3D8OverlayHybrid] imguiPanel=... buffDirect=1 count=... hoverBuff=0`。

v23.96 构建/部署证据：

```text
marker = v23.96-2026-06-07-d3d8-buff-hybrid-direct
ReleaseLogs SHA256 = 2AF787327509AC48093260A3CC0A43D67403700D39EA757F94EA370E102CB70A
Release     SHA256 = F6F885FF987D2306A8F2B38A7165F23958EEC1C0D5C0048A799565485277DFBD
runtime     SHA256 = 2AF787327509AC48093260A3CC0A43D67403700D39EA757F94EA370E102CB70A
runtime path = G:\code\mxd\Data\Plugins\SS\SS.dll
```

二进制字符串验证：

- `v23.96-2026-06-07-d3d8-buff-hybrid-direct` 存在。
- `d3d8_ui_rollback=1` 存在。
- `d3d8_buff_fastpath=1` 存在。
- `D3D8ImGuiOverlay` 存在，这是预期。
- `D3D8OverlayFastPath` 存在，覆盖面板关闭常驻帧。
- `D3D8OverlayHybrid` 存在，覆盖面板打开后的非 BUFF-tooltip 常驻帧。
- `D3D8DirectOverlay` / `D3D8OverlayDirectPerf` / `d3d8_direct_no_imgui_backend` 不应存在。

客户验证标准：

- 双击超级 BUFF 后，即使超级技能栏还开着，也应看到 `[D3D8OverlayHybrid] imguiPanel=1 buffDirect=1 count=... hoverBuff=0`。
- 如果鼠标正好悬停右上角 BUFF 图标看 tooltip，允许临时不走 hybrid；移开 BUFF 图标后应恢复 hybrid。
- 如果 v23.96 仍出现“全局一直卡”，下一步不要再改面板 UI，优先加运行期耗时采样区分：
  - `SkillOverlayBridgeGetIndependentBuffOverlayEntriesLite`
  - `RenderRetroSkillPanel`
  - `RenderIndependentBuffOverlayBarDirectD3D8`
  - 本地 BUFF 属性 hook / status bar hook
  - 包/解包 fail-closed 是否仍在热路径重试

### 17.6 2026-06-07 v23.97：右上角 BUFF hover 也不再回退整条 ImGui BUFF 栏

用户继续指出：如果看右上角 BUFF tooltip 就切回 ImGui BUFF 栏，hover 时仍可能卡。

v23.97 对 v23.96 的补充：

- `RenderIndependentBuffOverlayBar(...)` 不再作为 D3D8 hover BUFF 的整栏回退路径。
- BUFF 图标始终由 `RenderIndependentBuffOverlayBarDirectD3D8(...)` 绘制：
  - 面板关闭：`D3D8OverlayFastPath`
  - 面板打开：`D3D8OverlayHybrid`
  - 鼠标 hover 右上角 BUFF：仍然 direct 绘制图标
- hover 时只调用 `RenderIndependentBuffOverlayTooltipOnly(...)` 补一个 tooltip，不再用 ImGui 绘制整条 BUFF 栏。

v23.97 构建/部署证据：

```text
marker = v23.97-2026-06-07-d3d8-buff-direct-tooltip-only
ReleaseLogs SHA256 = CAE54EAFB13AE8072738ADAA4D9D1AA2B23FB5B892C208EDA9E2F6155BE8AC7F
Release     SHA256 = F5392E4CBD7735353006E0421A7877B1D2DE7CE4CAB7727B9FA4F118C9021F41
runtime     SHA256 = CAE54EAFB13AE8072738ADAA4D9D1AA2B23FB5B892C208EDA9E2F6155BE8AC7F
runtime path = G:\code\mxd\Data\Plugins\SS\SS.dll
```

二进制字符串验证：

- `v23.97-2026-06-07-d3d8-buff-direct-tooltip-only` 存在。
- `D3D8OverlayFastPath` 存在。
- `D3D8OverlayHybrid` 存在。
- `D3D8ImGuiOverlay` 存在，这是预期：面板/tooltip 仍复用旧 UI。
- `D3D8DirectOverlay` / `D3D8OverlayDirectPerf` / `d3d8_direct_no_imgui_backend` 不应存在。

客户验证标准：

- 双击 BUFF 后，面板开着也应出现 `[D3D8OverlayHybrid] ... buffDirect=1 ...`。
- 鼠标移到右上角 BUFF 看 tooltip 时，仍应出现 `buffDirect=1`，只是 `hoverBuff=1`。
- 如果看 tooltip 仍卡，下一步只优化 tooltip 绘制/文本缓存，不再让整条 BUFF 栏回到 ImGui。

### 17.7 2026-06-07 v23.98：不要继续猜 UI，加入 D3D8 overlay 分段耗时日志

用户提出关键质疑：客户电脑性能很好，仍然全局爆卡，不能继续默认一定是 UI 绘制。

v23.98 加入 `D3D8OverlayPerf` 低频分段日志，用于证明或排除 UI：

```text
[D3D8OverlayPerf] path=fast total=... entries=... fast=... count=... btn=...
[D3D8OverlayPerf] path=imgui total=... entries=... imguiFrame=... tooltipBtn=... panel=... cursor=... submit=... end=... count=... panelOn=... hoverBuff=... drawLists=... vertices=...
```

字段含义：

- `entries`：`SkillOverlayBridgeGetIndependentBuffOverlayEntriesLite(...)` 耗时。
- `imguiFrame`：`ImGui_ImplD3D8_NewFrame / ImGui_ImplWin32_NewFrame / ImGui::NewFrame`。
- `tooltipBtn`：BUFF tooltip-only + 超级按钮绘制。
- `panel`：`RenderRetroSkillPanel(...)` 和 quick slot 同步。
- `cursor`：旧自绘鼠标构造/绘制。
- `submit`：`ImGui::Render / ImGui_ImplD3D8_RenderDrawData` 以及 direct BUFF 绘制。
- `end`：`EndScene` 耗时。

判定：

- 如果 `total/submit/panel` 长期很高（例如明显多毫秒且跟卡顿同步），继续查 UI/D3D8。
- 如果这些值都很低，但客户仍全局卡，说明卡点不在 overlay 绘制，下一步转查：
  - 本地 BUFF 属性 hook
  - status bar BUFF slot hook / refresh
  - independent buff activate/deactivate 状态同步
  - 包/解包 fail-closed 是否仍在热路径
  - 是否有 first-chance exception / D3D reset 循环

v23.98 构建/部署证据：

```text
marker = v23.98-2026-06-07-d3d8-buff-perf-probe
ReleaseLogs SHA256 = DF947458C852461EC1D213036539EC6173E4ED5E33E9EE112AFDBB0B9140B660
Release     SHA256 = C1BDE87D56C49134D2740F58EB125871A25B04DC884E3B24A654843F8E662071
runtime     SHA256 = DF947458C852461EC1D213036539EC6173E4ED5E33E9EE112AFDBB0B9140B660
runtime path = G:\code\mxd\Data\Plugins\SS\SS.dll
```

---

## 18. 日志关键字速查

D3D8 模式排查时搜索：
- `[D3D] Detected D3D8 mode` — 模式检测
- `[D3D8] Setup` — hook 安装开始
- `[D3D8] display mode:` — 显示模式查询结果
- `[D3D8] dummy device creation failed` — dummy 设备创建失败
- `[D3D8] Present inline hooked` — Present hook 安装成功
- `[D3D8] Hook setup complete` — 全部 hook 安装完成
- `[D3D8] first Present: hwnd=` — 首帧获取窗口句柄
- `[D3D8-Tex] loaded` — 纹理加载结果
- `[D3D8-Present] SkillWnd:` — 技能窗口指针获取
- `[D3D8-Present] Ready` — 初始化就绪
- `[D3D8] Reset called` — 设备重置事件
- `[Toggle:d3d_btn] expanded=` — 面板开关状态
- `[Toggle] D3D8 mode: panel ON/OFF` — 面板切换
- `[Build] marker=v20.1-*` — 版本确认
- `D3D8 hook FAILED` — 整体失败

---

## 19. D3D8 vs D3D9 关键差异速查

| 特性 | D3D8 | D3D9 |
|------|------|------|
| SetFVF | 不存在，用 SetVertexShader(FVF) | 存在 |
| SetSamplerState | 不存在，通过 SetTextureStageState 设置 | 存在 |
| StateBlock API | CreateStateBlock(56)/ApplyStateBlock(54)/CaptureStateBlock(55)/DeleteStateBlock(56) | 不同 API |
| CreateTexture 参数 | 无 pSharedHandle | 有 pSharedHandle |
| D3DPRESENT_PARAMETERS | 无 MultiSampleQuality | 有 MultiSampleQuality |
| BackBufferFormat | 不接受 D3DFMT_UNKNOWN | 接受 |
| GetVertexShader | vtable[77]，返回 FVF 或 shader handle | 不同位置 |
| TSS 采样器 | TSS_MINFILTER=17, TSS_MAGFILTER=16, TSS_ADDRESSU=13, TSS_ADDRESSV=14 | 独立 SamplerState API |
