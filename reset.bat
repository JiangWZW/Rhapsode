@echo off
setlocal enabledelayedexpansion

rem ============================================================
rem  Rhapsode: Reset a scenario (clear save + its Chroma data)
rem  Usage:  reset.bat [scenario_name]
rem  Default: clears ALL scenarios
rem ============================================================

set "SERVER_DIR=%~dp0server"
set "SAVES_DIR=%SERVER_DIR%\saves"
set "CHROMA_DIR=%SERVER_DIR%\chroma"

rem A Story save is a set of files in SAVES_DIR: story.json (manifest),
rem world.json (shared durable state), and one <scene_id>.json blob per live
rem storyline (the root plus any forks, e.g. konosuba_f0_0.json). A saves dir
rem holds a single playthrough, so a scenario reset clears all of them.

if "%~1"=="" (
    echo Clearing ALL saves and embedding data...
    echo.

    if exist "%SAVES_DIR%" (
        del /q "%SAVES_DIR%\*.json" 2>nul
        echo   Deleted all saves in %SAVES_DIR%
    ) else (
        echo   No saves directory found.
    )

    if exist "%CHROMA_DIR%" (
        rmdir /s /q "%CHROMA_DIR%"
        echo   Deleted chroma directory: %CHROMA_DIR%
    ) else (
        echo   No chroma directory found.
    )
) else (
    echo Clearing scenario: %~1
    echo.

    if exist "%SAVES_DIR%" (
        del /q "%SAVES_DIR%\story.json" 2>nul
        del /q "%SAVES_DIR%\world.json" 2>nul
        del /q "%SAVES_DIR%\%~1*.json" 2>nul
        echo   Deleted saves for %~1 -- manifest, world, and scene blobs
    ) else (
        echo   No saves directory found.
    )

    if exist "%CHROMA_DIR%" (
        "%SERVER_DIR%\.venv\Scripts\python.exe" "%SERVER_DIR%\reset_chroma.py" "%CHROMA_DIR%" "%~1"
    ) else (
        echo   No chroma directory found.
    )
)

echo.
echo Done. Next run will start fresh.
