@echo off
setlocal EnableExtensions

title ESP32-P4 Build and Flash

set "PROJECT_DIR=%~dp0"
set "BUILD_DIR=build_multitouch_verify"
set "ESPPORT=%~1"
if "%ESPPORT%"=="" set "ESPPORT=COM20"

echo ============================================================
echo ESP32-P4 touch gamepad build and flash
echo Project: %PROJECT_DIR%
echo Port:    %ESPPORT%
echo ============================================================
echo.

if not exist "%PROJECT_DIR%CMakeLists.txt" (
    echo [ERROR] Project directory is invalid.
    goto :failed
)

where idf.py >nul 2>&1
if errorlevel 1 (
    echo [ERROR] idf.py was not found.
    echo Please open "ESP-IDF 5.4 CMD" from the Windows Start Menu,
    echo then run this script again.
    goto :failed
)

cd /d "%PROJECT_DIR%"

echo [1/4] Setting ESP-IDF target to esp32p4...
idf.py set-target esp32p4
if errorlevel 1 (
    echo [ERROR] Failed to set target esp32p4.
    goto :failed
)

echo.
echo [2/4] Building firmware...
idf.py -B "%BUILD_DIR%" build
if errorlevel 1 (
    echo.
    echo [ERROR] Build failed. Flashing was stopped to prevent an old
    echo firmware image from being written to the board.
    goto :failed
)

if not exist "%BUILD_DIR%\lvgl_demo_v9.bin" (
    echo [ERROR] The application binary was not generated.
    goto :failed
)

echo.
echo [3/4] Checking %ESPPORT%...
mode %ESPPORT% >nul 2>&1
if errorlevel 1 (
    echo [ERROR] %ESPPORT% is missing or occupied by another program.
    echo Close ESP-IDF Monitor, VS Code serial monitor, Arduino Monitor,
    echo PuTTY, MobaXterm, Tera Term, and other serial applications.
    goto :failed
)

echo.
echo [4/4] Flashing firmware to %ESPPORT% at 460800 baud...
idf.py -B "%BUILD_DIR%" -p %ESPPORT% -b 460800 flash
if errorlevel 1 (
    echo.
    echo [ERROR] Flashing failed.
    echo If connection failed, hold BOOT, tap RESET, release RESET,
    echo release BOOT, and run this file again.
    goto :failed
)

echo.
echo ============================================================
echo [SUCCESS] Firmware was built and flashed to %ESPPORT%.
echo ============================================================
echo.
choice /C YN /N /M "Open the serial monitor now? [Y/N]: "
if errorlevel 2 goto :success

echo.
echo Starting monitor. Press Ctrl+] to exit it.
idf.py -B "%BUILD_DIR%" -p %ESPPORT% monitor

:success
echo.
echo Completed successfully.
pause
exit /b 0

:failed
echo.
echo Operation stopped. No new firmware was flashed.
pause
exit /b 1
