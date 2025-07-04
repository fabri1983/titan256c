#include <genesis.h>
#include "utils.h"
#include "teddyBearLogo.h"
#include "titan256c.h"
#include "hvInterrupts.h"
#include "custom_font_res.h"
#include "customFont_consts.h"
#include "customFont.h"
#include "spr_eng_override.h"
#include "sphereTextAnimation.h"

typedef enum {
    TRANSITION_SCREEN, FALL_AND_BOUNCE, STATIONARY, FADE_OUT
} TITAN_256C_GAME_STATUS;

static TITAN_256C_GAME_STATUS currentGameStatus;

static u8 titan256cHIntMode;

static void setNextHintMode () {
    titan256cHIntMode = modu(titan256cHIntMode + 1, HINT_STRATEGY_TOTAL);
}

static void setPreviousHintMode () {
    if (titan256cHIntMode == 0)
        titan256cHIntMode = HINT_STRATEGY_TOTAL - 1;
    else
        titan256cHIntMode = modu(titan256cHIntMode - 1, HINT_STRATEGY_TOTAL);
}

static u16 currTileIndex;

static const char press_START_txt[] = "(press START to continue)";

static const char mode0_strat[] = "STRATEGY A";
static const char mode0_textA[] = "HInt in ASM";
static const char mode0_textB[] = "Called every 8 scanlines";
static const char mode0_textC[] = "Uses CPU to move colors into VDP CRAM";

static const char mode1_strat[] = "STRATEGY B";
static const char mode1_textA[] = "HInt in C";
static const char mode1_textB[] = "Called every 8 scanlines";
static const char mode1_textC[] = "Uses CPU to move colors into VDP CRAM";

static const char mode2_strat[] = "STRATEGY C";
static const char mode2_textA[] = "HInt in ASM";
static const char mode2_textB[] = "Called every 8 scanlines";
static const char mode2_textC[] = "Uses DMA to move colors into VDP CRAM";

static const char mode3_strat[] = "STRATEGY D";
static const char mode3_textA[] = "HInt in C";
static const char mode3_textB[] = "Called every 8 scanlines";
static const char mode3_textC[] = "Uses DMA to move colors into VDP CRAM";

static const char mode4_strat[] = "STRATEGY E";
static const char mode4_textA[] = "HInt in C";
static const char mode4_textB[] = "Called only once";
static const char mode4_textC[] = "Uses DMA to move colors into VDP CRAM";

static u16 buttonBitsChange = 0;
static u16 buttonBitsState = 0;
static u16 joyCurrent = 0;

/**
 * - joy: This tells us which joypad was used. It will usually be JOY_1 or JOY_2, but SGDK actually supports up to 8 joypads.</br>
 * - changed: This tells us whether the state of a button has changed over the last frame. 
 * If the current state is different from the state in the previous frame, this will be 1 (otherwise 0). </br>
 * - state: This will be 1 if the button is currently pressed and 0 if it isn’t.</br>
*/
static void joyHandler (u16 joy, u16 changed, u16 state) {
    joyCurrent = joy;
    // Only process JOY_1 and JOY_2
    if (joyCurrent < JOY_3) {
        buttonBitsChange = changed;
        buttonBitsState = state;
    }
}

static void joyStatusReset () {
    joyCurrent = 0;
    buttonBitsChange = 0;
    buttonBitsState = 0;
}

static u8 strategySelectionDelay = 0;
#define MENU_SELECTOR_DELAY_FRAMES 12

