#include <genesis.h>
#include "utils.h"
#include "segaLogo.h"
#include "teddyBearLogo.h"
#include "titan256c.h"
#include "titan_sphere_res.h"
#include "hvInterrupts.h"
#include "custom_font_res.h"
#include "customFont_consts.h"

static u16 titan256cHIntMode;

static u16 currTileIndex;

static const char* startText = "(press START to continue)";

static const char* mode0_strat = "STRATEGY 0";
static const char* mode0_textA = "HInt in ASM";
static const char* mode0_textB = "Called every 8 scanlines";
static const char* mode0_textC = "Uses CPU to move colors into VDP CRAM";

static const char* mode1_strat = "STRATEGY 1";
static const char* mode1_textA = "HInt in C";
static const char* mode1_textB = "Called every 8 scanlines";
static const char* mode1_textC = "Uses CPU to move colors into VDP CRAM";

static const char* mode2_strat = "STRATEGY 2";
static const char* mode2_textA = "HInt in ASM";
static const char* mode2_textB = "Called every 8 scanlines";
static const char* mode2_textC = "Uses DMA to move colors into VDP CRAM";

static const char* mode3_strat = "STRATEGY 3";
static const char* mode3_textA = "HInt in C";
static const char* mode3_textB = "Called every 8 scanlines";
static const char* mode3_textC = "Uses DMA to move colors into VDP CRAM";

static const char* mode4_strat = "STRATEGY 4";
static const char* mode4_textA = "HInt in C";
static const char* mode4_textB = "Called only once";
static const char* mode4_textC = "Uses DMA to move colors into VDP CRAM";

/// @brief Replacement for VDP_drawText(). This version uses the custom font.
/// @param str 
/// @param x 
/// @param y 
static void drawText (const char *str, u16 x, u16 y) {
    u16 basetile = TILE_ATTR_FULL(PAL0, 0, FALSE, FALSE, CUSTOM_TILE_FONT_INDEX);
    VDPPlane plane = VDP_getTextPlane();

    u16 data[128];
    const u8 *s;
    u16 *d;
    u16 i, pw, ph, len;

    // get the horizontal plane size (in cell)
    pw = (plane == WINDOW)?windowWidth:planeWidth;
    ph = (plane == WINDOW)?32:planeHeight;

    // string outside plane --> exit
    if ((x >= pw) || (y >= ph))
        return;

    // get string len
    len = strlen(str);
    // if string don't fit in plane, we cut it
    if (len > (pw - x))
        len = pw - x;

    // prepare the data
    s = (const u8*) str;
    d = data;
    i = len;
    while(i--)
        *d++ = (*s++ - 32);

    // VDP_setTileMapDataRowEx(..) take care of using temporary buffer to build the data so we are ok here
    VDP_setTileMapDataRowEx(plane, data, basetile, y, x, len, CPU);
}

