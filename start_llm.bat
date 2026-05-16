@echo off
set "PATH=C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.6\bin;%PATH%"

third_party\llama.cpp\build\bin\Release\llama-server.exe ^
    -m models\Qwen3-8B-Q4_K_M.gguf ^
    --port 8012 ^
    -ngl 99 ^
    -c 4096 ^
    --reasoning-format deepseek
