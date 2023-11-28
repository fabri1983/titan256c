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

#define TITAN_CHARS_GRADIENT_SCROLL_FREQ 4
#define TITAN_CHARS_GRADIENT_MAX_COLORS 42
#define TITAN_CURR_GRADIENT_ELEMS 15

u16* getGradientColorsBuffer ();
void updateCharsGradientColors ();

#define FADE_OUT_COLOR_STEPS 8 // Only use multiple of 2. Changing this value will affect assumptions made in fadingStepToBlack() for fading color calculations
#define FADE_OUT_STRIPS_SPLIT_CYCLES 3 // In how many parts do we split the strips visited for fading calculation to aliviate lenghty execution
void fadingStepToBlack (s16 currFadingStrip, u16 cycle);

void unpackPalettes ();
void freePalettes ();
u16* getUnpackedPtr ();

void set2FirstStripsPals ();

#endif