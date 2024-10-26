#include "types.h"

#define NUM_BOOT_TYPE_NAMES 4
const char *boot_type_names[NUM_BOOT_TYPE_NAMES] = {
    /*BOOT_TYPE_NONE     (0)*/ "NONE",
    /*BOOT_TYPE_DOL      (1)*/ "DOL",
    /*BOOT_TYPE_ONBOARD  (2)*/ "ONBOARD",
    /*BOOT_TYPE_USBGECKO (3)*/ "USBGECKO",
};

const char *get_boot_type_name(BOOT_TYPE type)
{
    if (type < 0 || type >= NUM_BOOT_TYPE_NAMES)
    {
        return "Unknown";
    }
    return boot_type_names[type];
}
