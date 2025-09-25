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
    echo Usage: %~nx0 [tcp^|rtu] [LISTEN_PORT] [TARGET_HOST] [TARGET_PORT] [SERVICE_NAME] [DISPLAY_NAME]
    exit /b
)

:: Save script path and arguments
set ScriptPath=%~dp0

:: Check if elevated admin rights
net session >nul 2>&1
if %errorlevel% neq 0 (
    echo Launch with elevated privileges...
::    powershell -Command "Start-Process '%~f0' -ArgumentList '%*' -Verb RunAs -WorkingDirectory '%ScriptPath%'"
    powershell -Command "Start-Process cmd -ArgumentList '/c \"\"%~f0\" %*\"' -Verb RunAs -WorkingDirectory '%ScriptPath%'"
    exit /b
)

:: Change to script directory
pushd "%ScriptPath%"

:: Configuration
set MODE=tcp
set ANTIMODE=rtu
set LISTEN_PORT=1502
set TARGET_HOST=127.0.0.1
set TARGET_PORT=502

:: Argumente prüfen und ggf. überschreiben
if not "%~1"=="" set MODE=%~1
if not "%~2"=="" set LISTEN_PORT=%~2
if not "%~3"=="" set TARGET_HOST=%~3
if not "%~4"=="" set TARGET_PORT=%~4

if "%MODE%" == "rtu" set "ANTIMODE=tcp"

set SERVICE_NAME=ModbusGateway
set DISPLAY_NAME=Modbus %MODE%-%ANTIMODE% Gateway

if not "%~5"=="" set SERVICE_NAME=%~5
if not "%~6"=="" set DISPLAY_NAME=%~6

if "%~7"=="" goto :after_description
:: Get all arguments from 5th to last and concatenate them to DISPLAY_NAME
setlocal enabledelayedexpansion
set "_DISPLAY_NAME=%~6"
set "i=1"
for %%x in (%*) do (
    if !i! GTR 6 (
        set "arg=%%x"
        if not "!arg!"=="" (
            set "_DISPLAY_NAME=!_DISPLAY_NAME! !arg!"
        )
    )
    set /a i+=1
)
endlocal & set "DISPLAY_NAME=%_DISPLAY_NAME%"
:after_description

:: Get current directory
set CURRENT_DIR=%CD%

:: Path to EXE including arguments
set BIN_PATH=%CURRENT_DIR%\modbus_gateway.exe %MODE% %LISTEN_PORT% %TARGET_HOST% %TARGET_PORT%
set DESCRIPTION=Gateway Service: Listen %MODE% %LISTEN_PORT% to %ANTIMODE% %TARGET_HOST%:%TARGET_PORT%

echo Installation Details:
echo   Executable: %BIN_PATH%
echo   Service Name: %SERVICE_NAME%
echo   Display Name: %DISPLAY_NAME%
echo   Mode (tcp^|rtu): %MODE%
echo   Listen Port: %LISTEN_PORT%
echo   Target Host: %TARGET_HOST%
echo   Target Port: %TARGET_PORT%
echo   Description: %DESCRIPTION%
echo   Start Type: Automatic
echo Ready to install the service.
pause

@echo on
sc.exe create "%SERVICE_NAME%" ^
    binPath= "%BIN_PATH%" ^
    DisplayName= "%DISPLAY_NAME%" ^
    start= auto
@echo off

echo.
if %ERRORLEVEL% EQU 0 (
    echo Service "%SERVICE_NAME%" created successfully.
    set /p "description=Overwrite service-description Text [or leave empty]: "
    sc description %SERVICE_NAME% "!description!"
) else (
    echo Failed to create service "%SERVICE_NAME%". Error code: %ERRORLEVEL%
    net helpmsg %ERRORLEVEL%
)
endlocal
pause
exit /b %ERRORLEVEL%
