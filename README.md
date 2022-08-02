# iplboot

A minimal GameCube IPL


## Usage

iplboot will attempt to load DOLs from the following locations in order:
- USB Gecko in Card Slot B
- SD Gecko in Card Slot B
- USB Gecko in Card Slot A
- SD Gecko in Card Slot A
- SD2SP2

You can use button shortcuts to keep alternate software on quick access. When loading from an SD card, iplboot will look for and load different filenames depending on what buttons are being held:

 Button Held | File Loaded
-------------|--------------
 *None*      | `/ipl.dol`
 A           | `/a.dol`
 B           | `/b.dol`
 X           | `/x.dol`
 Y           | `/y.dol`
 Z           | `/z.dol`
 Start       | `/start.dol`

CLI files are also supported.

If the selected shortcut file cannot be loaded, iplboot will fallback to `/ipl.dol`. If that cannot be loaded either, iplboot will reboot to the original IPL (GameCube intro and menu).

For example, this configuration would boot straight into Swiss by default, or GBI if you held B, or the original GameCube intro if you held any other button:
- `/ipl.dol` - Swiss
- `/b.dol` - GBI

This configuration would boot into the original GameCube intro by default, or Swiss if you held Z, or GBI if you held B:
- `/z.dol` - Swiss
- `/b.dol` - GBI

**Pro-tip:** You can prevent files from showing in Swiss by marking them as hidden files on the SD card.

If you hold multiple buttons, the highest in the table takes priority. Be careful not to touch any of the analog controls (sticks and triggers) when powering on as this is when they are calibrated.

iplboot also acts as a server for @emukidid's [usb-load](https://github.com/emukidid/gc-usb-load), should you want to use it for development purposes.

**Something not working?** See the [troubleshooting section](#troubleshooting).


## Installation

Download the designated appropriate file for your device from the [latest release](https://github.com/redolution/iplboot/releases/latest).

Prepare your SD card by copying DOLs onto the SD card and renaming them according the table above.

### PicoBoot

You can download ready-to-go firmware featuring iplboot directly from the [PicoBoot repo](https://github.com/webhdx/PicoBoot). Just follow the [installation guide](https://github.com/webhdx/PicoBoot#-installation-guide). This is likely what you want.

Otherwise, use `iplboot.dol` and follow the PicoBoot [compiling firmware](https://github.com/webhdx/PicoBoot#compiling-firmware) instructions to flash your Raspberry Pico.

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

iplboot displays useful diagnostic messages as it attempts to load the selected DOL. But it's so fast you may not have time to read or even see them. If you hold the down direction on the D-Pad, the messages will remain on screen until you let go.

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
