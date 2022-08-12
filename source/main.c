#include <stdio.h>
#include <stdlib.h>
#include <sdcard/gcsd.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <ogc/lwp_watchdog.h>
#include <fcntl.h>
#include <ogc/system.h>
#include <gccore.h>
#include "shortcut.h"
#include "filesystem.h"
#include "cli_args.h"

#include "stub.h"
#define STUB_ADDR  0x80001000
#define STUB_STACK 0x80003000

// Global State
// --------------------
int debug_enabled = false;
u16 all_buttons_held;
extern u8 __xfb[];
// --------------------

char *default_path = "/ipl.dol";

void scan_all_buttons_held()
{
    PAD_ScanPads();
    all_buttons_held = (
        PAD_ButtonsHeld(PAD_CHAN0) |
        PAD_ButtonsHeld(PAD_CHAN1) |
        PAD_ButtonsHeld(PAD_CHAN2) |
        PAD_ButtonsHeld(PAD_CHAN3)
    );
}

void wait_for_confirmation()
{
    // Wait until the A button or reset button is pressed.
    int cur_state = true;
    int last_state;
    do
    {
        VIDEO_WaitVSync();
        scan_all_buttons_held();
        last_state = cur_state;
        cur_state = all_buttons_held & PAD_BUTTON_A;
    }
    while (last_state || !cur_state);
}

void delay_exit()
{
    if (debug_enabled)
    {
        // When debug is enabled, always wait for confirmation before exit.
        kprintf("\nDEBUG: Press A to continue...\n");
        wait_for_confirmation();
        return;
    }
}

void load_parse_cli(struct __argv *argv, char *dol_path)
{
    size_t path_len = strlen(dol_path);
    if (path_len < 5 || strncmp(dol_path + path_len - 4, ".dol", 4) != 0)
    {
        kprintf("Not reading CLI file: DOL path does not end in \".dol\"\n");
        return;
    }

    char path[path_len + 1];
    memcpy(path, dol_path, path_len - 3);
    path[path_len - 3] = 'c';
    path[path_len - 2] = 'l';
    path[path_len - 1] = 'i';
    path[path_len    ] = '\0';

    kprintf("Reading %s\n", path);
    const char *cli;
    FS_RESULT result = fs_read_file_string(&cli, path);
    if (!result)
    {
        return;
    }

    parse_cli_args(argv, cli);
}

int load_fat(const char *slot_name, const DISC_INTERFACE *iface_, char **paths, int num_paths, u8 **dol, struct __argv *argv)
{
    int res = 0;

    kprintf("Trying %s\n", slot_name);

    FS_RESULT mount_result = fs_mount(iface_);
    if (mount_result != FS_OK)
    {
        kprintf("Couldn't mount %s: %s\n", slot_name, get_fs_result_message(mount_result));
        goto end;
    }

    char name[256];
    fs_get_volume_label(slot_name, name);
    kprintf("Mounted %s as %s\n", name, slot_name);

    for (int i = 0; i < num_paths; ++i)
    {
        char *path = paths[i];
        kprintf("Reading %s\n", path);
        FS_RESULT read_result = fs_read_file((void **)dol, path);
        if (read_result != FS_OK)
        {
            continue;
        }

        // Attempt to load and parse CLI file
        load_parse_cli(argv, path);

        res = 1;
        break;
    }

    kprintf("Unmounting %s\n", slot_name);
    fs_unmount();

end:
    return res;
}

unsigned int convert_int(unsigned int in)
{
    unsigned int out;
    char *p_in = (char *) &in;
    char *p_out = (char *) &out;
    p_out[0] = p_in[3];
    p_out[1] = p_in[2];
    p_out[2] = p_in[1];
    p_out[3] = p_in[0];
    return out;
}

#define PC_READY 0x80
#define PC_OK    0x81
#define GC_READY 0x88
#define GC_OK    0x89

int load_usb(char slot, u8 **dol_)
{
    kprintf("Trying USB Gecko in slot %c\n", slot);

    int channel, res = 1;

    switch (slot)
    {
    case 'B':
        channel = 1;
        break;

    case 'A':
    default:
        channel = 0;
        break;
    }

    if (!usb_isgeckoalive(channel))
    {
        kprintf("Not present\n");
        res = 0;
        goto end;
    }

    usb_flush(channel);

    char data;

    kprintf("Sending ready\n");
    data = GC_READY;
    usb_sendbuffer_safe(channel, &data, 1);

    kprintf("Waiting for ack...\n");
    while ((data != PC_READY) && (data != PC_OK))
        usb_recvbuffer_safe(channel, &data, 1);

    if(data == PC_READY)
    {
        kprintf("Respond with OK\n");
        // Sometimes the PC can fail to receive the byte, this helps
        usleep(100000);
        data = GC_OK;
        usb_sendbuffer_safe(channel, &data, 1);
    }

    kprintf("Getting DOL size\n");
    int size;
    usb_recvbuffer_safe(channel, &size, 4);
    size = convert_int(size);

    if (size <= 0)
    {
        kprintf("DOL is empty\n");
        return res;
    }
    kprintf("DOL size is %iB\n", size);

    u8 *dol = (u8 *)malloc(size);

    if(!dol)
    {
        res = 0;
        goto end;
    }

    kprintf("Receiving file...\n");
    unsigned char *pointer = dol;
    while (size > 0xF7D8)
    {
        usb_recvbuffer_safe(channel, (void *) pointer, 0xF7D8);
        size -= 0xF7D8;
        pointer += 0xF7D8;
    }
    if(size)
        usb_recvbuffer_safe(channel, (void *) pointer, size);

    *dol_ = dol;

end:
    return res;
}

