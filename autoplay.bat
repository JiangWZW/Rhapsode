@echo off
setlocal enabledelayedexpansion
title Rhapsode Auto-play

rem =============================================================================
rem  Interactive auto-play. Press Enter to keep each default.
rem  Starts the backend if needed, then runs the eval pipeline.
rem  Parallel: another window is fine — use a different output folder name.
rem  Deeper player defaults: experiments\session_pipeline\config.toml
rem  Dispatch: wiki/architecture/session-eval.md
rem =============================================================================

set "DEF_TURNS=3"
set "DEF_GUIDE=guides\default.md"
set "DEF_OUT_NAME="
set "DEF_CRITIQUE=0"
set "DEF_HOST=127.0.0.1"
set "DEF_PORT=8080"

set "ROOT=%~dp0"
set "PIPE=%ROOT%experiments\session_pipeline"
set "PY=%ROOT%server\.venv\Scripts\python.exe"
set "NO_PROXY=127.0.0.1,localhost"
set "no_proxy=127.0.0.1,localhost"
set "PATH=C:\Program Files\Graphviz\bin;%PATH%"

if not exist "%PY%" (
    echo ERROR: missing venv python:
    echo   %PY%
    goto :end_fail
)
if not exist "%PIPE%\run.py" (
    echo ERROR: missing %PIPE%\run.py
    goto :end_fail
)

echo.
echo  Rhapsode Auto-play — press Enter to accept the default in [brackets]
echo.

set /p "TURNS=Turns [%DEF_TURNS%]: "
if "%TURNS%"=="" set "TURNS=%DEF_TURNS%"

echo.
echo  Choose a player guide (brief that steers the auto-player):
set "GCOUNT=0"
set "DEF_GNUM=1"
for %%F in ("%PIPE%\guides\*.md") do (
    set /a GCOUNT+=1
    set "G!GCOUNT!=guides\%%~nxF"
    echo   !GCOUNT!. %%~nxF
    if /i "guides\%%~nxF"=="%DEF_GUIDE%" set "DEF_GNUM=!GCOUNT!"
)
set /a NONE_NUM=GCOUNT+1
echo   !NONE_NUM!. none — no experiment brief
echo.
set /p "GCHOICE=Guide number [%DEF_GNUM%]: "
if "%GCHOICE%"=="" set "GCHOICE=%DEF_GNUM%"
if "%GCHOICE%"=="!NONE_NUM!" (
    set "GUIDE="
) else (
    call set "GUIDE=%%G!GCHOICE!%%"
    if "!GUIDE!"=="" (
        echo  Invalid choice — using default %DEF_GUIDE%
        set "GUIDE=%DEF_GUIDE%"
    )
)

echo.
set /p "OUT_NAME=Output folder name under runs\ [timestamp]: "
if "%OUT_NAME%"=="" set "OUT_NAME=%DEF_OUT_NAME%"

set /p "CRITIQUE=LLM critique in report? 0/1 [%DEF_CRITIQUE%]: "
if "%CRITIQUE%"=="" set "CRITIQUE=%DEF_CRITIQUE%"

set /p "HOST=Host [%DEF_HOST%]: "
if "%HOST%"=="" set "HOST=%DEF_HOST%"

set /p "PORT=Port [%DEF_PORT%]: "
if "%PORT%"=="" set "PORT=%DEF_PORT%"

if "%OUT_NAME%"=="" (
    for /f %%i in ('powershell -NoProfile -Command "Get-Date -Format yyyyMMdd-HHmmss"') do set "OUT_NAME=%%i"
)
set "OUT_DIR=%PIPE%\runs\%OUT_NAME%"

echo.
echo  --------------------------------------------
echo  turns:    %TURNS%
echo  guide:    %GUIDE%
echo  host:     %HOST%:%PORT%
echo  out:      %OUT_DIR%
echo  critique: %CRITIQUE%
echo  --------------------------------------------
set /p "GO=Start pipeline? [Y/n]: "
if /i "%GO%"=="n" (
    echo Cancelled.
    goto :end_fail
)

set "GUIDE_ARGS="
if not "%GUIDE%"=="" set "GUIDE_ARGS=--guide %GUIDE%"

set "CRITIQUE_ARGS="
if "%CRITIQUE%"=="1" set "CRITIQUE_ARGS=--critique"

echo.
echo  Spawning backend via run.py (stdout → %OUT_DIR%\console.log) ...
cd /d "%PIPE%"
"%PY%" run.py --turns %TURNS% --host %HOST% --port %PORT% --out-dir "%OUT_DIR%" %GUIDE_ARGS% %CRITIQUE_ARGS%
set "EC=!ERRORLEVEL!"

echo.
if !EC!==0 (
    echo  Done. Report: %OUT_DIR%\report.md
    echo  Server log:   %OUT_DIR%\console.log
    echo  LLM profile:  %OUT_DIR%\llm_profile.jsonl
) else (
    echo  Finished with exit code !EC!. See %OUT_DIR%\report.md / console.log / llm_profile.jsonl
)
echo.
pause
exit /b !EC!

:end_fail
echo.
pause
exit /b 1
