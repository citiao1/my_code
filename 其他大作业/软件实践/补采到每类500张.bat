@echo off
chcp 65001 >nul
cd /d "%~dp0"
"D:\python_envs\software_practice_py311\Scripts\python.exe" "tf_rps_gesture_classifier.py" collect --class-name all --target-count 500
if errorlevel 1 (
    echo.
    echo 程序运行失败，请把上面的报错截图发给我。
    pause
)
