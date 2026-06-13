@echo off
title Rhapsode Frontend
cd /d "%~dp0frontend"
npx vite --host 127.0.0.1 --port 5173
pause