static void updateTransitionScreenOnJoyInput () {
    if (buttonBitsState & BUTTON_START) {
        currentGameStatus = FALL_AND_BOUNCE;
    }
    else {
        if (buttonBitsState & BUTTON_RIGHT) {
            if (strategySelectionDelay == 0) {
                setNextHintMode();
                strategySelectionDelay = MENU_SELECTOR_DELAY_FRAMES;
            }
        }
        else if (buttonBitsState & BUTTON_LEFT) {
            if (strategySelectionDelay == 0) {
                setPreviousHintMode();
                strategySelectionDelay = MENU_SELECTOR_DELAY_FRAMES;
            }
        }
    }

    if (strategySelectionDelay > 0)
        --strategySelectionDelay;
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

        JOY_setEventHandler(joyHandler);
        joyStatusReset();

    SYS_enableInts();

    VDP_setEnable(TRUE);

    u16 screenWidthTiles = screenWidth/8;
    u16 screenHeightTiles = screenHeight/8;

    while (currentGameStatus == TRANSITION_SCREEN) {

        updateTransitionScreenOnJoyInput();

        VDP_clearTextLine(screenHeightTiles / 2 - 5);
        VDP_clearTextLine(screenHeightTiles / 2 - 1);
        VDP_clearTextLine(screenHeightTiles / 2 + 0);
        VDP_clearTextLine(screenHeightTiles / 2 + 1);

        switch (titan256cHIntMode) {
        case HINT_STRATEGY_0:
            VDP_drawText(mode0_strat, (screenWidthTiles - strlen(mode0_strat)) / 2, screenHeightTiles / 2 - 5);
            drawText(mode0_textA, (screenWidthTiles - strlen(mode0_textA)) / 2, screenHeightTiles / 2 - 1);
            drawText(mode0_textB, (screenWidthTiles - strlen(mode0_textB)) / 2, screenHeightTiles / 2 + 0);
            drawText(mode0_textC, (screenWidthTiles - strlen(mode0_textC)) / 2, screenHeightTiles / 2 + 1);
            break;
        case HINT_STRATEGY_1:
            VDP_drawText(mode1_strat, (screenWidthTiles - strlen(mode1_strat)) / 2, screenHeightTiles / 2 - 5);
            drawText(mode1_textA, (screenWidthTiles - strlen(mode1_textA)) / 2, screenHeightTiles / 2 - 1);
            drawText(mode1_textB, (screenWidthTiles - strlen(mode1_textB)) / 2, screenHeightTiles / 2 + 0);
            drawText(mode1_textC, (screenWidthTiles - strlen(mode1_textC)) / 2, screenHeightTiles / 2 + 1);
            break;
        case HINT_STRATEGY_2:
            VDP_drawText(mode2_strat, (screenWidthTiles - strlen(mode2_strat)) / 2, screenHeightTiles / 2 - 5);
            drawText(mode2_textA, (screenWidthTiles - strlen(mode2_textA)) / 2, screenHeightTiles / 2 - 1);
            drawText(mode2_textB, (screenWidthTiles - strlen(mode2_textB)) / 2, screenHeightTiles / 2 + 0);
            drawText(mode2_textC, (screenWidthTiles - strlen(mode2_textC)) / 2, screenHeightTiles / 2 + 1);
            break;
        case HINT_STRATEGY_3:
            VDP_drawText(mode3_strat, (screenWidthTiles - strlen(mode3_strat)) / 2, screenHeightTiles / 2 - 5);
            drawText(mode3_textA, (screenWidthTiles - strlen(mode3_textA)) / 2, screenHeightTiles / 2 - 1);
            drawText(mode3_textB, (screenWidthTiles - strlen(mode3_textB)) / 2, screenHeightTiles / 2 + 0);
            drawText(mode3_textC, (screenWidthTiles - strlen(mode3_textC)) / 2, screenHeightTiles / 2 + 1);
            break;
        case HINT_STRATEGY_4:
            VDP_drawText(mode4_strat, (screenWidthTiles - strlen(mode4_strat)) / 2, screenHeightTiles / 2 - 5);
            drawText(mode4_textA, (screenWidthTiles - strlen(mode4_textA)) / 2, screenHeightTiles / 2 - 1);
            drawText(mode4_textB, (screenWidthTiles - strlen(mode4_textB)) / 2, screenHeightTiles / 2 + 0);
            drawText(mode4_textC, (screenWidthTiles - strlen(mode4_textC)) / 2, screenHeightTiles / 2 + 1);
            break;
        default: break;
        }

        VDP_clearTextLine(screenHeightTiles / 2 + 4);
        VDP_drawText(press_START_txt, (screenWidthTiles - strlen(press_START_txt)) / 2, screenHeightTiles / 2 + 4);

        SYS_doVBlankProcess();
    }

    VDP_clearPlane(VDP_getTextPlane(), TRUE);

    SYS_disableInts();

        SYS_setVBlankCallback(NULL);
        VDP_setHInterrupt(FALSE);
        SYS_setHIntCallback(NULL);

        JOY_setEventHandler(NULL);

    SYS_enableInts();

    SYS_doVBlankProcess();
}

static s16 yPosOffset = 0; // 0: no direction by default
static u16 yPos;
static s16 velocity = 0;
static bool isManualPosY = FALSE;

