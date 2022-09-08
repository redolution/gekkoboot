#include "cli_args.h"
#include <stdlib.h>
#include <string.h>
#include <ogc/system.h>

#define MAX_NUM_ARGV 1024

// 0 - Failure
// 1 - OK/Empty
int parse_cli_args(struct __argv *argv, const char *cli_option_str)
{
    kprintf("Parsing CLI args...\n");

    argv->argc = 0;
    argv->length = 0;
    argv->commandLine = NULL;

    const char *args[MAX_NUM_ARGV];
    size_t arg_lengths[MAX_NUM_ARGV];
    int argc = 0;
    size_t argv_size = 1;

    int found_arg_start = false;
    size_t arg_start_index = 0;
    size_t arg_end_index = 0;

    int eof = false;
    for (size_t i = 0; !eof; i++)
    {
        eof = cli_option_str[i] == '\0';

        // Check if we are at the end of a line.
        if (cli_option_str[i] == '\n' || eof)
        {
            // Check if we ever found a start to the arg.
            if (found_arg_start)
            {
                // Record the arg.
                size_t line_len = (arg_end_index - arg_start_index) + 1;
                args[argc] = cli_option_str + arg_start_index;
                arg_lengths[argc] = line_len;
                argc++;
                argv_size += line_len + 1;

                if (argc == MAX_NUM_ARGV)
                {
                    kprintf("Reached max of %i args.\n", MAX_NUM_ARGV);
                    break;
                }

                // Reset.
                found_arg_start = false;
            }
        }
        // Check if we have a non-whitespace character.
        else if (cli_option_str[i] != ' ' && cli_option_str[i] != '\t' && cli_option_str[i] != '\r')
        {
            // Record the start and end of the arg.
            if (!found_arg_start)
            {
                found_arg_start = true;
                arg_start_index = i;
            }
            arg_end_index = i;
        }
    }

    if (argc == 0)
    {
        kprintf("No args found\n");
        return 1;
    }

    kprintf("Found %i args. Size is %iB\n", argc, argv_size);

    char *command_line = (char *)malloc(argv_size);
    if (!command_line)
    {
        kprintf("Couldn't allocate memory for args\n");
        return 0;
    }

    size_t position = 0;
    for (int i = 0; i < argc; i++)
    {
        memcpy(command_line + position, args[i], arg_lengths[i]);
        position += arg_lengths[i];
        command_line[position] = '\0';
        position += 1;
    }
    command_line[position] = '\0';

    argv->argc = argc;
    argv->length = argv_size;
    argv->commandLine = command_line;

    return 1;
}