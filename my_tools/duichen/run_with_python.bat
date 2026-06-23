@echo off
cd /d "%~dp0"
python symmetry_tool.py --gui
if errorlevel 1 (
  echo.
  echo Failed to start. Please install Python 3 and run:
  echo pip install pillow
)
pause
