@echo off
setlocal enabledelayedexpansion

rem Downloads pre-built llama.cpp binary (CUDA 12) from GitHub releases.
rem Building from source requires MSVC 19.29+ (VS 2019 16.11+).

set "RELEASE=b7592"
set "DEST=third_party\llama.cpp\build\bin\Release"
set "URL=https://github.com/ggml-org/llama.cpp/releases/download/%RELEASE%/llama-%RELEASE%-bin-win-cuda-12.4-x64.zip"
set "ZIP=%TEMP%\llama-cuda-%RELEASE%.zip"

if exist "%DEST%\llama-server.exe" (
    echo  llama-server.exe already present in %DEST%
    echo  Delete it to re-download.
    exit /b 0
)

echo Downloading llama.cpp %RELEASE% (CUDA 12, Windows x64)...
curl -L -o "%ZIP%" "%URL%"
if errorlevel 1 (
    echo  Download failed.
    exit /b 1
)

if not exist "%DEST%" mkdir "%DEST%"
echo Extracting to %DEST% ...
tar -xf "%ZIP%" -C "%DEST%"
if errorlevel 1 (
    echo  Extraction failed.
    exit /b 1
)

del "%ZIP%" 2>nul
echo.
echo  llama.cpp %RELEASE% ready: %DEST%\llama-server.exe
