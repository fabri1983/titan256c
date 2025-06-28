#include <types.h>
#include <vdp.h>
#include <pal.h>
#include <sys.h>
#include <timer.h>
#include <tools.h> // KLog methods
#include "hvInterrupts.h"
#include "titan256c.h"
#include "custom_font_res.h"
#include "customFont_consts.h"

// u8 reg01; // Holds current VDP register 1 whole value (it holds other bits than VDP ON/OFF status)

// FORCE_INLINE void copyReg01 () {
//     reg01 = VDP_getReg(0x01);
// }

/// @brief Set bit 6 (64 decimal, 0x40 hexa) of reg 1.
/// @param reg01 VDP's Reg 1 holds other bits than just VDP ON/OFF status, so we need its current value.
static FORCE_INLINE void turnOffVDP (u8 reg01) {
    //reg01 &= ~0x40;
    //*(vu16*) VDP_CTRL_PORT = 0x8100 | reg01;
    *(vu16*) VDP_CTRL_PORT = 0x8100 | (reg01 & ~0x40);
}

/// @brief Set bit 6 (64 decimal, 0x40 hexa) of reg 1.
/// @param reg01 VDP's Reg 1 holds other bits than just VDP ON/OFF status, so we need its current value.
static FORCE_INLINE void turnOnVDP (u8 reg01) {
    //reg01 |= 0x40;
    //*(vu16*) VDP_CTRL_PORT = 0x8100 | reg01;
    *(vu16*) VDP_CTRL_PORT = 0x8100 | (reg01 | 0x40);
}

/**
 * Wait until HCounter 0xC00009 reaches nth position (actually the (n*2)th pixel since the VDP counts by 2)
*/
static FORCE_INLINE void waitHCounter_old (u8 n) {
    // HCOUNTER = VDP_HVCOUNTER_PORT + 1 = 0xC00009
    __asm volatile (
        "1:\n\t"
        "cmpi.b    %[hcLimit],0xC00009\n\t" // cmp: (0xC00009) - hcLimit
        "blo.s     1b"                      // Compares byte because hcLimit won't be > 160 for our practical cases
        // blo is for unsigned comparisons, same than bcs
        :
        : [hcLimit] "i" (n)
        : "cc"
    );
}

/**
 * Wait until HCounter 0xC00009 reaches nth position (actually the (n*2)th pixel since the VDP counts by 2).
*/
static FORCE_INLINE void waitHCounter_opt1 (u8 n) {
    u32 regA = VDP_HVCOUNTER_PORT + 1; // HCounter address is 0xC00009
    __asm volatile (
        "1:\n\t" 
        "cmp.b     (%0),%1\n\t" // cmp: n - (0xC00009). Compares byte because hcLimit won't be > 160 for our practical cases
        "bhi.s     1b"          // loop back if n is higher than (0xC00009)
        // bhi is for unsigned comparisons
        :
        : "a" (regA), "d" (n)
        : "cc"
    );
}

/**
 * Wait until HCounter 0xC00009 reaches nth position (actually the (n*2)th pixel since the VDP counts by 2).
*/
static FORCE_INLINE void waitHCounter_opt2 (u8 n) {
    u32* regA; // placeholder used to indicate the use of an Ax register
    __asm volatile (
        "move.l    #0xC00009,%0\n\t" // Load HCounter (VDP_HVCOUNTER_PORT + 1 = 0xC00009) into an An register
        "1:\n\t" 
        "cmp.b     (%0),%1\n\t" // cmp: n - (0xC00009). Compares byte because hcLimit won't be > 160 for our practical cases
        "bhi.s     1b"          // loop back if n is higher than (0xC00009)
        // bhi is for unsigned comparisons
        : "=a" (regA)
        : "d" (n)
        : "cc"
    );
}

/**
 * Wait until VCounter 0xC00008 reaches nth scanline position. Only valid with constants.
*/
static FORCE_INLINE void waitVCounterConst (u16 n) {
    u32 regA = VDP_HVCOUNTER_PORT; // VCounter address is 0xC00008
    __asm volatile (
        "1:\n\t"
        "cmpi.w     %[vcLimit],(%0)\n\t" // cmp: (0xC00008) - vcLimit
        "blo.s     1b"                   // if (0xC00008) < vcLimit then loop back
        // blo is for unsigned comparisons, same than bcs
        :
        : "a" (regA), [vcLimit] "i" (n << 8) // (n << 8) | 0xFF
        : "cc"
    );
}

/**
 * Wait until VCounter 0xC00008 reaches nth scanline position. Parameter n is loaded into a register.
 * The docs straight up say to not trust the value of the V counter during vblank, in that case use VDP_getAdjustedVCounter().
*/
static FORCE_INLINE void waitVCounterReg (u16 n) {
    // The docs straight up say to not trust the value of the V counter during vblank, in that case use VDP_getAdjustedVCounter().
    // - Sik: on PAL Systems it jumps from 0x102 (258) to 0x1CA (458) (assuming V28).
    // - Sik: Note that the 9th bit is not visible through the VCounter, so what happens from the 68000's viewpoint is that it reaches 0xFF (255), 
    // then counts from 0x00 to 0x02, then from 0xCA (202) to 0xFF (255), and finally starts back at 0x00.
    // - Stef: on PAL System the VCounter rollback occurs at 0xCA (202) so probably better to use n up to 202 to avoid that edge case.
    
    u32 regA = VDP_HVCOUNTER_PORT; // VCounter address is 0xC00008
    __asm volatile (
        "1:\n\t"
        "cmp.w     (%0),%1\n\t" // cmp: n - (0xC00008)
        "bhi.s     1b"          // loop back if n is higher than (0xC00008)
        // bhi is for unsigned comparisons
        :
        : "a" (regA), "d" (n << 8) // (n << 8) | 0xFF
        : "cc"
    );
}

void setHVCallbacks (u8 titan256cHIntMode) {
    switch (titan256cHIntMode) {
        // Call the HInt every N scanlines. Uses CPU for palette swapping. In ASM.
        case HINT_STRATEGY_0:
            SYS_setVBlankCallback(vertIntOnTitan256cCallback_HIntEveryN);
            VDP_setHIntCounter(TITAN_256C_STRIP_HEIGHT - 1);
            SYS_setHIntCallback(horizIntOnTitan256cCallback_CPU_EveryN_asm);
            break;
        // Call the HInt every N scanlines. Uses CPU for palette swapping. In C.
        case HINT_STRATEGY_1:
            SYS_setVBlankCallback(vertIntOnTitan256cCallback_HIntEveryN);
            VDP_setHIntCounter(TITAN_256C_STRIP_HEIGHT - 1);
            SYS_setHIntCallback(horizIntOnTitan256cCallback_CPU_EveryN);
            break;
        // Call the HInt every N scanlines. Uses DMA for palette swapping. In ASM.
        case HINT_STRATEGY_2:
            SYS_setVBlankCallback(vertIntOnTitan256cCallback_HIntEveryN);
            VDP_setHIntCounter(TITAN_256C_STRIP_HEIGHT - 1);
            SYS_setHIntCallback(horizIntOnTitan256cCallback_DMA_EveryN_asm);
            break;
        // Call the HInt every N scanlines. Uses DMA for palette swapping. In C.
        case HINT_STRATEGY_3:
            SYS_setVBlankCallback(vertIntOnTitan256cCallback_HIntEveryN);
            VDP_setHIntCounter(TITAN_256C_STRIP_HEIGHT - 1);
            SYS_setHIntCallback(horizIntOnTitan256cCallback_DMA_EveryN);
            break;
        // Use only one call to HInt to avoid method call overhead. Uses DMA for palette swapping. In C.
        case HINT_STRATEGY_4:
            SYS_setVBlankCallback(vertIntOnTitan256cCallback_HIntOneTime);
            VDP_setHIntCounter(0);
            SYS_setHIntCallback(horizIntOnTitan256cCallback_DMA_OneTime);
            break;
        default: break;
    }
}

static u8 titan256cHIntMode;
static u16 startingScanlineForBounceEffect = 0;

void setHIntScanlineStarterForBounceEffect (u16 yPos, u8 hintMode) {
    startingScanlineForBounceEffect = 0;
    if (yPos < (TITAN_256C_HEIGHT - (2 * TITAN_256C_STRIP_HEIGHT) - 1))
        startingScanlineForBounceEffect = (TITAN_256C_STRIP_HEIGHT - 1) - (yPos % TITAN_256C_STRIP_HEIGHT);
    titan256cHIntMode = hintMode;
}

static u16 vcounterManual;

HINTERRUPT_CALLBACK horizIntScanlineStarterForBounceEffectCallback () {
    ++vcounterManual;
    // if vcounterManual is not at the correct scanline then return
    if (vcounterManual-1 < startingScanlineForBounceEffect) {
        return;
    }

    switch (titan256cHIntMode) {
        case HINT_STRATEGY_0:
            if (startingScanlineForBounceEffect == 0)
                SYS_setHIntCallback(horizIntOnTitan256cCallback_CPU_EveryN_asm);
            // instead of VDP_setHIntCounter(TITAN_256C_STRIP_HEIGHT - 1) due to additionals read and write from/to internal regValues[]
            *((u16*) VDP_CTRL_PORT) = 0x8A00 | (TITAN_256C_STRIP_HEIGHT - 1);
            break;
        case HINT_STRATEGY_1:
            if (startingScanlineForBounceEffect == 0)
                SYS_setHIntCallback(horizIntOnTitan256cCallback_CPU_EveryN);
            // instead of VDP_setHIntCounter(TITAN_256C_STRIP_HEIGHT - 1) due to additionals read and write from/to internal regValues[]
            *((u16*) VDP_CTRL_PORT) = 0x8A00 | (TITAN_256C_STRIP_HEIGHT - 1);
            break;
        case HINT_STRATEGY_2:
            if (startingScanlineForBounceEffect == 0)
                SYS_setHIntCallback(horizIntOnTitan256cCallback_DMA_EveryN_asm);
            // instead of VDP_setHIntCounter(TITAN_256C_STRIP_HEIGHT - 1) due to additionals read and write from/to internal regValues[]
            *((u16*) VDP_CTRL_PORT) = 0x8A00 | (TITAN_256C_STRIP_HEIGHT - 1);
            break;
        case HINT_STRATEGY_3:
            if (startingScanlineForBounceEffect == 0)
                SYS_setHIntCallback(horizIntOnTitan256cCallback_DMA_EveryN);
            // instead of VDP_setHIntCounter(TITAN_256C_STRIP_HEIGHT - 1) due to additionals read and write from/to internal regValues[]
            *((u16*) VDP_CTRL_PORT) = 0x8A00 | (TITAN_256C_STRIP_HEIGHT - 1);
            break;
        case HINT_STRATEGY_4:
            if (startingScanlineForBounceEffect == 0)
                SYS_setHIntCallback(horizIntOnTitan256cCallback_DMA_OneTime);
            // instead of VDP_setHIntCounter(0xFF) due to additionals read and write from/to internal regValues[]
            *((u16*) VDP_CTRL_PORT) = 0x8A00 | 0xFF;
            break;
        default: break;
    }

    startingScanlineForBounceEffect = 0;
}

