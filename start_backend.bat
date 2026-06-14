@echo off
title Rhapsode Backend
set "PATH=C:\Program Files\Graphviz\bin;%PATH%"

rem Never route localhost LLM calls through an ambient proxy (empty-body 502s).
set "NO_PROXY=127.0.0.1,localhost"
set "no_proxy=127.0.0.1,localhost"
cd /d "%~dp0server"
"%~dp0server\.venv\Scripts\python.exe" -m uvicorn rhapsode.app:app --host 127.0.0.1 --port 8080
pause
