#include <genesis.h>
#include "utils.h"
#include "segaLogo.h"
#include "teddyBearLogo.h"
#include "titan256c.h"
#include "hvInterrupts.h"

#define HINT_MODES 3
static u16 titan256cHIntMode;

#define GAME_STATE_TITAN256C_FALLING 0
#define GAME_STATE_TITAN256C_SHOW 1
#define GAME_STATE_TITAN256C_FADING_TO_BLACK 2
#define GAME_STATE_TITAN256C_NEXT 3

static u16 currTileIndex;

static void titan256c () {

    PAL_setColors(0, palette_black, 64, DMA); // palette_black is an array of 64
    SYS_doVBlankProcess();

    VDP_setEnable(FALSE);

    currTileIndex = TILE_USER_INDEX;
    loadTitan256cTileSet(currTileIndex);
    currTileIndex = loadTitan256cTileMap(BG_B, currTileIndex);
    unpackPalettes();

    SYS_disableInts();
        // Call the HInt every N scanlines. Uses CPU for palette swapping
        if (titan256cHIntMode == 0) {
            SYS_setVBlankCallback(vertIntOnTitan256cCallback_HIntEveryN);
            VDP_setHIntCounter(TITAN_256C_STRIP_HEIGHT - 1);
            SYS_setHIntCallback(horizIntOnTitan256cCallback_CPU_EveryN);
        }
        // Call the HInt every N scanlines. Uses DMA for palette swapping
        else if (titan256cHIntMode == 1) {
            SYS_setVBlankCallback(vertIntOnTitan256cCallback_HIntEveryN);
            VDP_setHIntCounter(TITAN_256C_STRIP_HEIGHT - 1);
            SYS_setHIntCallback(horizIntOnTitan256cCallback_DMA_EveryN);
        }
        // Use only one call to HInt to avoid method call overhead. Uses DMA for palette swapping
        else if (titan256cHIntMode == 2) {
            SYS_setVBlankCallback(vertIntOnTitan256cCallback_HIntOneTime);
            VDP_setHIntCounter(0);
            SYS_setHIntCallback(horizIntOnTitan256cCallback_DMA_OneTime);
        }
        VDP_setHInterrupt(TRUE);
    SYS_enableInts();

    VDP_setEnable(TRUE);

    u16 gameState = GAME_STATE_TITAN256C_FALLING;
    u16 fadingStripCnt = 0;
    u16 prevFadingStrip = 0xFF;
    u16 fadingCycle = 0; // use to split the fading to black into N cycles, due to its lenghty execution

    while (gameState != GAME_STATE_TITAN256C_NEXT) {

        // Load 1st and 2nd strip's palette
        set2FirstStripsPals();

        // Update ramp color effect for the titan text section
        updateCharsGradientColors();

        if (gameState == GAME_STATE_TITAN256C_FALLING) {
            // if bounce effect finished then continue with next game state
            gameState = GAME_STATE_TITAN256C_SHOW;
        }
        else if (gameState == GAME_STATE_TITAN256C_SHOW) {
            u16 joyState = JOY_readJoypad(JOY_1);
            if (joyState & BUTTON_START) {
                gameState = GAME_STATE_TITAN256C_FADING_TO_BLACK;
            }
        }
        // update fading to black 2 palettes per strip
        else if (gameState == GAME_STATE_TITAN256C_FADING_TO_BLACK) {
            // advance 1 strip every N frames. This must be >= FADE_OUT_STRIPS_SPLIT_CYCLES used to execute the fading for current strip
            u16 currFadingStrip = divu(fadingStripCnt++, 3); // Use divu() for N non power of 2
            // strip changed? let's do one fading step. fadingCycle > 0 means there are fading cycles to complete for current strip
            if (currFadingStrip != prevFadingStrip || fadingCycle > 0) {
                // apply fade to black from currFadingStrip up to FADE_OUT_STEPS previous strips
                fadingStepToBlack_pals(currFadingStrip, fadingCycle, titan256cHIntMode);
                fadingStepToBlack_text(currFadingStrip);
                prevFadingStrip = currFadingStrip;
                ++fadingCycle;
                if (fadingCycle == FADE_OUT_STRIPS_SPLIT_CYCLES) fadingCycle = 0;
            }
            // already passed last strip? then fading is finished
            if (currFadingStrip == (TITAN_256C_STRIPS_COUNT + FADE_OUT_COLOR_STEPS - 1)) {
                gameState = GAME_STATE_TITAN256C_NEXT;
            }
        }

        SYS_doVBlankProcess();
    }

    SYS_disableInts();
        SYS_setVBlankCallback(NULL);
        VDP_setHInterrupt(FALSE);
        SYS_setHIntCallback(NULL);
    SYS_enableInts();

    VDP_clearPlane(BG_B, TRUE);
    VDP_clearPlane(BG_A, TRUE);
    PAL_setColor(0x0, 0x000); // Set BG color as Black
    SYS_doVBlankProcess();

    freePalettes();
    MEM_pack();
    currTileIndex = TILE_USER_INDEX;
}

static void basicEngineConfig () {
    // setup screen and tiles dimensions
    VDP_setScreenWidth320(); // enable H40 mode (only for NTSC?): 320x224
    if (IS_PAL_SYSTEM) {
        VDP_setScreenHeight240(); // Only for PAL
    } else {
        VDP_setScreenHeight224(); // Only for NTSC
    }

    VDP_setPlaneSize(64, 32, TRUE);
}

static void initGameStatus () {
    titan256cHIntMode = 0; // set initial color swap mode
}

int main (bool hard) {

    if (!hard) SYS_hardReset();

    displaySegaLogo();
    waitMillis(200);
    displayTeddyBearLogo();
    waitMillis(200);

    VDP_resetScreen(); // this reset scroll planes and other stuff used in logo effects

    basicEngineConfig();
    initGameStatus();

    for (;;) {        
        titan256c();
        titan256cHIntMode = modu(titan256cHIntMode + 1, HINT_MODES); // set to move into next Titan256c HInt mode
    }

    return 0;
}