static u16* titan256cPalsPtr; // 1st and 2nd strip's palette are loaded at the beginning of the display loop, so it points to 3rd strip
static u8 palIdx; // which pallete the titan256cPalsPtr uses
static u16* currGradPtr;
static u16 applyBlackPalPosY;
static u16 textRampEffectLimitTop;
static u16 textRampEffectLimitBottom;

FORCE_INLINE void setGradientPtrToBlack () {
    currGradPtr = (u16*) palette_black;
}

static FORCE_INLINE void varsSetup () {
    // Setup sending colors in u32 type
	//*((vu16*) VDP_CTRL_PORT) = 0x8F00 | 2; // instead of VDP_setAutoInc(2) due to additionals read and write from/to internal regValues[]

    u16 posYFalling = getYPosFalling();
    u16 stripN = min(TITAN_256C_HEIGHT/TITAN_256C_STRIP_HEIGHT - 1, (posYFalling / TITAN_256C_STRIP_HEIGHT) + 2);
    applyBlackPalPosY = TITAN_256C_HEIGHT - posYFalling;
    // This marks where to start painting all the image as black
    if (applyBlackPalPosY <= (TITAN_256C_STRIP_HEIGHT - 1))
        titan256cPalsPtr = (u16*) palette_black;
    // Otherwise use normal colors
    else
        titan256cPalsPtr = getPalettesData() + stripN * TITAN_256C_COLORS_PER_STRIP;

    // On even strips we know we use [PAL0,PAL1] so starts with palIdx=0. On odd strips is [PAL1,PAL2] so starts with palIdx=32.
    palIdx = ((posYFalling / TITAN_256C_STRIP_HEIGHT) % 2) == 0 ? 0 : TITAN_256C_COLORS_PER_STRIP;

    textRampEffectLimitTop = TITAN_256C_TEXT_STARTING_STRIP * TITAN_256C_STRIP_HEIGHT - posYFalling + TITAN_256C_TEXT_OFFSET_TOP;
    textRampEffectLimitBottom = TITAN_256C_TEXT_ENDING_STRIP * TITAN_256C_STRIP_HEIGHT - posYFalling + TITAN_256C_TEXT_OFFSET_BOTTOM;
    currGradPtr = getGradientColorsBuffer();
}

void vertIntOnTitan256cCallback_HIntEveryN () {
    varsSetup();
    vcounterManual = TITAN_256C_STRIP_HEIGHT - 1;
    // when on bouncing, set the HInt responsibly of proper set the starting scanline of the color swap HInt
    if (startingScanlineForBounceEffect != 0) {
        VDP_setHIntCounter(0);
        SYS_setHIntCallback(horizIntScanlineStarterForBounceEffectCallback);
        vcounterManual = 0;
    }

    updateTextGradientColors();
}

void vertIntOnTitan256cCallback_HIntOneTime () {
    varsSetup();
    vcounterManual = 0;
    VDP_setHIntCounter(0); // Every scanline

    updateTextGradientColors();
}

