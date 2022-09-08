#include "config.h"
#include "inih/ini.h"
#include "filesystem.h"
#include <string.h>
#include <malloc.h>
#include <ogc/system.h>

const char *default_config_path = "/iplboot.ini";

void default_config(CONFIG *config)
{
    // TODO: Should free strings if this function will ever be used more than once.
    config->debug_enabled = false;

    for (int i = 0; i < NUM_SHORTCUTS; ++i)
    {
        config->shortcut_actions[i].type = BOOT_TYPE_NONE;
        config->shortcut_actions[i].dol_path = NULL;
        config->shortcut_actions[i].dol_cli_options_strs = NULL;
        config->shortcut_actions[i].num_dol_cli_options_strs = 0;
    }
    config->shortcut_actions[0].type = BOOT_TYPE_DOL;
    config->shortcut_actions[0].dol_path = shortcuts[0].path;
}

void handle_boot_action(const char *value, BOOT_ACTION *action)
{
    if (strcmp(value, "ONBOARD") == 0)
    {
        action->type = BOOT_TYPE_ONBOARD;
    }
    else
    {
        action->type = BOOT_TYPE_DOL;
        action->dol_path = strdup(value);
    }
}

void handle_cli_options(const char *value, BOOT_ACTION *action)
{
    if (action->num_dol_cli_options_strs % 100 == 0)
    {
        action->dol_cli_options_strs = (const char **)realloc(
            action->dol_cli_options_strs,
            (action->num_dol_cli_options_strs + 100) * sizeof(const char *)
        );
    }
    action->dol_cli_options_strs[action->num_dol_cli_options_strs++] = strdup(value);
}

// 0 - Failure
// 1 - OK
int ini_parse_handler(void *_config, const char *section, const char *name, const char *value)
{
    CONFIG *config = (CONFIG *)_config;

    if (strcmp(name, "DEBUG") == 0)
    {
        config->debug_enabled = strcmp(value, "1") == 0;
        return 1;
    }

    for (int i = 0; i < NUM_SHORTCUTS; ++i)
    {
        if (strcmp(name, shortcuts[i].config_name) == 0)
        {
            handle_boot_action(value, &config->shortcut_actions[i]);
            return 1;
        }
        if (strcmp(name, shortcuts[i].config_cli_name) == 0)
        {
            handle_cli_options(value, &config->shortcut_actions[i]);
            return 1;
        }
    }

    kprintf("->> !! Unknown config entry: %s\n", name);
    // return 0;
    return 1;
}

// 0 - Failure
// 1 - OK
int parse_config(CONFIG *config, const char *config_str)
{
    default_config(config);
    return ini_parse_string(config_str, ini_parse_handler, config) == 0;
}

void print_config(CONFIG *config)
{
    kprintf("debug_enabled: %i\n", config->debug_enabled);
    for (int i = 0; i < NUM_SHORTCUTS; ++i)
    {
        kprintf("shortcuts[%s].action: %s\n", shortcuts[i].name, get_boot_type_name(config->shortcut_actions[i].type));
        if (config->shortcut_actions[i].type == BOOT_TYPE_DOL)
        {
            kprintf("shortcuts[%s].dol_path: %s\n", shortcuts[i].name, config->shortcut_actions[i].dol_path);
            // kprintf("shortcuts[%s].num_dol_cli_options_strs: %i\n", shortcuts[i].name, config->shortcut_actions[i].num_dol_cli_options_strs);
        }
    }
}
