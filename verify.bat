@echo off
setlocal

call build_ninja.bat
if errorlevel 1 exit /b 1

call build_test_scene.bat
if errorlevel 1 exit /b 1

server\.venv\Scripts\python.exe -m compileall -q server\rhapsode server\tests
if errorlevel 1 exit /b 1
server\.venv\Scripts\python.exe -m pytest -q server\tests
if errorlevel 1 exit /b 1
server\.venv\Scripts\python.exe -m ruff check server\rhapsode server\tests
if errorlevel 1 exit /b 1

call npm --prefix frontend test
if errorlevel 1 exit /b 1
call npm --prefix frontend run lint
if errorlevel 1 exit /b 1
call npm --prefix frontend run build
if errorlevel 1 exit /b 1

echo.
echo All verification checks passed.
