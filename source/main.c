#include <stdio.h>
#include <stdlib.h>
#include <sdcard/gcsd.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <ogc/lwp_watchdog.h>
#include <fcntl.h>
#include <ogc/system.h>
#include "ffshim.h"
#include "fatfs/ff.h"
#include "utils.h"
#include "version.h"

#include "stub.h"

#define VERBOSE_LOGGING 0

u8 *dol = NULL;
int dol_argc = 0;
#define MAX_NUM_ARGV 1024
char *dol_argv[MAX_NUM_ARGV];
u16 all_buttons_held;

char *default_path = "/ipl.dol";
struct shortcut {
  u16 pad_buttons;
  char *path;
} shortcuts[] = {
  {PAD_BUTTON_A,     "/a.dol"    },
  {PAD_BUTTON_B,     "/b.dol"    },
  {PAD_BUTTON_X,     "/x.dol"    },
  {PAD_BUTTON_Y,     "/y.dol"    },
  {PAD_TRIGGER_Z,    "/z.dol"    },
  {PAD_BUTTON_START, "/start.dol"},
  // NOTE: Shouldn't use L, R or Joysticks as analog inputs are calibrated on boot.
  // Should also avoid D-Pad as it is used for special functionality.
};
int num_shortcuts = sizeof(shortcuts)/sizeof(shortcuts[0]);

void dol_alloc(int size)
{
    int mram_size = (SYS_GetArenaHi() - SYS_GetArenaLo());
    kprintf("Memory available: %iB\n", mram_size);

    kprintf("DOL size is %iB\n", size);

    if (size <= 0)
    {
        kprintf("Empty DOL\n");
        return;
    }

    dol = (u8 *) memalign(32, size);

    if (!dol)
    {
        kprintf("Couldn't allocate memory\n");
    }
}

void load_parse_cli(char *path)
{
    int path_length = strlen(path);
    path[path_length - 3] = 'c';
    path[path_length - 2] = 'l';
    path[path_length - 1] = 'i';

    kprintf("Reading %s\n", path);
    FIL file ATTRIBUTE_ALIGN (32);
    FRESULT result = f_open(&file, path, FA_READ);
    if (result != FR_OK)
    {
        if (result == FR_NO_FILE)
        {
            kprintf("CLI file not found\n");
        }
        else
        {
            kprintf("Failed to open CLI file: %s\n", get_fresult_message(result));
        }
        return;
    }

    size_t size = f_size(&file);
    kprintf("CLI file size is %iB\n", size);

    if (size <= 0)
    {
        kprintf("Empty CLI file\n");
        return;
    }

    char *cli = (char *) malloc(size + 1);

    if (!cli)
    {
        kprintf("Couldn't allocate memory for CLI file\n");
        return;
    }

    UINT _;
    f_read(&file, cli, size, &_);
    f_close(&file);

    if (cli[size - 1] != '\0')
    {
      cli[size] = '\0';
      size++;
    }

    // Parse CLI file
    // https://github.com/emukidid/swiss-gc/blob/a0fa06d81360ad6d173acd42e4dd5495e268de42/cube/swiss/source/swiss.c#L1236
    dol_argv[dol_argc] = path;
    dol_argc++;

    // First argument is at the beginning of the file
    if (cli[0] != '\r' && cli[0] != '\n')
    {
        dol_argv[dol_argc] = cli;
        dol_argc++;
    }

    // Search for the others after each newline
    for (int i = 0; i < size; i++)
    {
        if (cli[i] == '\r' || cli[i] == '\n')
        {
            cli[i] = '\0';
        }
        else if (cli[i - 1] == '\0')
        {
            dol_argv[dol_argc] = cli + i;
            dol_argc++;
            if (dol_argc >= MAX_NUM_ARGV)
            {
                kprintf("Reached max of %i args.\n", MAX_NUM_ARGV);
                break;
            }
        }
    }
    
    kprintf("Found %i CLI args\n", dol_argc);

    #if VERBOSE_LOGGING
    for (int i = 0; i < dol_argc; ++i) {
        kprintf("arg%i: %s\n", i, dol_argv[i]);
    }
    #endif
}

int load_fat(const char *slot_name, const DISC_INTERFACE *iface_, char **paths, int num_paths)
{
    int res = 0;

    kprintf("Trying %s\n", slot_name);

    FATFS fs;
    iface = iface_;
    FRESULT mount_result = f_mount(&fs, "", 1);
    if (mount_result != FR_OK)
    {
        kprintf("Couldn't mount %s: %s\n", slot_name, get_fresult_message(mount_result));
        goto end;
    }

    char name[256];
    f_getlabel(slot_name, name, NULL);
    kprintf("Mounted %s as %s\n", name, slot_name);

    for (int i = 0; i < num_paths; ++i)
    {
        char *path = paths[i];
        kprintf("Reading %s\n", path);
        FIL file ATTRIBUTE_ALIGN (32);
        FRESULT open_result = f_open(&file, path, FA_READ);
        if (open_result != FR_OK)
        {
            kprintf("Failed to open file: %s\n", get_fresult_message(open_result));
            continue;
        }

        size_t size = f_size(&file);
        dol_alloc(size);
        if (!dol)
        {
            continue;
        }
        UINT _;
        f_read(&file, dol, size, &_);
        f_close(&file);

        // Attempt to load and parse CLI file
        load_parse_cli(path);

        res = 1;
        break;
    }

    kprintf("Unmounting %s\n", slot_name);
    iface->shutdown();
    iface = NULL;

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

int load_usb(char slot)
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

    kprintf("Waiting for ack (5s timeout)...\n");
    u64 current_time, start_time = gettime();

    while ((data != PC_READY) && (data != PC_OK))
    {
        current_time = gettime();
        if (diff_sec(start_time, current_time) >= 5) 
        {
            kprintf("PC did not respond in time\n");
            res = 0;
            goto end;
        }

        usb_recvbuffer_safe_ex(channel, &data, 1, 10); // 10 retries
    }

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

    dol_alloc(size);
    unsigned char* pointer = dol;

    if(!dol)
    {
        res = 0;
        goto end;
    }

    kprintf("Receiving file...\n");
    while (size > 0xF7D8)
    {
        usb_recvbuffer_safe(channel, (void *) pointer, 0xF7D8);
        size -= 0xF7D8;
        pointer += 0xF7D8;
    }
    if(size)
        usb_recvbuffer_safe(channel, (void *) pointer, size);

end:
    return res;
}

