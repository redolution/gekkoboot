#ifndef INC_SHORTCUT_H
#define INC_SHORTCUT_H
#include <gctypes.h>

#define NUM_SHORTCUTS 7

typedef struct
{
    const u16 pad_buttons;
    const char *const path;
} SHORTCUT;

extern SHORTCUT shortcuts[NUM_SHORTCUTS];

#endif
