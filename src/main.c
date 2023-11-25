#include <genesis.h>
#include "utils.h"
#include "segaLogo.h"
#include "sgdkLogo.h"
#include "teddyBearLogo.h"
#include "titan256c.h"
#include "hvInterrupts.h"

static u16 currTileIndex = TILE_USER_INDEX;

static void loadTitan256cTileSet () {
    const TileSet* tileset = titanRGB.tileset;
    VDP_loadTileSet(tileset, currTileIndex, DMA); // only DMA because total tileset size is bigger than DMA buffer if using DMA_QUEUE_COPY
    
}

static void loadTitan256cTileMap (VDPPlane plane) {
    u16 baseTileAttribs = TILE_ATTR_FULL(PAL0, 0, FALSE, FALSE, currTileIndex);
    currTileIndex += titanRGB.tileset->numTile;
    const TileMap* tilemap = titanRGB.tilemap;
    VDP_setTileMapEx(plane, tilemap, baseTileAttribs, 0, 0, 0, 0, TITAN_256C_WIDTH/8, TITAN_256C_HEIGHT/8, DMA_QUEUE_COPY); // only DMA or DMA_QUEUE_COPY (if tilemap is compressed) to avoid glitches
}

// static void setTitan256cFirstPaletteColor () {
//     // copy every color at pos 1 to pos 0 on PAL0, so changing the BG color as the Titan 512 demo does (although this version is only every 8 scanlines)
//     for (u16 i=0; i < TITAN_256C_STRIPS_COUNT * TITAN_256C_COLORS_PER_STRIP; i += TITAN_256C_COLORS_PER_STRIP) {
//         palTitanRGB.data[i] = palTitanRGB.data[i + 1];
//     }
// }

static u16 titan256cHIntMode;

static void titan256c () {

    PAL_setColors(0, palette_black, 64, DMA); // palette_black is an array of 64
    SYS_doVBlankProcess();

    VDP_setEnable(FALSE);

    loadTitan256cTileSet();
    loadTitan256cTileMap(BG_B);
    unpackPalettes(&palTitanRGB);
    resetGradientColors();

    VDP_setEnable(TRUE);

    SYS_doVBlankProcess(); // in case we need to flush DMA queue

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

    u16 animStatus = 0;
    bool faceToBlackIsDone = FALSE;
    u16 fadingStripCnt = 0;
    u16 prevFadingStrip = 0;

    while (TRUE) {
        // Load 1st and 2nd strip's palette
        set2FirstStripsPals();

        // update fading to black 2 palettes per strip
        if (animStatus == 1) {
            ++fadingStripCnt;
            s16 currFadingStrip = fadingStripCnt >> 2; // advance 1 strip every 4 frames. Use divu() for N non power of 2
            // strip changed? let's do one fading step
            if (currFadingStrip != prevFadingStrip) {
                prevFadingStrip = currFadingStrip;
                // apply fade to black from currFadingStrip up to FADE_OUT_STEPS previous strips (limit is strip 0)
                for (s16 i=currFadingStrip; i >= max(currFadingStrip - FADE_OUT_STEPS, 0); --i) {
                    fadingStepToBlack(i);
                }
            }
            // moved ahead last strip? then fading is finished
            if (currFadingStrip > (screenHeight / TITAN_256C_STRIP_HEIGHT)) {
                faceToBlackIsDone = TRUE;
            }
        }

        beforeVBlankProcOnTitan256c_DMA_QUEUE();
        SYS_doVBlankProcess();
        afterVBlankProcOnTitan256c_VDP_or_DMA();

        if (animStatus == 0) {
            u16 joyState = JOY_readJoypad(JOY_1);
            if (joyState & BUTTON_START) {
                ++animStatus;
            }
        }
        else if (animStatus == 1 && faceToBlackIsDone) {
            ++animStatus;
            titan256cHIntMode = modu(titan256cHIntMode + 1, 3); // set to move into next Titan256c HInt mode
            break;
        }
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

    freePalettes(&palTitanRGB);
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
    titan256cHIntMode = 0;
}

int main (bool hard) {

    if (!hard) SYS_hardReset();

    // displaySegaLogo();
    // waitMillis(200);
    // displaySgdkLogo();
    // waitMillis(200);
    // displayTeddyBearLogo();
    // waitMillis(200);

    basicEngineConfig(); // setup basic engine configurations

    initGameStatus();

    while (1) {
        VDP_resetScreen();
        titan256c();
    }

    return 0;
}
