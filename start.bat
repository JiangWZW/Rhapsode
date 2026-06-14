@echo off
title Rhapsode
set "PATH=C:\Program Files\Graphviz\bin;C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.6\bin;%PATH%"

rem Never route localhost LLM calls through an ambient proxy (empty-body 502s).
set "NO_PROXY=127.0.0.1,localhost"
set "no_proxy=127.0.0.1,localhost"

cd /d "%~dp0"
start "Rhapsode LLM" third_party\llama.cpp\build\bin\Release\llama-server.exe -m models\Qwen3-8B-Q4_K_M.gguf --port 8012 -ngl 99 -c 8192 -np 1 -fa on --reasoning-format deepseek

cd /d "%~dp0server"
start "Rhapsode Backend" .venv\Scripts\python.exe -m uvicorn rhapsode.app:app --host 127.0.0.1 --port 8080

cd /d "%~dp0frontend"
start "Rhapsode Frontend" npx vite --host 127.0.0.1 --port 5173

timeout /t 5 /nobreak >nul
echo.
echo  Rhapsode is running:
echo    LLM:      http://127.0.0.1:8012 (Qwen3-8B, GPU)
echo    Backend:  http://127.0.0.1:8080
echo    Frontend: http://127.0.0.1:5173
echo.
echo  Press any key to stop all servers...
pause >nul

taskkill /fi "windowtitle eq Rhapsode LLM*" /t /f >nul 2>&1
taskkill /fi "windowtitle eq Rhapsode Backend*" /t /f >nul 2>&1
taskkill /fi "windowtitle eq Rhapsode Frontend*" /t /f >nul 2>&1
echo  Stopped.
