#include <genesis.h>
#include "utils.h"
#include "segaLogo.h"
#include "teddyBearLogo.h"
#include "titan256c.h"
#include "hvInterrupts.h"

#define HINT_MODES 4
static u16 titan256cHIntMode;

static u16 currTileIndex;

static void titan256cDisplay () {

    PAL_setColors(0, palette_black, 64, DMA); // palette_black is an array of 64
    blackCurrentGradientPtr();

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
            SYS_setHIntCallback(horizIntOnTitan256cCallback_CPU_EveryN_asm);
        }
        // Call the HInt every N scanlines. Uses CPU for palette swapping
        if (titan256cHIntMode == 1) {
            SYS_setVBlankCallback(vertIntOnTitan256cCallback_HIntEveryN);
            VDP_setHIntCounter(TITAN_256C_STRIP_HEIGHT - 1);
            SYS_setHIntCallback(horizIntOnTitan256cCallback_CPU_EveryN);
        }
        // Call the HInt every N scanlines. Uses DMA for palette swapping
        else if (titan256cHIntMode == 2) {
            SYS_setVBlankCallback(vertIntOnTitan256cCallback_HIntEveryN);
            VDP_setHIntCounter(TITAN_256C_STRIP_HEIGHT - 1);
            SYS_setHIntCallback(horizIntOnTitan256cCallback_DMA_EveryN);
        }
        // Use only one call to HInt to avoid method call overhead. Uses DMA for palette swapping
        else if (titan256cHIntMode == 3) {
            SYS_setVBlankCallback(vertIntOnTitan256cCallback_HIntOneTime);
            VDP_setHIntCounter(0);
            SYS_setHIntCallback(horizIntOnTitan256cCallback_DMA_OneTime);
        }
        VDP_setHInterrupt(TRUE);
    SYS_enableInts();

    VDP_setEnable(TRUE);

    u16 yPos = TITAN_256C_HEIGHT - 1; // initial value, same than set in hvinterrupts.c
    s16 velocity = 0;

    // disable the fading effect on titan text
    setCurrentFadingStripForText(0);

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

        // Load 2 strip palettes depending on the Y position (in strips) of the image
        load2Pals(yPos / TITAN_256C_STRIP_HEIGHT);

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
        load2Pals(0);

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
        load2Pals(0);

        // enable the fading effect on titan text calculated on VInt
        setCurrentFadingStripForText(currFadingStrip);

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

