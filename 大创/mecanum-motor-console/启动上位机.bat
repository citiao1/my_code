@echo off
setlocal
cd /d "%~dp0"

echo ========================================
echo   C30D Web Console and VOFA Bridge
echo ========================================
echo   [1] USB serial bridge
echo   [2] BLE bridge
echo   [Q] Quit
echo.
choice /c 12Q /n /m "Select connection: "

if errorlevel 3 exit /b 0
if errorlevel 2 goto use_ble
set "BRIDGE_TRANSPORT=serial"
goto launch_bridge

:use_ble
set "BRIDGE_TRANSPORT=ble"

:launch_bridge
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0bridge\start-bridge.ps1" -Transport "%BRIDGE_TRANSPORT%"
set "BRIDGE_EXIT=%ERRORLEVEL%"
echo.
if not "%BRIDGE_EXIT%"=="0" echo Startup failed. See the error above.
if "%BRIDGE_EXIT%"=="0" echo Bridge stopped.
pause
endlocal & exit /b %BRIDGE_EXIT%
