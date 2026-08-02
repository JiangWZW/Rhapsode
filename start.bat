@echo off
setlocal enabledelayedexpansion
title Rhapsode
set "PATH=C:\Program Files\Graphviz\bin;C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.6\bin;%PATH%"

rem Load server/.env (if present) without overriding existing env vars.
if exist "%~dp0server\.env" (
    for /f "usebackq eol=# tokens=1,* delims==" %%a in ("%~dp0server\.env") do (
        if not defined %%a set "%%a=%%b"
    )
)

if not defined RHAPSODE_HOST set "RHAPSODE_HOST=127.0.0.1"
if not defined RHAPSODE_PORT set "RHAPSODE_PORT=8080"

set /p "VERBOSE_CHOICE=Enable verbose C++ logging? [Y/N]: "
if /i "%VERBOSE_CHOICE%"=="Y" (
    set "RHAPSODE_VERBOSE_LOG=1"
    echo  Verbose C++ logging: ON
) else (
    echo  Verbose C++ logging: OFF
)
echo.

cd /d "%~dp0server"
start "Rhapsode Backend" .venv\Scripts\python.exe -m uvicorn rhapsode.app:app --host !RHAPSODE_HOST! --port !RHAPSODE_PORT!

cd /d "%~dp0frontend"
start "Rhapsode Frontend" npx vite --host 127.0.0.1 --port 5173

timeout /t 5 /nobreak >nul
echo.
echo  Rhapsode is running:
echo    Backend:  http://!RHAPSODE_HOST!:!RHAPSODE_PORT!
echo    Frontend: http://127.0.0.1:5173
echo.
echo  Press any key to stop all servers...
pause >nul

taskkill /fi "windowtitle eq Rhapsode Backend*" /t /f >nul 2>&1
taskkill /fi "windowtitle eq Rhapsode Frontend*" /t /f >nul 2>&1
echo  Stopped.
