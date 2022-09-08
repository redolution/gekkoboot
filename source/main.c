#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <ogc/lwp_watchdog.h>
#include <fcntl.h>
#include <ogc/system.h>
#include <gccore.h>
#include "types.h"
#include "config.h"
#include "shortcut.h"
#include "filesystem.h"
#include "cli_args.h"

#ifndef DOLPHIN_BUILD
    #include <sdcard/gcsd.h>
    #include "stub.h"
    #define STUB_ADDR  0x80001000
    #define STUB_STACK 0x80003000
    extern u8 __xfb[];
#else
    #include <sdcard/wiisd_io.h>
    static void *__xfb = NULL;
#endif

// Global State
// --------------------
int debug_enabled = false;
u16 all_buttons_held;
// --------------------

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

// 0 - Failure
// 1 - OK/Does not exist
int read_dol_file(u8 **dol_file, const char *path)
{
    *dol_file = NULL;

    kprintf("Trying DOL file: %s\n", path);
    FS_RESULT result = fs_read_file((void **)dol_file, path);
    if (result == FS_OK)
    {
        kprintf("->> DOL loaded\n");
    }
    return (
           result == FS_OK
        || result == FS_NO_FILE
        || result == FS_FILE_EMPTY
    );
}

// 0 - Failure
// 1 - OK/Does not exist
int read_config_file(const char **config_file, const char *path)
{
    *config_file = NULL;

    kprintf("Trying config file: %s\n", path);
    FS_RESULT result = fs_read_file_string(config_file, path);
    if (result == FS_OK)
    {
        kprintf("->> Config loaded\n");
    }
    return (
           result == FS_OK
        || result == FS_NO_FILE
        || result == FS_FILE_EMPTY
    );
}

// 0 - Failure
// 1 - OK/Does not exist/Skipped
int read_cli_file(const char **cli_file, const char *dol_path)
{
    *cli_file = NULL;

    size_t path_len = strlen(dol_path);
    if (path_len < 5 || strncmp(dol_path + path_len - 4, ".dol", 4) != 0)
    {
        kprintf("Not reading CLI file: DOL path does not end in \".dol\"\n");
        return 1;
    }

    char path[path_len + 1];
    memcpy(path, dol_path, path_len - 3);
    path[path_len - 3] = 'c';
    path[path_len - 2] = 'l';
    path[path_len - 1] = 'i';
    path[path_len    ] = '\0';

    kprintf("Trying CLI file: %s\n", path);
    FS_RESULT result = fs_read_file_string(cli_file, path);
    if (result == FS_OK)
    {
        kprintf("->> CLI file loaded\n");
    }
    return (
           result == FS_OK
        || result == FS_NO_FILE
        || result == FS_FILE_EMPTY
    );
}

