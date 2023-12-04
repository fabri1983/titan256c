#include <genesis.h>
#include "utils.h"
#include "segaLogo.h"
#include "teddyBearLogo.h"
#include "titan256c.h"
#include "hvInterrupts.h"

#define HINT_MODES 3
static u16 titan256cHIntMode;

static u16 currTileIndex;

static void titan256cDisplay () {

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

    u16 yPos = TITAN_256C_HEIGHT - 1;
    s16 velocity = 0;

    // Fall and bounce effect
    for (;;) {
        // update bouncing effect every 4 frames
        if ((vtimer % 4) == 0) {
            velocity -= 1;
        }
        if ((yPos + velocity) <= 0) {
            yPos = 0;
            velocity = -(velocity * 6) / 10; // decay in velocity as it bounces off the ground
        }
        else yPos += velocity;

        setYPosFalling(yPos);
        VDP_setVerticalScrollVSync(BG_B, yPos);

        // Load 2 strip palettes 
        load2StripsPals(yPos / TITAN_256C_STRIP_HEIGHT);

        // Update ramp color effect for the titan text section
        updateTextGradientColors(0);

        SYS_doVBlankProcess();

        // if bounce effect finished then continue with next game state
        if (yPos == 0 && velocity == 0) break;
    }

    u16 fadingStripCnt = 0;
    u16 prevFadingStrip = 0;
    u16 fadingCycleCurrStrip = 0; // use to split the fading to black into N cycles, due to its lenghty execution

    // Titan display
    for (;;) {
        // Load 1st and 2nd strip's palette
        load2StripsPals(0);

        // Update ramp color effect for the titan text section
        updateTextGradientColors(0);

        SYS_doVBlankProcess();

        u16 joyState = JOY_readJoypad(JOY_1);
        if (joyState & BUTTON_START) break;
    }

    // Fade to black effect
    for (;;) {
        // advance 1 strip every N frames. This must be >= FADE_OUT_STRIPS_SPLIT_CYCLES used to execute the fading for current strip
        u16 currFadingStrip = fadingStripCnt++ / 4; // Use divu() for N non power of 2
        // already passed last strip? then fading is finished
        if (currFadingStrip == (TITAN_256C_STRIPS_COUNT + FADE_OUT_COLOR_STEPS)) {
            break;
        }

        // Load 1st and 2nd strip's palette
        load2StripsPals(0);

        // Update ramp color effect for the titan text section
        updateTextGradientColors(currFadingStrip);

        if (fadingCycleCurrStrip < FADE_OUT_STRIPS_SPLIT_CYCLES) {
            // apply fade to black from currFadingStrip up to FADE_OUT_STEPS previous strips
            fadingStepToBlack_pals(currFadingStrip, fadingCycleCurrStrip, titan256cHIntMode);
            ++fadingCycleCurrStrip;
        }

        if (currFadingStrip != prevFadingStrip) {
            prevFadingStrip = currFadingStrip;
            fadingCycleCurrStrip = 0;
        }

        SYS_doVBlankProcess();
    }

    SYS_disableInts();
        SYS_setVBlankCallback(NULL);
        VDP_setHInterrupt(FALSE);
        SYS_setHIntCallback(NULL);
    SYS_enableInts();

    VDP_clearPlane(BG_B, TRUE);
    PAL_setColor(0, 0x000); // Set BG color as Black
    SYS_doVBlankProcess();

    freePalettes();
    MEM_pack();
    currTileIndex = TILE_USER_INDEX;
}

static void basicEngineConfig () {
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

    basicEngineConfig();
    initGameStatus();

    for (;;) {        
        titan256cDisplay();
        titan256cHIntMode = modu(titan256cHIntMode + 1, HINT_MODES); // set to move into next Titan256c HInt mode
    }

    return 0;
}
