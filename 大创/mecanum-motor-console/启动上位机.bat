@echo off
setlocal
cd /d "%~dp0"
echo LQ208 serial console - 115200 8N1
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0bridge\start-bridge.ps1" -Transport serial -Baud 115200 %*
set "BRIDGE_EXIT=%ERRORLEVEL%"
if not "%BRIDGE_EXIT%"=="0" echo Startup failed. See the error above.
pause
endlocal & exit /b %BRIDGE_EXIT%
