#include "shortcut.h"
#include <ogc/pad.h>

SHORTCUT shortcuts[NUM_SHORTCUTS] = {
	{PAD_BUTTON_A, "/a.dol"},
	{PAD_BUTTON_B, "/b.dol"},
	{PAD_BUTTON_X, "/x.dol"},
	{PAD_BUTTON_Y, "/y.dol"},
	{PAD_TRIGGER_Z, "/z.dol"},
	{PAD_BUTTON_START, "/start.dol"},
	// NOTE: Shouldn't use L, R or Joysticks as analog inputs are calibrated on boot.
        // Should also avoid D-Pad as it is used for special functionality.
};
