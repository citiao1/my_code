@echo off
setlocal
cd /d "%~dp0relay-server"
if not exist node_modules (
  npm install
)
npm start