extern u8 __xfb[];

void delay_exit() {
    // Wait while the d-pad down direction or reset button is held.
    if (all_buttons_held & PAD_BUTTON_DOWN)
    {
        kprintf("(release d-pad down to continue)\n");
    }
    if (SYS_ResetButtonDown())
    {
        kprintf("(release reset button to continue)\n");
    }

    while (all_buttons_held & PAD_BUTTON_DOWN || SYS_ResetButtonDown())
    {
        VIDEO_WaitVSync();
        PAD_ScanPads();
        all_buttons_held = (
            PAD_ButtonsHeld(PAD_CHAN0) |
            PAD_ButtonsHeld(PAD_CHAN1) |
            PAD_ButtonsHeld(PAD_CHAN2) |
            PAD_ButtonsHeld(PAD_CHAN3)
        );
    }
}

int main()
{
    // GCVideo takes a while to boot up.
    // If VIDEO_GetPreferredMode is called before it's done,
    // it will not see the "component cable", and default to interlaced mode,
    // causing extraneous mode switches in the chainloaded application.
    u64 start = gettime();
    if (SYS_GetProgressiveScan()) {
        while (!VIDEO_HaveComponentCable()) {
            if (diff_sec(start, gettime()) >= 1) {
                SYS_SetProgressiveScan(0);
                break;
            }
        }
    }

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

    kprintf("\n\ngekkoboot %s\n", version);

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

    PAD_ScanPads();

    all_buttons_held = (
        PAD_ButtonsHeld(PAD_CHAN0) |
        PAD_ButtonsHeld(PAD_CHAN1) |
        PAD_ButtonsHeld(PAD_CHAN2) |
        PAD_ButtonsHeld(PAD_CHAN3)
    );

    if (all_buttons_held & PAD_BUTTON_LEFT || SYS_ResetButtonDown())
    {
        // Since we've disabled the Qoob, we wil reboot to the Nintendo IPL
        kprintf("Skipped. Rebooting into original IPL...\n");
        delay_exit();
        return 0;
    }

    char *paths[2];
    int num_paths = 0;

    for (int i = 0; i < num_shortcuts; i++) {
      if (all_buttons_held & shortcuts[i].pad_buttons) {
        paths[num_paths++] = shortcuts[i].path;
        break;
      }
    }

    paths[num_paths++] = default_path;

    if (load_usb('B')) goto load;

    if (load_fat("sdb", &__io_gcsdb, paths, num_paths)) goto load;

    if (load_usb('A')) goto load;

    if (load_fat("sda", &__io_gcsda, paths, num_paths)) goto load;

    if (load_fat("sd2", &__io_gcsd2, paths, num_paths)) goto load;

    if (load_fat("gcl", &__io_gcode, paths, num_paths)) goto load;

load:
    if (!dol)
    {
        kprintf("No DOL found! Halting.");
        while (true) {
            VIDEO_WaitVSync();
        }
    }
    
    struct __argv dolargs;
    dolargs.commandLine = (char *) NULL;
    dolargs.length = 0;
    
    // https://github.com/emukidid/swiss-gc/blob/f5319aab248287c847cb9468325ebcf54c993fb1/cube/swiss/source/aram/sidestep.c#L350
    if (dol_argc)
    {
        dolargs.argvMagic = ARGV_MAGIC;
        dolargs.argc = dol_argc;
        dolargs.length = 1;

        for (int i = 0; i < dol_argc; i++)
        {
            size_t arg_length = strlen(dol_argv[i]) + 1;
            dolargs.length += arg_length;
        }

        kprintf("CLI argv size is %iB\n", dolargs.length);
        dolargs.commandLine = (char *) malloc(dolargs.length);

        if (!dolargs.commandLine)
        {
            kprintf("Couldn't allocate memory for CLI argv\n");
            dolargs.length = 0;
        }
        else
        {
            unsigned int position = 0;
            for (int i = 0; i < dol_argc; i++)
            {
                size_t arg_length = strlen(dol_argv[i]) + 1;
                memcpy(dolargs.commandLine + position, dol_argv[i], arg_length);
                position += arg_length;
            }
            dolargs.commandLine[dolargs.length - 1] = '\0';
            DCStoreRange(dolargs.commandLine, dolargs.length);
        }
    }

    memcpy((void *) STUB_ADDR, stub, (size_t) stub_size);
    DCStoreRange((void *) STUB_ADDR, (u32) stub_size);

    delay_exit();

    SYS_ResetSystem(SYS_SHUTDOWN, 0, FALSE);
    SYS_SwitchFiber((intptr_t) dol, 0,
                    (intptr_t) dolargs.commandLine, dolargs.length,
                    STUB_ADDR, STUB_STACK);
    return 0;
}
