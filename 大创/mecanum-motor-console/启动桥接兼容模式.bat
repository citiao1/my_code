@echo off
setlocal
cd /d "%~dp0"
echo LQ208 bridge compatibility mode - 115200 8N1
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0bridge\start-bridge.ps1" -Transport serial -Baud 115200 %*
pause
endlocal
