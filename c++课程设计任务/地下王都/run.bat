@echo off
setlocal
cd /d "%~dp0"

if not exist build\dungeon.exe (
    call build.bat
    if errorlevel 1 exit /b 1
)

build\dungeon.exe
pause
endlocal
