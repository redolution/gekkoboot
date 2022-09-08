#ifndef INC_TYPES_H
#define INC_TYPES_H
#include <gctypes.h>

typedef enum
{
    BOOT_TYPE_NONE = 0,
    BOOT_TYPE_DOL,
    BOOT_TYPE_ONBOARD
    // Changes to this enum should also be made to boot_type_names in types.c
} BOOT_TYPE;

const char *get_boot_type_name(BOOT_TYPE type);

typedef struct
{
    BOOT_TYPE type;
    const char *dol_path;
    const char **dol_cli_options_strs;
    int num_dol_cli_options_strs;
} BOOT_ACTION;

typedef struct
{
    BOOT_TYPE type;
    u8 *dol_file;
    struct __argv argv;
} BOOT_PAYLOAD;

#endif


// Only used to silence erroneous undefined error in IDE
#ifdef TB_BUS_CLOCK_FIX
#ifndef TB_BUS_CLOCK
#define TB_BUS_CLOCK 0
#endif
#endif
