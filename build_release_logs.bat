@echo off
setlocal

cd /d "%~dp0"

set "VCVARS32=C:\Program Files\Microsoft Visual Studio\18\Professional\VC\Auxiliary\Build\vcvars32.bat"
if not exist "%VCVARS32%" (
    echo ERROR: vcvars32.bat not found: "%VCVARS32%"
    exit /b 1
)

where /Q cl
if errorlevel 1 (
    set "PATH=%SystemRoot%\System32;%SystemRoot%;%SystemRoot%\System32\Wbem;%SystemRoot%\System32\WindowsPowerShell\v1.0\"
    set "INCLUDE="
    set "LIB="
    set "LIBPATH="
    call "%VCVARS32%"
    if errorlevel 1 (
        echo ERROR: vcvars32.bat failed!
        exit /b 1
    )
    echo === vcvars32 OK ===
) else (
    echo === using existing MSVC environment ===
)

if not exist "build\ReleaseLogs" mkdir "build\ReleaseLogs"
del /Q "build\ReleaseLogs\*.obj" "build\ReleaseLogs\*.res" "build\ReleaseLogs\*.rsp" "build\ReleaseLogs\SS.dll" "build\ReleaseLogs\hook.dll" "build\ReleaseLogs\*.pdb" "build\ReleaseLogs\*.ilk" 2>nul

echo [0/4] Skipping Reader runtime publish (json/local package mode) ...

echo [1/4] Compiling resources ...
rc /nologo /fo"build\ReleaseLogs\resource.res" src\resource.rc
if errorlevel 1 (
    echo ERROR: Resource compilation failed!
    exit /b 1
)

> "build\ReleaseLogs\compile.rsp" (
    echo /nologo
    echo /O2
    echo /Ob2
    echo /Oi
    echo /Gy
    echo /Gw
    echo /MD
    echo /utf-8
    echo /EHsc
    echo /DNDEBUG
    echo /DSSW_ENABLE_RUNTIME_LOGS=1
    echo /DSSW_ENABLE_DIAGNOSTIC_LOGS=0
    echo /I"src"
    echo /I"src\third_party\imgui"
    echo /I"src\third_party\imgui\backends"
    echo /c
    echo /Fo"build\ReleaseLogs\\"
    echo src\dllmain.cpp
    echo src\hook\win32_input_spoof.cpp
    echo src\runtime\feature_switches.cpp
    echo src\runtime\crash_capture.cpp
    echo src\runtime\init_pipeline.cpp
    echo src\runtime\cleanup_pipeline.cpp
    echo src\util\runtime_paths.cpp
    echo src\util\skill_config_package.cpp
    echo src\skill\skill_local_data.cpp
    echo src\skill\skill_packet_rewrite_router.cpp
    echo src\skill\skill_overlay_source.cpp
    echo src\skill\skill_overlay_source_manager.cpp
    echo src\skill\skill_overlay_source_game.cpp
    echo src\skill\skill_overlay_bridge.cpp
    echo src\ui\retro_skill_app.cpp
    echo src\ui\retro_skill_assets.cpp
    echo src\ui\retro_render_backend.cpp
    echo src\ui\retro_skill_panel.cpp
    echo src\ui\retro_skill_state.cpp
    echo src\ui\retro_skill_text_dwrite.cpp
    echo src\ui\overlay_input_utils.cpp
    echo src\ui\overlay_cursor_utils.cpp
    echo src\ui\overlay_style_utils.cpp
    echo src\ui\super_imgui_overlay.cpp
    echo src\ui\super_imgui_overlay_d3d8.cpp
    echo src\d3d8\d3d8_renderer.cpp
    echo src\third_party\imgui\imgui.cpp
    echo src\third_party\imgui\imgui_draw.cpp
    echo src\third_party\imgui\imgui_tables.cpp
    echo src\third_party\imgui\imgui_widgets.cpp
    echo src\third_party\imgui\backends\imgui_impl_dx9.cpp
    echo src\third_party\imgui\backends\imgui_impl_d3d8.cpp
    echo src\third_party\imgui\backends\imgui_impl_win32.cpp
)

