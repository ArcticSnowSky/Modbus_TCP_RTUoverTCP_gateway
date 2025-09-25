@echo off
setlocal enabledelayedexpansion

:: Show help if requested
set HELP_FLAG=false
if "%~1"=="-h" set HELP_FLAG=true
if "%~1"=="/h" set HELP_FLAG=true
if "%~1"=="--help" set HELP_FLAG=true
if "%~1"=="/?" set HELP_FLAG=true
if "%~1"=="?" set HELP_FLAG=true
if "%HELP_FLAG%"=="true" (
    echo Usage: %~nx0 [SERVICE_NAME]
    exit /b
)

:: Save script path and arguments
set ScriptPath=%~dp0

:: Check if elevated admin rights
net session >nul 2>&1
if %errorlevel% neq 0 (
    echo Launch with elevated privileges...
    set COMMAND=Start-Process '%~f0' -Verb RunAs -WorkingDirectory '%ScriptPath%'
    if not "%~1"=="" set command=!COMMAND! -ArgumentList '%*'
    powershell -Command "!COMMAND!"
    exit /b
)

:: Change to script directory
pushd "%ScriptPath%"

:: Configuration
set SERVICE_NAME=ModbusGateway
:: Argumente prüfen und ggf. überschreiben
if not "%~1"=="" set SERVICE_NAME=%~1

echo Ready to uninstall service: %SERVICE_NAME%
pause

@echo on
sc.exe delete %SERVICE_NAME%
@echo off
pause