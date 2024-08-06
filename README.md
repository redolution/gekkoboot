# gekkoboot

A utilitarian bootloader for the GameCube

## Usage

gekkoboot will attempt to load DOLs from the following locations in order:
- USB Gecko in Card Slot B
- SD Gecko in Card Slot B
- USB Gecko in Card Slot A
- SD Gecko in Card Slot A
- SD2SP2
- GC Loader

You can use button shortcuts to keep alternate software on quick access.
When loading from an SD card, gekkoboot will look for and load different filenames
depending on what buttons are being held:

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

If the selected shortcut file cannot be loaded, gekkoboot will fall back to
`/ipl.dol`. If that cannot be loaded either, the next device will be searched.
If all fails, gekkoboot will reboot to the onboard IPL (original GameCube intro
and menu).

Holding D-Pad Left or the reset button will skip gekkoboot functionality and
reboot straight into the onboard IPL.

For example, this configuration would boot straight into Swiss by default,
or GBI if you held B, or the original GameCube intro if you held D-Pad Left:
- `/ipl.dol` - Swiss
- `/b.dol` - GBI

This configuration would boot into the original GameCube intro by default,
or Swiss if you held Z, or GBI if you held B:
- `/z.dol` - Swiss
- `/b.dol` - GBI

**Pro-tip:** You can prevent files from showing in Swiss by marking them as
hidden files on the SD card.

If you hold multiple buttons, the highest in the table takes priority.
Be careful not to touch any of the analog controls (sticks and triggers) when
powering on as this is when they are calibrated.

gekkoboot also acts as a server for @emukidid's [usb-load](https://github.com/emukidid/gc-usb-load),
should you want to use it for development purposes.

**Something not working?** See the [troubleshooting section](#troubleshooting).


## Installation

Download and extract the [latest release].

Prepare your SD card by copying DOLs onto the SD card and renaming them
according the table above.

### PicoBoot

gekkoboot is bundled with the [PicoBoot] firmware.
Just follow the [installation guide][pb-install].

You can also update gekkoboot separately from the PicoBoot firmware,
using the supplied `gekkoboot_pico.uf2` (depends on [PicoBoot PR 107][pb-pr-107]).

[PicoBoot]: https://github.com/webhdx/PicoBoot
[pb-install]: https://github.com/webhdx/PicoBoot/wiki/Installation-guide
[pb-pr-107]: https://github.com/webhdx/PicoBoot/pull/107

### Qoob

Qoob Pro only: use the Qoob USB flash utility to install `gekkoboot_qoob_pro.gcb`
as a BIOS like you normally would.

A modified copy of the updater is also provided for both versions of the Qoob.
It can be run via Swiss, or the original Qoob BIOS, from an ISO9660 DVD or over
the nework using a Broadband Adapter[^qoob-bba].

The updater files are `qoob_pro_gekkoboot_upgrade.elf` and
`qoob_sx_gekkoboot_upgrade.elf` respectively.
When burning them to a disc, you may need to pad the image with a large (~1GiB)
file, to ensure the drive can read it properly.

The recommended method however is to burn Swiss to a DVD, and run the updater
from an SD card.

On Qoob SX, you will need to hold D-Pad left while turning the system on to boot
into the "backup" BIOS, otherwise write protection will be enabled.
This means that a DVD drive or a BBA is required to flash anyting to a Qoob SX.\
If this is inconvenient, consider upgrading to PicoBoot.

> [!TIP]
> Alternatively, if gekkoboot is already installed, or you have access to another
> boot method, you can disconnect the modchip from its cable and reconnect it
> while the system is running.
> This will disable write protection, and you can run the updater from there.

[^qoob-bba]: See the Qoob documentation for more information.

### ViperGC

Flash `gekkoboot_viper.vgc` as a BIOS like you normally would.

The hardware flasher is recommended, as it will be impossible to reflash the
Viper without it once gekkoboot is installed.
If you've lost it, consider upgrading to PicoBoot.

### GameCube Memory Card

Copy `gekkoboot_memcard.gci` to a memory card using [GCMM] or Swiss.
It will be saved as `boot.dol` and can be used in conjunction with the various
[game save exploits](https://www.gc-forever.com/wiki/index.php?title=Booting_homebrew#Game_Save_Exploits).

[GCMM]: https://github.com/suloku/gcmm


## Troubleshooting

gekkoboot displays useful diagnostic messages as it attempts to load the selected DOL.
But it's so fast you may not have time to read or even see them.
If you hold the down direction on the D-Pad, the messages will remain on screen
until you let go.

When choosing a shortcut button, beware that some software checks for buttons
held at boot to alter certain behaviors.
If your software behaves differently when booted through gekkoboot, ensure the
assigned shortcut button is not used in this way.
For example, it is not recommended to assign Swiss to the B button as holding B
causes Swiss to disable the DVD drive.

Also on Qoob SX, the "backup" BIOS will run before gekkoboot, so it may interfere
with some shortcuts.

## Compiling

You should only need to compile gekkoboot if you want to modify to the source
code.
Otherwise, you can just download prebuilt binaries from the releases tab.

Prerequisites:
- [devkitPPC](https://devkitpro.org/wiki/Getting_Started),
  the compiler toolchain for GameCube and Wii homebrew
  (install the gamecube-dev group)\
  Ensure that the `DEVKITPRO` and `DEVKITPPC` environment variables are set
  correctly
- [Git](https://git-scm.com/), for cloning the repo
- [Meson](https://mesonbuild.com/), the build system
- [Ninja](https://ninja-build.org/), the build executor
- [7-Zip](https://www.7-zip.org/) (p7zip), to compress the executable
- [Python 3](https://www.python.org/), required by build scripts
- A C++ compiler for your build machine, for some build tools
- For Qoob Pro: an IPL ROM dump, see below

Note: on Windows, you'll want to install all the dependencies inside of MSYS2.

```console
# Clone this repo
$ git clone https://github.com/redolution/gekkoboot.git

# Ensure devkitPPC is in PATH
$ export PATH="$DEVKITPPC/bin:$PATH"

# Initialize the build system
$ meson setup . build --cross-file=devkitPPC.ini

# Build it!
$ meson compile -C build

# The binaries will be in the build directory
$ ls build
...
gekkoboot_memcard.gci
gekkoboot_pico.uf2
gekkoboot_qoob_pro.gcb
gekkoboot_viper.vgc
qoob_pro_gekkoboot_upgrade.elf
qoob_sx_gekkoboot_upgrade.elf
...
```

### Qoob Pro

Qoob Pro requires BS1 (early initialization code) from the original bootrom.
This can be obtained from an existing Qoob Pro BIOS or any IPL dump.

To use an existing Qoob Pro BIOS (easiest method),
grab `gekkoboot_qoob_pro.gcb` from the [latest release].

Alternatively, you can dump the IPL ROM from your own GameCube using Swiss:
1. Ensure file management is enabled in Swiss settings: press B, select the gear
   icon at bottom right, press R until you reach "Advanced Settings",
   scroll down to "File Management", press right to enable, save and exit.
2. Switch to the system device: Select the eject icon at bottom left,
   scroll right to "System", press A.
3. Scroll down to `ipl.bin`, press Z then X, and copy to your SD card.
4. Copy `ipl.bin` from your SD card to your computer.

You should now have a suitable IPL ROM.
1. Rename the file to `ipl.rom` and place it in the `res` directory.
1. If you've already run `meson setup`,
   run it again with `--reconfigure` for it to find the file.
1. The output should now say `Qoob Pro support: YES`.
1. Continue building normally.

[latest release]: https://github.com/redolution/gekkoboot/releases/latest
