#include "shortcut.h"
#include <ogc/pad.h>

SHORTCUT shortcuts[NUM_SHORTCUTS] = {
    {"default", 0,                "/ipl.dol"  },
    {"A",       PAD_BUTTON_A,     "/a.dol"    },
    {"B",       PAD_BUTTON_B,     "/b.dol"    },
    {"X",       PAD_BUTTON_X,     "/x.dol"    },
    {"Y",       PAD_BUTTON_Y,     "/y.dol"    },
    {"Z",       PAD_TRIGGER_Z,    "/z.dol"    },
    {"START",   PAD_BUTTON_START, "/start.dol"},
    // NOTE: Shouldn't use L, R or Joysticks as analog inputs are calibrated on boot.
    // Should also avoid D-Pad as it is used for special functionality.
};
