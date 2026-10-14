@echo off
chcp 65001 >nul
set BASE_DIR=D:\桌面\My_ele_contest2\LQ_MSPM0GX_LIB_V2.0.0\Keil
set LOG=%BASE_DIR%\build_check.log
set PROJ=%BASE_DIR%\LQ_MSPM0GX_LIB.uvprojx
"D:\keil5\UV4\UV4.exe" -r "%PROJ%" -j0 -o "%LOG%"
echo ExitCode: %ERRORLEVEL%
