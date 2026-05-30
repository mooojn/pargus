@echo off
title Pargus Process Manager
echo ==========================================
echo       Starting Pargus Application
echo ==========================================
echo.

echo [1/2] Launching Backend API in WSL...
start "Pargus API Server" wsl bash -c "source .venv-wsl/bin/activate && python3 -m uvicorn api.main:app --reload --host 127.0.0.1 --port 8000; exec bash"

echo [2/2] Launching Frontend (Next.js)...
start "Pargus Frontend" cmd /k "title Pargus Frontend && cd /d %~dp0frontend && npm run dev"

echo.
echo All services launched!
echo Backend API: http://127.0.0.1:8000
echo Frontend Dev: http://localhost:3000
echo.
pause
