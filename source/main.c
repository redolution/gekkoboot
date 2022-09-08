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

// Only used to silence erroneous undefined error in IDE
#ifdef TB_BUS_CLOCK_FIX
#ifndef TB_BUS_CLOCK
#define TB_BUS_CLOCK 0
#endif
#endif

// Global State
// --------------------
int debug_enabled = false;
u16 all_buttons_held;
extern u8 __xfb[];
// --------------------

typedef struct
{
    u8 *dol_file;
    struct __argv argv;
} BOOT_PAYLOAD;

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
    }
}

void read_dol_file(u8 **dol_file, const char *path)
{
    *dol_file = NULL;

    kprintf("Reading %s\n", path);
    fs_read_file((void **)dol_file, path);
}

void read_cli_file(const char **cli_file, const char *dol_path)
{
    *cli_file = NULL;

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
    fs_read_file_string(cli_file, path);
}

// 0 - Device should not be used.
// 1 - Device should be used.
int load_shortcut_files(BOOT_PAYLOAD *payload, int shortcut_index)
{
    // Attempt to read shortcut paths from from mounted FAT device.
    u8 *dol_file = NULL;
    const char *dol_path = shortcuts[shortcut_index].path;
    read_dol_file(&dol_file, dol_path);
    if (!dol_file && shortcut_index != 0)
    {
        shortcut_index = 0;
        dol_path = shortcuts[shortcut_index].path;
        read_dol_file(&dol_file, dol_path);
    }
    if (!dol_file)
    {
        return 0;
    }

    // Attempt to read CLI file.
    const char *cli_file;
    read_cli_file(&cli_file, dol_path);

    // Parse CLI file.
    if (cli_file)
    {
        parse_cli_args(&payload->argv, cli_file);
        free((void *)cli_file);
    }

    payload->dol_file = dol_file;
    return 1;
}

// 0 - Device should not be used.
// 1 - Device should be used.
int load_fat(BOOT_PAYLOAD *payload, const char *slot_name, const DISC_INTERFACE *iface, int shortcut_index)
{
    int res = 0;

    kprintf("Trying %s\n", slot_name);

    // Mount device.
    FS_RESULT result = fs_mount(iface);
    if (result != FS_OK)
    {
        kprintf("Couldn't mount %s: %s\n", slot_name, get_fs_result_message(result));
        goto end;
    }

    char volume_label[256];
    fs_get_volume_label(slot_name, volume_label);
    kprintf("Mounted %s as %s\n", volume_label, slot_name);

    // Attempt to load shortcut files.
    res = load_shortcut_files(payload, shortcut_index);

    kprintf("Unmounting %s\n", slot_name);
    fs_unmount();

end:
    return res;
}

unsigned int convert_int(unsigned int in)
{
    unsigned int out;
    char *p_in = (char *)&in;
    char *p_out = (char *)&out;
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

// 0 - Device should not be used.
// 1 - Device should be used.
int load_usb(BOOT_PAYLOAD *payload, char slot)
{
    int res = 0;
    int channel = slot == 'B' ? 1 : 0;

    kprintf("Trying USB Gecko in slot %c\n", slot);

    if (!usb_isgeckoalive(channel))
    {
        kprintf("Not present\n");
        return res;
    }

    res = 1;

    usb_flush(channel);

    char data;

    kprintf("Sending ready\n");
    data = GC_READY;
    usb_sendbuffer_safe(channel, &data, 1);

    kprintf("Waiting for ack...\n");
    while ((data != PC_READY) && (data != PC_OK))
    {
        usb_recvbuffer_safe(channel, &data, 1);
    }

    if (data == PC_READY)
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

    u8 *dol_file = (u8 *)malloc(size);
    if (!dol_file)
    {
        kprintf("Couldn't allocate memory for DOL file\n");
        return res;
    }

    kprintf("Receiving file...\n");
    unsigned char *pointer = dol_file;
    while (size > 0xF7D8)
    {
        usb_recvbuffer_safe(channel, (void *)pointer, 0xF7D8);
        size -= 0xF7D8;
        pointer += 0xF7D8;
    }
    if (size)
    {
        usb_recvbuffer_safe(channel, (void *)pointer, size);
    }

    payload->dol_file = dol_file;
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
    // Since we've disabled the Qoob, we wil reboot to the Nintendo IPL

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
        kprintf("Skipped. Rebooting into original IPL...\n");
        delay_exit();
        return 0;
    }

    int mram_size = SYS_GetArenaHi() - SYS_GetArenaLo();
    kprintf("Memory available: %iB\n", mram_size);

    // Detect selected shortcut.
    int shortcut_index = 0;
    for (int i = 1; i < NUM_SHORTCUTS; i++)
    {
        if (all_buttons_held & shortcuts[i].pad_buttons)
        {
            shortcut_index = i;
            break;
        }
    }

    // Init payload.
    BOOT_PAYLOAD payload;
    payload.dol_file = NULL;
    payload.argv.argc = 0;
    payload.argv.length = 0;
    payload.argv.commandLine = NULL;
    payload.argv.argvMagic = ARGV_MAGIC;

    // Attempt to load from each device.
    int res = (
           load_usb(&payload, 'B')
        || load_fat(&payload, "sdb", &__io_gcsdb, shortcut_index)
        || load_usb(&payload, 'A')
        || load_fat(&payload, "sda", &__io_gcsda, shortcut_index)
        || load_fat(&payload, "sd2", &__io_gcsd2, shortcut_index)
    );

    if (!res || !payload.dol_file)
    {
        // If we reach here, all attempts to load a DOL failed
        kprintf("No DOL loaded. Rebooting into original IPL...\n");
        delay_exit();
        return 0;
    }

    // Print DOL args.
    if (debug_enabled)
    {
        if (payload.argv.length > 0)
        {
            kprintf("\nDEBUG: About to print CLI args. Press A to continue...\n");
            wait_for_confirmation();
            kprintf("----------\n");
            size_t position = 0;
            for (int i = 0; i < payload.argv.argc; ++i)
            {
                kprintf("arg%i: %s\n", i, payload.argv.commandLine + position);
                position += strlen(payload.argv.commandLine + position) + 1;
            }
            kprintf("----------\n\n");
        }
        else
        {
            kprintf("DEBUG: No CLI args\n");
        }
    }

    // Prepare DOL argv.
    if (payload.argv.length > 0)
    {
        DCStoreRange(payload.argv.commandLine, payload.argv.length);
    }

    // Load stub.
    memcpy((void *)STUB_ADDR, stub, stub_size);
    DCStoreRange((void *)STUB_ADDR, stub_size);

    delay_exit();

    // Boot DOL.
    SYS_ResetSystem(SYS_SHUTDOWN, 0, FALSE);
    SYS_SwitchFiber((intptr_t)payload.dol_file, 0,
                    (intptr_t)payload.argv.commandLine, payload.argv.length,
                    STUB_ADDR, STUB_STACK);

    // Will never reach here.
    return 0;
}
