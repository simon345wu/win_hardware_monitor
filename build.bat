@echo off
setlocal enabledelayedexpansion

:: Check common VS installation paths
set "VS_PATH="
for %%e in (Community Professional Enterprise BuildTools) do (
    if exist "C:\Program Files\Microsoft Visual Studio\2022\%%e\VC\Auxiliary\Build\vcvarsall.bat" (
        set "VS_PATH=C:\Program Files\Microsoft Visual Studio\2022\%%e"
        goto :found_vs
    )
)

echo Visual Studio 2022 not found!
exit /b 1

:found_vs
echo Found Visual Studio in: !VS_PATH!
set "VCVARS=!VS_PATH!\VC\Auxiliary\Build\vcvarsall.bat"
set "CMAKE=!VS_PATH!\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"

echo Setting up environment variables...
call "!VCVARS!" x64

echo Creating build directory...
if not exist build mkdir build
cd build

echo Configuring project with CMake...
"!CMAKE!" -G "Visual Studio 17 2022" -A x64 ..
if %errorlevel% neq 0 (
    echo CMake configuration failed!
    exit /b %errorlevel%
)

echo Building project...
"!CMAKE!" --build . --config Release
if %errorlevel% neq 0 (
    echo Build failed!
    exit /b %errorlevel%
)

echo Build successful! Executable is in build\Release\win_hardware_monitor.exe
exit /b 0