// 0 - Device should not be used.
// 1 - Device should be used.
int load_config(BOOT_PAYLOAD *payload, int shortcut_index)
{
    // Attempt to read config file from mounted FAT device.
    const char *config_file;
    if (!read_config_file(&config_file, default_config_path))
    {
        return 1;
    }
    if (!config_file)
    {
        return 0;
    }

    // Config file was found.
    // Default to no action in case of failure.
    int res = 1;
    payload->type = BOOT_TYPE_NONE;

    // Parse config file.
    CONFIG config;
    if (!parse_config(&config, config_file))
    {
        kprintf("->> !! Failed to parse config file\n");
        return res;
    }

    // Set global state.
    if (config.debug_enabled && !debug_enabled)
    {
        kprintf("DEBUG: Debug enabled by config.\n");
        debug_enabled = true;
    }

    // Print config.
    if (debug_enabled)
    {
        kprintf("\nDEBUG: About to print config. Press A to continue...\n");
        wait_for_confirmation();
        kprintf("----------\n");
        print_config(&config);
        kprintf("----------\n\n");
        kprintf("DEBUG: Done printing. Press A to continue...\n");
        wait_for_confirmation();
    }

    // Choose boot action.
    if (config.shortcut_actions[shortcut_index].type == BOOT_TYPE_NONE)
    {
        kprintf("\"%s\" shortcut not configured\n", shortcuts[shortcut_index].name);
        shortcut_index = 0;
    }
    kprintf("->> Using \"%s\" shortcut\n", shortcuts[shortcut_index].name);
    BOOT_ACTION *action = &config.shortcut_actions[shortcut_index];

    // Process boot action.
    if (action->type == BOOT_TYPE_ONBOARD)
    {
        kprintf("->> Shortcut action: Reboot to onboard IPL\n");
        payload->type = BOOT_TYPE_ONBOARD;
        return res;
    }

    if (action->type != BOOT_TYPE_DOL)
    {
        // Should never happen.
        kprintf("->> !! Internal Error: Unexpeted boot type: %i\n", action->type);
        return res;
    }

    kprintf("->> Shortcut action: Boot DOL\n");

    // Read DOL file.
    u8 *dol_file;
    if (!read_dol_file(&dol_file, action->dol_path) || !dol_file)
    {
        kprintf("->> !! Unable to read DOL\n");
        return res;
    }

    // Attempt to read CLI file.
    const char *cli_file;
    if (!read_cli_file(&cli_file, action->dol_path))
    {
        return res;
    }
    if (!cli_file)
    {
        kprintf("->> No CLI file\n");
    }

    // Combine CLI options from config and CLI file.
    const char **cli_options_strs = NULL;
    int num_cli_options_strs = 0;
    if (action->num_dol_cli_options_strs > 0)
    {
        cli_options_strs = action->dol_cli_options_strs;
        num_cli_options_strs = action->num_dol_cli_options_strs;
    }
    if (cli_file)
    {
        cli_options_strs = realloc(
            cli_options_strs,
            (num_cli_options_strs + 1) * sizeof(const char *)
        );
        cli_options_strs[num_cli_options_strs++] = cli_file;
    }

    // Parse CLI options.
    if (num_cli_options_strs > 0)
    {
        int parse_res = parse_cli_args(&payload->argv, cli_options_strs, num_cli_options_strs);
        if (cli_file)
        {
            free((void *)cli_file);
        }
        if (!parse_res)
        {
            return res;
        }
    }

    // Return DOL boot payload.
    payload->type = BOOT_TYPE_DOL;
    payload->dol_file = dol_file;
    return res;
}

// 0 - Device should not be used.
// 1 - Device should be used.
int load_shortcut_files(BOOT_PAYLOAD *payload, int shortcut_index)
{
    // Attempt to read shortcut paths from from mounted FAT device.
    u8 *dol_file = NULL;
    const char *dol_path = shortcuts[shortcut_index].path;
    if (!read_dol_file(&dol_file, dol_path))
    {
        return 1;
    }
    if (!dol_file && shortcut_index != 0)
    {
        shortcut_index = 0;
        dol_path = shortcuts[shortcut_index].path;
        if (!read_dol_file(&dol_file, dol_path))
        {
            return 1;
        }
    }
    if (!dol_file)
    {
        return 0;
    }

    kprintf("Will boot DOL\n");

    // Attempt to read CLI file.
    const char *cli_file;
    if (!read_cli_file(&cli_file, dol_path))
    {
        return 1;
    }
    if (!cli_file)
    {
        kprintf("->> No CLI file\n");
    }

    // Parse CLI file.
    if (cli_file)
    {
        int res = parse_cli_args(&payload->argv, &cli_file, 1);
        free((void *)cli_file);
        if (!res)
        {
            return 1;
        }
    }

    payload->type = BOOT_TYPE_DOL;
    payload->dol_file = dol_file;
    return 1;
}

// 0 - Device should not be used.
// 1 - Device should be used.
int load_fat(BOOT_PAYLOAD *payload, const char *device_name, const DISC_INTERFACE *iface, int shortcut_index)
{
    int res = 0;

    kprintf("Trying %s\n", device_name);

    // Mount device.
    FS_RESULT result = fs_mount(iface);
    if (result != FS_OK)
    {
        kprintf("Couldn't mount %s: %s\n", device_name, get_fs_result_message(result));
        goto end;
    }

    char volume_label[256];
    fs_get_volume_label(device_name, volume_label);
    kprintf("Mounted \"%s\" volume from %s\n", volume_label, device_name);

    // Attempt to load config or shortcut files.
    res = (
           load_config(payload, shortcut_index)
        || load_shortcut_files(payload, shortcut_index)
    );

    kprintf("Unmounting %s\n", device_name);
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

    payload->type = BOOT_TYPE_DOL;
    payload->dol_file = dol_file;
    return res;
}

