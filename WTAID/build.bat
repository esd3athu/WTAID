@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
echo Setting up build...

REM Create obj directory
if not exist "build\obj" mkdir "build\obj"

cl /std:c++20 /utf-8 /EHsc /I src /I include /Fobuild\obj\ /Fe:WTAID.exe src/main.cpp src/network/HTTPClient.cpp src/data/DataParser.cpp src/data/IndicatorsParser.cpp src/data/TelemetryProcessor.cpp src/fmdb/FMDatabase.cpp src/indicators/Indicator.cpp src/indicators/IndicatorRegistry.cpp src/indicators/AlertSystem.cpp src/indicators/BuiltinIndicators.cpp src/ui/GUI.cpp src/config/ConfigManager.cpp src/ui/ProfileDialog.cpp src/ui/ColorPickerDialog.cpp src/ui/DataOutputWindow.cpp src/ui/IndicatorSelectorDialog.cpp src/overlay/OverlayRenderer.cpp src/overlay/OverlayWindow.cpp /link /SUBSYSTEM:WINDOWS winhttp.lib comctl32.lib user32.lib gdi32.lib shell32.lib comdlg32.lib d3d11.lib d2d1.lib dwrite.lib dxgi.lib
if %ERRORLEVEL% EQU 0 (
    echo.
    echo ========================================
    echo Build SUCCESS! WTAID.exe created.
    echo ========================================
    dir WTAID.exe
) else (
    echo.
    echo Build FAILED with error code: %ERRORLEVEL%
)
