@echo off
setlocal
cd /d "%~dp0"

echo ========================================
echo   Pure BLE Speed Loop Console
echo ========================================
echo.
set "BLE_DEVICE="
set /p "BLE_DEVICE=BLE device name/address (optional, Enter to scan): "
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0bridge\start-bridge.ps1" -Transport ble -Device "%BLE_DEVICE%"
set "BRIDGE_EXIT=%ERRORLEVEL%"
echo.
if not "%BRIDGE_EXIT%"=="0" echo Startup failed. See the error above.
if "%BRIDGE_EXIT%"=="0" echo Bridge stopped.
pause
endlocal & exit /b %BRIDGE_EXIT%
