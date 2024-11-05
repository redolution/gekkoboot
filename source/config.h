#ifndef INC_CONFIG_H
#define INC_CONFIG_H
#include "shortcut.h"
#include "types.h"

typedef struct {
	int debug_enabled;
	BOOT_ACTION shortcut_actions[NUM_SHORTCUTS];
} CONFIG;

extern const char *default_config_path;
int
parse_config(CONFIG *config, const char *config_str);
void
print_config(CONFIG *config);

#endif
