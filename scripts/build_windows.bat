@echo off
setlocal

if "%PJSIP_DIR%"=="" (
  echo Set PJSIP_DIR to the PJSIP install root.
  exit /b 1
)

if not exist build mkdir build

cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DPJSIP_DIR="%PJSIP_DIR%"
if errorlevel 1 exit /b 1

cmake --build build --config Release
