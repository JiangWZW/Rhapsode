@echo off
setlocal enabledelayedexpansion

call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\Common7\Tools\VsDevCmd.bat" -arch=amd64

if not exist "third_party\boost\boost\graph\adjacency_list.hpp" (
    call "third_party\bootstrap_boost.bat"
    if errorlevel 1 exit /b 1
)

rem VsDevCmd does not add MSVC lib/include on this machine — fix manually
set "MSVC_ROOT=C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\VC\Tools\MSVC\14.28.29910"
set "WINSDK=C:\Program Files (x86)\Windows Kits\10"
set "WINSDK_VER=10.0.26100.0"

set "INCLUDE=!MSVC_ROOT!\include;!WINSDK!\include\!WINSDK_VER!\ucrt;!WINSDK!\include\!WINSDK_VER!\shared;!WINSDK!\include\!WINSDK_VER!\um;!WINSDK!\include\!WINSDK_VER!\winrt;!WINSDK!\include\!WINSDK_VER!\cppwinrt"
set "LIB=!MSVC_ROOT!\lib\x64;!WINSDK!\lib\!WINSDK_VER!\ucrt\x64;!WINSDK!\lib\!WINSDK_VER!\um\x64"

set "NEED_CONFIGURE=0"
if not exist build\CMakeCache.txt (
    set "NEED_CONFIGURE=1"
) else (
    findstr /C:"CMAKE_GENERATOR:INTERNAL=Ninja" build\CMakeCache.txt >nul 2>&1
    if not errorlevel 1 (
        echo Removing Ninja build cache; switching to MSVC...
        rmdir /s /q build
        set "NEED_CONFIGURE=1"
    )
)

if !NEED_CONFIGURE!==1 (
    cmake --preset msvc2019
    if errorlevel 1 exit /b 1
)

cmake --build build --config Release
if errorlevel 1 exit /b 1

echo.
echo  Build complete — server\rhapsode\_core.pyd ready.
