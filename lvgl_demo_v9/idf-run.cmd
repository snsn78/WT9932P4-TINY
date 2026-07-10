@echo off
setlocal

set "IDF_ROOT=E:\Vscode_codes\esp-idf-v5.4"
set "PY_ROOT=C:\Users\20953\AppData\Local\Programs\Python\Python312"
set "GIT_CMD=D:\openclaw\git\Git\cmd"

if not exist "%IDF_ROOT%\export.bat" (
    echo ERROR: ESP-IDF export script not found at "%IDF_ROOT%\export.bat"
    exit /b 1
)

set "PATH=%PY_ROOT%;%PY_ROOT%\Scripts;%GIT_CMD%;C:\Windows\system32;C:\Windows;C:\Windows\System32\Wbem;C:\Windows\System32\WindowsPowerShell\v1.0\;%PATH%"

call "%IDF_ROOT%\export.bat"
if errorlevel 1 exit /b %errorlevel%

idf.py %*
