/**
 * Teddy Bear animation author: fabri1983@gmail.com
 * https://github.com/fabri1983
*/
#include <types.h>
#include <vdp.h>
#include <vdp_bg.h>
#include <vdp_tile.h>
#include <memory.h>
#include <joy.h>
#include <sys.h>
#include <sprite_eng.h>
#include <string.h>
#include "teddyBearLogo.h"
#include "utils.h"
#include "logo_res.h"

#define STR_VERSION "v2.12 (Nov 2025)"
#define STR_VERSION_LEN 17 // String version length including \0

void displayTeddyBearLogo ()
{
    //
    // Setup SGDK Teddy Bear animation
    //

    // Init Sprites engine with fixed vram size (in tiles)
    SPR_initEx(sprDefTeddyBearAnim.maxNumTile);

    u16 tileIndexNext = TILE_USER_INDEX;
    u16 teddyBearAnimTileAttribsBase = TILE_ATTR_FULL(PAL3, 0, FALSE, FALSE, tileIndexNext);
    u16 sprX = (screenWidth - sprDefTeddyBearAnim.w) / 2;
    u16 sprY = (screenHeight - sprDefTeddyBearAnim.h) / 2;
    Sprite* teddyBearAnimSpr = SPR_addSpriteEx(&sprDefTeddyBearAnim, sprX, sprY, teddyBearAnimTileAttribsBase,
        SPR_FLAG_AUTO_TILE_UPLOAD | SPR_FLAG_DISABLE_ANIMATION_LOOP);
    tileIndexNext += sprDefTeddyBearAnim.maxNumTile;
    
    // Draw first frame with only black palette to later do the fade in
    PAL_setPalette(PAL3, palette_black, CPU);
    SPR_update();

    //
    // Fade In sprite and text
    //

    // Draw SGDK version string
    VDP_setTextPalette(PAL3);
    VDP_drawText((const char*) STR_VERSION, screenWidth/8 - STR_VERSION_LEN, screenHeight/8 - 1);

    // Fade in to Sprite palette (previously loaded at PAL3)
    PAL_fadeIn(PAL3*16, PAL3*16 + (16-1), sprDefTeddyBearAnim.palette->data, 30, TRUE);
    // process fading immediately
    do {
        SYS_doVBlankProcess();
        const u16 joyState = JOY_readJoypad(JOY_1);
        if (joyState & BUTTON_START)
            break;
    }
    while (PAL_doFadeStep());
    // final update
    SYS_doVBlankProcess();

    //
    // Display loop for SGDK Teddy Bear animation
    //

    for (;;)
    {
        const u16 joyState = JOY_readJoypad(JOY_1);
        if (joyState & BUTTON_START)
            break;

        if (SPR_isAnimationDone(teddyBearAnimSpr)) {
            // Leave some time the last animation frame in the screen
            waitMs_(250);
            break;
        }

        SPR_update();

        SYS_doVBlankProcess();
    }

    // Fade out all graphics to Black
    PAL_fadeOutAll(30, FALSE);

    SPR_end();
    SYS_doVBlankProcess();

    // restore planes
    VDP_clearPlane(BG_A, TRUE);
    // Restore SGDK's font palette (VDP_resetScreen() doesn't restore it accordingly)
    VDP_setTextPalette(PAL0);
    // restore SGDK's default palettes
    PAL_setPalette(PAL0, palette_grey, DMA);
    PAL_setPalette(PAL1, palette_red, DMA);
    PAL_setPalette(PAL2, palette_green, DMA);
    PAL_setPalette(PAL3, palette_blue, DMA);
}