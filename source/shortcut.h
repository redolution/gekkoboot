#ifndef INC_SHORTCUT_H
#define INC_SHORTCUT_H
#include <gctypes.h>

#define NUM_SHORTCUTS 6

typedef struct {
	u16 pad_buttons;
	char *path;
} SHORTCUT;

extern SHORTCUT shortcuts[NUM_SHORTCUTS];

#endif
