# iplboot

A minimal GameCube IPL


## Usage

iplboot will attempt to load DOLs from the following devices in order:
1. SD Gecko in Card Slot B
2. SD Gecko in Card Slot A
3. SD2SP2
4. USB Gecko in Card Slot B
5. USB Gecko in Card Slot A

For each device, iplboot checks for the presence of the following files in order:
1. `iplboot.ini`
2. File matching held button (`a.dol`, `x.dol`, etc.)
3. `ipl.dol`

If no file is found, the next device is attempted. After all devices are attempted, the system will reboot into the onboard IPL (original GameCube intro and menu).

Creating an `iplboot.ini` configuration file is the recommended route.

**:warning: NOTE:** Be careful not to touch any of the analog controls (sticks and triggers) when powering on as this is when they are calibrated.

**Something not working?** See the [troubleshooting section](#troubleshooting).

### Configuration

Create a file named `iplboot.ini` at the root of your SD card. This file should be formatted using INI syntax, which is basic `NAME=some value`. The following parameters are supported:

**:warning: All values are case sensitive. :warning:**

 Parameter        | Description
------------------|-------------
 `{SHORTCUT}`     | Shortcut action.
 `{SHORTCUT}_ARG` | CLI argument passed to shortcut DOL.
 `DEBUG`          | Set to `1` to enable debug mode.

Replace `{SHORTCUT}` with one of the following: `A`, `B`, `X`, `Y`, `Z`, `START`, `DEFAULT`.

Shortcut action my be one of:

 Value      | Description
------------|-------------
 A filepath | Path to a DOL to load. All paths are relative to the device root.
 `ONBOARD`  | Reboot into the onboard IPL (original GameCube intro and menu).
 `USBGECKO` | Attempt to load from USB Gecko.

Holding a button during boot will activate the shortcut with the matching name. If no button is held, `DEFAULT` is used (if unspecified, the `DEFAULT` action is `/ipl.dol`).

Specify one `{SHORTCUT}_ARG` per CLI argument. You may specify as many as you like. Also consider using a CLI file.

For example, this configuration would boot straight into Swiss by default, GBI if you held B, or the original GameCube intro if you held Z:

```
DEFAULT=swiss.dol
Z=ONBOARD
B=gbi.dol
```

This configuration would boot into the original GameCube intro by default, Swiss if you held Z, or GBI if you held X or Y:

```
DEFAULT=ONBOARD
Z=swiss.dol
X=gbi.dol
X_ARG=--zoom=2
Y=gbi.dol
Y_ARG=--zoom=3
Y_ARG=--vfilter=.5:.5:.0:.5:.0:.5
```

Comments may be included by starting the line with the `#` character. These lines will be ignored.

See the [Special Features](#special-features) section for additional functionality.

### Button Files

**:warning: If a config file is found, this behavior does not apply.**

The following buttons can be used as shortcuts to load the associated filenames when a configuration file is not present:

 Button Held | File Loaded
-------------|--------------
 A           | `a.dol`
 B           | `b.dol`
 X           | `x.dol`
 Y           | `y.dol`
 Z           | `z.dol`
 Start       | `start.dol`

Holding a button during boot will activate the shortcut. If no button is held, `ipl.dol` is used.

For example, this configuration would boot straight into Swiss by default, GBI if you held B, or the original GameCube intro if you held D-Pad Left:
- `/ipl.dol` - Swiss
- `/b.dol` - GBI

This configuration would boot into the original GameCube intro by default, Swiss if you held Z, or GBI if you held B:
- `/z.dol` - Swiss
- `/b.dol` - GBI

**Pro-tip:** You can prevent files from showing in Swiss by marking them as hidden files on the SD card.

### Special Features

CLI files are supported. They will be append after any CLI args defined in the config file.

Holding D-Pad Left or the reset button will skip iplboot functionality and reboot straight into the onboard IPL.

Holding D-Pad Down enables debug mode. You can hold this along with a shortcut button.

iplboot also acts as a server for @emukidid's [usb-load](https://github.com/emukidid/gc-usb-load), should you want to use it for development purposes.

## Installation

Download the designated appropriate file for your device from the [latest release](https://github.com/redolution/iplboot/releases/latest).

Prepare your SD card by creating a configuration file and/or copying DOLs onto the SD card according to either the [Configuration](#configuration) or [Button Files](#button-files) instructions above.

### PicoBoot

You can download ready-to-go firmware featuring iplboot directly from the [PicoBoot repo](https://github.com/webhdx/PicoBoot). Just follow the [installation guide](https://github.com/webhdx/PicoBoot/wiki/Installation-guide). This is likely what you want.

Otherwise, use `iplboot.dol` and follow the PicoBoot [compiling firmware](https://github.com/webhdx/PicoBoot/wiki/Compiling-PicoBoot-firmware) instructions to flash your Raspberry Pico.

### Qoob Pro

Flash `iplboot.gcb` to your Qoob Pro as a BIOS.

### Qoob SX

Qoob SX is not currently supported due to size constraints.

The last version available for the Qoob SX is [r5.2](https://github.com/redolution/iplboot/releases/tag/r5.2). Download `qoob_sx_iplboot_upgrade.elf` and flash it to your Qoob SX as a BIOS.

### ViperGC

Flash `iplboot.vgc` to your ViperGC as a BIOS.

### GameCube Memory Card

Use `iplboot_xz.gci` and a GameCube memory card manager such as [GCMM](https://github.com/suloku/gcmm) to copy to your memory card. It will be saved as `boot.dol` and can be used in conjunction with the various [save game exploits](https://www.gc-forever.com/wiki/index.php?title=Booting_homebrew#Game_Save_Exploits).


## Troubleshooting

Enable debug mode by holding d-pad in the down direction. This will allow you to read the diagnostic messages as well as enable more verbose output. Look for warning messages about unrecognized configuration parameters, file read failures, etc.

When choosing a shortcut button, beware that some software checks for buttons held at boot to alter certain behaviors. If your software behaves differently when booted through iplboot, ensure the assigned shortcut button is not used in this way. For example, it is not recommended to assign Swiss to the B button as holding B causes Swiss to disable the DVD drive.


## Compiling

You should only need to compile if you want to modify to the iplboot source code. Otherwise, you can just download prebuilt binaries from the [latest release](https://github.com/redolution/iplboot/releases/latest).

1. Ensure you have the latest version of [devkitPro](https://devkitpro.org/wiki/Getting_Started) installed.
2. Ensure you have the latest version of [libogc2](https://github.com/extremscorner/libogc2) installed.
3. Ensure your devkitPro environment variables are set: `DEVKITPRO` and `DEVKITPPC`
4. Python3 is required.

### PicoBoot

1. Run `make dol`.
2. Follow installation instructions above using the newly created `build/iplboot.dol`.

### Qoob Pro

Qoob Pro requires BS1 (early hardware initialization code) from the original IPL. This can be obtained from an existing Qoob Pro BIOS or any IPL dump.

To use an existing Qoob Pro BIOS (easiest method):
1. Download `iplboot.gcb` from the [latest release](https://github.com/redolution/iplboot/releases/latest).
2. Rename `iplboot.gcb` to `ipl.rom` and place it at the root of this project.

Alternatively, you can dump the IPL ROM from your own GameCube using Swiss:
1. Ensure file management is enabled in Swiss settings: Press B, select the gear icon at bottom right, press R until you reach "Advanced Settings", scroll down to "File Management", press right to enable, save and exit.
2. Switch to the system device: Select the eject icon at bottom left, scroll right to "System", press A.
3. Scroll down to `ipl.bin`, press Z then X, and copy to your SD card.
4. Copy `ipl.bin` from your SD card to the root of this project. Rename it to `ipl.rom`.

You should now have `ipl.rom` at the project root.

1. Run `make qoobpro`.
2. Follow installation instructions above using the newly created `build/iplboot.gcb`.

### Qoob SX

NOTE: Qoob SX is currently nonfunctional as the resulting BIOS is too large.

1. Download Qoob BIOS v1.3c `Qoob (1.3c).rar` from this [archive post](https://www.gc-forever.com/forums/viewtopic.php?f=36&t=23) and extract `qoob_sx_13c_upgrade.elf` (MD5: `2292630e7604bc2ae8bfa4d3e4ba5941`) to the root of this project.
2. Run `make qoobsx`.
3. Follow installation instructions above using the newly created `build/qoob_sx_iplboot_upgrade.elf`

### Viper

1. Run `make viper`.
2. Follow installation instructions above using the newly created `build/iplboot.vgc`.

### GameCube Memory Card

1. Run `make gci`.
2. Follow installation instructions above using the newly created `build/iplboot.gci`.

*Optional:* If you want to reduce the file size by ~50% so it will take up less space on your memory card, you can create a compressed version.

1. Run `make gci_compressed`.
2. Follow installation instructions above using the newly created `build/iplboot_xz.gci` (Notice the `_xz` suffix).

### Compressed DOL

Should you need it, you can create a compressed version of the base DOL.

1. Run `make dol_compressed`.
2. Follow installation instructions above using the newly created `build/iplboot_xz.dol` (Notice the `_xz` suffix).