HINTERRUPT_CALLBACK horizIntOnTitan256cCallback_CPU_EveryN_asm () {
    // 1292-1320 cycles total (budget is 480~488 cycles per scanline - 120 cost of Hint Callback)
    __asm volatile (
        ".prepare_regs_%=:\n"
        "   movea.l     %[currGradPtr],%%a0\n"       // a0: currGradPtr
        "   movea.l     %[titan256cPalsPtr],%%a1\n"  // a1: titan256cPalsPtr
        "   lea         0xC00004,%%a2\n"             // a2: VDP_CTRL_PORT 0xC00004
        "   lea         -4(%%a2),%%a3\n"             // a3: VDP_DATA_PORT 0xC00000
        "   lea         5(%%a2),%%a4\n"              // a4: HCounter address 0xC00009 (VDP_HVCOUNTER_PORT + 1)
        "   move.b      %[hcLimit],%%d7\n"           // d7: HCounter limit
        "   move.w      %[turnOff],%%a5\n"           // a5: VDP's register with display OFF value
        "   move.l      %%a6,%%usp\n"                // save a6 in the user stack (a6 holds the Stack Frame and if not saved then the code fails)
                                                     // Make sure no interrupt happen during the time of this snippet
        "   movea.w     %[turnOn],%%a6\n"            // a6: VDP's register with display ON value

      //".setGradColorForText_flag_%=:\n"
      //"   moveq       #0,%%d4\n"                             // d4: setGradColorForText = 0 (FALSE)
      //"   move.w      %[vcounterManual],%%d0\n"              // d0: vcounterManual
      //"   cmp.w       %[textRampEffectLimitTop],%%d0\n"      // cmp: vcounterManual - textRampEffectLimitTop
      //"   blo.s       .color_batch_1_cmd\n"                  // branch if (vcounterManual < textRampEffectLimitTop) (opposite than vcounterManual >= textRampEffectLimitTop)
      //"   cmp.w       %[textRampEffectLimitBottom],%%d0\n"   // cmp: vcounterManual - textRampEffectLimitBottom
      //"   bhi.s       .color_batch_1_cmd\n"                  // branch if (vcounterManual > textRampEffectLimitBottom) (opposite than vcounterManual <= textRampEffectLimitBottom)
      //"   moveq       #1,%%d4\n"                             // d4: setGradColorForText = 1 (TRUE)
        ".setGradColorForText_flag_%=:\n"
        "   move.w      %[vcounterManual],%%d0\n"              // d0: vcounterManual
            // translate vcounterManual to base textRampEffectLimitTop:
        "   sub.w       %[textRampEffectLimitTop],%%d0\n"      // d0: vcounterManual - textRampEffectLimitTop
            // _TEXT_RAMP_EFFECT_HEIGHT is the amount of scanlines the text ramp effect takes:
        "   cmpi.w      %[_TEXT_RAMP_EFFECT_HEIGHT],%%d0\n"    // d0 -= TEXT_RAMP_EFFECT_HEIGHT
        "   scs.b       %%d4\n"                                // if d0 >= 0 => d4=0xFF (enable bg color), if d0 < 0 => d4=0x00 (no bg color)
        // Next instructions not needed, logic works fine without them. I don't get it.
      //"   bls.s       .color_batch_1_cmd\n"                  // branch if d0 <= TEXT_RAMP_EFFECT_HEIGHT (at this moment d4 is correctly set)
      //"   moveq       #0,%%d4\n"                             // d4=0x00 (no bg color)

		".color_batch_1_cmd:\n"
			// cmdAddress = palIdx == 0 ? 0xC0000000 : 0xC0400000;
            // set base command address once and then we'll add the right offset in next color batch blocks
		"   move.l      #0xC0000000,%%d6\n"     // d6: cmdAddress = 0xC0000000
		"   tst.b       %[palIdx]\n"            // palIdx == 0?
		"   beq.s       .set_bgColor_1_%=\n"
		"   move.l      #0xC0400000,%%d6\n"     // d6: cmdAddress = 0xC0400000
		".set_bgColor_1_%=:\n"
		"   moveq       #0,%%d5\n"              // d5: bgColor = 0
            // from here on, d5=0 as long as d4 == 0, so no need to reset d5 in other color batch blocks
		"   tst.b       %%d4\n"                 // d4: setGradColorForText => if setGradColorForText == 0
		"   beq.s       .color_batch_1_pal\n"
		"   move.w      (%%a0)+,%%d5\n"         // d5: bgColor = *currGradPtr++;
		".color_batch_1_pal:\n"
		"   move.l      (%%a1)+,%%d0\n"         // d0: colors2_A = *((u32*) (titan256cPalsPtr + 0)); // 2 colors
		"   move.l      (%%a1)+,%%d1\n"         // d1: colors2_B = *((u32*) (titan256cPalsPtr + 2)); // next 2 colors
        // wait HCounter
        "1:\n"
        "   cmp.b       (%%a4),%%d7\n"          // cmp: d7 - (a4). Compare byte size given that d7 won't be > 160 for our practical cases
        "   bhi.s       1b\n"                   // loop back if d7 is higher than (a4)
		// turn off VDP
		"   move.w      %%a5,(%%a2)\n"          // *(vu16*) VDP_CTRL_PORT = 0x8100 | (reg01 & ~0x40);
		// send colors
		"   move.l      %%d6,(%%a2)\n"          // *((vu32*) VDP_CTRL_PORT) = cmdAddress;
		"   move.l      %%d0,(%%a3)\n"          // *((vu32*) VDP_DATA_PORT) = colors2_A;
		"   move.l      %%d1,(%%a3)\n"          // *((vu32*) VDP_DATA_PORT) = colors2_B;
		"   move.l      #0xC0000000,(%%a2)\n"   // *((vu32*) VDP_CTRL_PORT) = 0xC0000000; // VDP_WRITE_CRAM_ADDR(0): write to CRAM color index 0 multiplied by 2
		"   move.w      %%d5,(%%a3)\n"          // *((vu16*) VDP_DATA_PORT) = bgColor;
		// turn on VDP
		"   move.w      %%a6,(%%a2)\n"          // *(vu16*) VDP_CTRL_PORT = 0x8100 | (reg01 | 0x40);

		".color_batch_2_cmd:\n"
			// cmdAddress = palIdx == 0 ? 0xC0080000 : 0xC0480000;
		"   addi.l      #0x80000,%%d6\n"        // d6: cmdAddress += 0x80000 // previous batch advanced 4 colors
		".color_batch_2_pal:\n"
		//"   move.l      (%%a1)+,%%d0\n"         // d0: colors2_A = *((u32*) (titan256cPalsPtr + 4)); // 2 colors
		//"   move.l      (%%a1)+,%%d1\n"         // d1: colors2_B = *((u32*) (titan256cPalsPtr + 6)); // next 2 colors
		//"   move.l      (%%a1)+,%%d2\n"         // d2: colors2_C = *((u32*) (titan256cPalsPtr + 8)); // next 2 colors
		//"   move.l      (%%a1)+,%%d3\n"         // d3: colors2_D = *((u32*) (titan256cPalsPtr + 10)); // next 2 colors
        "   movem.l     (%%a1)+,%%d0-%%d3\n"
        // wait HCounter
        "1:\n"
        "   cmp.b       (%%a4),%%d7\n"          // cmp: d7 - (a4). Compare byte size given that d7 won't be > 160 for our practical cases
        "   bhi.s       1b\n"                   // loop back if d7 is higher than (a4)
		// turn off VDP
		"   move.w      %%a5,(%%a2)\n"          // *(vu16*) VDP_CTRL_PORT = 0x8100 | (reg01 & ~0x40);
		// send colors
		"   move.l      %%d6,(%%a2)\n"          // *((vu32*) VDP_CTRL_PORT) = cmdAddress;
		"   move.l      %%d0,(%%a3)\n"          // *((vu32*) VDP_DATA_PORT) = colors2_A;
		"   move.l      %%d1,(%%a3)\n"          // *((vu32*) VDP_DATA_PORT) = colors2_B;
		"   move.l      %%d2,(%%a3)\n"          // *((vu32*) VDP_DATA_PORT) = colors2_C;
		"   move.l      %%d3,(%%a3)\n"          // *((vu32*) VDP_DATA_PORT) = colors2_D;
		// turn on VDP
		"   move.w      %%a6,(%%a2)\n"          // *(vu16*) VDP_CTRL_PORT = 0x8100 | (reg01 | 0x40);

		".color_batch_3_cmd:\n"
			// cmdAddress = palIdx == 0 ? 0xC0180000 : 0xC0580000;
		"   addi.l      #0x100000,%%d6\n"       // d6: cmdAddress += 0x100000 // previous batch advanced 8 colors
		".set_bgColor_2_%=:\n"
        "   tst.b       %%d4\n"                 // d4: setGradColorForText => if setGradColorForText == 0
		"   beq.s       .color_batch_3_pal\n"
		"   move.w      (%%a0)+,%%d5\n"         // d5: bgColor = *currGradPtr++;
		".color_batch_3_pal:\n"
		"   move.l      (%%a1)+,%%d0\n"         // d0: colors2_A = *((u32*) (titan256cPalsPtr + 12)); // 2 colors
		"   move.l      (%%a1)+,%%d1\n"         // d1: colors2_B = *((u32*) (titan256cPalsPtr + 14)); // next 2 colors
        // wait HCounter
        "1:\n"
        "   cmp.b       (%%a4),%%d7\n"          // cmp: d7 - (a4). Compare byte size given that d7 won't be > 160 for our practical cases
        "   bhi.s       1b\n"                   // loop back if d7 is higher than (a4)
		// turn off VDP
		"   move.w      %%a5,(%%a2)\n"          // *(vu16*) VDP_CTRL_PORT = 0x8100 | (reg01 & ~0x40);
		// send colors
		"   move.l      %%d6,(%%a2)\n"          // *((vu32*) VDP_CTRL_PORT) = cmdAddress;
		"   move.l      %%d0,(%%a3)\n"          // *((vu32*) VDP_DATA_PORT) = colors2_A;
		"   move.l      %%d1,(%%a3)\n"          // *((vu32*) VDP_DATA_PORT) = colors2_B;
		"   move.l      #0xC0000000,(%%a2)\n"   // *((vu32*) VDP_CTRL_PORT) = 0xC0000000; // VDP_WRITE_CRAM_ADDR(0): write to CRAM color index 0 multiplied by 2
		"   move.w      %%d5,(%%a3)\n"          // *((vu16*) VDP_DATA_PORT) = bgColor;
		// turn on VDP
		"   move.w      %%a6,(%%a2)\n"          // *(vu16*) VDP_CTRL_PORT = 0x8100 | (reg01 | 0x40);

		".color_batch_4_cmd:\n"
			// cmdAddress = palIdx == 0 ? 0xC0200000 : 0xC0600000;
		"   addi.l      #0x80000,%%d6\n"        // d6: cmdAddress += 0x80000 // previous batch advanced 4 colors
		".color_batch_4_pal:\n"
		//"   move.l      (%%a1)+,%%d0\n"         // d0: colors2_A = *((u32*) (titan256cPalsPtr + 16)); // 2 colors
		//"   move.l      (%%a1)+,%%d1\n"         // d1: colors2_B = *((u32*) (titan256cPalsPtr + 18)); // next 2 colors
		//"   move.l      (%%a1)+,%%d2\n"         // d2: colors2_C = *((u32*) (titan256cPalsPtr + 20)); // next 2 colors
		//"   move.l      (%%a1)+,%%d3\n"         // d3: colors2_D = *((u32*) (titan256cPalsPtr + 22)); // next 2 colors
        "   movem.l     (%%a1)+,%%d0-%%d3\n"
        // wait HCounter
        "1:\n"
        "   cmp.b       (%%a4),%%d7\n"          // cmp: d7 - (a4). Compare byte size given that d7 won't be > 160 for our practical cases
        "   bhi.s       1b\n"                   // loop back if d7 is higher than (a4)
		// turn off VDP
		"   move.w      %%a5,(%%a2)\n"          // *(vu16*) VDP_CTRL_PORT = 0x8100 | (reg01 & ~0x40);
		// send colors
		"   move.l      %%d6,(%%a2)\n"          // *((vu32*) VDP_CTRL_PORT) = cmdAddress;
		"   move.l      %%d0,(%%a3)\n"          // *((vu32*) VDP_DATA_PORT) = colors2_A;
		"   move.l      %%d1,(%%a3)\n"          // *((vu32*) VDP_DATA_PORT) = colors2_B;
		"   move.l      %%d2,(%%a3)\n"          // *((vu32*) VDP_DATA_PORT) = colors2_C;
		"   move.l      %%d3,(%%a3)\n"          // *((vu32*) VDP_DATA_PORT) = colors2_D;
		// turn on VDP
		"   move.w      %%a6,(%%a2)\n"          // *(vu16*) VDP_CTRL_PORT = 0x8100 | (reg01 | 0x40);

		".color_batch_5_cmd:\n"
			// cmdAddress = palIdx == 0 ? 0xC0300000 : 0xC0700000;
		"   addi.l      #0x100000,%%d6\n"       // d6: cmdAddress += 0x100000 // previous batch advanced 8 colors
		".set_bgColor_3_%=:\n"
		"   tst.b       %%d4\n"                 // d4: setGradColorForText => if setGradColorForText == 0
		"   beq.s       .color_batch_5_pal\n"
		"   move.w      (%%a0)+,%%d5\n"         // d5: bgColor = *currGradPtr++;
		".color_batch_5_pal:\n"
		"   move.l      (%%a1)+,%%d0\n"         // d0: colors2_A = *((u32*) (titan256cPalsPtr + 24)); // 2 colors
		"   move.l      (%%a1)+,%%d1\n"         // d1: colors2_B = *((u32*) (titan256cPalsPtr + 26)); // next 2 colors
        // wait HCounter
        "1:\n"
        "   cmp.b       (%%a4),%%d7\n"          // cmp: d7 - (a4). Compare byte size given that d7 won't be > 160 for our practical cases
        "   bhi.s       1b\n"                   // loop back if d7 is higher than (a4)
		// turn off VDP
		"   move.w      %%a5,(%%a2)\n"          // *(vu16*) VDP_CTRL_PORT = 0x8100 | (reg01 & ~0x40);
		// send colors
		"   move.l      %%d6,(%%a2)\n"          // *((vu32*) VDP_CTRL_PORT) = cmdAddress;
		"   move.l      %%d0,(%%a3)\n"          // *((vu32*) VDP_DATA_PORT) = colors2_A;
		"   move.l      %%d1,(%%a3)\n"          // *((vu32*) VDP_DATA_PORT) = colors2_B;
		"   move.l      #0xC0000000,(%%a2)\n"   // *((vu32*) VDP_CTRL_PORT) = 0xC0000000; // VDP_WRITE_CRAM_ADDR(0): write to CRAM color index 0 multiplied by 2
		"   move.w      %%d5,(%%a3)\n"          // *((vu16*) VDP_DATA_PORT) = bgColor;
		// turn on VDP
		"   move.w      %%a6,(%%a2)\n"          // *(vu16*) VDP_CTRL_PORT = 0x8100 | (reg01 | 0x40);

		".color_batch_6_cmd:\n"
			// cmdAddress = palIdx == 0 ? 0xC0380000 : 0xC0780000;
		"   addi.l      #0x80000,%%d6\n"        // d6: cmdAddress += 0x80000 // previous batch advanced 4 colors
		".color_batch_6_pal:\n"
		"   move.l      (%%a1)+,%%d0\n"         // d0: colors2_A = *((u32*) (titan256cPalsPtr + 28)); // 2 colors
		"   move.l      (%%a1)+,%%d1\n"         // d1: colors2_B = *((u32*) (titan256cPalsPtr + 30)); // next 2 colors
        // wait HCounter
        "1:\n"
        "   cmp.b       (%%a4),%%d7\n"          // cmp: d7 - (a4). Compare byte size given that d7 won't be > 160 for our practical cases
        "   bhi.s       1b\n"                   // loop back if d7 is higher than (a4)
		// turn off VDP
		"   move.w      %%a5,(%%a2)\n"          // *(vu16*) VDP_CTRL_PORT = 0x8100 | (reg01 & ~0x40);
		// send colors
		"   move.l      %%d6,(%%a2)\n"          // *((vu32*) VDP_CTRL_PORT) = cmdAddress;
		"   move.l      %%d0,(%%a3)\n"          // *((vu32*) VDP_DATA_PORT) = colors2_A;
		"   move.l      %%d1,(%%a3)\n"          // *((vu32*) VDP_DATA_PORT) = colors2_B;
		// turn on VDP
		"   move.w      %%a6,(%%a2)\n"          // *(vu16*) VDP_CTRL_PORT = 0x8100 | (reg01 | 0x40);

        // restore a6 before is too late (if not then it crashes)
        "   move.l      %%usp,%%a6\n"

		".set_bgColor_4_%=:\n"
		"   tst.b       %%d4\n"                 // d4: setGradColorForText => if setGradColorForText == 0
		"   beq.s       .accomodate_vars_A_%=\n"
		"   move.w      (%%a0)+,%%d5\n"         // d5: bgColor = *currGradPtr++;
        "   move.l      %%a0,%[currGradPtr]\n"  // store current pointer value currGradPtr

		".accomodate_vars_A_%=:\n"
        "   move.w      %[vcounterManual],%%d0\n"                  // d0: vcounterManual
        "   addq.w      %[_TITAN_256C_STRIP_HEIGHT],%%d0\n"        // d0: vcounterManual += TITAN_256C_STRIP_HEIGHT
        "   move.w      %%d0,%[vcounterManual]\n"                  // store current value of vcounterManual
        "   move.b      %[palIdx],%%d1\n"                          // d1: palIdx
        "   eori.b      %[_TITAN_256C_COLORS_PER_STRIP],%%d1\n"    // d1: palIdx ^= TITAN_256C_COLORS_PER_STRIP // cycles between 0 and 32
        "   move.b      %%d1,%[palIdx]\n"                          // store current value of palIdx

		".color_batch_7_pal:\n"
        // wait HCounter
        "1:\n"
        "   cmp.b       (%%a4),%%d7\n"          // cmp: d7 - (a4). Compare byte size given that d7 won't be > 160 for our practical cases
        "   bhi.s       1b\n"                   // loop back if d7 is higher than (a4)
		// send colors
		"   move.l      #0xC0000000,(%%a2)\n"   // *((vu32*) VDP_CTRL_PORT) = 0xC0000000; // VDP_WRITE_CRAM_ADDR(0): write to CRAM color index 0 multiplied by 2
		"   move.w      %%d5,(%%a3)\n"          // *((vu16*) VDP_DATA_PORT) = bgColor;

        ".accomodate_vars_B_%=:\n"
        "   cmp.w       %[applyBlackPalPosY],%%d0\n"    // cmp: vcounterManual - applyBlackPalPosY
        "   blo.s       2f\n"                           // branch if (vcounterManual < applyBlackPalPosY)
        "   move.l      %[palette_black],%%a1\n"        // a1 = palette_black
        "2:\n"
        "   move.l      %%a1,%[titan256cPalsPtr]\n"     // store current pointer value of a1 into variable titan256cPalsPtr
		: 
        [currGradPtr] "+m" (currGradPtr),
		[titan256cPalsPtr] "+m" (titan256cPalsPtr),
		[vcounterManual] "+m" (vcounterManual),
		[palIdx] "+m" (palIdx)
		: 
		[textRampEffectLimitTop] "m" (textRampEffectLimitTop),
		[textRampEffectLimitBottom] "m" (textRampEffectLimitBottom),
		[applyBlackPalPosY] "m" (applyBlackPalPosY),
		[palette_black] "m" (palette_black),
		[turnOff] "i" (0x8100 | (0x74 & ~0x40)), // 0x8134
		[turnOn] "i" (0x8100 | (0x74 | 0x40)), // 0x8174
        [hcLimit] "i" (156),
		[_TITAN_256C_COLORS_PER_STRIP] "i" (TITAN_256C_COLORS_PER_STRIP),
		[_TITAN_256C_STRIP_HEIGHT] "i" (TITAN_256C_STRIP_HEIGHT),
        [_TEXT_RAMP_EFFECT_HEIGHT] "i" ((TITAN_256C_TEXT_ENDING_STRIP - TITAN_256C_TEXT_STARTING_STRIP) * TITAN_256C_STRIP_HEIGHT + TITAN_256C_TEXT_OFFSET_BOTTOM)
		:
        // backup registers used in the asm implementation including the scratch pad since this code is used in an interrupt call.
        // a6 is backed up at the beginning of the asm block to avoid overriding the Stack Frame value. If not, then glitches happen.
		"d0","d1","d2","d3","d4","d5","d6","d7","a0","a1","a2","a3","a4","a5","cc","memory"
    );
}

