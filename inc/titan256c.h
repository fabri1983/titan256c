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
// in case you were to split any calculation over the colors of strip by an odd divisor
#define TITAN_256C_COLORS_PER_STRIP_REMINDER(n) (TITAN_256C_COLORS_PER_STRIP - n*(TITAN_256C_COLORS_PER_STRIP/n))
#define TITAN_256C_TEXT_STARTING_STRIP 21
#define TITAN_256C_TEXT_ENDING_STRIP 25

#define TITAN_CHARS_GRADIENT_SCROLL_FREQ 4
#define TITAN_CHARS_GRADIENT_MAX_COLORS 42
#define TITAN_CHARS_CURR_GRADIENT_ELEMS 16

u16* getGradientColorsBuffer ();
void updateTextGradientColors (u16 currFadingStrip);

#define FADE_OUT_COLOR_STEPS 8 // Changing this value will affect assumptions made in fadingStepToBlack_pals() method for fading color calculations
#define FADE_OUT_STRIPS_SPLIT_CYCLES 4 // How many parts do we split the strips visited for fading calculation to aliviate lenghty execution
void fadingStepToBlack_pals (u16 currFadingStrip, u16 cycle, u16 titan256cHIntMode);

void setYPosFalling (u16 value);
u16 getYPosFalling ();

void loadTitan256cTileSet (u16 currTileIndex);
u16 loadTitan256cTileMap (VDPPlane plane, u16 currTileIndex);
void unpackPalettes ();
void freePalettes ();
u16* getUnpackedPtr ();

/// Loads the palettes belonging to strip stripN and stripN + 1.
void load2StripsPals (u16 stripN);

#endif