@echo off
setlocal
cd /d "%~dp0"
echo LQ208 direct Web Serial console - no serial bridge
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0bridge\start-serial-console.ps1" %*
set "BRIDGE_EXIT=%ERRORLEVEL%"
if not "%BRIDGE_EXIT%"=="0" echo Startup failed. See the error above.
pause
endlocal & exit /b %BRIDGE_EXIT%