int main()
{
    VIDEO_Init();
    PAD_Init();
    GXRModeObj *rmode = VIDEO_GetPreferredMode(NULL);
    VIDEO_Configure(rmode);
    VIDEO_SetNextFramebuffer(__xfb);
    VIDEO_SetBlack(FALSE);
    VIDEO_Flush();
    VIDEO_WaitVSync();
    VIDEO_WaitVSync();
    CON_Init(__xfb, 0, 0, rmode->fbWidth, rmode->xfbHeight, rmode->fbWidth * VI_DISPLAY_PIX_SZ);

    kprintf("\n\niplboot\n");

    // Disable Qoob
    u32 val = 6 << 24;
    u32 addr = 0xC0000000;
    EXI_Lock(EXI_CHANNEL_0, EXI_DEVICE_1, NULL);
    EXI_Select(EXI_CHANNEL_0, EXI_DEVICE_1, EXI_SPEED8MHZ);
    EXI_Imm(EXI_CHANNEL_0, &addr, 4, EXI_WRITE, NULL);
    EXI_Sync(EXI_CHANNEL_0);
    EXI_Imm(EXI_CHANNEL_0, &val, 4, EXI_WRITE, NULL);
    EXI_Sync(EXI_CHANNEL_0);
    EXI_Deselect(EXI_CHANNEL_0);
    EXI_Unlock(EXI_CHANNEL_0);

    // Set the timebase properly for games
    // Note: fuck libogc and dkppc
    u32 t = ticks_to_secs(SYS_Time());
    settime(secs_to_ticks(t));

    scan_all_buttons_held();

    // Check if d-pad down direction or reset button is held.
    if (all_buttons_held & PAD_BUTTON_DOWN || SYS_ResetButtonDown())
    {
        kprintf("DEBUG: Debug enabled.\n");
        debug_enabled = true;
    }

    if (all_buttons_held & PAD_BUTTON_LEFT || SYS_ResetButtonDown())
    {
        // Since we've disabled the Qoob, we wil reboot to the Nintendo IPL
        kprintf("Skipped. Rebooting into original IPL...\n");
        delay_exit();
        return 0;
    }

    int mram_size = SYS_GetArenaHi() - SYS_GetArenaLo();
    kprintf("Memory available: %iB\n", mram_size);

    char *paths[2];
    int num_paths = 0;

    for (int i = 0; i < NUM_SHORTCUTS; i++) {
      if (all_buttons_held & shortcuts[i].pad_buttons) {
        paths[num_paths++] = shortcuts[i].path;
        break;
      }
    }

    paths[num_paths++] = default_path;

    u8 *dol;
    struct __argv argv;
    argv.argvMagic = ARGV_MAGIC;

    if (load_usb('B', &dol)) goto load;

    if (load_fat("sdb", &__io_gcsdb, paths, num_paths, &dol, &argv)) goto load;

    if (load_usb('A', &dol)) goto load;

    if (load_fat("sda", &__io_gcsda, paths, num_paths, &dol, &argv)) goto load;

    if (load_fat("sd2", &__io_gcsd2, paths, num_paths, &dol, &argv)) goto load;

load:
    if (!dol)
    {
        // If we reach here, all attempts to load a DOL failed
        // Since we've disabled the Qoob, we wil reboot to the Nintendo IPL
        kprintf("No DOL loaded. Rebooting into original IPL...\n");
        delay_exit();
        return 0;
    }

    // Print DOL args.
    if (debug_enabled)
    {
        if (argv.length > 0)
        {
            kprintf("\nDEBUG: About to print CLI args. Press A to continue...\n");
            wait_for_confirmation();
            kprintf("----------\n");
            size_t position = 0;
            for (int i = 0; i < argv.argc; ++i)
            {
                kprintf("arg%i: %s\n", i, argv.commandLine + position);
                position += strlen(argv.commandLine + position) + 1;
            }
            kprintf("----------\n\n");
        }
        else
        {
            kprintf("DEBUG: No CLI args\n");
        }
    }
    
    // Prepare DOL argv.
    if (argv.length > 0)
    {
        DCStoreRange(argv.commandLine, argv.length);
    }

    memcpy((void *) STUB_ADDR, stub, stub_size);
    DCStoreRange((void *) STUB_ADDR, stub_size);

    delay_exit();

    SYS_ResetSystem(SYS_SHUTDOWN, 0, FALSE);
    SYS_SwitchFiber((intptr_t) dol, 0,
                    (intptr_t) argv.commandLine, argv.length,
                    STUB_ADDR, STUB_STACK);
    return 0;
}
