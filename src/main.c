#include <genesis.h>
#include "segaLogo.h"
#include "sgdkLogo.h"
#include "teddyBearLogo.h"
#include "titan256c.h"
#include "hvInterrupts.h"
#include "utils.h"

static u16 currTileIndex;

static void loadTitan256cTileSet () {
    const TileSet* tileset = titanRGB.tileset;
    VDP_loadTileSet(tileset, currTileIndex, DMA); // only DMA or DMA_QUEUE_COPY (if tileset is compressed) to avoid glitches
    currTileIndex += tileset->numTile;
}

static void loadTitan256cTileMap (u16 tileAttribIndex) {
    u16 baseTileAttribs = TILE_ATTR_FULL(PAL0, 0, FALSE, FALSE, tileAttribIndex);
    tileAttribIndex += titanRGB.tileset->numTile;
    // even strips goes into BG_A, odd strips into BG_B (it seems doesn't matter the order)
    VDPPlane plane = BG_B;//(i % 2) == 0 ? BG_A : BG_B;
    const TileMap* tilemap = titanRGB.tilemap;
    VDP_setTileMapEx(plane, tilemap, baseTileAttribs, 0, 0, 0, 0, TITAN_256C_WIDTH/8, TITAN_256C_HEIGHT/8, DMA); // only DMA or DMA_QUEUE_COPY (if tilemap is compressed) to avoid glitches
}

static void setTitan256cFirstPaletteColor () {
    // copy every color at pos 1 to pos 0 on PAL0, so changing the BG color as the Titan 512 demo does (although this version is only every 8 scanlines)
    // for (u16 i=0; i < TITAN_256C_STRIPS_COUNT * TITAN_256C_COLORS_PER_STRIP; i += TITAN_256C_COLORS_PER_STRIP) {
    //     palTitanRGB.data[i] = palTitanRGB.data[i + 1];
    // }
}

#define TITAN256C_HINT_MODE 1

static void titan256c () {

    PAL_setColors(0, palette_black, 64, DMA_QUEUE);
    SYS_doVBlankProcess();

    VDP_setEnable(FALSE);

    currTileIndex = TILE_USER_INDEX; // this resets the index for all previous loaded tiles
    u16 firstTileAttribIndex = currTileIndex;
    loadTitan256cTileSet();
    loadTitan256cTileMap(firstTileAttribIndex);
    unpackPalettes(&palTitanRGB);
    setTitan256cFirstPaletteColor();

    VDP_setEnable(TRUE);

    SYS_doVBlankProcess(); // in case we need to flush DMA queue

    SYS_disableInts();
        // Call the HInt every N scanlines. Uses CPU for palette swapping
        #if TITAN256C_HINT_MODE == 0
        SYS_setVBlankCallback(vertIntOnTitan256cCallback_HIntEveryN);
        VDP_setHIntCounter(TITAN_256C_STRIP_HEIGHT - 1);
        SYS_setHIntCallback(horizIntOnTitan256cCallback_CPU_EveryN);
        #endif
        // Call the HInt every N scanlines. Uses DMA for palette swapping
        #if TITAN256C_HINT_MODE == 1
        SYS_setVBlankCallback(vertIntOnTitan256cCallback_HIntEveryN);
        VDP_setHIntCounter(TITAN_256C_STRIP_HEIGHT - 1);
        SYS_setHIntCallback(horizIntOnTitan256cCallback_DMA_EveryN);
        #endif
        // Use only one call to HInt to avoid method call overhead. Uses DMA for palette swapping
        #if TITAN256C_HINT_MODE == 2
        SYS_setVBlankCallback(vertIntOnTitan256cCallback_HIntOneTime);
        VDP_setHIntCounter(0);
        SYS_setHIntCallback(horizIntOnTitan256cCallback_DMA_OneTime);
        #endif
        VDP_setHInterrupt(TRUE);
    SYS_enableInts();

    while (TRUE) {
        // Load 1st and 2nd strip's palette
        PAL_setColors(0, getUnpackedPtr(), TITAN_256C_COLORS_PER_STRIP * 2, DMA_QUEUE);

        beforeVBlankProcOnTitan256c_DMA_QUEUE();
        SYS_doVBlankProcess();
        afterVBlankProcOnTitan256c_VDP_or_DMA();
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

int main (bool hard) {

    if (!hard) SYS_hardReset();

    // displaySegaLogo();
    // waitMillis(200);
    // displaySgdkLogo();
    // waitMillis(200);
    // displayTeddyBearLogo();
    // waitMillis(200);

    basicEngineConfig(); // setup basic engine configurations

    while (1) {
        VDP_resetScreen();
        titan256c();
    }

    return 0;
}