HINTERRUPT_CALLBACK horizIntOnTitan256cCallback_CPU_EveryN () {
    // test if current HCounter is in the range for text gradient effect
    bool setGradColorForText = vcounterManual >= textRampEffectLimitTop && vcounterManual <= textRampEffectLimitBottom;

    /*
        Every command is CRAM address to start write 4 colors (2 times u32 bits)
        u32 cmd1st = VDP_WRITE_CRAM_ADDR((u32)(palIdx * 2));
        u32 cmd2nd = VDP_WRITE_CRAM_ADDR((u32)((palIdx + 4) * 2));
        u32 cmd3rd = VDP_WRITE_CRAM_ADDR((u32)((palIdx + 8) * 2));
        u32 cmd4th = VDP_WRITE_CRAM_ADDR((u32)((palIdx + 12) * 2));
        u32 cmd5th = VDP_WRITE_CRAM_ADDR((u32)((palIdx + 16) * 2));
        u32 cmd6th = VDP_WRITE_CRAM_ADDR((u32)((palIdx + 20) * 2));
        u32 cmd7th = VDP_WRITE_CRAM_ADDR((u32)((palIdx + 24) * 2));
        u32 cmd8th = VDP_WRITE_CRAM_ADDR((u32)((palIdx + 28) * 2));
        cmd     palIdx = 0      palIdx = 32
        1       0xC0000000      0xC0400000
        2       0xC0080000      0xC0480000
        3       0xC0100000      0xC0500000
        4       0xC0180000      0xC0580000
        5       0xC0200000      0xC0600000
        6       0xC0280000      0xC0680000
        7       0xC0300000      0xC0700000
        8       0xC0380000      0xC0780000
    */

    u32 cmdAddress;
    u16 bgColor=0;
    u32 colors2_A, colors2_B, colors2_C, colors2_D;
    const u8 hcLimit = 150;

    // Value under current conditions is always 0x74
    //u8 reg01 = VDP_getReg(0x01); // Holds current VDP register 1 value (NOTE: it holds other bits than VDP ON/OFF status)
    // NOTE: here is OK to call VDP_getReg(0x01) only if we didn't previously change the VDP's reg 1 using direct access without VDP_setReg()

    cmdAddress = palIdx == 0 ? 0xC0000000 : 0xC0400000;
    colors2_A = *((u32*) (titan256cPalsPtr + 0)); // 2 colors
    colors2_B = *((u32*) (titan256cPalsPtr + 2)); // next 2 colors
    if (setGradColorForText) bgColor = *currGradPtr++;
    waitHCounter_opt1(hcLimit);
    turnOffVDP(0x74);
    *((vu32*) VDP_CTRL_PORT) = cmdAddress;
    *((vu32*) VDP_DATA_PORT) = colors2_A;
    *((vu32*) VDP_DATA_PORT) = colors2_B;
    *((vu32*) VDP_CTRL_PORT) = 0xC0000000; // VDP_WRITE_CRAM_ADDR(0): write to CRAM color index 0 multiplied by 2
    *((vu16*) VDP_DATA_PORT) = bgColor;
    turnOnVDP(0x74);

    cmdAddress = palIdx == 0 ? 0xC0080000 : 0xC0480000;
    colors2_A = *((u32*) (titan256cPalsPtr + 4)); // 2 colors
    colors2_B = *((u32*) (titan256cPalsPtr + 6)); // next 2 colors
    colors2_C = *((u32*) (titan256cPalsPtr + 8)); // next2 colors
    colors2_D = *((u32*) (titan256cPalsPtr + 10)); // next 2 colors
    waitHCounter_opt1(hcLimit);
    turnOffVDP(0x74);
    *((vu32*) VDP_CTRL_PORT) = cmdAddress;
    *((vu32*) VDP_DATA_PORT) = colors2_A;
    *((vu32*) VDP_DATA_PORT) = colors2_B;
    *((vu32*) VDP_DATA_PORT) = colors2_C;
    *((vu32*) VDP_DATA_PORT) = colors2_D;
    turnOnVDP(0x74);

    cmdAddress = palIdx == 0 ? 0xC0180000 : 0xC0580000;
    colors2_A = *((u32*) (titan256cPalsPtr + 12)); // 2 colors
    colors2_B = *((u32*) (titan256cPalsPtr + 14)); // next 2 colors
    if (setGradColorForText) bgColor = *currGradPtr++;
    waitHCounter_opt1(hcLimit);
    turnOffVDP(0x74);
    *((vu32*) VDP_CTRL_PORT) = cmdAddress;
    *((vu32*) VDP_DATA_PORT) = colors2_A;
    *((vu32*) VDP_DATA_PORT) = colors2_B;
    *((vu32*) VDP_CTRL_PORT) = 0xC0000000; // VDP_WRITE_CRAM_ADDR(0): write to CRAM color index 0 multiplied by 2
    *((vu16*) VDP_DATA_PORT) = bgColor;
    turnOnVDP(0x74);

    cmdAddress = palIdx == 0 ? 0xC0200000 : 0xC0600000;
    colors2_A = *((u32*) (titan256cPalsPtr + 16)); // 2 colors
    colors2_B = *((u32*) (titan256cPalsPtr + 18)); // next 2 colors
    colors2_C = *((u32*) (titan256cPalsPtr + 20)); // next colors
    colors2_D = *((u32*) (titan256cPalsPtr + 22)); // next 2 colors
    waitHCounter_opt1(hcLimit);
    turnOffVDP(0x74);
    *((vu32*) VDP_CTRL_PORT) = cmdAddress;
    *((vu32*) VDP_DATA_PORT) = colors2_A;
    *((vu32*) VDP_DATA_PORT) = colors2_B;
    *((vu32*) VDP_DATA_PORT) = colors2_C;
    *((vu32*) VDP_DATA_PORT) = colors2_D;
    turnOnVDP(0x74);

    cmdAddress = palIdx == 0 ? 0xC0300000 : 0xC0700000;
    colors2_A = *((u32*) (titan256cPalsPtr + 24)); // 2 colors
    colors2_B = *((u32*) (titan256cPalsPtr + 26)); // next 2 colors
    if (setGradColorForText) bgColor = *currGradPtr++;
    waitHCounter_opt1(hcLimit);
    turnOffVDP(0x74);
    *((vu32*) VDP_CTRL_PORT) = cmdAddress;
    *((vu32*) VDP_DATA_PORT) = colors2_A;
    *((vu32*) VDP_DATA_PORT) = colors2_B;
    *((vu32*) VDP_CTRL_PORT) = 0xC0000000; // VDP_WRITE_CRAM_ADDR(0): write to CRAM color index 0 multiplied by 2
    *((vu16*) VDP_DATA_PORT) = bgColor;
    turnOnVDP(0x74);

    cmdAddress = palIdx == 0 ? 0xC0380000 : 0xC0780000;
    colors2_A = *((u32*) (titan256cPalsPtr + 28)); // 2 colors
    colors2_B = *((u32*) (titan256cPalsPtr + 30)); // next 2 colors
    waitHCounter_opt1(hcLimit);
    turnOffVDP(0x74);
    *((vu32*) VDP_CTRL_PORT) = cmdAddress;
    *((vu32*) VDP_DATA_PORT) = colors2_A;
    *((vu32*) VDP_DATA_PORT) = colors2_B;
    turnOnVDP(0x74);

    if (setGradColorForText) bgColor = *currGradPtr++;
    vcounterManual += TITAN_256C_STRIP_HEIGHT;
    palIdx ^= TITAN_256C_COLORS_PER_STRIP; // cycles between 0 and 32
    //palIdx = palIdx == 0 ? 32 : 0;
    //palIdx = (palIdx + 32) & 63; // (palIdx + 32) % 64 => x mod y = x & (y-1) when y is power of 2
    waitHCounter_opt1(hcLimit);
    *((vu32*) VDP_CTRL_PORT) = 0xC0000000; // VDP_WRITE_CRAM_ADDR(0): write to CRAM color index 0 multiplied by 2
    *((vu16*) VDP_DATA_PORT) = bgColor;

    titan256cPalsPtr += TITAN_256C_COLORS_PER_STRIP; // advance to next strip's palette
    if (vcounterManual >= applyBlackPalPosY)
        titan256cPalsPtr = (u16*) palette_black;
}

