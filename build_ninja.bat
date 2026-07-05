@echo off
setlocal enabledelayedexpansion

rem ---------------------------------------------------------------------------
rem build_ninja.bat — local Ninja build for this machine.
rem
rem This PC has a partial MSVC/Windows SDK install: VsDevCmd does not export the
rem standard INCLUDE/LIB paths, and rc.exe / mt.exe (resource compiler and
rem manifest tool) are missing entirely. build.bat targets the VS 2019 generator
rem for portability; this script uses the Ninja "default" preset plus the
rem workarounds those missing tools require.
rem
rem Both scripts share the build\ directory and swap the CMake cache when the
rem generator differs, so alternating between them just triggers a reconfigure.
rem ---------------------------------------------------------------------------

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
    if errorlevel 1 (
        echo Removing non-Ninja build cache; switching to Ninja...
        rmdir /s /q build
        set "NEED_CONFIGURE=1"
    )
)

if !NEED_CONFIGURE!==1 (
    rem Extra flags work around the missing rc.exe / mt.exe:
    rem   - probe with a static library so the compiler check does not link an exe
    rem   - skip manifest embedding on every linked target
    cmake --preset default ^
        -DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY ^
        "-DCMAKE_EXE_LINKER_FLAGS=/MANIFEST:NO" ^
        "-DCMAKE_SHARED_LINKER_FLAGS=/MANIFEST:NO" ^
        "-DCMAKE_MODULE_LINKER_FLAGS=/MANIFEST:NO"
    if errorlevel 1 exit /b 1
)

cmake --build build
if errorlevel 1 exit /b 1

echo.
echo  Build complete — server\rhapsode\_core.pyd ready.
