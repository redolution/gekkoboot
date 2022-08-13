#include "shortcut.h"
#include <ogc/pad.h>

SHORTCUT shortcuts[NUM_SHORTCUTS] = {
    {"default", 0,                "DEFAULT", "DEFAULT_ARG", "/ipl.dol"  },
    {"A",       PAD_BUTTON_A,     "A",       "A_ARG",       "/a.dol"    },
    {"B",       PAD_BUTTON_B,     "B",       "B_ARG",       "/b.dol"    },
    {"X",       PAD_BUTTON_X,     "X",       "X_ARG",       "/x.dol"    },
    {"Y",       PAD_BUTTON_Y,     "Y",       "Y_ARG",       "/y.dol"    },
    {"Z",       PAD_TRIGGER_Z,    "Z",       "Z_ARG",       "/z.dol"    },
    {"START",   PAD_BUTTON_START, "START",   "START_ARG",   "/start.dol"},
    // NOTE: Shouldn't use L, R or Joysticks as analog inputs are calibrated on boot.
    // Should also avoid D-Pad as it is used for special functionality.
};