HINTERRUPT_CALLBACK horizIntOnTitan256cCallback_DMA_EveryN_asm () {
    // 1106-1132 cycles total (budget is 480~488 cycles per scanline - 120 cost of Hint Callback)
    __asm volatile (
        ".prepare_regs_%=:\n"
        "   movea.l     %[currGradPtr],%%a0\n"       // a0: currGradPtr
        "   movea.l     %[titan256cPalsPtr],%%a1\n"  // a1: titan256cPalsPtr
        "   lea         0xC00004,%%a2\n"             // a2: VDP_CTRL_PORT 0xC00004
        "   lea         -4(%%a2),%%a3\n"             // a3: VDP_DATA_PORT 0xC00000
        "   lea         5(%%a2),%%a4\n"              // a4: HCounter address 0xC00009 (VDP_HVCOUNTER_PORT + 1)
        "   move.b      %[hcLimit],%%d6\n"           // d6: HCounter limit
        "   move.w      %[turnOff],%%a5\n"           // a5: VDP's register with display OFF value
        "   move.l      %%a6,%%usp\n"                // save a6 in the user stack (a6 holds the Stack Frame and if not saved then the code fails)
                                                     // Make sure no interrupt happen during the time of this snippet
        "   movea.w     %[turnOn],%%a6\n"            // a6: VDP's register with display ON value

      //".setGradColorForText_flag_%=:\n"
      //"   moveq       #0,%%d3\n"                             // d3: setGradColorForText = 0 (FALSE)
      //"   move.w      %[vcounterManual],%%d0\n"              // d0: vcounterManual
      //"   cmp.w       %[textRampEffectLimitTop],%%d0\n"      // cmp: vcounterManual - textRampEffectLimitTop
      //"   blo.s       .set_bgColor_1_%=\n"                   // branch if (vcounterManual < textRampEffectLimitTop) (opposite than vcounterManual >= textRampEffectLimitTop)
      //"   cmp.w       %[textRampEffectLimitBottom],%%d0\n"   // cmp: vcounterManual - textRampEffectLimitBottom
      //"   bhi.s       .set_bgColor_1_%=\n"                   // branch if (vcounterManual > textRampEffectLimitBottom) (opposite than vcounterManual <= textRampEffectLimitBottom)
      //"   moveq       #1,%%d3\n"                             // d3: setGradColorForText = 1 (TRUE)
        ".setGradColorForText_flag_%=:\n"
        "   move.w      %[vcounterManual],%%d0\n"              // d0: vcounterManual
            // translate vcounterManual to base textRampEffectLimitTop:
        "   sub.w       %[textRampEffectLimitTop],%%d0\n"      // d0: vcounterManual - textRampEffectLimitTop
            // _TEXT_RAMP_EFFECT_HEIGHT is the amount of scanlines the text ramp effect takes:
        "   cmpi.w      %[_TEXT_RAMP_EFFECT_HEIGHT],%%d0\n"    // d0 -= TEXT_RAMP_EFFECT_HEIGHT
        "   scs.b       %%d3\n"                                // if d0 >= 0 => d3=0xFF (enable bg color), if d0 < 0 => d3=0x00 (no bg color)
        // Next instructions not needed, logic works fine without them. I don't get it.
      //"   bls.s       .set_bgColor_1_%=\n"                   // branch if d0 <= TEXT_RAMP_EFFECT_HEIGHT (at this moment d3 is correctly set)
      //"   moveq       #0,%%d3\n"                             // d3=0x00 (no bg color)

		".set_bgColor_1_%=:\n"
		"   moveq       #0,%%d4\n"              // d4: bgColor = 0
            // from here on, d4=0 as long as d3 == 0, so no need to reset d4 in other color batch blocks
		"   tst.b       %%d3\n"                 // d3: setGradColorForText => if setGradColorForText == 0
		"   beq.s       .dma_address_1_%=\n"
		"   move.w      (%%a0)+,%%d4\n"         // d4: bgColor = *currGradPtr++;
        // Write bgColor as titan256cPalsPtr's first color, otherwise it will overwrite the BG color manually set.
        // This only valid for palIdx == 0 but we do it always to avoid another if statement
        "   move.w      %%d4,(%%a1)\n"          // *titan256cPalsPtr = bgColor;
        ".dma_address_1_%=:\n"
        "   move.l      %%a1,%%d2\n"            // d2: titan256cPalsPtr
        "   lsr.l       #1,%%d2\n"              // d2: fromAddrForDMA = (u32) titan256cPalsPtr >> 1;
            // NOTE: previous lsr.l can be replaced by lsr.w in case we don't need to use d2: fromAddrForDMA >> 16
        "   move.w      #0x9500,%%d0\n"         // d0: 0x9500
        "   or.b        %%d2,%%d0\n"            // d0: 0x9500 | (u8)(fromAddrForDMA)
        "   move.w      %%d2,-(%%sp)\n"
        "   move.w      #0x9600,%%d1\n"         // d1: 0x9600
        "   or.b        (%%sp)+,%%d1\n"         // d1: 0x9600 | (u8)(fromAddrForDMA >> 8)
        "   swap        %%d2\n"                 // d2: fromAddrForDMA >> 16
        "   andi.w      #0x007f,%%d2\n"         // d2: (fromAddrForDMA >> 16) & 0x7f
            // NOTE: previous & 0x7f operation might be discarded if higher bits are somehow already zeroed
        "   ori.w       #0x9700,%%d2\n"         // d2: 0x9700 | (u8)((fromAddrForDMA >> 16) & 0x7f)
        // wait HCounter
        "1:\n"
        "   cmp.b       (%%a4),%%d6\n"          // cmp: d6 - (a4). Compare byte size given that d6 won't be > 160 for our practical cases
        "   bhi.s       1b\n"                   // loop back if d6 is higher than (a4)
        // set BG color before setup DMA
        "   move.l      #0xC0000000,(%%a2)\n"   // *((vu32*) VDP_CTRL_PORT) = 0xC0000000; // VDP_WRITE_CRAM_ADDR(0): write to CRAM color index 0 multiplied by 2
		"   move.w      %%d4,(%%a3)\n"          // *((vu16*) VDP_DATA_PORT) = bgColor;
        // Setup DMA length (in word here)
        //"   move.w      %[_DMA_9300_LEN_DIV_3],(%%a2)\n"  // *((vu16*) VDP_CTRL_PORT) = 0x9300 | ((TITAN_256C_COLORS_PER_STRIP/3) & 0xff);
        //"   move.w      %[_DMA_9400_LEN_DIV_3],(%%a2)\n"  // *((vu16*) VDP_CTRL_PORT) = 0x9400 | (((TITAN_256C_COLORS_PER_STRIP/3) >> 8) & 0xff);
        // Setup DMA length (in long word here)
        "   move.l      %[_DMA_9300_9400_LEN_DIV_3],(%%a2)\n"
        // Setup DMA address
        "   move.w      %%d0,(%%a2)\n"          // *((vu16*) VDP_CTRL_PORT) = 0x9500 | (u8)(fromAddrForDMA);
        "   move.w      %%d1,(%%a2)\n"          // *((vu16*) VDP_CTRL_PORT) = 0x9600 | (u8)(fromAddrForDMA >> 8);
        "   move.w      %%d2,(%%a2)\n"          // *((vu16*) VDP_CTRL_PORT) = 0x9700 | (u8)((fromAddrForDMA >> 16) & 0x7f);
        "   lea         (%c[_TITAN_256C_COLORS_PER_STRIP_DIV_3]*2,%%a1),%%a1\n"  // titan256cPalsPtr += TITAN_256C_COLORS_PER_STRIP/3;

		".set_pal_cmd_for_dma_1_%=:\n"
            // palCmdForDMA = palIdx == 0 ? 0xC0000080 : 0xC0400080;
            // set base command address once and then we'll add the right offset in next sets
		"   move.l      #0xC0000080,%%d5\n"     // d5: palCmdForDMA = 0xC0000080
		"   tst.b       %[palIdx]\n"            // palIdx == 0?
		"   beq.s       1f\n"
		"   move.l      #0xC0400080,%%d5\n"     // d5: palCmdForDMA = 0xC0400080
        // wait HCounter
        "1:\n"
        "   cmp.b       (%%a4),%%d6\n"          // cmp: d6 - (a4). Compare byte size given that d6 won't be > 160 for our practical cases
        "   bhi.s       1b\n"                   // loop back if d6 is higher than (a4)
		// turn off VDP
		"   move.w      %%a5,(%%a2)\n"          // *(vu16*) VDP_CTRL_PORT = 0x8100 | (reg01 & ~0x40);
        // trigger DMA transfer
        "   move.l      %%d5,(%%a2)\n"          // *((vu32*) VDP_CTRL_PORT) = palCmdForDMA;
		// turn on VDP
		"   move.w      %%a6,(%%a2)\n"          // *(vu16*) VDP_CTRL_PORT = 0x8100 | (reg01 | 0x40);

        ".set_bgColor_2_%=:\n"
		"   tst.b       %%d3\n"                 // d3: setGradColorForText => if setGradColorForText == 0
		"   beq.s       .dma_address_2_%=\n"
		"   move.w      (%%a0)+,%%d4\n"         // d4: bgColor = *currGradPtr++;
        ".dma_address_2_%=:\n"
        "   move.l      %%a1,%%d2\n"            // d2: titan256cPalsPtr
        "   lsr.l       #1,%%d2\n"              // d2: fromAddrForDMA = (u32) titan256cPalsPtr >> 1;
            // NOTE: previous lsr.l can be replaced by lsr.w in case we don't need to use d2: fromAddrForDMA >> 16
        "   move.b      %%d2,%%d0\n"            // d0: 0x9500 | (u8)(fromAddrForDMA)
        "   move.w      %%d2,-(%%sp)\n"
        "   move.b      (%%sp)+,%%d1\n"         // d1: 0x9600 | (u8)(fromAddrForDMA >> 8)
        "   swap        %%d2\n"                 // d2: fromAddrForDMA >> 16
        "   andi.w      #0x007f,%%d2\n"         // d2: (fromAddrForDMA >> 16) & 0x7f
            // NOTE: previous & 0x7f operation might be discarded if higher bits are somehow already zeroed
        "   ori.w       #0x9700,%%d2\n"         // d2: 0x9700 | ((fromAddrForDMA >> 16) & 0x7f)
        // wait HCounter
        "1:\n"
        "   cmp.b       (%%a4),%%d6\n"          // cmp: d6 - (a4). Compare byte size given that d6 won't be > 160 for our practical cases
        "   bhi.s       1b\n"                   // loop back if d6 is higher than (a4)
        // set BG color before setup DMA
        "   move.l      #0xC0000000,(%%a2)\n"   // *((vu32*) VDP_CTRL_PORT) = 0xC0000000; // VDP_WRITE_CRAM_ADDR(0): write to CRAM color index 0 multiplied by 2
		"   move.w      %%d4,(%%a3)\n"          // *((vu16*) VDP_DATA_PORT) = bgColor;
        // Setup DMA length (in word here)
        //"   move.w      %[_DMA_9300_LEN_DIV_3],(%%a2)\n"  // *((vu16*) VDP_CTRL_PORT) = 0x9300 | ((TITAN_256C_COLORS_PER_STRIP/3) & 0xff);
        //"   move.w      %[_DMA_9400_LEN_DIV_3],(%%a2)\n"  // *((vu16*) VDP_CTRL_PORT) = 0x9400 | (((TITAN_256C_COLORS_PER_STRIP/3) >> 8) & 0xff);
        // Setup DMA length (in long word here)
        "   move.l      %[_DMA_9300_9400_LEN_DIV_3],(%%a2)\n"
        // Setup DMA address
        "   move.w      %%d0,(%%a2)\n"          // *((vu16*) VDP_CTRL_PORT) = 0x9500 | (u8)(fromAddrForDMA);
        "   move.w      %%d1,(%%a2)\n"          // *((vu16*) VDP_CTRL_PORT) = 0x9600 | (u8)(fromAddrForDMA >> 8);
        "   move.w      %%d2,(%%a2)\n"          // *((vu16*) VDP_CTRL_PORT) = 0x9700 | (u8)((fromAddrForDMA >> 16) & 0x7f);
        "   lea         (%c[_TITAN_256C_COLORS_PER_STRIP_DIV_3]*2,%%a1),%%a1\n"  // titan256cPalsPtr += TITAN_256C_COLORS_PER_STRIP/3;

		".set_pal_cmd_for_dma_2_%=:\n"
            // palCmdForDMA = palIdx == 0 ? 0xC0140080 : 0xC0540080; // advance command for next TITAN_256C_COLORS_PER_STRIP/3 colors
        "   addi.l      #0x00140000,%%d5\n"     // d5: cmdAddress += 0x00140000
        // wait HCounter
        "1:\n"
        "   cmp.b       (%%a4),%%d6\n"          // cmp: d6 - (a4). Compare byte size given that d6 won't be > 160 for our practical cases
        "   bhi.s       1b\n"                   // loop back if d6 is higher than (a4)
		// turn off VDP
		"   move.w      %%a5,(%%a2)\n"          // *(vu16*) VDP_CTRL_PORT = 0x8100 | (reg01 & ~0x40);
        // trigger DMA transfer
        "   move.l      %%d5,(%%a2)\n"          // *((vu32*) VDP_CTRL_PORT) = palCmdForDMA;
		// turn on VDP
		"   move.w      %%a6,(%%a2)\n"          // *(vu16*) VDP_CTRL_PORT = 0x8100 | (reg01 | 0x40);

        ".set_bgColor_3_%=:\n"
		"   tst.b       %%d3\n"                 // d3: setGradColorForText => if setGradColorForText == 0
		"   beq.s       .dma_address_3_%=\n"
		"   move.w      (%%a0)+,%%d4\n"         // d4: bgColor = *currGradPtr++;
        ".dma_address_3_%=:\n"
        "   move.l      %%a1,%%d2\n"            // d2: titan256cPalsPtr
        "   lsr.l       #1,%%d2\n"              // d2: fromAddrForDMA = (u32) titan256cPalsPtr >> 1;
            // NOTE: previous lsr.l can be replaced by lsr.w in case we don't need to use d2: fromAddrForDMA >> 16
        "   move.b      %%d2,%%d0\n"            // d0: 0x9500 | (u8)(fromAddrForDMA)
        "   move.w      %%d2,-(%%sp)\n"
        "   move.b      (%%sp)+,%%d1\n"         // d1: 0x9600 | (u8)(fromAddrForDMA >> 8)
        "   swap        %%d2\n"                 // d2: fromAddrForDMA >> 16
        "   andi.w      #0x007f,%%d2\n"         // d2: (fromAddrForDMA >> 16) & 0x7f
            // NOTE: previous & 0x7f operation might be discarded if higher bits are somehow already zeroed
        "   ori.w       #0x9700,%%d2\n"         // d2: 0x9700 | ((fromAddrForDMA >> 16) & 0x7f)
        // wait HCounter
        "1:\n"
        "   cmp.b       (%%a4),%%d6\n"          // cmp: d6 - (a4). Compare byte size given that d6 won't be > 160 for our practical cases
        "   bhi.s       1b\n"                   // loop back if d6 is higher than (a4)
        // set BG color before setup DMA
        "   move.l      #0xC0000000,(%%a2)\n"   // *((vu32*) VDP_CTRL_PORT) = 0xC0000000; // VDP_WRITE_CRAM_ADDR(0): write to CRAM color index 0 multiplied by 2
		"   move.w      %%d4,(%%a3)\n"          // *((vu16*) VDP_DATA_PORT) = bgColor;
        // Setup DMA length (in word here)
        //"   move.w      %[_DMA_9300_LEN_DIV_3_REM],(%%a2)\n"  // *((vu16*) VDP_CTRL_PORT) = 0x9300 | ((TITAN_256C_COLORS_PER_STRIP/3 + REMAINDER) & 0xff);
        //"   move.w      %[_DMA_9400_LEN_DIV_3_REM],(%%a2)\n"  // *((vu16*) VDP_CTRL_PORT) = 0x9400 | (((TITAN_256C_COLORS_PER_STRIP/3 + TITAN_256C_COLORS_PER_STRIP_REMAINDER(3)) >> 8) & 0xff);
        // Setup DMA length (in long word here)
        "   move.l      %[_DMA_9300_9400_LEN_DIV_3_REM],(%%a2)\n"
        // Setup DMA address
        "   move.w      %%d0,(%%a2)\n"          // *((vu16*) VDP_CTRL_PORT) = 0x9500 | (u8)(fromAddrForDMA);
        "   move.w      %%d1,(%%a2)\n"          // *((vu16*) VDP_CTRL_PORT) = 0x9600 | (u8)(fromAddrForDMA >> 8);
        "   move.w      %%d2,(%%a2)\n"          // *((vu16*) VDP_CTRL_PORT) = 0x9700 | (u8)((fromAddrForDMA >> 16) & 0x7f);
        "   lea         (%c[_TITAN_256C_COLORS_PER_STRIP_DIV_3_REM]*2,%%a1),%%a1\n"  // titan256cPalsPtr += TITAN_256C_COLORS_PER_STRIP/3 + REMAINDER;

		".set_pal_cmd_for_dma_3_%=:\n"
            // palCmdForDMA = palIdx == 0 ? 0xC0280080 : 0xC0680080; // advance command for next TITAN_256C_COLORS_PER_STRIP/3 colors
        "   addi.l      #0x00140000,%%d5\n"     // d5: cmdAddress += 0x00140000
        // wait HCounter
        "1:\n"
        "   cmp.b       (%%a4),%%d6\n"          // cmp: d6 - (a4). Compare byte size given that d6 won't be > 160 for our practical cases
        "   bhi.s       1b\n"                   // loop back if d6 is higher than (a4)
		// turn off VDP
		"   move.w      %%a5,(%%a2)\n"          // *(vu16*) VDP_CTRL_PORT = 0x8100 | (reg01 & ~0x40);
        // trigger DMA transfer
        "   move.l      %%d5,(%%a2)\n"          // *((vu32*) VDP_CTRL_PORT) = palCmdForDMA;
		// turn on VDP
		"   move.w      %%a6,(%%a2)\n"          // *(vu16*) VDP_CTRL_PORT = 0x8100 | (reg01 | 0x40);

        // restore a6 before is too late
        "   move.l      %%usp,%%a6\n"

        ".set_bgColor_4_%=:\n"
		"   tst.b       %%d3\n"                 // d3: setGradColorForText => if setGradColorForText = 0
		"   beq.s       .accomodate_vars_%=\n"
		"   move.w      (%%a0)+,%%d4\n"         // d4: bgColor = *currGradPtr++;
        "   move.l      %%a0,%[currGradPtr]\n"  // store current value of currGradPtr

		".accomodate_vars_%=:\n"
        "   move.w      %[vcounterManual],%%d0\n"                // d0: vcounterManual
        "   addq.w      %[_TITAN_256C_STRIP_HEIGHT],%%d0\n"      // d0: vcounterManual += TITAN_256C_STRIP_HEIGHT
        "   move.w      %%d0,%[vcounterManual]\n"                // store current value of vcounterManual
        "   move.b      %[palIdx],%%d1\n"                        // d1: palIdx
        "   eori.b      %[_TITAN_256C_COLORS_PER_STRIP],%%d1\n"  // d1: palIdx ^= TITAN_256C_COLORS_PER_STRIP // cycles between 0 and 32
        "   move.b      %%d1,%[palIdx]\n"                        // store current value of palIdx
        "   cmp.w       %[applyBlackPalPosY],%%d0\n"             // cmp: vcounterManual - applyBlackPalPosY
        "   blo.s       2f\n"                                    // branch if (vcounterManual < applyBlackPalPosY)
        "   move.l      %[palette_black],%%a1\n"                 // a1 = palette_black
        "2:\n"
        "   move.l      %%a1,%[titan256cPalsPtr]\n"              // store current value of titan256cPalsPtr

        // wait HCounter
        "1:\n"
        "   cmp.b       (%%a4),%%d6\n"          // cmp: d6 - (a4). Compare byte size given that d6 won't be > 160 for our practical cases
        "   bhi.s       1b\n"                   // loop back if d6 is higher than (a4)
        // set last BG color
        "   move.l      #0xC0000000,(%%a2)\n"   // *((vu32*) VDP_CTRL_PORT) = 0xC0000000; // VDP_WRITE_CRAM_ADDR(0): write to CRAM color index 0 multiplied by 2
		"   move.w      %%d4,(%%a3)\n"          // *((vu16*) VDP_DATA_PORT) = bgColor;
        : 
        [currGradPtr] "+m" (currGradPtr),
		[titan256cPalsPtr] "+m" (titan256cPalsPtr),
		[vcounterManual] "+m" (vcounterManual),
		[palIdx] "+m" (palIdx)
		: 
		[textRampEffectLimitTop] "m" (textRampEffectLimitTop),
		[textRampEffectLimitBottom] "m" (textRampEffectLimitBottom),
		[applyBlackPalPosY] "m" (applyBlackPalPosY),
		[palette_black] "m" (palette_black),
		[turnOff] "i" (0x8100 | (0x74 & ~0x40)), // 0x8134
		[turnOn] "i" (0x8100 | (0x74 | 0x40)), // 0x8174
        [hcLimit] "i" (154),
		[_TITAN_256C_COLORS_PER_STRIP] "i" (TITAN_256C_COLORS_PER_STRIP),
        [_TITAN_256C_COLORS_PER_STRIP_DIV_3] "i" (TITAN_256C_COLORS_PER_STRIP/3),
        [_TITAN_256C_COLORS_PER_STRIP_DIV_3_REM] "i" (TITAN_256C_COLORS_PER_STRIP/3 + TITAN_256C_COLORS_PER_STRIP_REMAINDER(3)),
        [_DMA_9300_LEN_DIV_3] "i" (0x9300 | ((TITAN_256C_COLORS_PER_STRIP/3) & 0xff)),
        [_DMA_9400_LEN_DIV_3] "i" (0x9400 | (((TITAN_256C_COLORS_PER_STRIP/3) >> 8) & 0xff)),
        [_DMA_9300_9400_LEN_DIV_3] "i" ( ((0x9300 | ((TITAN_256C_COLORS_PER_STRIP/3) & 0xff)) << 16) | (0x9400 | (((TITAN_256C_COLORS_PER_STRIP/3) >> 8) & 0xff)) ),
        [_DMA_9300_LEN_DIV_3_REM] "i" (0x9300 | ((TITAN_256C_COLORS_PER_STRIP/3 + TITAN_256C_COLORS_PER_STRIP_REMAINDER(3)) & 0xff)),
        [_DMA_9400_LEN_DIV_3_REM] "i" (0x9400 | (((TITAN_256C_COLORS_PER_STRIP/3 + TITAN_256C_COLORS_PER_STRIP_REMAINDER(3)) >> 8) & 0xff)),
        [_DMA_9300_9400_LEN_DIV_3_REM] "i" ( ((0x9300 | ((TITAN_256C_COLORS_PER_STRIP/3 + TITAN_256C_COLORS_PER_STRIP_REMAINDER(3)) & 0xff)) << 16) | (0x9400 | (((TITAN_256C_COLORS_PER_STRIP/3 + TITAN_256C_COLORS_PER_STRIP_REMAINDER(3)) >> 8) & 0xff)) ),
		[_TITAN_256C_STRIP_HEIGHT] "i" (TITAN_256C_STRIP_HEIGHT),
        [_TEXT_RAMP_EFFECT_HEIGHT] "i" ((TITAN_256C_TEXT_ENDING_STRIP - TITAN_256C_TEXT_STARTING_STRIP) * TITAN_256C_STRIP_HEIGHT + TITAN_256C_TEXT_OFFSET_BOTTOM)
		:
        // backup registers used in the asm implementation including the scratch pad since this code is used in an interrupt call.
        // a6 is backed up at the beginning of the asm block to avoid overriding the Stack Frame value. If not, then glitches happen.
		"d0","d1","d2","d3","d4","d5","d6","a0","a1","a2","a3","a4","a5","cc","memory"
    );
}

