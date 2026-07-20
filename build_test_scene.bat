@echo off
setlocal enabledelayedexpansion

call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\Common7\Tools\VsDevCmd.bat" -arch=amd64

set "MSVC_ROOT=C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\VC\Tools\MSVC\14.28.29910"
set "WINSDK=C:\Program Files (x86)\Windows Kits\10"
set "WINSDK_VER=10.0.26100.0"

set "INCLUDE=!MSVC_ROOT!\include;!WINSDK!\include\!WINSDK_VER!\ucrt;!WINSDK!\include\!WINSDK_VER!\shared;!WINSDK!\include\!WINSDK_VER!\um;!WINSDK!\include\!WINSDK_VER!\winrt;!WINSDK!\include\!WINSDK_VER!\cppwinrt"
set "LIB=!MSVC_ROOT!\lib\x64;!WINSDK!\lib\!WINSDK_VER!\ucrt\x64;!WINSDK!\lib\!WINSDK_VER!\um\x64"

if not exist build\CMakeCache.txt (
    cmake --preset msvc2019
    if errorlevel 1 exit /b 1
)

cmake --build build --config Release --target test_scene
if errorlevel 1 exit /b 1

echo.
echo === Running test_scene ===
ctest --test-dir build --build-config Release --output-on-failure %*
if errorlevel 1 exit /b 1
