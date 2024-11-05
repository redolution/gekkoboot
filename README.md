# gekkoboot

A utilitarian bootloader for the GameCube

Supported targets:
- PicoBoot
- Qoob Pro and Qoob SX
- ViperGC and ViperGC Extreme
- Savegame exploits
- Swiss in-game reset (IGR)

## Usage

gekkoboot will attempt to load DOLs from the following devices in order:
1. USB Gecko in Card Slot B
2. SD Gecko in Card Slot B
3. USB Gecko in Card Slot A
4. SD Gecko in Card Slot A
5. SD2SP2

For each device, gekkoboot checks for the presence of the following files in order:
1. `gekkoboot.ini`
2. File matching held button (`a.dol`, `x.dol`, etc.)
3. `ipl.dol`

If no file is found, the next device is attempted. After all devices are attempted, the system will reboot into the onboard IPL (original GameCube intro and menu).

Creating an `gekkoboot.ini` configuration file is the recommended route.

> [!IMPORTANT]
> Be careful not to touch any of the analog controls (sticks and triggers) when powering on as this is when they are calibrated.

**Something not working?** See the [troubleshooting section](#troubleshooting).

### Configuration

Create a file named `gekkoboot.ini` at the root of your SD card. This file should be formatted using INI syntax, which is basic `NAME=some value`. The following parameters are supported:

> [!IMPORTANT]
> All values are case sensitive.

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

For example, this configuration would boot straight into Swiss by default,
or GBI if you held B, or the original GameCube intro if you held Z:

```ini
DEFAULT=swiss.dol
B=gbi.dol
Z=ONBOARD
```

This configuration would boot into the original GameCube intro by default,
or Swiss if you held Z, or GBI if you held X or Y:

```ini
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

> [!IMPORTANT]
> If a config file is found, this behavior does not apply.

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

For example, this configuration would boot straight into Swiss by default,
or GBI if you held B, or the original GameCube intro if you held D-Pad Left:
- `/ipl.dol` - Swiss
- `/b.dol` - GBI

This configuration would boot into the original GameCube intro by default,
or Swiss if you held Z, or GBI if you held B:
- `/z.dol` - Swiss
- `/b.dol` - GBI

> [!TIP]
> You can prevent files from showing in Swiss by marking them as hidden files on the SD card.

### Special Features

CLI files are supported. They will be append after any CLI args defined in the config file.

Holding D-Pad Left or the reset button will skip gekkoboot functionality and
reboot straight into the onboard IPL.

Holding D-Pad Down enables debug mode. You can hold this along with a shortcut button.

gekkoboot also acts as a server for @emukidid's [usb-load](https://github.com/emukidid/gc-usb-load),
should you want to use it for development purposes.

## Installation

Download and extract the [latest release].

Prepare your SD card by creating a configuration file and/or copying DOLs onto the SD card and renaming them
according to either the [Configuration](#configuration) or [Button Files](#button-files) instructions above.

### PicoBoot

gekkoboot is bundled with the [PicoBoot] firmware.
Just follow the [update guide][pb-update].

You can also update gekkoboot separately from the PicoBoot firmware,
using the supplied `gekkoboot_pico.uf2`, and following the same procedure
(requires PicoBoot 0.4 or later).

[PicoBoot]: https://github.com/webhdx/PicoBoot
[pb-update]: https://support.webhdx.dev/gc/picoboot/update-picoboot

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
~~If this is inconvenient, consider upgrading to PicoBoot.~~
This will be improved in a future release.

> [!TIP]
> Alternatively, if gekkoboot is already installed, or you have access to another
> boot method, you can disconnect the modchip from its cable and reconnect it
> while the system is running.
> This will disable write protection, and you can run the updater from there.

[^qoob-bba]: See the Qoob documentation for more information.

### ViperGC

> [!IMPORTANT]
> ViperGC is the most limited modchip family.
> Because of its design, it's incapable of overriding BS1 or communicating with
> software after the initial boot.\
> This means that support is considered best-effort,
> and some upcoming features may never be available for Viper.
>
> Please consider upgrading to PicoBoot.

Flash `gekkoboot_viper.vgc` as a BIOS like you normally would.

The hardware flasher is recommended, as it is required to recover from bricks,
and there is no way to update gekkoboot via software after the initial installation.\
Software flashing is planned.

### GameCube Memory Card

Copy `gekkoboot_memcard.gci` to a memory card using [GCMM] or Swiss.
It will be saved as `boot.dol` and can be used in conjunction with the various
[game save exploits](https://www.gc-forever.com/wiki/index.php?title=Booting_homebrew#Game_Save_Exploits).

[GCMM]: https://github.com/suloku/gcmm

### Swiss in-game reset (IGR)

1. Copy the contents of the `swiss_igr` folder to your SD card,
   retaining the directory structure.
1. In Swiss, go to global game settings, and set in-game reset to "Apploader".

> [!NOTE]
> When using a Qoob, the recommended IGR mode is "Reboot".
> Swiss will re-enable the modchip and do a hard reset,
> triggering the usual boot process.

## Troubleshooting

Enable debug mode by holding d-pad in the down direction. This will allow you to read the diagnostic messages as well as enable more verbose output. Look for warning messages about unrecognized configuration parameters, file read failures, etc.

If multiple shortcut buttons are held, the highest in the table takes priority.

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
