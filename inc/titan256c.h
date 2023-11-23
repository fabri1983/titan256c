#ifndef TITAN_256C_H
#define TITAN_256C_H

#include <types.h>
#include <vdp_bg.h>
#include <sprite_eng.h>
#include "titan256c_res.h"

#define TITAN_256C_STRIPS_COUNT 28
#define TITAN_256C_WIDTH 320 // In pixels
#define TITAN_256C_HEIGHT 224 // In pixels
#define TITAN_256C_STRIP_HEIGHT 8
#define TITAN_256C_COLORS_PER_STRIP 32
#define TITAN_256C_COLORS_PER_STRIP_REMINDER(n) (TITAN_256C_COLORS_PER_STRIP - n*(TITAN_256C_COLORS_PER_STRIP/n))

void unpackPalettes (const Palette32AllStrips* pals32);
void freePalettes (const Palette32AllStrips* pals32);
u16* getUnpackedPtr ();

#define TITAN_CHARS_GRADIENT_SCROLL_FREQ 4
#define TITAN_CHARS_GRADIENT_MAX_COLORS 42
#define TITAN_CURR_GRADIENT_ELEMS 15
const u16 titanCharsGradientColors[TITAN_CHARS_GRADIENT_MAX_COLORS];

#endif