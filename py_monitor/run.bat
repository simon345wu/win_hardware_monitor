@echo off
setlocal enabledelayedexpansion

echo ===================================================
echo   Python Hardware Monitor Auto Setup and Runner
echo ===================================================

where python >nul 2>nul
if %errorlevel% neq 0 (
    echo [ERROR] Python not found in system PATH.
    echo Please install Python 3.8+ and add it to your system variables.
    pause
    exit /b 1
)

if not exist .venv (
    echo [INFO] Creating Python virtual environment...
    python -m venv .venv
    if !errorlevel! neq 0 (
        echo [ERROR] Failed to create virtual environment.
        pause
        exit /b !errorlevel!
    )
    echo [INFO] Virtual environment created successfully.
)

echo [INFO] Installing and upgrading dependencies...
call .venv\Scripts\activate.bat

python -m pip install --upgrade pip >nul
pip install -r requirements.txt
if !errorlevel! neq 0 (
    echo [ERROR] Dependency installation failed.
    pause
    exit /b !errorlevel!
)

echo [INFO] Starting Windows Hardware Performance Monitor (Python)...
python main.py
if !errorlevel! neq 0 (
    echo [WARNING] Application exited with error code: !errorlevel!
    pause
)

echo [INFO] Done.
exit /b 0
