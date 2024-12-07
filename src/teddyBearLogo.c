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

void displayTeddyBearLogo ()
{
    //
    // Initalization
    //

    // Setup VDP
    VDP_setPlaneSize(64, 32, TRUE);
    PAL_setPalette(PAL0, palette_black, CPU);
    PAL_setColor(15, 0xCCC); // SGDK's font color

    // Fill top plane with solid black tiles (tile index 1 seems to be a SGDK system tile)
    VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL0, 0, FALSE, FALSE, 1), 0, 0, screenWidth/8, screenHeight/8);

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
    // Setup SGDK version text
    //

    // Draw SGDK version number
    const char* sgdk_version = "v2.0 (Nov 2024)";
    VDP_drawText(sgdk_version, screenWidth/8 - (strlen(sgdk_version) + 1), screenHeight/8 - 1);

    //
    // Fade In sprite and text
    //

    // Fade in to Sprite palette (previously located at PAL3)
    PAL_fadeIn(PAL3*16, PAL3*16 + 15, sprDefTeddyBearAnim.palette->data, 30, FALSE);

    //
    // Display loop for SGDK Teddy Bear animation
    //

    for (;;)
    {
        const u16 joyState = JOY_readJoypad(JOY_1);

        if (joyState & BUTTON_START) {
            break;
        }

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
    SYS_doVBlankProcess();

    SPR_end();
    SYS_doVBlankProcess();

    // restore planes
    VDP_clearPlane(BG_A, TRUE);
    // restore SGDK's default palettes
    PAL_setPalette(PAL0, palette_grey, CPU);
    PAL_setPalette(PAL1, palette_red, CPU);
    PAL_setPalette(PAL2, palette_green, CPU);
    PAL_setPalette(PAL3, palette_blue, CPU);
}