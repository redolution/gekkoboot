# iplboot

A minimal GameCube IPL


## Usage

iplboot will attempt to load DOLs from the following locations in order:
- USB Gecko in Card Slot B
- SD Gecko in Card Slot B
- USB Gecko in Card Slot A
- SD Gecko in Card Slot A
- SD2SP2

When loading from an SD card, it will look for and load different filenames depending on what buttons are being held. This is useful to keep alternate software on quick access.

 Button Held | File Loaded
-------------|--------------
 *None*      | `/ipl.dol`
 A           | `/a.dol`
 B           | `/b.dol`
 X           | `/x.dol`
 Y           | `/y.dol`
 Z           | `/z.dol`
 Start       | `/start.dol`
 D-Left      | `/left.dol`
 D-Right     | `/right.dol`
 D-Up        | `/up.dol`

If the selected file cannot be loaded, iplboot will reboot to the orginal IPL (GameCube intro and menu) instead.

For example, this configuration would boot straight into Swiss by default, or GBI if you held B, or the orginal GameCube intro if you held any other button:
- `/ipl.dol` - Swiss
- `/b.dol` - GBI

This configuration would boot into the orginal GameCube intro by default, or Swiss if you held Z, or GBI if you held B:
- `/z.dol` - Swiss
- `/b.dol` - GBI

**Pro-tip:** You can prevent files from showing in Swiss by marking them as hidden files on the SD card.

If you hold multiple buttons, the highest in the table takes priority. Also note that all directions are D-Pad buttons, *not* joysticks. Careful not to touch any of the analog controls (sticks and triggers) when powering on as this is when they are calibrated.

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

Qoob SX is not currently supported.

### ViperGC

Flash `iplboot.vgc` to your ViperGC as a BIOS.

### GameCube Memory Card

Use `iplboot_xz.gci` and a GameCube memory card manager such as [GCMM](https://github.com/suloku/gcmm) to copy to your memory card. It will be saved as `boot.dol` and can be used in conjunction with the various [save game exploits](https://www.gc-forever.com/wiki/index.php?title=Booting_homebrew#Game_Save_Exploits).


## Troubleshooting

iplboot displays useful diagnostic messages as it attempts to load the selected DOL. But it's so fast you may not have time to read or even see them. If you hold the down direction on the D-Pad, the messages will remain on screen until you let go.


## Compiling

You should only need to compile if you want to modify to the iplboot source code. Otherwise, you can just download prebuilt binaries from the [latest release](https://github.com/redolution/iplboot/releases/latest).

1. Ensure you have the latest version of [devkitPro](https://devkitpro.org/wiki/Getting_Started) installed.
2. Ensure you have the latest version of [libogc2](https://github.com/extremscorner/libogc2) installed.
3. Ensure your devkitPro environment variables are set: `DEVKITPRO` and `DEVKITPPC`

### PicoBoot

1. Run `make dol`.
2. Follow usage instructions above using the newly created `build/iplboot.dol`.

### Qoob Pro

1. Obtain the gc-pal-10 IPL ROM (MD5: `0cdda509e2da83c85bfe423dd87346cc`) and copy it to the root of this project as `ipl.rom`. This is currently the only IPL that has been verified to work.
2. Run `make qoobpro`.
3. Follow usage instructions above using the newly created `build/iplboot.gcb`.

### Qoob SX

NOTE: Qoob SX is currently nonfunctional as the resulting BIOS is too large.

1. Ensure [dolxz] is installed.
2. Ensure [doltool] is installed.
3. Obtain `qoob_sx_13c_upgrade.elf` and copy it to the root of this project. (TODO: MD5 and more details about the file.)
4. Run `make qoobsx`.
5. Follow usage instructions above using the newly created `build/qoob_sx_iplboot_upgrade.elf`

### Viper

1. Run `make viper`.
2. Follow usage instructions above using the newly created `build/iplboot.vgc`.

### GameCube Memory Card

1. Ensure [dol2gci] is installed.
2. Run `make gci`.
3. Follow usage instructions above using the newly created `build/iplboot.gci`.

*Optional:* If you want to reduce the file size by ~50% so it will take up less space on your memory card, you can create a compressed version.

1. Ensure [dol2gci] is installed.
2. Ensure [dolxz] is installed.
3. Run `make gci_compressed`.
4. Follow usage instructions above using the newly created `build/iplboot_xz.gci` (Notice the `_xz` suffix).

### Compressed DOL

Should you need it, you can create a compressed version of the base DOL.

1. Ensure [dolxz] is installed.
2. Run `make dol_compressed`.
3. Follow usage instructions above using the newly created `build/iplboot_xz.dol` (Notice the `_xz` suffix).


[dolxz]: https://github.com/yo1dog/dolxz
[dol2gci]: https://github.com/emukidid/swiss-gc/tree/master/buildtools
[doltool]: https://github.com/redolution/Devkitpro-installer/blob/trunk/tools/gamecube/doltool/