#ifdef DOLPHIN_BUILD
int _main();
int main()
{
    int res = _main();
    kprintf("DOLPHIN: Exited with %i. Press A to exit...\n", res);
    wait_for_confirmation();
    return res;
}
int _main()
#else
int main()
#endif
{
    VIDEO_Init();
    PAD_Init();
    GXRModeObj *rmode = VIDEO_GetPreferredMode(NULL);
    VIDEO_Configure(rmode);
#ifdef DOLPHIN_BUILD
    __xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
#endif
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

#ifdef DOLPHIN_BUILD
    kprintf("DOLPHIN: Press button...\n");
    do {VIDEO_WaitVSync(); scan_all_buttons_held();}
    while (all_buttons_held == 0 || all_buttons_held == PAD_BUTTON_DOWN);
#endif
    scan_all_buttons_held();

    // Check if d-pad down direction or reset button is held.
    if (all_buttons_held & PAD_BUTTON_DOWN || SYS_ResetButtonDown())
    {
        kprintf("DEBUG: Debug enabled.\n");
        debug_enabled = true;
    }

    if (all_buttons_held & PAD_BUTTON_LEFT || SYS_ResetButtonDown())
    {
        kprintf("Skip enabled. Rebooting into original IPL...\n\n");
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
            kprintf("->> \"%s\" shortcut selected\n", shortcuts[i].name);
            shortcut_index = i;
            break;
        }
    }
    if (shortcut_index == 0)
    {
        kprintf("->> Using default shortcut\n");
    }

    // Init payload.
    BOOT_PAYLOAD payload;
    payload.type = BOOT_TYPE_NONE;
    payload.dol_file = NULL;
    payload.argv.argc = 0;
    payload.argv.length = 0;
    payload.argv.commandLine = NULL;
    payload.argv.argvMagic = ARGV_MAGIC;

    // Attempt to load from each device.
    int res = (
#ifndef DOLPHIN_BUILD
           load_fat(&payload, "SD Gecko in slot B", &__io_gcsdb, shortcut_index)
        || load_fat(&payload, "SD Gecko in slot A", &__io_gcsda, shortcut_index)
        || load_fat(&payload, "SD2SP2", &__io_gcsd2, shortcut_index)
        || load_usb(&payload, 'B')
        || load_usb(&payload, 'A')
#else
           load_fat(&payload, "Wii SD", &__io_wiisd, shortcut_index)
#endif
    );

    if (!res)
    {
        // If we reach here, we did not find a device with any shortcut files.
        kprintf("\nNo shortcuts found\n");
        kprintf("Rebooting into onboard IPL...\n\n");
        delay_exit();
        return 0;
    }

    if (payload.type == BOOT_TYPE_NONE)
    {
        // If we reach here, we found a device with shortcut files but failed to load any shortcut.
        kprintf("\nUnable to load shortcut\n");
        kprintf("Press A to reboot into onboard IPL...\n\n");
        wait_for_confirmation();
        return 0;
    }

    if (payload.type == BOOT_TYPE_ONBOARD)
    {
        kprintf("Rebooting into onboard IPL...\n\n");
        delay_exit();
        return 0;
    }

    if (payload.type != BOOT_TYPE_DOL)
    {
        // Should never happen.
        kprintf("\n->> !! Internal Error: Unexpected boot type: %i\n", payload.type);
        kprintf("Press A to reboot into onboard IPL...\n\n");
        wait_for_confirmation();
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

    kprintf("Booting DOL...\n");

#ifndef DOLPHIN_BUILD
    // Load stub.
    memcpy((void *)STUB_ADDR, stub, stub_size);
    DCStoreRange((void *)STUB_ADDR, stub_size);

    delay_exit();

    if (debug_enabled)
    {
        kprintf("DEBUG: Loading DOL...\n");
    }

    // Boot DOL.
    SYS_ResetSystem(SYS_SHUTDOWN, 0, FALSE);
    SYS_SwitchFiber((intptr_t)payload.dol_file, 0,
                    (intptr_t)payload.argv.commandLine, payload.argv.length,
                    STUB_ADDR, STUB_STACK);

    // Will never reach here.
    return 0;
#else
    delay_exit();

    kprintf("DOLPHIN: Success!\n");
    return 0;
#endif
}