HINTERRUPT_CALLBACK horizIntOnTitan256cCallback_DMA_EveryN () {
    // 1510~1524 cycles

    // test if current HCounter is in the range for text gradient effect
    bool setGradColorForText = vcounterManual >= textRampEffectLimitTop && vcounterManual <= textRampEffectLimitBottom;

    /*
        With 3 DMA commands:
        Every command is CRAM address to start DMA MOVIE_DATA_COLORS_PER_STRIP/3 colors
        u32 palCmdForDMA_A = VDP_DMA_CRAM_ADDR(((u32)palIdx + 0) * 2);
        u32 palCmdForDMA_B = VDP_DMA_CRAM_ADDR(((u32)palIdx + TITAN_256C_COLORS_PER_STRIP/3) * 2);
        u32 palCmdForDMA_C = VDP_DMA_CRAM_ADDR(((u32)palIdx + 2*(TITAN_256C_COLORS_PER_STRIP/3)) * 2);
        cmd     palIdx = 0      palIdx = 32
        A       0xC0000080      0xC0400080
        B       0xC0140080      0xC0540080
        C       0xC0280080      0xC0680080
    */

    u32 palCmdForDMA;
    u32 fromAddrForDMA;
    u16 fromAddrForDMA_hi;
    u16 bgColor = 0;
    const u8 hcLimit = 154;

    // Value under current conditions is always 0x74
    //u8 reg01 = VDP_getReg(0x01); // Holds current VDP register 1 value (NOTE: it holds other bits than VDP ON/OFF status)
    // NOTE: here is OK to call VDP_getReg(0x01) only if we didn't previously change the VDP's reg 1 using direct access without VDP_setReg()

    fromAddrForDMA = (u32) titan256cPalsPtr >> 1; // here we manipulate the memory address not its content
    fromAddrForDMA_hi = 0x9700 | ((fromAddrForDMA >> 16) & 0x7f);
    if (setGradColorForText) {
        bgColor = *currGradPtr++;
        // Write bgColor as titan256cPalsPtr's first color, otherwise it will overwrite the BG color manually set.
        // This only valid for palIdx == 0 but we do it always to avoid another if statement
        *titan256cPalsPtr = bgColor;
    }

    MEMORY_BARRIER();
    waitHCounter_opt2(hcLimit);
    // set BG color before setup DMA
    *((vu32*) VDP_CTRL_PORT) = 0xC0000000; // VDP_WRITE_CRAM_ADDR(0): write to CRAM color index 0 multiplied by 2
    *((vu16*) VDP_DATA_PORT) = bgColor;
    // Setup DMA length (in long word here): low at higher word, high at lower word
    *((vu32*) VDP_CTRL_PORT) = ((0x9300 | (u8)(TITAN_256C_COLORS_PER_STRIP/3)) << 16) |
            (0x9400 | (u8)((TITAN_256C_COLORS_PER_STRIP/3) >> 8));
    // Setup DMA address
    *((vu16*) VDP_CTRL_PORT) = 0x9500 | (u8)(fromAddrForDMA);
    *((vu16*) VDP_CTRL_PORT) = 0x9600 | (u8)(fromAddrForDMA >> 8);
    *((vu16*) VDP_CTRL_PORT) = fromAddrForDMA_hi; // needed to avoid glitches with black palette

    titan256cPalsPtr += TITAN_256C_COLORS_PER_STRIP/3;
    palCmdForDMA = palIdx == 0 ? 0xC0000080 : 0xC0400080;

    MEMORY_BARRIER();
    waitHCounter_opt2(hcLimit);
    turnOffVDP(0x74);
    *((vu32*) VDP_CTRL_PORT) = palCmdForDMA; // Trigger DMA transfer
    turnOnVDP(0x74);

    fromAddrForDMA = (u32) titan256cPalsPtr >> 1; // here we manipulate the memory address not its content
    fromAddrForDMA_hi = 0x9700 | ((fromAddrForDMA >> 16) & 0x7f);
    if (setGradColorForText) bgColor = *currGradPtr++;

    MEMORY_BARRIER();
    waitHCounter_opt2(hcLimit);
    // set BG color before setup DMA
    *((vu32*) VDP_CTRL_PORT) = 0xC0000000; // VDP_WRITE_CRAM_ADDR(0): write to CRAM color index 0 multiplied by 2
    *((vu16*) VDP_DATA_PORT) = bgColor;
    // Setup DMA length (in long word here): low at higher word, high at lower word
    *((vu32*) VDP_CTRL_PORT) = ((0x9300 | (u8)(TITAN_256C_COLORS_PER_STRIP/3)) << 16) |
            (0x9400 | (u8)((TITAN_256C_COLORS_PER_STRIP/3) >> 8));
    // Setup DMA address
    *((vu16*) VDP_CTRL_PORT) = 0x9500 | (u8)(fromAddrForDMA);
    *((vu16*) VDP_CTRL_PORT) = 0x9600 | (u8)(fromAddrForDMA >> 8);
    *((vu16*) VDP_CTRL_PORT) = fromAddrForDMA_hi; // needed to avoid glitches with black palette

    titan256cPalsPtr += TITAN_256C_COLORS_PER_STRIP/3;
    palCmdForDMA = palIdx == 0 ? 0xC0140080 : 0xC0540080; // advance command for next TITAN_256C_COLORS_PER_STRIP/3 colors
MEMORY_BARRIER();
    waitHCounter_opt2(hcLimit);
    turnOffVDP(0x74);
    *((vu32*) VDP_CTRL_PORT) = palCmdForDMA; // Trigger DMA transfer
    turnOnVDP(0x74);

    fromAddrForDMA = (u32) titan256cPalsPtr >> 1; // here we manipulate the memory address not its content
    fromAddrForDMA_hi = 0x9700 | ((fromAddrForDMA >> 16) & 0x7f);
    if (setGradColorForText) bgColor = *currGradPtr++;

MEMORY_BARRIER();
    waitHCounter_opt2(hcLimit);
    // set BG color before setup DMA
    *((vu32*) VDP_CTRL_PORT) = 0xC0000000; // VDP_WRITE_CRAM_ADDR(0): write to CRAM color index 0 multiplied by 2
    *((vu16*) VDP_DATA_PORT) = bgColor;
    // Setup DMA length (in long word here): low at higher word, high at lower word
    *((vu32*) VDP_CTRL_PORT) = ((0x9300 | (u8)(TITAN_256C_COLORS_PER_STRIP/3 + TITAN_256C_COLORS_PER_STRIP_REMAINDER(3))) << 16) |
            (0x9400 | (u8)((TITAN_256C_COLORS_PER_STRIP/3 + TITAN_256C_COLORS_PER_STRIP_REMAINDER(3)) >> 8));
    // Setup DMA address
    *((vu16*) VDP_CTRL_PORT) = 0x9500 | (u8)(fromAddrForDMA);
    *((vu16*) VDP_CTRL_PORT) = 0x9600 | (u8)(fromAddrForDMA >> 8);
    *((vu16*) VDP_CTRL_PORT) = fromAddrForDMA_hi; // needed to avoid glitches with black palette

    titan256cPalsPtr += TITAN_256C_COLORS_PER_STRIP/3 + TITAN_256C_COLORS_PER_STRIP_REMAINDER(3);
    palCmdForDMA = palIdx == 0 ? 0xC0280080 : 0xC0680080; // advance command for next TITAN_256C_COLORS_PER_STRIP/3 colors

    // Prepare vars for next HInt here so we can aliviate the waitHCounter loop and exit the HInt sooner
    vcounterManual += TITAN_256C_STRIP_HEIGHT;
    //titan256cPalsPtr += TITAN_256C_COLORS_PER_STRIP; // advance to next strip's palettes (if pointer wasn't incremented previously)
    palIdx ^= TITAN_256C_COLORS_PER_STRIP; // cycles between 0 and 32
    if (vcounterManual >= applyBlackPalPosY)
        titan256cPalsPtr = (u16*) palette_black;

MEMORY_BARRIER();
    waitHCounter_opt2(hcLimit);
    turnOffVDP(0x74);
    *((vu32*) VDP_CTRL_PORT) = palCmdForDMA; // Trigger DMA transfer
    turnOnVDP(0x74);

    if (setGradColorForText) bgColor = *currGradPtr++;
MEMORY_BARRIER();
    waitHCounter_opt2(hcLimit);
    // set last BG color
    *((vu32*) VDP_CTRL_PORT) = 0xC0000000; // VDP_WRITE_CRAM_ADDR(0): write to CRAM color index 0 multiplied by 2
    *((vu16*) VDP_DATA_PORT) = bgColor;
}

