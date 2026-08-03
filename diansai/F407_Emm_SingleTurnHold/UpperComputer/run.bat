@echo off
cd /d "%~dp0"
python motor_control.py
if errorlevel 1 pause
