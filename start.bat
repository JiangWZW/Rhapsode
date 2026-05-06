@echo off
title Rhapsode

cd /d "%~dp0server"
start "Rhapsode Backend" /min .venv\Scripts\python.exe -m uvicorn rhapsode.app:app --host 127.0.0.1 --port 8080

cd /d "%~dp0frontend"
start "Rhapsode Frontend" /min npx vite --host 127.0.0.1 --port 5173

timeout /t 3 /nobreak >nul
echo.
echo  Rhapsode is running:
echo    Backend:  http://127.0.0.1:8080
echo    Frontend: http://127.0.0.1:5173
echo.
echo  Press any key to stop both servers...
pause >nul

taskkill /fi "windowtitle eq Rhapsode Backend*" /t /f >nul 2>&1
taskkill /fi "windowtitle eq Rhapsode Frontend*" /t /f >nul 2>&1
echo  Stopped.