static void showTransitionScreen () {
    const char* startText = "(press START to continue)";

    const char* mode0_textA = "HInt in ASM";
    const char* mode0_textB = "Called every 8 scanlines";
    const char* mode0_textC = "Uses CPU to move colors into VDP CRAM";

    const char* mode1_textA = "HInt in C";
    const char* mode1_textB = "Called every 8 scanlines";
    const char* mode1_textC = "Uses CPU to move colors into VDP CRAM";

    const char* mode2_textA = "HInt in C";
    const char* mode2_textB = "Called every 8 scanlines";
    const char* mode2_textC = "Uses DMA to move colors into VDP CRAM";

    const char* mode3_textA = "HInt in C";
    const char* mode3_textB = "Called only once";
    const char* mode3_textC = "Uses DMA to move colors into VDP CRAM";

    SYS_disableInts();
        SYS_setVBlankCallback(vertIntOnDrawTextCallback);
        VDP_setHIntCounter(screenHeight/2 + 16 - 1); // scanline location for startText
        SYS_setHIntCallback(horizIntOnDrawTextCallback);
        VDP_setHInterrupt(TRUE);
    SYS_enableInts();

    u16 screenWidthTiles = screenWidth/8;
    u16 screenHeightTiles = screenHeight/8;
    switch (titan256cHIntMode) {
        case 0:
            VDP_drawText(mode0_textA, (screenWidthTiles - strlen(mode0_textA)) / 2, screenHeightTiles / 2 - 1);
            VDP_drawText(mode0_textB, (screenWidthTiles - strlen(mode0_textB)) / 2, screenHeightTiles / 2 - 0);
            VDP_drawText(mode0_textC, (screenWidthTiles - strlen(mode0_textC)) / 2, screenHeightTiles / 2 + 1);
            break;
        case 1:
            VDP_drawText(mode1_textA, (screenWidthTiles - strlen(mode1_textA)) / 2, screenHeightTiles / 2 - 1);
            VDP_drawText(mode1_textB, (screenWidthTiles - strlen(mode1_textB)) / 2, screenHeightTiles / 2 - 0);
            VDP_drawText(mode1_textC, (screenWidthTiles - strlen(mode1_textC)) / 2, screenHeightTiles / 2 + 1);
            break;
        case 2:
            VDP_drawText(mode2_textA, (screenWidthTiles - strlen(mode2_textA)) / 2, screenHeightTiles / 2 - 1);
            VDP_drawText(mode2_textB, (screenWidthTiles - strlen(mode2_textB)) / 2, screenHeightTiles / 2 - 0);
            VDP_drawText(mode2_textC, (screenWidthTiles - strlen(mode2_textC)) / 2, screenHeightTiles / 2 + 1);
            break;
        case 3:
            VDP_drawText(mode3_textA, (screenWidthTiles - strlen(mode3_textA)) / 2, screenHeightTiles / 2 - 1);
            VDP_drawText(mode3_textB, (screenWidthTiles - strlen(mode3_textB)) / 2, screenHeightTiles / 2 - 0);
            VDP_drawText(mode3_textC, (screenWidthTiles - strlen(mode3_textC)) / 2, screenHeightTiles / 2 + 1);
            break;
        default: break;
    }
    VDP_drawText(startText, (screenWidthTiles - strlen(startText)) / 2, screenHeightTiles / 2 + 2);

    for (;;) {
        const u16 joyState = JOY_readJoypad(JOY_1);
        if (joyState & BUTTON_START) {
            break;
        }
        SYS_doVBlankProcess();
    }

    switch (titan256cHIntMode) {
        case 0:
            VDP_clearText((screenWidthTiles - strlen(mode0_textA)) / 2, screenHeightTiles / 2 - 1, strlen(mode0_textA));
            VDP_clearText((screenWidthTiles - strlen(mode0_textB)) / 2, screenHeightTiles / 2 - 0, strlen(mode0_textB));
            VDP_clearText((screenWidthTiles - strlen(mode0_textC)) / 2, screenHeightTiles / 2 + 1, strlen(mode0_textC));
            break;
        case 1:
            VDP_clearText((screenWidthTiles - strlen(mode1_textA)) / 2, screenHeightTiles / 2 - 1, strlen(mode1_textA));
            VDP_clearText((screenWidthTiles - strlen(mode1_textB)) / 2, screenHeightTiles / 2 - 0, strlen(mode1_textB));
            VDP_clearText((screenWidthTiles - strlen(mode1_textC)) / 2, screenHeightTiles / 2 + 1, strlen(mode1_textC));
            break;
        case 2:
            VDP_clearText((screenWidthTiles - strlen(mode2_textA)) / 2, screenHeightTiles / 2 - 1, strlen(mode2_textA));
            VDP_clearText((screenWidthTiles - strlen(mode2_textB)) / 2, screenHeightTiles / 2 - 0, strlen(mode2_textB));
            VDP_clearText((screenWidthTiles - strlen(mode2_textC)) / 2, screenHeightTiles / 2 + 1, strlen(mode2_textC));
            break;
        case 3:
            VDP_clearText((screenWidthTiles - strlen(mode3_textA)) / 2, screenHeightTiles / 2 - 1, strlen(mode3_textA));
            VDP_clearText((screenWidthTiles - strlen(mode3_textB)) / 2, screenHeightTiles / 2 - 0, strlen(mode3_textB));
            VDP_clearText((screenWidthTiles - strlen(mode3_textC)) / 2, screenHeightTiles / 2 + 1, strlen(mode3_textC));
            break;
        default: break;
    }
    VDP_clearText((screenWidthTiles - strlen(startText)) / 2, screenHeightTiles / 2 + 2, strlen(startText));
    SYS_doVBlankProcess();

    SYS_disableInts();
        SYS_setVBlankCallback(NULL);
        VDP_setHInterrupt(FALSE);
        SYS_setHIntCallback(NULL);
    SYS_enableInts();
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
    titan256cHIntMode = 0; // set initial color swap strategy
}

int main (bool hard) {

    if (!hard) SYS_hardReset();

    // displaySegaLogo();
    // waitMs_(200);
    // displayTeddyBearLogo();
    // waitMs_(200);

    basicEngineConfig();
    initGameStatus();

    for (;;) {
        showTransitionScreen();
        titan256cDisplay();
        // move into next Titan256c HInt mode
        titan256cHIntMode = modu(titan256cHIntMode + 1, HINT_MODES);
    }

    return 0;
}
