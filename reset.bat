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

    set "SAVE_FILE=%SAVES_DIR%\%~1.json"
    if exist "!SAVE_FILE!" (
        del "!SAVE_FILE!"
        echo   Deleted save: !SAVE_FILE!
    ) else (
        echo   No save found: !SAVE_FILE!
    )

    rem Delete matching ChromaDB collections via Python
    if exist "%CHROMA_DIR%" (
        "%SERVER_DIR%\.venv\Scripts\python.exe" -c "import chromadb, sys; c=chromadb.PersistentClient(path=r'%CHROMA_DIR%'); scene_id=sys.argv[1]; deleted=[]; [deleted.append(col.name) or c.delete_collection(col.name) for col in c.list_collections() if col.name.startswith(scene_id)]; print(f'  Deleted {len(deleted)} collection(s): {deleted}' if deleted else '  No matching collections found.')" "%~1"
    ) else (
        echo   No chroma directory found.
    )
)

echo.
echo Done. Next run will start fresh.
