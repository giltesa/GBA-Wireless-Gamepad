@echo off
setlocal

echo ================================
echo      ESP32 FLASH TOOL
echo ================================
echo.

REM Check esptool
py -m esptool version >nul 2>&1
if errorlevel 1 (
    echo [ERROR] esptool is not installed.
    echo Run:
    echo py -m pip install esptool
    pause
    exit /b
)

echo Available COM ports:
echo.

wmic path Win32_SerialPort get DeviceID,Caption

echo.
set /p PORT=Enter COM port (example COM3):

if "%PORT%"=="" (
    echo Invalid COM port.
    pause
    exit /b
)

echo.
echo Using port: %PORT%
echo.
echo Flashing ESP32...
echo.

py -m esptool --chip esp32 --port %PORT% --baud 115200 write_flash -z ^
0x1000 bootloader.bin ^
0x8000 partitions_singleapp.bin ^
0x10000 gba-bt-hid-fw-esp32.bin

if errorlevel 1 (
    echo.
    echo [ERROR] Flash failed.
    pause
    exit /b
)

echo.
echo ================================
echo FLASH COMPLETED SUCCESSFULLY
echo ================================
echo.

pause