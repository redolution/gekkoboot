#ifndef INC_CLI_ARGS_H
#define INC_CLI_ARGS_H
#include <gctypes.h>

int
parse_cli_args(struct __argv *argv, const char **cli_options_strs, int num_cli_options_strs);

#endif
