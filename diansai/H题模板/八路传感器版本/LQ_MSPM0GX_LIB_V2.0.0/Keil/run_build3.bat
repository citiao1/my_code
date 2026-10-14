@echo off
chcp 65001 >nul
set BASE_DIR=d:\桌面\My_ele_contest2\LQ_MSPM0GX_LIB_V2.0.0\Keil
set LOG=%BASE_DIR%\build_log.txt
"D:\keil5\UV4\UV4.exe" -r "%BASE_DIR%\LQ_MSPM0GX_LIB.uvprojx" -j0 > "%LOG%" 2>&1
echo ExitCode: %ERRORLEVEL%
type "%LOG%"
