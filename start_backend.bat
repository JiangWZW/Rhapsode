@echo off
title Rhapsode Backend
set "PATH=C:\Program Files\Graphviz\bin;%PATH%"

rem Never route localhost LLM calls through an ambient proxy (empty-body 502s).
set "NO_PROXY=127.0.0.1,localhost"
set "no_proxy=127.0.0.1,localhost"
cd /d "%~dp0server"

rem --- Per-session diagnostics log -------------------------------------------
rem Captures the full runtime output (Python logging AND the C++ pipeline logs,
rem which both go to stderr) to a timestamped file while still showing it live.
rem Set RHAPSODE_VERBOSE_LOG=1 before launching for full narrator prompts/responses.
if not exist logs mkdir logs
for /f %%i in ('powershell -NoProfile -Command "Get-Date -Format yyyyMMdd_HHmmss"') do set "STAMP=%%i"
set "LOGFILE=logs\session_%STAMP%.log"
echo Logging this session to: %CD%\%LOGFILE%
echo.

powershell -NoProfile -Command "$ErrorActionPreference='Continue'; & '%~dp0server\.venv\Scripts\python.exe' -m uvicorn rhapsode.app:app --host 127.0.0.1 --port 8080 2>&1 | Tee-Object -FilePath '%LOGFILE%'"
pause
