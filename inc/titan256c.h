#ifndef TITAN_256C_H
#define TITAN_256C_H

#include <types.h>
#include <vdp_bg.h>
#include <sprite_eng.h>
#include "titan256c_res.h"
#include "compatibilities.h"
#include "titan256c_consts.h"

#define HINT_STRATEGY_0 0
#define HINT_STRATEGY_1 1
#define HINT_STRATEGY_2 2
#define HINT_STRATEGY_3 3
#define HINT_STRATEGY_TOTAL 4

const u16* getTitanCharsGradientColors ();
u16* getGradientColorsBuffer ();
/// Use 0 if no fading required
void setCurrentFadingStripForText (u16 currFadingStrip_);
void updateTextGradientColors ();

#define FADE_OUT_COLOR_STEPS 8 // Changing this value will affect assumptions made in fadingStepToBlack_pals() method for fading color calculations
#define FADE_OUT_STRIPS_SPLIT_CYCLES 4 // How many parts do we split the strips visited for fading calculation to aliviate lenghty execution
void fadingStepToBlack_pals (u16 currFadingStrip, u16 cycle, u16 titan256cHIntMode);

void setYPosFalling (u16 value);
u16 getYPosFalling ();

void loadTitan256cTileSet (u16 currTileIndex);
u16 loadTitan256cTileMap (VDPPlane plane, u16 currTileIndex);

void unpackPalettes ();
void freePalettes ();
u16* getPalettesData ();

/// Loads the palettes belonging to strip startingScreenStrip and startingScreenStrip + 1.
void enqueue2Pals (u16 startingScreenStrip);

#endif // TITAN_256C_H