HINTERRUPT_CALLBACK horizIntOnTitan256cCallback_DMA_OneTime () {
    // Giving the nature of the VDP, the hint callback it will be invoked one more time until the VDP_setHIntCounter(0xFF) makes effect.
    // So we need to cancel this invocation and only proceed normally with the next one.
    if (vcounterManual == 0) {
        vcounterManual = TITAN_256C_STRIP_HEIGHT - 1;

        // Disable the hint callback by setting HintCounter to max number 255. This takes effect next invocation.
        // Instead of VDP_setHIntCounter(0xFF) due to additionals read and write from/to internal regValues[]
        *((u16*) VDP_CTRL_PORT) = 0x8A00 | 0xFF;

        return;
    }

    // Value under current conditions is always 0x74
    //u8 reg01 = VDP_getReg(0x01); // Holds current VDP register 1 value (NOTE: it holds other bits than VDP ON/OFF status)
    // NOTE: here is OK to call VDP_getReg(0x01) only if we didn't previously change the VDP's reg 1 using direct access without VDP_setReg()

    // Simulates waiting the first call to VDP_setHIntCounter(TITAN_256C_STRIP_HEIGHT - 1)
    waitVCounterReg(TITAN_256C_STRIP_HEIGHT - 1 - 1);
    // at this point GET_VCOUNTER value is TITAN_256C_STRIP_HEIGHT - 1

    for (;;) {
        // SCANLINE 1st starts right here

        // exits when we reach last image strip which it doesn't need any palette to be loaded
        if (vcounterManual > (TITAN_256C_HEIGHT - TITAN_256C_STRIP_HEIGHT - 1)) { // valid for NTSC and PAL since titan image size is fixed
            return;
        }

    	// test if current HCounter is in the range for text gradient effect
        bool setGradColorForText = vcounterManual >= textRampEffectLimitTop && vcounterManual <= textRampEffectLimitBottom;

        /*
            With 3 DMA commands:
            Every command is CRAM address to start DMA MOVIE_DATA_COLORS_PER_STRIP/3 colors
            u32 palCmdForDMA_A = VDP_DMA_CRAM_ADDR(((u32)palIdx + 0) * 2);
            u32 palCmdForDMA_B = VDP_DMA_CRAM_ADDR(((u32)palIdx + TITAN_256C_COLORS_PER_STRIP/3) * 2);
            u32 palCmdForDMA_C = VDP_DMA_CRAM_ADDR(((u32)palIdx + 2*(TITAN_256C_COLORS_PER_STRIP/3)) * 2);
            cmd     palIdx = 0      palIdx = 32
            A       0xC0000080      0xC0400080
            B       0xC0140080      0xC0540080
            C       0xC0280080      0xC0680080
        */

        u32 palCmdForDMA;
        u32 fromAddrForDMA;
        u16 fromAddrForDMA_hi;
        u16 bgColor=0;
        const u8 hcLimit = 156;

        fromAddrForDMA = (u32) titan256cPalsPtr >> 1; // here we manipulate the memory address not its content
        fromAddrForDMA_hi = 0x9700 | ((fromAddrForDMA >> 16) & 0x7f);
        if (setGradColorForText) bgColor = *currGradPtr++;

MEMORY_BARRIER();
        waitHCounter_opt2(hcLimit);

        // SCANLINE 2nd starts here (few pixels ahead)

        // set BG color before setup DMA
        *((vu32*) VDP_CTRL_PORT) = 0xC0000000; // VDP_WRITE_CRAM_ADDR(0): write to CRAM color index 0 multiplied by 2
        *((vu16*) VDP_DATA_PORT) = bgColor;
        // Setup DMA length (in long word here): low at higher word, high at lower word
        *((vu32*) VDP_CTRL_PORT) = ((0x9300 | (u8)(TITAN_256C_COLORS_PER_STRIP/3)) << 16) |
            (0x9400 | (u8)((TITAN_256C_COLORS_PER_STRIP/3) >> 8));
        // Setup DMA address
        *((vu16*) VDP_CTRL_PORT) = 0x9500 | (u8)(fromAddrForDMA);
        *((vu16*) VDP_CTRL_PORT) = 0x9600 | (u8)(fromAddrForDMA >> 8);
        *((vu16*) VDP_CTRL_PORT) = fromAddrForDMA_hi; // needed to avoid glitches with black palette

        titan256cPalsPtr += TITAN_256C_COLORS_PER_STRIP/3;
        palCmdForDMA = palIdx == 0 ? 0xC0000080 : 0xC0400080;
        waitHCounter_opt2(hcLimit);

        // SCANLINE 3rd starts here (few pixels ahead)

        turnOffVDP(0x74);
        *((vu32*) VDP_CTRL_PORT) = palCmdForDMA; // trigger DMA transfer
        turnOnVDP(0x74);

        fromAddrForDMA = (u32) titan256cPalsPtr >> 1; // here we manipulate the memory address not its content
        fromAddrForDMA_hi = 0x9700 | ((fromAddrForDMA >> 16) & 0x7f);
        if (setGradColorForText) bgColor = *currGradPtr++;

MEMORY_BARRIER();
        waitHCounter_opt2(hcLimit);

        // SCANLINE 4th starts here (few pixels ahead)

        // set BG color before setup DMA
        *((vu32*) VDP_CTRL_PORT) = 0xC0000000; // VDP_WRITE_CRAM_ADDR(0): write to CRAM color index 0 multiplied by 2
        *((vu16*) VDP_DATA_PORT) = bgColor;
        // Setup DMA length (in long word here): low at higher word, high at lower word
        *((vu32*) VDP_CTRL_PORT) = ((0x9300 | (u8)(TITAN_256C_COLORS_PER_STRIP/3)) << 16) |
            (0x9400 | (u8)((TITAN_256C_COLORS_PER_STRIP/3) >> 8));
        // Setup DMA address
        *((vu16*) VDP_CTRL_PORT) = 0x9500 | (u8)(fromAddrForDMA);
        *((vu16*) VDP_CTRL_PORT) = 0x9600 | (u8)(fromAddrForDMA >> 8);
        *((vu16*) VDP_CTRL_PORT) = fromAddrForDMA_hi; // needed to avoid glitches with black palette

        titan256cPalsPtr += TITAN_256C_COLORS_PER_STRIP/3;
        palCmdForDMA = palIdx == 0 ? 0xC0140080 : 0xC0540080; // advance command for next TITAN_256C_COLORS_PER_STRIP/3 colors
        waitHCounter_opt2(hcLimit);

        // SCANLINE 5th starts here (few pixels ahead)

        turnOffVDP(0x74);
        *((vu32*) VDP_CTRL_PORT) = palCmdForDMA; // trigger DMA transfer
        turnOnVDP(0x74);

        fromAddrForDMA = (u32) titan256cPalsPtr >> 1; // here we manipulate the memory address not its content
        fromAddrForDMA_hi = 0x9700 | ((fromAddrForDMA >> 16) & 0x7f);
        if (setGradColorForText) bgColor = *currGradPtr++;

MEMORY_BARRIER();
        waitHCounter_opt2(hcLimit);

        // SCANLINE 6th starts here (few pixels ahead)

        // set BG color before setup DMA
        *((vu32*) VDP_CTRL_PORT) = 0xC0000000; // VDP_WRITE_CRAM_ADDR(0): write to CRAM color index 0 multiplied by 2
        *((vu16*) VDP_DATA_PORT) = bgColor;
        // Setup DMA length (in long word here): low at higher word, high at lower word
        *((vu32*) VDP_CTRL_PORT) = ((0x9300 | (u8)(TITAN_256C_COLORS_PER_STRIP/3 + TITAN_256C_COLORS_PER_STRIP_REMAINDER(3))) << 16) |
            (0x9400 | (u8)((TITAN_256C_COLORS_PER_STRIP/3 + TITAN_256C_COLORS_PER_STRIP_REMAINDER(3)) >> 8));
        // Setup DMA address
        *((vu16*) VDP_CTRL_PORT) = 0x9500 | (u8)(fromAddrForDMA);
        *((vu16*) VDP_CTRL_PORT) = 0x9600 | (u8)(fromAddrForDMA >> 8);
        *((vu16*) VDP_CTRL_PORT) = fromAddrForDMA_hi; // needed to avoid glitches with black palette

        titan256cPalsPtr += TITAN_256C_COLORS_PER_STRIP/3 + TITAN_256C_COLORS_PER_STRIP_REMAINDER(3);
        palCmdForDMA = palIdx == 0 ? 0xC0280080 : 0xC0680080; // advance command for next TITAN_256C_COLORS_PER_STRIP/3 colors
        waitHCounter_opt2(hcLimit);

        // SCANLINE 7th starts here (few pixels ahead)

        turnOffVDP(0x74);
        *((vu32*) VDP_CTRL_PORT) = palCmdForDMA; // trigger DMA transfer
        turnOnVDP(0x74);

        if (setGradColorForText) bgColor = *currGradPtr++;
        //titan256cPalsPtr += TITAN_256C_COLORS_PER_STRIP; // advance to next strip's palettes (if pointer wasn't incremented previously)
        palIdx ^= TITAN_256C_COLORS_PER_STRIP; // cycles between 0 and 32

MEMORY_BARRIER();
        waitHCounter_opt2(hcLimit);

        // SCANLINE 8th starts here (few pixels ahead)

        // set last BG color
        *((vu32*) VDP_CTRL_PORT) = 0xC0000000; // VDP_WRITE_CRAM_ADDR(0): write to CRAM color index 0 multiplied by 2
        *((vu16*) VDP_DATA_PORT) = bgColor;

        if (vcounterManual >= applyBlackPalPosY)
            titan256cPalsPtr = (u16*) palette_black;

        vcounterManual += TITAN_256C_STRIP_HEIGHT;
        waitVCounterReg(vcounterManual);
    }
}