echo [2/4] Compiling sources ...
cl @"build\ReleaseLogs\compile.rsp"
if errorlevel 1 (
    echo ERROR: Compilation failed!
    exit /b 1
)

> "build\ReleaseLogs\link.rsp" (
    echo /nologo
    echo /DLL
    echo /DEBUG:NONE
    echo /INCREMENTAL:NO
    echo /OPT:REF
    echo /OPT:ICF
    echo /RELEASE
    echo /OUT:"build\ReleaseLogs\SS.dll"
    echo build\ReleaseLogs\dllmain.obj
    echo build\ReleaseLogs\win32_input_spoof.obj
    echo build\ReleaseLogs\feature_switches.obj
    echo build\ReleaseLogs\crash_capture.obj
    echo build\ReleaseLogs\init_pipeline.obj
    echo build\ReleaseLogs\cleanup_pipeline.obj
    echo build\ReleaseLogs\runtime_paths.obj
    echo build\ReleaseLogs\skill_config_package.obj
    echo build\ReleaseLogs\skill_local_data.obj
    echo build\ReleaseLogs\skill_packet_rewrite_router.obj
    echo build\ReleaseLogs\skill_overlay_source.obj
    echo build\ReleaseLogs\skill_overlay_source_manager.obj
    echo build\ReleaseLogs\skill_overlay_source_game.obj
    echo build\ReleaseLogs\skill_overlay_bridge.obj
    echo build\ReleaseLogs\retro_skill_app.obj
    echo build\ReleaseLogs\retro_skill_assets.obj
    echo build\ReleaseLogs\retro_render_backend.obj
    echo build\ReleaseLogs\retro_skill_panel.obj
    echo build\ReleaseLogs\retro_skill_state.obj
    echo build\ReleaseLogs\retro_skill_text_dwrite.obj
    echo build\ReleaseLogs\overlay_input_utils.obj
    echo build\ReleaseLogs\overlay_cursor_utils.obj
    echo build\ReleaseLogs\overlay_style_utils.obj
    echo build\ReleaseLogs\super_imgui_overlay.obj
    echo build\ReleaseLogs\super_imgui_overlay_d3d8.obj
    echo build\ReleaseLogs\d3d8_renderer.obj
    echo build\ReleaseLogs\imgui.obj
    echo build\ReleaseLogs\imgui_draw.obj
    echo build\ReleaseLogs\imgui_tables.obj
    echo build\ReleaseLogs\imgui_widgets.obj
    echo build\ReleaseLogs\imgui_impl_dx9.obj
    echo build\ReleaseLogs\imgui_impl_d3d8.obj
    echo build\ReleaseLogs\imgui_impl_win32.obj
    echo build\ReleaseLogs\resource.res
    echo d3d9.lib
    echo user32.lib
    echo gdi32.lib
    echo shell32.lib
    echo ole32.lib
    echo oleaut32.lib
    echo crypt32.lib
)

echo [3/4] Linking SuperSkillWnd release-log DLL ...
link @"build\ReleaseLogs\link.rsp"
if errorlevel 1 (
    echo ERROR: Link failed!
    exit /b 1
)

powershell -NoProfile -ExecutionPolicy Bypass -File "tools\strip_pe_debug_directory.ps1" "build\ReleaseLogs\SS.dll"
if errorlevel 1 (
    echo ERROR: PE debug directory strip failed!
    exit /b 1
)

copy /Y "build\ReleaseLogs\SS.dll" "build\ReleaseLogs\hook.dll" >nul

echo.
echo [4/4] Release log build complete.
echo === RELEASE LOG BUILD OK: build\ReleaseLogs\SS.dll ===
endlocal
