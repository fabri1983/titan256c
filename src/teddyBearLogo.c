/**
 * Teddy Bear animation author: fabri1983@gmail.com
 * https://github.com/fabri1983
*/
#include <genesis.h>
#include "teddyBearLogo.h"
#include "utils.h"

void displayTeddyBearLogo ()
{
    //
    // Initalization
    //

    // Setup VDP
    VDP_setPlaneSize(64, 32, TRUE);
    PAL_setPalette(PAL0, palette_black, DMA);

    // Fill top plane with solid black tiles (tile index 1 seems to be a SGDK system tile)
    VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL0, 0, FALSE, FALSE, 1), 0, 0, 40, 28);

    //
    // Setup SGDK Teddy Bear animation
    //

    // Init Sprites engine with default values (CAUTION: 420 reserved tiles in VRAM)
    SPR_init();

    u16 tileIndexNext = TILE_USER_INDEX;
    u16 teddyBearAnimTileAttribsBase = TILE_ATTR_FULL(PAL3, 0, FALSE, FALSE, tileIndexNext);
    u16 sprX = (screenWidth - sprDefTeddyBearAnim.w) / 2;
    u16 sprY = (screenHeight - sprDefTeddyBearAnim.h) / 2;
    Sprite* teddyBearAnimSpr = SPR_addSpriteSafe(&sprDefTeddyBearAnim, sprX, sprY, teddyBearAnimTileAttribsBase);
    tileIndexNext += sprDefTeddyBearAnim.maxNumTile;
    
    // Draw first frame with only black palette to later do the fade in
    PAL_setPalette(PAL3, palette_black, DMA);
    SPR_update();

    //
    // Setup SGDK version text
    //

    // PAL3 color 15th is white. Remember SGDK always uses color 15th of the chosen palette for text colouring
    VDP_setTextPalette(PAL3);
    // Draw SGDK version number
    const char* sgdk_version = "v2.0 (July 2024)";
    VDP_drawText(sgdk_version, screenWidth/8 - (strlen(sgdk_version) + 1), screenHeight/8 - 1);

    //
    // Fade In sprite and text
    //

    // Fade in to Sprite palette (previously defined to be located at PAL3)
    PAL_fadeIn(PAL3*16, PAL3*16 + 16, sprDefTeddyBearAnim.palette->data, 30, FALSE);

    // Leave some time the last animation frame in the screen
    waitMs_(250);

    //
    // Display loop for SGDK Teddy Bear animation
    //

    // Set animation 0 (is the only one though)
    SPR_setAnim(teddyBearAnimSpr, 0);

    u8 frameCnt = 0;

    while (1)
    {
        const u16 joyState = JOY_readJoypad(JOY_1);

        if (joyState & BUTTON_START) {
            break;
        }

        // Is the last frame of the current animation? (The sprite has only one animation )
        if (frameCnt == teddyBearAnimSpr->animation->numFrame) {
            // Leave some time the last animation frame in the screen
            waitMs_(500);
            break;
        }

        SPR_update();
        SYS_doVBlankProcess();

        // Reaching 1 means the waiting time for current animation frame is done
        if (teddyBearAnimSpr->timer == 1)
            ++frameCnt;
    }

    // Fade out all graphics to Black
    PAL_fadeOutAll(30, FALSE);

    SPR_end();

    SYS_doVBlankProcess();

    // restore SGDK's default palete for text
    VDP_setTextPalette(PAL0);
    PAL_setPalette(PAL0, palette_grey, DMA);
    VDP_clearPlane(BG_A, TRUE);
}