static void showTransitionScreen () {

    VDP_setEnable(FALSE);

    // SGDK's font uses color index 15th to colorize its font, so we set it with the color used by our custom font
    PAL_setColor(15, custom_font_round_pal.data[1]);

    // Add the new font tiles after the SGDK's current font tiles
    VDP_loadTileSet(&custom_font_round_tileset, CUSTOM_TILE_FONT_INDEX, CPU);
    // Load the palette used by the custom font
    PAL_setColors(0, custom_font_round_pal.data, CUSTOM_FONT_PAL_COLORS_COUNT, CPU);

    SYS_disableInts();
        SYS_setVBlankCallback(vertIntOnDrawTextCallback);
        VDP_setHIntCounter(screenHeight/2 + (4*8) - 1); // scanline location for startText
        SYS_setHIntCallback(horizIntOnDrawTextCallback);
        VDP_setHInterrupt(TRUE);
    SYS_enableInts();

    VDP_setEnable(TRUE);

    u16 screenWidthTiles = screenWidth/8;
    u16 screenHeightTiles = screenHeight/8;
    switch (titan256cHIntMode) {
        case HINT_STRATEGY_0:
            VDP_drawText(mode0_strat, (screenWidthTiles - strlen(mode0_textA)) / 2, screenHeightTiles / 2 - 5);
            drawText(mode0_textA, (screenWidthTiles - strlen(mode0_textA)) / 2, screenHeightTiles / 2 - 1);
            drawText(mode0_textB, (screenWidthTiles - strlen(mode0_textB)) / 2, screenHeightTiles / 2 - 0);
            drawText(mode0_textC, (screenWidthTiles - strlen(mode0_textC)) / 2, screenHeightTiles / 2 + 1);
            break;
        case HINT_STRATEGY_1:
            VDP_drawText(mode1_strat, (screenWidthTiles - strlen(mode1_textA)) / 2, screenHeightTiles / 2 - 5);
            drawText(mode1_textA, (screenWidthTiles - strlen(mode1_textA)) / 2, screenHeightTiles / 2 - 1);
            drawText(mode1_textB, (screenWidthTiles - strlen(mode1_textB)) / 2, screenHeightTiles / 2 - 0);
            drawText(mode1_textC, (screenWidthTiles - strlen(mode1_textC)) / 2, screenHeightTiles / 2 + 1);
            break;
        case HINT_STRATEGY_2:
            VDP_drawText(mode2_strat, (screenWidthTiles - strlen(mode2_textA)) / 2, screenHeightTiles / 2 - 5);
            drawText(mode2_textA, (screenWidthTiles - strlen(mode2_textA)) / 2, screenHeightTiles / 2 - 1);
            drawText(mode2_textB, (screenWidthTiles - strlen(mode2_textB)) / 2, screenHeightTiles / 2 - 0);
            drawText(mode2_textC, (screenWidthTiles - strlen(mode2_textC)) / 2, screenHeightTiles / 2 + 1);
            break;
        case HINT_STRATEGY_3:
            VDP_drawText(mode3_strat, (screenWidthTiles - strlen(mode3_textA)) / 2, screenHeightTiles / 2 - 5);
            drawText(mode3_textA, (screenWidthTiles - strlen(mode3_textA)) / 2, screenHeightTiles / 2 - 1);
            drawText(mode3_textB, (screenWidthTiles - strlen(mode3_textB)) / 2, screenHeightTiles / 2 - 0);
            drawText(mode3_textC, (screenWidthTiles - strlen(mode3_textC)) / 2, screenHeightTiles / 2 + 1);
            break;
        case HINT_STRATEGY_4:
            VDP_drawText(mode4_strat, (screenWidthTiles - strlen(mode4_textA)) / 2, screenHeightTiles / 2 - 5);
            drawText(mode4_textA, (screenWidthTiles - strlen(mode4_textA)) / 2, screenHeightTiles / 2 - 1);
            drawText(mode4_textB, (screenWidthTiles - strlen(mode4_textB)) / 2, screenHeightTiles / 2 - 0);
            drawText(mode4_textC, (screenWidthTiles - strlen(mode4_textC)) / 2, screenHeightTiles / 2 + 1);
            break;
        default: break;
    }
    drawText(startText, (screenWidthTiles - strlen(startText)) / 2, screenHeightTiles / 2 + 4);

    for (;;) {
        const u16 joyState = JOY_readJoypad(JOY_1);
        if (joyState & BUTTON_START) {
            break;
        }
        SYS_doVBlankProcess();
    }

    SYS_disableInts();
        SYS_setVBlankCallback(NULL);
        VDP_setHInterrupt(FALSE);
        SYS_setHIntCallback(NULL);
    SYS_enableInts();

    SYS_doVBlankProcess();

    VDP_clearPlane(VDP_getTextPlane(), TRUE);
}

#if SPHERE_TEXT_ANIMATION == TRUE
static void toggleSphereTextAnimations (Sprite* titanSphereText_1_AnimSpr, Sprite* titanSphereText_2_AnimSpr) {
    // we check directly against VISIBLE because sprites settings are only VISIBLE or HIDDEN since their creation
    if (SPR_getVisibility(titanSphereText_1_AnimSpr) == VISIBLE) {
        if (SPR_getAnimationDone(titanSphereText_1_AnimSpr)) {
            SPR_setVisibility(titanSphereText_1_AnimSpr, HIDDEN);
            SPR_setVisibility(titanSphereText_2_AnimSpr, VISIBLE);
            SPR_setAnimAndFrame(titanSphereText_2_AnimSpr, 0, 0); // reset animation 0 to first frame
            titanSphereText_2_AnimSpr->status &= ~0x0010; // set NOT STATE_ANIMATION_DONE from sprite_eng.c
        }
    }
    else {
        if (SPR_getAnimationDone(titanSphereText_2_AnimSpr)) {
            SPR_setVisibility(titanSphereText_2_AnimSpr, HIDDEN);
            SPR_setVisibility(titanSphereText_1_AnimSpr, VISIBLE);
            SPR_setAnimAndFrame(titanSphereText_1_AnimSpr, 0, 0); // reset animation 0 to first frame
            titanSphereText_1_AnimSpr->status &= ~0x0010; // set NOT STATE_ANIMATION_DONE from sprite_eng.c
        }
    }
}
#endif

