@echo off
title Rhapsode Backend
set "PATH=C:\Program Files\Graphviz\bin;%PATH%"
cd /d "%~dp0server"
"%~dp0server\.venv\Scripts\python.exe" -m uvicorn rhapsode.app:app --host 127.0.0.1 --port 8080
pause
