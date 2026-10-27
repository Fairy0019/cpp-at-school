@echo off
setlocal
cd /d "%~dp0"

if not exist build mkdir build

g++ -std=c++17 -Wall -Wextra -O2 -finput-charset=UTF-8 -fexec-charset=UTF-8 -o build\dungeon.exe src\*.cpp

if errorlevel 1 (
    echo.
    echo Build failed.
    pause
    exit /b 1
)

echo.
echo Build succeeded: build\dungeon.exe
endlocal