static void titan256cDisplay () {

    PAL_setColors(0, palette_black, 64, DMA); // palette_black is an array of 64
    setBlackCurrentGradientPtr();

    SYS_doVBlankProcess();

    VDP_setEnable(FALSE);

    currTileIndex = TILE_USER_INDEX;
    loadTitan256cTileSet(currTileIndex);
    loadTitan256cTileMap(BG_B, currTileIndex);
    currTileIndex += titanRGB.tileset->numTile;
    unpackPalettes();

    setSphereTextColorsIntoTitanPalettes(sprDefTitanSphereText_1_Anim);

    SPR_initEx(sprDefTitanSphereText_1_Anim.maxNumTile + sprDefTitanSphereText_2_Anim.maxNumTile); // 137 + 127 tiles

    Sprite* titanSphereText_1_AnimSpr = SPR_addSpriteExSafe(&sprDefTitanSphereText_1_Anim, 
        TITAN_SPHERE_TILEMAP_START_X_POS * 8, TITAN_SPHERE_TILEMAP_START_Y_POS * 8, 
        TILE_ATTR_FULL(PAL0, 0, FALSE, FALSE, currTileIndex),
        SPR_FLAG_AUTO_TILE_UPLOAD | SPR_FLAG_AUTO_VRAM_ALLOC);
    SPR_setAnim(titanSphereText_1_AnimSpr, 0); // set animation 0 (is the only one though)
    // always visible or always hidden along the three effects of the scene
    SPR_setVisibility(titanSphereText_1_AnimSpr, VISIBLE);
    currTileIndex += sprDefTitanSphereText_1_Anim.maxNumTile;

    Sprite* titanSphereText_2_AnimSpr = SPR_addSpriteExSafe(&sprDefTitanSphereText_2_Anim, 
        TITAN_SPHERE_TILEMAP_START_X_POS * 8, TITAN_SPHERE_TILEMAP_START_Y_POS * 8, 
        TILE_ATTR_FULL(PAL0, 0, FALSE, FALSE, currTileIndex),
        SPR_FLAG_AUTO_TILE_UPLOAD | SPR_FLAG_AUTO_VRAM_ALLOC);
    SPR_setAnim(titanSphereText_2_AnimSpr, 0); // set animation 0 (is the only one though)
    // always visible or always hidden along the three effects of the scene
    SPR_setVisibility(titanSphereText_2_AnimSpr, HIDDEN);
    currTileIndex += sprDefTitanSphereText_2_Anim.maxNumTile;

    SYS_disableInts();
        setHVCallbacks(titan256cHIntMode);
        VDP_setHInterrupt(TRUE);
    SYS_enableInts();

    VDP_setEnable(TRUE);

    // disable the fading effect on titan text
    setCurrentFadingStripForText(0);

    u16 yPos = TITAN_256C_HEIGHT;
    s16 velocity = 0;

    // Fall and bounce effect
    for (;;) {
        // update bouncing velocity every 4 frames
        if ((vtimer % 4) == 0) {
            velocity -= 1;
        }
        // Do this due to signed type of velocity. When touching floor then decay velocity just a bit
        if ((yPos + velocity) <= 0) {
            yPos = 0;
            velocity = (-velocity * 6) / 10; // decay in velocity as it bounces off the ground
        }
        // while in the mid air apply translation
        else yPos += velocity;

        // The bouncing effect has the side effect of offseting strips into scanlines not alligned with expected strips distribution,
        // hence we need to "offset" the scanline at which the HInt gets into action.
        //setHIntScanlineStarterForBounceEffect(yPos, titan256cHIntMode);

        setYPosFalling(yPos);
        VDP_setVerticalScrollVSync(BG_B, yPos);

        #if SPHERE_TEXT_ANIMATION == TRUE
        //updateSphereTextColor();
        #endif

        updateTextGradientColors();

        // Enqueue 2 strips palettes depending on the Y position (in strips) of the image
        enqueue2Pals(yPos / TITAN_256C_STRIP_HEIGHT);

        #if SPHERE_TEXT_ANIMATION == TRUE
        SPR_setPosition(titanSphereText_1_AnimSpr, TITAN_SPHERE_TILEMAP_START_X_POS * 8, TITAN_SPHERE_TILEMAP_START_Y_POS * 8 - yPos);
        SPR_setPosition(titanSphereText_2_AnimSpr, TITAN_SPHERE_TILEMAP_START_X_POS * 8, TITAN_SPHERE_TILEMAP_START_Y_POS * 8 - yPos);
        toggleSphereTextAnimations(titanSphereText_1_AnimSpr, titanSphereText_2_AnimSpr);
        SPR_update();
        #endif

        SYS_doVBlankProcess();

        // if bounce effect finished then continue with next game state
        if (yPos == 0 && velocity == 0) break;
    }

    // Titan display
    for (;;) {
        #if SPHERE_TEXT_ANIMATION == TRUE
        //updateSphereTextColor();
        #endif

        updateTextGradientColors();

        // Load 1st and 2nd strip's palette
        enqueue2Pals(0);

        #if SPHERE_TEXT_ANIMATION == TRUE
        toggleSphereTextAnimations(titanSphereText_1_AnimSpr, titanSphereText_2_AnimSpr);
        SPR_update();
        #endif

        SYS_doVBlankProcess();

        u16 joyState = JOY_readJoypad(JOY_1);
        if (joyState & BUTTON_START) break;
    }

    u8 fadingStripCnt = 0;
    u8 prevFadingStrip = 0;
    u8 fadingCycleCurrStrip = 0; // use to split the fading to black into N cycles, due to its lenghty execution

    // Fade to black effect
    for (;;) {
        // advance 1 strip every N frames. N = FADE_OUT_STRIPS_SPLIT_CYCLES (used to execute the fading for current strip)
        u8 currFadingStrip = fadingStripCnt / FADE_OUT_STRIPS_SPLIT_CYCLES; // Use divu() for N non power of 2
        ++fadingStripCnt;
        // already passed last strip? then fading is finished
        if (currFadingStrip == (TITAN_256C_STRIPS_COUNT + FADE_OUT_COLOR_STEPS)) {
            break;
        }

        // Enable the fading effect on titan text calculated on VInt
        setCurrentFadingStripForText(currFadingStrip);

        #if SPHERE_TEXT_ANIMATION == TRUE
        //updateSphereTextColor();
        #endif

        updateTextGradientColors();

        // Load 1st and 2nd strip's palettes
        enqueue2Pals(0);

        // apply fade to black from currFadingStrip up to FADE_OUT_STEPS previous strips
        fadingStepToBlack_pals(currFadingStrip, fadingCycleCurrStrip);
        ++fadingCycleCurrStrip;

        if (currFadingStrip != prevFadingStrip) {
            prevFadingStrip = currFadingStrip;
            fadingCycleCurrStrip = 0;
        }

        #if SPHERE_TEXT_ANIMATION == TRUE
        toggleSphereTextAnimations(titanSphereText_1_AnimSpr, titanSphereText_2_AnimSpr);
        SPR_update();
        #endif

        SYS_doVBlankProcess();
    }

    SYS_disableInts();
        SYS_setVBlankCallback(NULL);
        VDP_setHInterrupt(FALSE);
        SYS_setHIntCallback(NULL);
    SYS_enableInts();

    VDP_clearPlane(BG_B, TRUE);
    PAL_setColor(0, 0x000); // Set BG color as Black

    SPR_end();

    SYS_doVBlankProcess();

    freePalettes();
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
    titan256cHIntMode = HINT_STRATEGY_0; // set initial HInt color swap strategy
}

static void prepareNextHintMode () {
    titan256cHIntMode = modu(titan256cHIntMode + 1, HINT_STRATEGY_TOTAL);
}

int main (bool hardReset) {

	// on soft reset do like a hard reset
	if (!hardReset) {
		VDP_waitDMACompletion(); // avoids some glitches as per Genesis Manual's Addendum section
		SYS_hardReset();
	}

    // displaySegaLogo();
    // waitMs_(200);
    displayTeddyBearLogo();
    waitMs_(200);

    basicEngineConfig();
    initGameStatus();

    for (;;) {
        showTransitionScreen();
        titan256cDisplay();
        prepareNextHintMode();
    }

    return 0;
}