static void updateBounceEffectOnJoyInput () {
    if (buttonBitsState & BUTTON_START) {
        isManualPosY = FALSE;
    }

    if (buttonBitsChange & BUTTON_UP) {
        if (buttonBitsState & BUTTON_UP) {
            isManualPosY = TRUE;
            yPosOffset = 1; // scanlines up
            velocity = 0;
            yPos += yPosOffset;
            yPos = min(yPos, TITAN_256C_HEIGHT);
        }
        else {
            if (yPosOffset > 0) // check if the other direction is not being used
                yPosOffset = 0;
            buttonBitsChange &= ~BUTTON_UP; // released
        }
    }
    if (buttonBitsChange & BUTTON_DOWN) {
        if (buttonBitsState & BUTTON_DOWN) {
            isManualPosY = TRUE;
            velocity = 0;
            yPosOffset = -1; // scanlines down
            yPos += yPosOffset;
        }
        else {
            if (yPosOffset < 0) // check if the other direction is not being used
                yPosOffset = 0;
            buttonBitsChange &= ~BUTTON_DOWN; // released
        }
    }
}

static void titan256cDisplay () {

    PAL_setColors(0, palette_black, 64, DMA_QUEUE); // palette_black is an array of 64
    setGradientPtrToBlack();

    SYS_doVBlankProcess();

    VDP_setEnable(FALSE);

    SYS_disableInts();

        currTileIndex = TILE_USER_INDEX;
        loadTitan256cTileSet(currTileIndex);
        loadTitan256cTileMap(BG_B, currTileIndex);
        currTileIndex += titanRGB.tileset->numTile;
        unpackPalettes();

        #if TITAN_SPHERE_TEXT_ANIMATION
        setupSphereTextAnimations();
        #endif

        setHVCallbacks(titan256cHIntMode);
        VDP_setHInterrupt(TRUE);

        JOY_setEventHandler(joyHandler);

    SYS_enableInts();

    VDP_setEnable(TRUE);

    //---------------------------------
    // TITAN BOUNCING EFFECT
    //---------------------------------

    joyStatusReset();

    // disable the fading effect on titan text
    setCurrentFadingStripForText(0);

    yPos = TITAN_256C_HEIGHT;
    isManualPosY = FALSE;
    velocity = 0;

    // Fall and bounce effect loop
    while (currentGameStatus == FALL_AND_BOUNCE) {

        updateBounceEffectOnJoyInput();

        if (!isManualPosY) {
            // Update bouncing velocity every 4 frames
            if ((vtimer % 4) == 0) {
                velocity += 1;
            }

            // When at or move through floor then decay velocity just a bit
            s16 new_yPos = yPos - velocity;
            if (new_yPos <= 0) {
                new_yPos = 0;
                // Decay in velocity and change sign of direction as it bounces off the ground
                velocity = divs((velocity * -6), 10);
            }
            
            // Apply translation
            yPos = (u16) new_yPos;

            // If bounce effect finished then continue with next game state
            if ((yPos | velocity) == 0) {
                currentGameStatus = STATIONARY;
                break;
            }
        }
        else {
            if (yPos == 0) {
                currentGameStatus = STATIONARY;
                break;
            }
        }

        // The bouncing effect has the side effect of making rendered strips to be not alligned with expected 
        // palettes distribution, hence we need to shift the scanline at which the HInt gets into action.
        //setHIntScanlineStarterForBounceEffect(yPos, titan256cHIntMode);

        setYPosFalling(yPos);
        VDP_setVerticalScrollVSync(BG_B, yPos);

        #if TITAN_SPHERE_TEXT_ANIMATION && TITAN_SPHERE_TEXT_ANIMATION_CHANGE_COLOR
        if (titan256cHIntMode < (HINT_STRATEGY_TOTAL - 1))
            updateSphereTextColor();
        #endif

        // NOTE: this is called from VInt
        //updateTextGradientColors();

        // Enqueue 2 strips palettes depending on the Y position (in strips) of the image
        enqueue2Pals(yPos / TITAN_256C_STRIP_HEIGHT);

        #if TITAN_SPHERE_TEXT_ANIMATION
        if (titan256cHIntMode < (HINT_STRATEGY_TOTAL - 1)) {
            sphereTextAnimationsPosition(TITAN_SPHERE_TILEMAP_START_X_POS * 8, TITAN_SPHERE_TILEMAP_START_Y_POS * 8 - yPos);
            toggleSphereTextAnimations();
            spr_eng_update(); //SPR_update();
        }
        #endif

        SYS_doVBlankProcess();
    }

    //---------------------------------
    // TITAN DISPLAY LOOP
    //---------------------------------

    joyStatusReset();

    // Titan display loop
    while (currentGameStatus == STATIONARY) {
        #if TITAN_SPHERE_TEXT_ANIMATION && TITAN_SPHERE_TEXT_ANIMATION_CHANGE_COLOR
        if (titan256cHIntMode < (HINT_STRATEGY_TOTAL - 1))
            updateSphereTextColor();
        #endif

        // NOTE: this is called from VInt
        //updateTextGradientColors();

        // Load 1st and 2nd strip's palette
        enqueue2Pals(0);

        if (buttonBitsState & (BUTTON_START | BUTTON_A | BUTTON_B | BUTTON_C)){
            currentGameStatus = FADE_OUT;
            break;
        }

        #if TITAN_SPHERE_TEXT_ANIMATION
        if (titan256cHIntMode < (HINT_STRATEGY_TOTAL - 1)) {
            updateSphereTextAnimFrameOnJoyInput(&buttonBitsChange, &buttonBitsState);
            toggleSphereTextAnimations();
            spr_eng_update(); //SPR_update();
        }
        #endif

        SYS_doVBlankProcess();
    }

    //---------------------------------
    // TITAN FADING TO BLACK EFFECT
    //---------------------------------

    joyStatusReset();

    u8 fadingStripCnt = 0;
    u8 prevFadingStrip = 0;
    u8 fadingCycleCurrStrip = 0; // use to split the fading to black into FADE_OUT_COLOR_STEPS cycles, due to its lenghty execution

    // Fade to black effect loop
    while (currentGameStatus == FADE_OUT) {
        // advance 1 strip every N frames. N = FADE_OUT_STRIPS_SPLIT_CYCLES (used to execute the fading for current strip)
        u8 currFadingStrip = fadingStripCnt / FADE_OUT_STRIPS_SPLIT_CYCLES; // Use divu() for N non power of 2
        ++fadingStripCnt;
        // already passed last strip? then fading is finished
        if (currFadingStrip == (TITAN_256C_STRIPS_COUNT + FADE_OUT_COLOR_STEPS)) {
            currentGameStatus = TRANSITION_SCREEN;
            break;
        }

        // Enable the fading effect on titan text calculated on VInt
        setCurrentFadingStripForText(currFadingStrip);

        #if TITAN_SPHERE_TEXT_ANIMATION && TITAN_SPHERE_TEXT_ANIMATION_CHANGE_COLOR
        if (titan256cHIntMode < (HINT_STRATEGY_TOTAL - 1))
            updateSphereTextColor();
        #endif

        // NOTE: this is called from VInt
        //updateTextGradientColors();

        // Load 1st and 2nd strip's palettes
        enqueue2Pals(0);

        // apply fade to black from currFadingStrip up to FADE_OUT_COLOR_STEPS previous strips
        fadingStepToBlack_pals(currFadingStrip, fadingCycleCurrStrip);
        ++fadingCycleCurrStrip;

        if (currFadingStrip != prevFadingStrip) {
            prevFadingStrip = currFadingStrip;
            fadingCycleCurrStrip = 0;
        }

        #if TITAN_SPHERE_TEXT_ANIMATION
        if (titan256cHIntMode < (HINT_STRATEGY_TOTAL - 1)) {
            toggleSphereTextAnimations();
            spr_eng_update(); //SPR_update();
        }
        #endif

        SYS_doVBlankProcess();
    }

    SYS_disableInts();

        SYS_setVBlankCallback(NULL);
        VDP_setHInterrupt(FALSE);
        SYS_setHIntCallback(NULL);

        JOY_setEventHandler(NULL);

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
    currentGameStatus = TRANSITION_SCREEN;
    titan256cHIntMode = HINT_STRATEGY_0; // set initial HInt color swap strategy
}

int main (bool hardReset) {

	// on soft reset do like a hard reset
	if (!hardReset) {
		VDP_waitDMACompletion(); // avoids some glitches as per Genesis Manual's Addendum section
		SYS_hardReset();
	}

    displayTeddyBearLogo();
    waitMs_(200);

    basicEngineConfig();
    initGameStatus();

    for (;;) {
        showTransitionScreen();
        titan256cDisplay();
        setNextHintMode();
    }

    return 0;
}
