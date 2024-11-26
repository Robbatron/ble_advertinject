# BLE AdvertInject for Flipper Zero

This Flipper application injects advertisement broadcast packets to Apple, Android, and Windows devices, which may be up to 5 meters away.

## Installation Guide

### Step 1: Install RogueMaster Firmware
1. The easiest way to install RogueMaster firmware is through the [web installer](https://lab.flipper.net/?url=https%3A%2F%2Frogue-master.net%2F%3Ffile%3DRM1103-2222-0.420.0-b2f8dce.tgz&channel=RM1103-2222-0.420.0-b2f8dce&version=0.420.0)
2. Follow the web installer instructions to install RogueMaster
   * This app requires RogueMaster firmware to function properly

### Step 2: Clone the Project
1. Clone or download this `ble_advertinject` project
2. Place the `ble_advertinject` directory in the `applications/external` directory of your RogueMaster firmware source code

### Step 3: Build the Application

On MacOS:
```bash
./fbt fap_ble_advertinject
```

On Windows:
```bash
.\fbt.cmd fap_ble_advertinject
```

The compiled application (`ble_advertinject.fap`) will be created in:
```
build/f7-firmware-C/.extapps/
```

If you can't find the .fap file in the expected location, you can use the find command:

```bash
find . -name "ble_advertinject.fap"
```

On Windows:
```bash
dir /s "ble_advertinject.fap"
```

### Step 4: Install on Flipper Zero
1. Download and install [qFlipper](https://flipperzero.one/update)
2. Connect your Flipper Zero to your computer
3. Using qFlipper's file browser:
   * Navigate to `SD Card/apps/Bluetooth/` on your Flipper Zero
   * Drag and drop `ble_advertinject.fap` into this directory

You're now ready to use BLE AdvertInject on your Flipper Zero!