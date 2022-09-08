#include "cli_args.h"
#include <stdlib.h>
#include <string.h>
#include <ogc/system.h>

#define MAX_NUM_ARGV 1024

// 0 - Failure
// 1 - OK/Empty
int parse_cli_args(struct __argv *argv, char *cli_option_str)
{
    kprintf("Parsing CLI args...\n");

    argv->argc = 0;
    argv->length = 0;
    argv->commandLine = NULL;

    char *cli = cli_option_str;
    char *dol_argv[MAX_NUM_ARGV];
    int dol_argc = 0;
    int size = strlen(cli_option_str);

    // Parse CLI file
    // https://github.com/emukidid/swiss-gc/blob/a0fa06d81360ad6d173acd42e4dd5495e268de42/cube/swiss/source/swiss.c#L1236
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

    if (!dol_argc)
    {
        return 0;
    }

    // https://github.com/emukidid/swiss-gc/blob/f5319aab248287c847cb9468325ebcf54c993fb1/cube/swiss/source/aram/sidestep.c#L350
    argv->argc = dol_argc;
    argv->length = 1;

    for (int i = 0; i < dol_argc; i++)
    {
        size_t arg_length = strlen(dol_argv[i]) + 1;
        argv->length += arg_length;
    }

    kprintf("CLI argv size is %iB\n", argv->length);
    argv->commandLine = (char *)malloc(argv->length);

    if (!argv->commandLine)
    {
        kprintf("Couldn't allocate memory for CLI argv\n");
        argv->length = 0;
        return 0;
    }

    unsigned int position = 0;
    for (int i = 0; i < dol_argc; i++)
    {
        size_t arg_length = strlen(dol_argv[i]) + 1;
        memcpy(argv->commandLine + position, dol_argv[i], arg_length);
        position += arg_length;
    }
    argv->commandLine[argv->length - 1] = '\0';

    return 1;
}