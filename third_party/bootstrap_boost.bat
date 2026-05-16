@echo off
setlocal enabledelayedexpansion

set "ROOT_DIR=%~dp0"
set "BOOST_VERSION=1.86.0"
set "BOOST_DIR=%ROOT_DIR%boost"
set "CACHE_DIR=%ROOT_DIR%_cache"
set "ARCHIVE_NAME=boost_1_86_0.tar.bz2"
set "ARCHIVE_PATH=%CACHE_DIR%\%ARCHIVE_NAME%"
set "BOOST_URL=https://archives.boost.io/release/1.86.0/source/%ARCHIVE_NAME%"

if exist "%BOOST_DIR%\boost\graph\adjacency_list.hpp" (
    echo Boost already present at "%BOOST_DIR%".
    exit /b 0
)

if not exist "%CACHE_DIR%" mkdir "%CACHE_DIR%"

if not exist "%ARCHIVE_PATH%" (
    echo Downloading Boost %BOOST_VERSION%...
    powershell -NoProfile -ExecutionPolicy Bypass -Command "Invoke-WebRequest -Uri '%BOOST_URL%' -OutFile '%ARCHIVE_PATH%'"
    if errorlevel 1 (
        echo Failed to download Boost from %BOOST_URL%
        exit /b 1
    )
) else (
    echo Using cached archive "%ARCHIVE_PATH%".
)

set "TMP_DIR=%ROOT_DIR%_tmp_boost_extract"
if exist "%TMP_DIR%" rmdir /s /q "%TMP_DIR%"
mkdir "%TMP_DIR%"

echo Extracting Boost archive...
tar -xjf "%ARCHIVE_PATH%" -C "%TMP_DIR%"
if errorlevel 1 (
    echo Failed to extract Boost archive.
    rmdir /s /q "%TMP_DIR%"
    exit /b 1
)

if exist "%BOOST_DIR%" rmdir /s /q "%BOOST_DIR%"
for /d %%D in ("%TMP_DIR%\boost_*") do (
    move "%%D" "%BOOST_DIR%" >nul
    goto moved_ok
)

echo Could not locate extracted Boost folder.
rmdir /s /q "%TMP_DIR%"
exit /b 1

:moved_ok
rmdir /s /q "%TMP_DIR%"

if not exist "%BOOST_DIR%\boost\graph\adjacency_list.hpp" (
    echo Boost extraction completed but graph headers are missing.
    exit /b 1
)

echo Boost ready at "%BOOST_DIR%".
exit /b 0
