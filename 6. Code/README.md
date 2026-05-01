
# Flashing the ESP32 Firmware

This project includes precompiled ESP32 firmware binaries, so you do **not** need to compile anything yourself.

All required files are located in:

```text
6. Code
```

---

# Requirements

## Hardware

- ESP32 board
- USB cable
- Windows PC

## Files included

```text
flash_esp32.bat
bootloader.bin
gba-bt-hid-fw-esp32.bin
partitions_singleapp.bin
```

---

# Method 1 (Recommended) — Automatic Flash Tool

This is the easiest method.

## Step 1 — Install Python

Download and install Python:

https://www.python.org/downloads/windows/

IMPORTANT:

During installation, enable:

```text
Add Python to PATH
```

---

## Step 2 — Install esptool

Open PowerShell and run:

```powershell
py -m pip install esptool
```

---

## Step 3 — Connect the ESP32

Connect the ESP32 board to your PC using USB.

Wait a few seconds for Windows to detect it.

---

## Step 4 — Run the Flash Tool

Open the folder:

```text
6. Code
```

Double-click:

```text
flash_esp32.bat
```

The tool will display available COM ports, for example:

```text
Available COM ports:

  [1] COM3 - Silicon Labs CP210x USB to UART Bridge
  [2] COM5 - USB Serial Device
```

Type the number corresponding to your ESP32 and press ENTER.

Example:

```text
1
```

The firmware will then be flashed automatically.

---

# Method 2 — Manual Flashing

If the batch file does not work, you can flash manually.

Open PowerShell inside the `6. Code` folder and run:

```powershell
py -m esptool --chip esp32 --port COM3 --baud 115200 write_flash -z ^
0x1000 bootloader.bin ^
0x8000 partitions_singleapp.bin ^
0x10000 gba-bt-hid-fw-esp32.bin
```

Replace:

```text
COM3
```

with your actual ESP32 COM port.

---

# How to Find the Correct COM Port

## Option 1 — Device Manager

Open:

```text
Device Manager → Ports (COM & LPT)
```

Example:

```text
Silicon Labs CP210x USB to UART Bridge (COM3)
```

Your port is:

```text
COM3
```

---

# Flash Successful

If flashing completes correctly, you should see:

```text
Hash of data verified.
Leaving...
Hard resetting via RTS pin...
```

The ESP32 firmware is now installed and ready to use.