static u16 textColor = 0xE00; // initial color is the same than titanCharsGradientColors[0]
static s8 textColorIndex = 0;
static s8 textColorDirection = 1;
static u16 vintCycle = 0;

void vertIntOnDrawTextCallback () {
    // resets text color to what custom font expects
    PAL_setColor(CUSTOM_FONT_COLOR_INDEX, custom_font_round_pal.data[1]);
    PAL_setColor(15, custom_font_round_pal.data[1]); // SGDK's font color index (15)

    if (vintCycle == 4) {
        vintCycle = 0;
        // get next text color value for the HInt
        textColor = *(getTitanCharsGradientColors() + textColorIndex);
        // calculate next color index going back and forth
        textColorIndex += textColorDirection;
        if (textColorIndex >= TITAN_TEXT_GRADIENT_MAX_COLORS) {
            textColorIndex -= 2;
            textColorDirection = -1;
        }
        else if (textColorIndex <= 0) {
            textColorDirection = 1;
        }
    }

    ++vintCycle;
}

HINTERRUPT_CALLBACK horizIntOnDrawTextCallback () {
    waitHCounter_old(150);
    turnOffVDP(0x74);
    *((vu32*) VDP_CTRL_PORT) = VDP_WRITE_CRAM_ADDR((u32)(CUSTOM_FONT_COLOR_INDEX * 2));
    *((vu16*) VDP_DATA_PORT) = textColor;
    *((vu32*) VDP_CTRL_PORT) = VDP_WRITE_CRAM_ADDR((u32)(15 * 2));
    *((vu16*) VDP_DATA_PORT) = textColor;
    turnOnVDP(0x74);
}