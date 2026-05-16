@echo off
setlocal enabledelayedexpansion

call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\Common7\Tools\VsDevCmd.bat" -arch=amd64

if not exist "third_party\boost\boost\graph\adjacency_list.hpp" (
    call "third_party\bootstrap_boost.bat"
    if errorlevel 1 exit /b 1
)

rem VsDevCmd does not add MSVC lib/include on this machine ??fix manually
set "MSVC_ROOT=C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\VC\Tools\MSVC\14.28.29910"
set "WINSDK=C:\Program Files (x86)\Windows Kits\10"
set "WINSDK_VER=10.0.26100.0"

set "INCLUDE=!MSVC_ROOT!\include;!WINSDK!\include\!WINSDK_VER!\ucrt;!WINSDK!\include\!WINSDK_VER!\shared;!WINSDK!\include\!WINSDK_VER!\um;!WINSDK!\include\!WINSDK_VER!\winrt;!WINSDK!\include\!WINSDK_VER!\cppwinrt"
set "LIB=!MSVC_ROOT!\lib\x64;!WINSDK!\lib\!WINSDK_VER!\ucrt\x64;!WINSDK!\lib\!WINSDK_VER!\um\x64"

if not exist build (
    cmake --preset default
    if errorlevel 1 exit /b 1
)

cmake --build build
if errorlevel 1 exit /b 1

echo.
echo  Build complete ??server\rhapsode\_core.pyd ready.

