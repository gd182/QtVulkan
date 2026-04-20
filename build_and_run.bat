@echo off
setlocal

set "QT_BIN="
set "MINGW_BIN="
set "SOURCE_DIR="
set "BUILD_DIR=%SOURCE_DIR%\build\Desktop_Qt_6_8_3_MinGW_64_bit-Отладка"
set "CMAKE="
set "EXE=%BUILD_DIR%\appQtVulkan.exe"
set "WINDEPLOYQT=%QT_BIN%\windeployqt6.exe"

set "PATH=%QT_BIN%;%MINGW_BIN%;%PATH%"

echo [0/3] Closing previous instance...
taskkill /f /im appQtVulkan.exe >nul 2>&1

echo [1/3] Building...
"%CMAKE%" --build "%BUILD_DIR%" --target all
if errorlevel 1 (
    echo Build FAILED.
    pause
    exit /b 1
)

echo [2/3] Deploying DLLs...
"%WINDEPLOYQT%" --qmldir "%SOURCE_DIR%" "%EXE%"
if errorlevel 1 (
    echo Deploy FAILED.
    pause
    exit /b 1
)

echo [3/3] Running...
echo ----------------------------------------
"%EXE%"
set "EXIT_CODE=%errorlevel%"
echo ----------------------------------------
echo Exit code: %EXIT_CODE%
pause
