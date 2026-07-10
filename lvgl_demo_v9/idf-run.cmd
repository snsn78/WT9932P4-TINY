@echo off
setlocal

if not defined IDF_PATH (
    echo ERROR: IDF_PATH is not set.
    echo Run ESP-IDF export.bat first, or set IDF_PATH to your ESP-IDF directory.
    exit /b 1
)

if not exist "%IDF_PATH%\export.bat" (
    echo ERROR: ESP-IDF export script not found at "%IDF_PATH%\export.bat"
    echo Set IDF_PATH to your ESP-IDF directory, then run this script again.
    exit /b 1
)

call "%IDF_PATH%\export.bat"
if errorlevel 1 exit /b %errorlevel%

idf.py %*
