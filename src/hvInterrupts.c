#include <types.h>
#include <vdp.h>
#include <pal.h>
#include <sys.h>
#include <timer.h>
#include <tools.h> // KLog methods
#include "hvInterrupts.h"
#include "titan256c.h"
#include "font_consts.h"

// u8 reg01; // Holds current VDP register 1 whole value (it holds other bits than VDP ON/OFF status)

// FORCE_INLINE void copyReg01 () {
//     reg01 = VDP_getReg(0x01);
// }

/// @brief Set bit 6 (64 decimal, 0x40 hexa) of reg 1.
/// @param reg01 VDP's Reg 1 holds other bits than just VDP ON/OFF status, so we need its current value.
FORCE_INLINE void turnOffVDP (u8 reg01) {
    //reg01 &= ~0x40;
    //*(vu16*) VDP_CTRL_PORT = 0x8100 | reg01;
    *(vu16*) VDP_CTRL_PORT = 0x8100 | (reg01 & ~0x40);
}

/// @brief Set bit 6 (64 decimal, 0x40 hexa) of reg 1.
/// @param reg01 VDP's Reg 1 holds other bits than just VDP ON/OFF status, so we need its current value.
FORCE_INLINE void turnOnVDP (u8 reg01) {
    //reg01 |= 0x40;
    //*(vu16*) VDP_CTRL_PORT = 0x8100 | reg01;
    *(vu16*) VDP_CTRL_PORT = 0x8100 | (reg01 | 0x40);
}

/**
 * Wait until HCounter 0xC00009 reaches nth position (actually the (n*2)th pixel since the VDP counts by 2)
*/
FORCE_INLINE void waitHCounter_old (u16 n) {
    // VDP_HVCOUNTER_PORT + 1 = 0xC00009 (HCOUNTER)
    ASM_STATEMENT __volatile__ (
        ".loopHC%=:\n"
        "    cmpi.b    %[hcLimit], 0xC00009\n"    // cmp: (0xC00009) - hcLimit
        "    blo       .loopHC%=\n"                 // Compares byte because hcLimit won't be > 160 for our practical cases
            // blo is for unsigned comparisons, same than bcs
        :
        : [hcLimit] "i" (n)
        :
    );
}

/**
 * Wait until HCounter 0xC00009 reaches nth position (actually the (n*2)th pixel since the VDP counts by 2).
*/
FORCE_INLINE void waitHCounter (u16 n) {
    u32* regA=0; // placeholder used to indicate the use of an An register
    ASM_STATEMENT __volatile__ (
        "    move.l    #0xC00009, %0\n"    // Load HCounter (VDP_HVCOUNTER_PORT + 1 = 0xC00009) into any An register
        ".loopHC%=:\n" 
        "    cmp.b     (%0), %1\n"         // cmp: n - (0xC00009). Compares byte because hcLimit won't be > 160 for our practical cases
        "    bhi       .loopHC%=\n"        // loop back if n is higher than (0xC00009)
            // bhi is for unsigned comparisons
        : "+a" (regA)
        : "d" (n)
        :
    );
}

/**
 * Wait until VCounter 0xC00008 reaches nth scanline position. Only valid with constants.
*/
FORCE_INLINE void waitVCounterConst (u16 n) {
    u32* regA=0; // placeholder used to indicate the use of an An register
    ASM_STATEMENT __volatile__ (
        "    move.l    #0xC00008, %0\n"     // Load V Counter address into any An register
        ".LoopVC%=:\n"
        "    cmpi.w     %[vcLimit],(%0)\n"  // cmp: (0xC00008) - vcLimit
        "    blo       .LoopVC%=\n"         // if (0xC00008) < vcLimit then loop back
            // blo is for unsigned comparisons, same than bcs
        : "+a" (regA)
        : [vcLimit] "i" (n << 8) // (n << 8) | 0xFF
        :
    );
}

/**
 * Wait until VCounter 0xC00008 reaches nth scanline position. Parameter n is loaded into a register.
*/
FORCE_INLINE void waitVCounterReg (u16 n) {
    u32* regA=0; // placeholder used to indicate the use of an An register
    ASM_STATEMENT __volatile__ (
        "    move.l    #0xC00008, %0\n"    // Load V Counter address into any An register
        ".LoopVC%=:\n"
        "    cmp.w     (%0), %1\n"         // cmp: n - (0xC00008)
        "    bhi       .LoopVC%=\n"        // loop back if n is higher than (0xC00008)
            // bhi is for unsigned comparisons
        : "+a" (regA)
        : "d" (n << 8) // (n << 8) | 0xFF
        :
    );
}

/**
 * \brief Writes into VDP_CTRL_PORT (0xC00004) the setup for DMA (length and source address). 
 * Optimizations may apply manually if you know the source address is only 8 bits or 12 bits, and same for the length parameter.
 * \param len How many colors to move.
 * \param fromAddr Must be >> 1.
*/
void NO_INLINE setupDMAForPals (u16 len, u32 fromAddr) {
    // Uncomment if you previously change it to 1 (CPU access to VRAM is 1 byte lenght, and 2 bytes length for CRAM and VSRAM)
    //VDP_setAutoInc(2);

    vu16* palDmaPtr = (vu16*) VDP_CTRL_PORT;

    // Setup DMA length (in word here): low and high
    *palDmaPtr = 0x9300 | (len & 0xff);
    *palDmaPtr = 0x9400 | ((len >> 8) & 0xff); // This step is useless if the length has only set first 8 bits

    // Setup DMA address
    *palDmaPtr = 0x9500 | (fromAddr & 0xff);
    *palDmaPtr = 0x9600 | ((fromAddr >> 8) & 0xff); // This step is useless if the address has only set first 8 bits
    *palDmaPtr = 0x9700 | ((fromAddr >> 16) & 0x7f); // This step is useless if the address has only set first 12 bits
}

void setHVCallbacks (u16 titan256cHIntMode) {
    switch (titan256cHIntMode) {
        // Call the HInt every N scanlines. Uses CPU for palette swapping
        case HINT_STRATEGY_0:
            SYS_setVBlankCallback(vertIntOnTitan256cCallback_HIntEveryN);
            VDP_setHIntCounter(TITAN_256C_STRIP_HEIGHT - 1);
            SYS_setHIntCallback(horizIntOnTitan256cCallback_CPU_EveryN_asm);
            break;
        // Call the HInt every N scanlines. Uses CPU for palette swapping
        case HINT_STRATEGY_1:
            SYS_setVBlankCallback(vertIntOnTitan256cCallback_HIntEveryN);
            VDP_setHIntCounter(TITAN_256C_STRIP_HEIGHT - 1);
            SYS_setHIntCallback(horizIntOnTitan256cCallback_CPU_EveryN);
            break;
        // Call the HInt every N scanlines. Uses DMA for palette swapping
        case HINT_STRATEGY_2:
            SYS_setVBlankCallback(vertIntOnTitan256cCallback_HIntEveryN);
            VDP_setHIntCounter(TITAN_256C_STRIP_HEIGHT - 1);
            SYS_setHIntCallback(horizIntOnTitan256cCallback_DMA_EveryN);
            break;
        // Use only one call to HInt to avoid method call overhead. Uses DMA for palette swapping
        case HINT_STRATEGY_3:
            SYS_setVBlankCallback(vertIntOnTitan256cCallback_HIntOneTime);
            VDP_setHIntCounter(0);
            SYS_setHIntCallback(horizIntOnTitan256cCallback_DMA_OneTime);
            break;
        default: break;
    }
}

static u16 titan256cHIntMode;
static u16 startingScanlineForBounceEffect = 0;

void setHIntScanlineStarterForBounceEffect (u16 yPos, u16 hintMode) {
    startingScanlineForBounceEffect = 0;
    if (yPos < (TITAN_256C_HEIGHT - (2 * TITAN_256C_STRIP_HEIGHT) - 1))
        startingScanlineForBounceEffect = (TITAN_256C_STRIP_HEIGHT - 1) - (yPos % TITAN_256C_STRIP_HEIGHT);
    titan256cHIntMode = hintMode;
}

HINTERRUPT_CALLBACK horizIntScanlineStarterForBounceEffectCallback () {
    if (GET_VCOUNTER < startingScanlineForBounceEffect) {
        return;
    }
    switch (titan256cHIntMode) {
        case HINT_STRATEGY_0:
            SYS_setHIntCallback(horizIntOnTitan256cCallback_CPU_EveryN_asm);
            // instead of VDP_setHIntCounter(TITAN_256C_STRIP_HEIGHT - 1) due to additionals read and write from/to internal regValues[]
            *((u16*) VDP_CTRL_PORT) = 0x8A00 | (TITAN_256C_STRIP_HEIGHT - 1);
            break;
        case HINT_STRATEGY_1:
            SYS_setHIntCallback(horizIntOnTitan256cCallback_CPU_EveryN);
            // instead of VDP_setHIntCounter(TITAN_256C_STRIP_HEIGHT - 1) due to additionals read and write from/to internal regValues[]
            *((u16*) VDP_CTRL_PORT) = 0x8A00 | (TITAN_256C_STRIP_HEIGHT - 1);
            break;
        case HINT_STRATEGY_2:
            SYS_setHIntCallback(horizIntOnTitan256cCallback_DMA_EveryN);
            // instead of VDP_setHIntCounter(TITAN_256C_STRIP_HEIGHT - 1) due to additionals read and write from/to internal regValues[]
            *((u16*) VDP_CTRL_PORT) = 0x8A00 | (TITAN_256C_STRIP_HEIGHT - 1);
            break;
        case HINT_STRATEGY_3:
            SYS_setHIntCallback(horizIntOnTitan256cCallback_DMA_OneTime);
            // instead of VDP_setHIntCounter(0xFF) due to additionals read and write from/to internal regValues[]
            *((u16*) VDP_CTRL_PORT) = 0x8A00 | 0xFF;
            break;
        default: break;
    }
    startingScanlineForBounceEffect = 0;
}

u16 vcounterManual;
u16* titan256cPalsPtr; // 1st and 2nd strip's palette are loaded at the beginning of the display loop, so this ptr starts at 3rd strip
u8 palIdx; // 3rd strip starts with palettes at [PAL0,PAL1]
u16* currGradPtr;
u16 applyBlackPalPosY;
u16 textRampEffectLimitTop;
u16 textRampEffectLimitBottom;

void blackCurrentGradientPtr () {
    currGradPtr = (u16*) palette_black;
}

FORCE_INLINE void varsSetup () {
    // Needed for DMA of colors in u32 type, and it seems is neeed for CPU too (had some black screen flickering if not set)
	*((vu16*) VDP_CTRL_PORT) = 0x8F00 | 2; // instead of VDP_setAutoInc(2) due to additionals read and write from/to internal regValues[]
    
    u16 posYFalling = getYPosFalling();
    u16 stripN = min(TITAN_256C_HEIGHT/TITAN_256C_STRIP_HEIGHT - 1, (posYFalling / TITAN_256C_STRIP_HEIGHT) + 2);
    titan256cPalsPtr = getPalettesData() + stripN * TITAN_256C_COLORS_PER_STRIP;
    applyBlackPalPosY = TITAN_256C_HEIGHT - posYFalling;
    // On even strips we know we use [PAL0,PAL1] so starts with palIdx=0. On odd strips is [PAL1,PAL2] so starts with palIdx=32.
    palIdx = ((posYFalling / TITAN_256C_STRIP_HEIGHT) % 2) == 0 ? 0 : TITAN_256C_COLORS_PER_STRIP;
    currGradPtr = getGradientColorsBuffer();
    textRampEffectLimitTop = TITAN_256C_TEXT_STARTING_STRIP * TITAN_256C_STRIP_HEIGHT - posYFalling + TITAN_256C_TEXT_OFFSET_TOP;
    textRampEffectLimitBottom = TITAN_256C_TEXT_ENDING_STRIP * TITAN_256C_STRIP_HEIGHT - posYFalling + TITAN_256C_TEXT_OFFSET_BOTTOM;
}

void vertIntOnTitan256cCallback_HIntEveryN () {
    varsSetup();
    vcounterManual = TITAN_256C_STRIP_HEIGHT - 1;
    if (vcounterManual >= applyBlackPalPosY)
        titan256cPalsPtr = (u16*) palette_black;
    // when on bouncing, set the HInt responsibly to properly set the starting scanline of the color swap HInt
    if (startingScanlineForBounceEffect != 0) {
        VDP_setHIntCounter(0);
        SYS_setHIntCallback(horizIntScanlineStarterForBounceEffectCallback);
        vcounterManual = startingScanlineForBounceEffect;
    }
}

void vertIntOnTitan256cCallback_HIntOneTime () {
    varsSetup();
    if ((TITAN_256C_STRIP_HEIGHT - 1) >= applyBlackPalPosY)
        titan256cPalsPtr = (u16*) palette_black;
    VDP_setHIntCounter(0);
    // when on bouncing, set the HInt responsibly to properly set the starting scanline of the color swap HInt
    if (startingScanlineForBounceEffect != 0) {
        SYS_setHIntCallback(horizIntScanlineStarterForBounceEffectCallback);
    }
}

HINTERRUPT_CALLBACK horizIntOnTitan256cCallback_CPU_EveryN_asm () {
    // 1380-1408 cycles
    ASM_STATEMENT __volatile__ (
        ".prepare_regs_%=:\n"
        "   move.l      %[currGradPtr],%%a0\n"         // a0: currGradPtr
        "   move.l      %[titan256cPalsPtr],%%a1\n"    // a1: titan256cPalsPtr
        "   movea.l     #0xC00004,%%a2\n"              // a2: VDP_CTRL_PORT 0xC00004
        "   movea.l     #0xC00000,%%a3\n"              // a3: VDP_DATA_PORT 0xC00000
        "   movea.l     #0xC00009,%%a4\n"              // a4: HCounter address 0xC00009
        "   move.b      #154,%%d7\n"                   // d7: 154 is the HCounter limit

        // ".setGradColorForText_flag_%=:\n"
        // "   moveq       #0,%%d4\n"                            // d4: setGradColorForText = 0 (FALSE)
        // "   move.w      %[vcounterManual],%%d0\n"             // d0: vcounterManual
        // "   cmp.w       %[textRampEffectLimitTop],%%d0\n"     // cmp: vcounterManual - textRampEffectLimitTop
        // "   blo         .color_batch_1_cmd\n"                 // branch if (vcounterManual < textRampEffectLimitTop) (opposite than vcounterManual >= textRampEffectLimitTop)
        // "   cmp.w       %[textRampEffectLimitBottom],%%d0\n"  // cmp: vcounterManual - textRampEffectLimitBottom
        // "   bhi         .color_batch_1_cmd\n"                 // branch if (vcounterManual > textRampEffectLimitBottom) (opposite than vcounterManual <= textRampEffectLimitBottom)
        // "   moveq       #1,%%d4\n"                            // d4: setGradColorForText = 1 (TRUE)

        ".setGradColorForText_flag_%=:\n"
        "   move.w      %[vcounterManual],%%d0\n"              // d0: vcounterManual
            // translate vcounterManual to base textRampEffectLimitTop:
        "   sub.w       %[textRampEffectLimitTop],%%d0\n"      // d0: vcounterManual - textRampEffectLimitTop
            // i_TEXT_RAMP_EFFECT_HEIGHT is the amount of scanlines the text ramp effect takes:
        "   cmpi.w      %[i_TEXT_RAMP_EFFECT_HEIGHT],%%d0\n"   // d0 -= i_TEXT_RAMP_EFFECT_HEIGHT
        "   scs.b       %%d4\n"                                // if d0 >= 0 => d4=0xFF (enable bg color), if d0 < 0 => d4=0x00 (no bg color)
        // Next instructions not needed, logic works fine without them. I don't get it.
        //"   bls         .color_batch_1_cmd\n"                  // branch if d0 <= i_TEXT_RAMP_EFFECT_HEIGHT (at this moment d4 is correctly set)
        //"   moveq       #0,%%d4\n"                             // d4=0x00 (no bg color)

		".color_batch_1_cmd:\n"
			// cmdAddress = palIdx == 0 ? 0xC0000000 : 0xC0400000;
            // set base command address once and then we'll add the right offset in next color batch blocks
		"   move.l      #0xC0000000,%%d6\n"     // d6: cmdAddress = 0xC0000000
		"   tst.b       %[palIdx]\n"            // palIdx == 0?
		"   beq         .set_bgColor1\n"
		"   move.l      #0xC0400000,%%d6\n"     // d6: cmdAddress = 0xC0400000
		".set_bgColor1:\n"
		"   moveq       #0,%%d5\n"              // d5: bgColor1 = 0
            // from here on, d5=0 as long as d4 == 0, so no need to reset d5 in other color batch blocks
		"   tst.b       %%d4\n"                 // d4: setGradColorForText => if setGradColorForText = 0 (FALSE)
		"   beq         .color_batch_1_pal\n"
		"   move.w      (%%a0)+,%%d5\n"         // d5: bgColor1 = *(currGradPtr + 0);
		".color_batch_1_pal:\n"
		"   move.l      (%%a1)+,%%d0\n"         // d0: colors2_A = *((u32*) (titan256cPalsPtr + 0)); // 2 colors
		"   move.l      (%%a1)+,%%d1\n"         // d1: colors2_B = *((u32*) (titan256cPalsPtr + 2)); // next 2 colors
        // wait HCounter
        ".waitHCounter1_%=:\n"
        "   cmp.b       (%%a4),%%d7\n"          // cmp: d7 - (a4). Compare byte size given that d7 won't be > 160 for our practical cases
        "   bhi         .waitHCounter1_%=\n"    // loop back if d7 is higher than (a4)
		// turnOffVDP
		"   move.w      %[turnoff],(%%a2)\n"    // *(vu16*) VDP_CTRL_PORT = 0x8100 | (reg01 & ~0x40);
		// send colors
		"   move.l      %%d6,(%%a2)\n"          // *((vu32*) VDP_CTRL_PORT) = cmdAddress;
		"   move.l      %%d0,(%%a3)\n"          // *((vu32*) VDP_DATA_PORT) = colors2_A;
		"   move.l      %%d1,(%%a3)\n"          // *((vu32*) VDP_DATA_PORT) = colors2_B;
		"   move.l      #0xC0000000,(%%a2)\n"   // *((vu32*) VDP_CTRL_PORT) = 0xC0000000; // VDP_WRITE_CRAM_ADDR(0): write to CRAM color index 0 multiplied by 2
		"   move.w      %%d5,(%%a3)\n"          // *((vu16*) VDP_DATA_PORT) = bgColor1;
		// turnOnVDP
		"   move.w      %[turnon],(%%a2)\n"     // *(vu16*) VDP_CTRL_PORT = 0x8100 | (reg01 | 0x40);

		".color_batch_2_cmd:\n"
			// cmdAddress = palIdx == 0 ? 0xC0080000 : 0xC0480000;
		"   addi.l      #0x80000,%%d6\n"        // d6: cmdAddress += 0x80000 // previous batch advanced 4 colors
		".color_batch_2_pal:\n"
		"   move.l      (%%a1)+,%%d0\n"         // d0: colors2_A = *((u32*) (titan256cPalsPtr + 4)); // 2 colors
		"   move.l      (%%a1)+,%%d1\n"         // d1: colors2_B = *((u32*) (titan256cPalsPtr + 6)); // next 2 colors
		"   move.l      (%%a1)+,%%d2\n"         // d2: colors2_C = *((u32*) (titan256cPalsPtr + 8)); // next 2 colors
		"   move.l      (%%a1)+,%%d3\n"         // d3: colors2_D = *((u32*) (titan256cPalsPtr + 10)); // next 2 colors
        // wait HCounter
        ".waitHCounter2_%=:\n"
        "   cmp.b       (%%a4),%%d7\n"          // cmp: d7 - (a4). Compare byte size given that d7 won't be > 160 for our practical cases
        "   bhi         .waitHCounter2_%=\n"    // loop back if d7 is higher than (a4)
		// turnOffVDP
		"   move.w      %[turnoff],(%%a2)\n"    // *(vu16*) VDP_CTRL_PORT = 0x8100 | (reg01 & ~0x40);
		// send colors
		"   move.l      %%d6,(%%a2)\n"          // *((vu32*) VDP_CTRL_PORT) = cmdAddress;
		"   move.l      %%d0,(%%a3)\n"          // *((vu32*) VDP_DATA_PORT) = colors2_A;
		"   move.l      %%d1,(%%a3)\n"          // *((vu32*) VDP_DATA_PORT) = colors2_B;
		"   move.l      %%d2,(%%a3)\n"          // *((vu32*) VDP_DATA_PORT) = colors2_C;
		"   move.l      %%d3,(%%a3)\n"          // *((vu32*) VDP_DATA_PORT) = colors2_D;
		// turnOnVDP
		"   move.w      %[turnon],(%%a2)\n"     // *(vu16*) VDP_CTRL_PORT = 0x8100 | (reg01 | 0x40);

		".color_batch_3_cmd:\n"
			// cmdAddress = palIdx == 0 ? 0xC0180000 : 0xC0580000;
		"   addi.l      #0x100000,%%d6\n"       // d6: cmdAddress += 0x100000 // previous batch advanced 8 colors
		".set_bgColor2:\n"
        "   tst.b       %%d4\n"                 // d4: setGradColorForText => if setGradColorForText = 0 (FALSE)
		"   beq         .color_batch_3_pal\n"
		"   move.w      (%%a0)+,%%d5\n"         // d5: bgColor2 = *(currGradPtr + 1);
		".color_batch_3_pal:\n"
		"   move.l      (%%a1)+,%%d0\n"         // d0: colors2_A = *((u32*) (titan256cPalsPtr + 12)); // 2 colors
		"   move.l      (%%a1)+,%%d1\n"         // d1: colors2_B = *((u32*) (titan256cPalsPtr + 14)); // next 2 colors
        // wait HCounter
        ".waitHCounter3_%=:\n"
        "   cmp.b       (%%a4),%%d7\n"          // cmp: d7 - (a4). Compare byte size given that d7 won't be > 160 for our practical cases
        "   bhi         .waitHCounter3_%=\n"    // loop back if d7 is higher than (a4)
		// turnOffVDP
		"   move.w      %[turnoff],(%%a2)\n"    // *(vu16*) VDP_CTRL_PORT = 0x8100 | (reg01 & ~0x40);
		// send colors
		"   move.l      %%d6,(%%a2)\n"          // *((vu32*) VDP_CTRL_PORT) = cmdAddress;
		"   move.l      %%d0,(%%a3)\n"          // *((vu32*) VDP_DATA_PORT) = colors2_A;
		"   move.l      %%d1,(%%a3)\n"          // *((vu32*) VDP_DATA_PORT) = colors2_B;
		"   move.l      #0xC0000000,(%%a2)\n"   // *((vu32*) VDP_CTRL_PORT) = 0xC0000000; // VDP_WRITE_CRAM_ADDR(0): write to CRAM color index 0 multiplied by 2
		"   move.w      %%d5,(%%a3)\n"          // *((vu16*) VDP_DATA_PORT) = bgColor2;
		// turnOnVDP
		"   move.w      %[turnon],(%%a2)\n"     // *(vu16*) VDP_CTRL_PORT = 0x8100 | (reg01 | 0x40);

		".color_batch_4_cmd:\n"
			// cmdAddress = palIdx == 0 ? 0xC0200000 : 0xC0600000;
		"   addi.l      #0x80000,%%d6\n"        // d6: cmdAddress += 0x80000 // previous batch advanced 4 colors
		".color_batch_4_pal:\n"
		"   move.l      (%%a1)+,%%d0\n"         // d0: colors2_A = *((u32*) (titan256cPalsPtr + 16)); // 2 colors
		"   move.l      (%%a1)+,%%d1\n"         // d1: colors2_B = *((u32*) (titan256cPalsPtr + 18)); // next 2 colors
		"   move.l      (%%a1)+,%%d2\n"         // d2: colors2_C = *((u32*) (titan256cPalsPtr + 20)); // next 2 colors
		"   move.l      (%%a1)+,%%d3\n"         // d3: colors2_D = *((u32*) (titan256cPalsPtr + 22)); // next 2 colors
        // wait HCounter
        ".waitHCounter4_%=:\n"
        "   cmp.b       (%%a4),%%d7\n"          // cmp: d7 - (a4). Compare byte size given that d7 won't be > 160 for our practical cases
        "   bhi         .waitHCounter4_%=\n"    // loop back if d7 is higher than (a4)
		// turnOffVDP
		"   move.w      %[turnoff],(%%a2)\n"    // *(vu16*) VDP_CTRL_PORT = 0x8100 | (reg01 & ~0x40);
		// send colors
		"   move.l      %%d6,(%%a2)\n"          // *((vu32*) VDP_CTRL_PORT) = cmdAddress;
		"   move.l      %%d0,(%%a3)\n"          // *((vu32*) VDP_DATA_PORT) = colors2_A;
		"   move.l      %%d1,(%%a3)\n"          // *((vu32*) VDP_DATA_PORT) = colors2_B;
		"   move.l      %%d2,(%%a3)\n"          // *((vu32*) VDP_DATA_PORT) = colors2_C;
		"   move.l      %%d3,(%%a3)\n"          // *((vu32*) VDP_DATA_PORT) = colors2_D;
		// turnOnVDP
		"   move.w      %[turnon],(%%a2)\n"     // *(vu16*) VDP_CTRL_PORT = 0x8100 | (reg01 | 0x40);

		".color_batch_5_cmd:\n"
			// cmdAddress = palIdx == 0 ? 0xC0300000 : 0xC0700000;
		"   addi.l      #0x100000,%%d6\n"       // d6: cmdAddress += 0x100000 // previous batch advanced 8 colors
		".set_bgColor3:\n"
		"   tst.b       %%d4\n"                 // d4: setGradColorForText => if setGradColorForText = 0 (FALSE)
		"   beq         .color_batch_5_pal\n"
		"   move.w      (%%a0)+,%%d5\n"         // d5: bgColor3 = *(currGradPtr + 2);
		".color_batch_5_pal:\n"
		"   move.l      (%%a1)+,%%d0\n"         // d0: colors2_A = *((u32*) (titan256cPalsPtr + 24)); // 2 colors
		"   move.l      (%%a1)+,%%d1\n"         // d1: colors2_B = *((u32*) (titan256cPalsPtr + 26)); // next 2 colors
        // wait HCounter
        ".waitHCounter5_%=:\n"
        "   cmp.b       (%%a4),%%d7\n"          // cmp: d7 - (a4). Compare byte size given that d7 won't be > 160 for our practical cases
        "   bhi         .waitHCounter5_%=\n"    // loop back if d7 is higher than (a4)
		// turnOffVDP
		"   move.w      %[turnoff],(%%a2)\n"    // *(vu16*) VDP_CTRL_PORT = 0x8100 | (reg01 & ~0x40);
		// send colors
		"   move.l      %%d6,(%%a2)\n"          // *((vu32*) VDP_CTRL_PORT) = cmdAddress;
		"   move.l      %%d0,(%%a3)\n"          // *((vu32*) VDP_DATA_PORT) = colors2_A;
		"   move.l      %%d1,(%%a3)\n"          // *((vu32*) VDP_DATA_PORT) = colors2_B;
		"   move.l      #0xC0000000,(%%a2)\n"   // *((vu32*) VDP_CTRL_PORT) = 0xC0000000; // VDP_WRITE_CRAM_ADDR(0): write to CRAM color index 0 multiplied by 2
		"   move.w      %%d5,(%%a3)\n"          // *((vu16*) VDP_DATA_PORT) = bgColor3;
		// turnOnVDP
		"   move.w      %[turnon],(%%a2)\n"     // *(vu16*) VDP_CTRL_PORT = 0x8100 | (reg01 | 0x40);

		".color_batch_6_cmd:\n"
			// cmdAddress = palIdx == 0 ? 0xC0380000 : 0xC0780000;
		"   addi.l      #0x80000,%%d6\n"        // d6: cmdAddress += 0x80000 // previous batch advanced 4 colors
		".color_batch_6_pal:\n"
		"   move.l      (%%a1)+,%%d0\n"         // d0: colors2_A = *((u32*) (titan256cPalsPtr + 28)); // 2 colors
		"   move.l      (%%a1)+,%%d1\n"         // d1: colors2_B = *((u32*) (titan256cPalsPtr + 30)); // next 2 colors
        // wait HCounter
        ".waitHCounter6_%=:\n"
        "   cmp.b       (%%a4),%%d7\n"          // cmp: d7 - (a4). Compare byte size given that d7 won't be > 160 for our practical cases
        "   bhi        .waitHCounter6_%=\n"     // loop back if d7 is higher than (a4)
		// turnOffVDP
		"   move.w      %[turnoff],(%%a2)\n"    // *(vu16*) VDP_CTRL_PORT = 0x8100 | (reg01 & ~0x40);
		// send colors
		"   move.l      %%d6,(%%a2)\n"          // *((vu32*) VDP_CTRL_PORT) = cmdAddress;
		"   move.l      %%d0,(%%a3)\n"          // *((vu32*) VDP_DATA_PORT) = colors2_A;
		"   move.l      %%d1,(%%a3)\n"          // *((vu32*) VDP_DATA_PORT) = colors2_B;
		// turnOnVDP
		"   move.w      %[turnon],(%%a2)\n"     // *(vu16*) VDP_CTRL_PORT = 0x8100 | (reg01 | 0x40);

		".set_bgColor4:\n"
		"   tst.b       %%d4\n"                 // d4: setGradColorForText => if setGradColorForText = 0 (FALSE)
		"   beq         .accomodate_vars_A\n"
		"   move.w      (%%a0)+,%%d5\n"         // d5: bgColor4 = *(currGradPtr + 3);
        "   move.l      %%a0,%[currGradPtr]\n"  // store current value currGradPtr

		".accomodate_vars_A:\n"
        "   move.w      %[vcounterManual],%%d0\n"                  // d0: vcounterManual
        "   addq.w      %[i_TITAN_256C_STRIP_HEIGHT],%%d0\n"       // d0: vcounterManual += TITAN_256C_STRIP_HEIGHT
        "   move.w      %%d0,%[vcounterManual]\n"                  // store current value of vcounterManual
        "   move.b      %[palIdx],%%d1\n"                          // d1: palIdx
        "   eori.b      %[i_TITAN_256C_COLORS_PER_STRIP],%%d1\n"   // d1: palIdx ^= TITAN_256C_COLORS_PER_STRIP // cycles between 0 and 32
        "   move.b      %%d1,%[palIdx]\n"                          // store current value of palIdx

		".color_batch_7_pal:\n"
        // wait HCounter
        ".waitHCounter7_%=:\n"
        "   cmp.b       (%%a4),%%d7\n"          // cmp: d7 - (a4). Compare byte size given that d7 won't be > 160 for our practical cases
        "   bhi         .waitHCounter7_%=\n"    // loop back if d7 is higher than (a4)
		// turnOffVDP
		"   move.w      %[turnoff],(%%a2)\n"    // *(vu16*) VDP_CTRL_PORT = 0x8100 | (reg01 & ~0x40);
		// send colors
		"   move.l      #0xC0000000,(%%a2)\n"   // *((vu32*) VDP_CTRL_PORT) = 0xC0000000; // VDP_WRITE_CRAM_ADDR(0): write to CRAM color index 0 multiplied by 2
		"   move.w      %%d5,(%%a3)\n"          // *((vu16*) VDP_DATA_PORT) = bgColor4;
		// turnOnVDP
		"   move.w      %[turnon],(%%a2)\n"     // *(vu16*) VDP_CTRL_PORT = 0x8100 | (reg01 | 0x40);

        ".accomodate_vars_B:\n"
        "   cmp.w       %[applyBlackPalPosY],%%d0\n"   // cmp: vcounterManual - applyBlackPalPosY
        "   blo         .accomodate_vars_C\n"          // branch if (vcounterManual < applyBlackPalPosY)
        "   move.w      %[palette_black],%%a1\n"       // a1 = palette_black;
            // we can use .w in above instruction bc palette_black is located at RAM 0x0000---- (only word part is effectively used)
        "   bra         .fin%=\n"
        ".accomodate_vars_C:\n"
        "   move.l      %%a1,%[titan256cPalsPtr]\n"    // store current value of a1 into variable titan256cPalsPtr
        ".fin%=:\n"
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
		[turnoff] "i" (0x8100 | (116 & ~0x40)), // 0x8134
		[turnon] "i" (0x8100 | (116 | 0x40)), // 0x8174
		[i_TITAN_256C_COLORS_PER_STRIP] "i" (TITAN_256C_COLORS_PER_STRIP),
		[i_TITAN_256C_STRIP_HEIGHT] "i" (TITAN_256C_STRIP_HEIGHT),
        [i_TEXT_RAMP_EFFECT_HEIGHT] "i" ((TITAN_256C_TEXT_ENDING_STRIP - TITAN_256C_TEXT_STARTING_STRIP) * TITAN_256C_STRIP_HEIGHT + TITAN_256C_TEXT_OFFSET_BOTTOM)
		:
		"d2","d3","d4","d5","d6","d7","a2","a3","a4","cc","memory"  // backup registers used in the asm implementation, except scratch pad
    );
}

HINTERRUPT_CALLBACK horizIntOnTitan256cCallback_CPU_EveryN () {

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

    //u32 offset = 0x80000; // in case we use cmdAddress += offset
    u32 cmdAddress;
    u16 bgColor1=0, bgColor2=0, bgColor3=0, bgColor4=0;
    u32 colors2_A, colors2_B, colors2_C, colors2_D;

    if (setGradColorForText) {
        bgColor1 = *(currGradPtr + 0);
        bgColor2 = *(currGradPtr + 1);
        bgColor3 = *(currGradPtr + 2);
        bgColor4 = *(currGradPtr + 3);
    }

    // Value under current conditions is always 116
    //u8 reg01 = VDP_getReg(0x01); // Holds current VDP register 1 value (it holds other bits than VDP ON/OFF status)
    // NOTE: here is OK to call VDP_getReg(0x01) only if we didn't previously change the the VDP's reg 1 using direct access without VDP_setReg()

    cmdAddress = palIdx == 0 ? 0xC0000000 : 0xC0400000;
    colors2_A = *((u32*) (titan256cPalsPtr + 0)); // 2 colors
    colors2_B = *((u32*) (titan256cPalsPtr + 2)); // next 2 colors
    waitHCounter_old(145);
    turnOffVDP(116);
    *((vu32*) VDP_CTRL_PORT) = cmdAddress;
    *((vu32*) VDP_DATA_PORT) = colors2_A;
    *((vu32*) VDP_DATA_PORT) = colors2_B;
        *((vu32*) VDP_CTRL_PORT) = 0xC0000000; // VDP_WRITE_CRAM_ADDR(0): write to CRAM color index 0 multiplied by 2
        *((vu16*) VDP_DATA_PORT) = bgColor1;
    turnOnVDP(116);

    cmdAddress = palIdx == 0 ? 0xC0080000 : 0xC0480000;
    colors2_A = *((u32*) (titan256cPalsPtr + 4)); // 2 colors
    colors2_B = *((u32*) (titan256cPalsPtr + 6)); // next 2 colors
    colors2_C = *((u32*) (titan256cPalsPtr + 8)); // next2 colors
    colors2_D = *((u32*) (titan256cPalsPtr + 10)); // next 2 colors
    waitHCounter_old(145);
    turnOffVDP(116);
    *((vu32*) VDP_CTRL_PORT) = cmdAddress;
    *((vu32*) VDP_DATA_PORT) = colors2_A;
    *((vu32*) VDP_DATA_PORT) = colors2_B;
    *((vu32*) VDP_DATA_PORT) = colors2_C;
    *((vu32*) VDP_DATA_PORT) = colors2_D;
    turnOnVDP(116);

    cmdAddress = palIdx == 0 ? 0xC0180000 : 0xC0580000;
    colors2_A = *((u32*) (titan256cPalsPtr + 12)); // 2 colors
    colors2_B = *((u32*) (titan256cPalsPtr + 14)); // next 2 colors
    waitHCounter_old(145);
    turnOffVDP(116);
    *((vu32*) VDP_CTRL_PORT) = cmdAddress;
    *((vu32*) VDP_DATA_PORT) = colors2_A;
    *((vu32*) VDP_DATA_PORT) = colors2_B;
        *((vu32*) VDP_CTRL_PORT) = 0xC0000000; // VDP_WRITE_CRAM_ADDR(0): write to CRAM color index 0 multiplied by 2
        *((vu16*) VDP_DATA_PORT) = bgColor2;
    turnOnVDP(116);

    cmdAddress = palIdx == 0 ? 0xC0200000 : 0xC0600000;
    colors2_A = *((u32*) (titan256cPalsPtr + 16)); // 2 colors
    colors2_B = *((u32*) (titan256cPalsPtr + 18)); // next 2 colors
    colors2_C = *((u32*) (titan256cPalsPtr + 20)); // next colors
    colors2_D = *((u32*) (titan256cPalsPtr + 22)); // next 2 colors
    waitHCounter_old(145);
    turnOffVDP(116);
    *((vu32*) VDP_CTRL_PORT) = cmdAddress;
    *((vu32*) VDP_DATA_PORT) = colors2_A;
    *((vu32*) VDP_DATA_PORT) = colors2_B;
    *((vu32*) VDP_DATA_PORT) = colors2_C;
    *((vu32*) VDP_DATA_PORT) = colors2_D;
    turnOnVDP(116);

    cmdAddress = palIdx == 0 ? 0xC0300000 : 0xC0700000;
    colors2_A = *((u32*) (titan256cPalsPtr + 24)); // 2 colors
    colors2_B = *((u32*) (titan256cPalsPtr + 26)); // next 2 colors
    waitHCounter_old(145);
    turnOffVDP(116);
    *((vu32*) VDP_CTRL_PORT) = cmdAddress;
    *((vu32*) VDP_DATA_PORT) = colors2_A;
    *((vu32*) VDP_DATA_PORT) = colors2_B;
        *((vu32*) VDP_CTRL_PORT) = 0xC0000000; // VDP_WRITE_CRAM_ADDR(0): write to CRAM color index 0 multiplied by 2
        *((vu16*) VDP_DATA_PORT) = bgColor3;
    turnOnVDP(116);

    cmdAddress = palIdx == 0 ? 0xC0380000 : 0xC0780000;
    colors2_A = *((u32*) (titan256cPalsPtr + 28)); // 2 colors
    colors2_B = *((u32*) (titan256cPalsPtr + 30)); // next 2 colors
    waitHCounter_old(145);
    turnOffVDP(116);
    *((vu32*) VDP_CTRL_PORT) = cmdAddress;
    *((vu32*) VDP_DATA_PORT) = colors2_A;
    *((vu32*) VDP_DATA_PORT) = colors2_B;
    turnOnVDP(116);

    currGradPtr += setGradColorForText ? 4 : 0; // advance 4 colors if condition is met
    vcounterManual += TITAN_256C_STRIP_HEIGHT;
    palIdx ^= TITAN_256C_COLORS_PER_STRIP; // cycles between 0 and 32
    //palIdx = palIdx == 0 ? 32 : 0;
    //palIdx = (palIdx + 32) & 63; // (palIdx + 32) % 64 => x mod y = x & (y-1) when y is power of 2
    waitHCounter_old(150);
    turnOffVDP(116);
        *((vu32*) VDP_CTRL_PORT) = 0xC0000000; // VDP_WRITE_CRAM_ADDR(0): write to CRAM color index 0 multiplied by 2
        *((vu16*) VDP_DATA_PORT) = bgColor4;
    turnOnVDP(116);

    if (vcounterManual >= applyBlackPalPosY)
        titan256cPalsPtr = (u16*) palette_black;
    else 
        titan256cPalsPtr += TITAN_256C_COLORS_PER_STRIP; // advance to next strip's palette
}

HINTERRUPT_CALLBACK horizIntOnTitan256cCallback_DMA_EveryN () {

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
    u16 bgColor1=0, bgColor2=0, bgColor3=0, bgColor4=0;

    if (setGradColorForText) {
        bgColor1 = *(currGradPtr + 0);
        bgColor2 = *(currGradPtr + 1);
        bgColor3 = *(currGradPtr + 2);
        bgColor4 = *(currGradPtr + 3);
    }

    // Value under current conditions is always 116
    //u8 reg01 = VDP_getReg(0x01); // Holds current VDP register 1 value (it holds other bits than VDP ON/OFF status)
    // NOTE: here is OK to call VDP_getReg(0x01) only if we didn't previously change the the VDP's reg 1 using direct access without VDP_setReg()

    fromAddrForDMA = (u32) titan256cPalsPtr >> 1;
    titan256cPalsPtr += TITAN_256C_COLORS_PER_STRIP/3;
    palCmdForDMA = palIdx == 0 ? 0xC0000080 : 0xC0400080;
MEMORY_BARRIER();
    waitHCounter(154);
    // set GB color before setup DMA
    *((vu32*) VDP_CTRL_PORT) = 0xC0000000; // VDP_WRITE_CRAM_ADDR(0): write to CRAM color index 0 multiplied by 2
    *((vu16*) VDP_DATA_PORT) = bgColor1;
    //setupDMAForPals(TITAN_256C_COLORS_PER_STRIP/3, fromAddrForDMA);
    // Setup DMA length (in word here)
    *((vu16*) VDP_CTRL_PORT) = 0x9300 | ((TITAN_256C_COLORS_PER_STRIP/3) & 0xff);
    //*((vu16*) VDP_CTRL_PORT) = 0x9400 | (((TITAN_256C_COLORS_PER_STRIP/3) >> 8) & 0xff);
    // Setup DMA address
    *((vu16*) VDP_CTRL_PORT) = 0x9500 | (fromAddrForDMA & 0xff);
    *((vu16*) VDP_CTRL_PORT) = 0x9600 | ((fromAddrForDMA >> 8) & 0xff);
    //*((vu16*) VDP_CTRL_PORT) = 0x9700 | ((fromAddrForDMA >> 16) & 0x7f);
    waitHCounter(154);
    turnOffVDP(116);
    *((vu32*) VDP_CTRL_PORT) = palCmdForDMA; // trigger DMA transfer
    turnOnVDP(116);

    fromAddrForDMA = (u32) titan256cPalsPtr >> 1;
    titan256cPalsPtr += TITAN_256C_COLORS_PER_STRIP/3;
    palCmdForDMA = palIdx == 0 ? 0xC0140080 : 0xC0540080;
MEMORY_BARRIER();
    waitHCounter(154);
    // set GB color before setup DMA
    *((vu32*) VDP_CTRL_PORT) = 0xC0000000; // VDP_WRITE_CRAM_ADDR(0): write to CRAM color index 0 multiplied by 2
    *((vu16*) VDP_DATA_PORT) = bgColor2;
    //setupDMAForPals(TITAN_256C_COLORS_PER_STRIP/3, fromAddrForDMA);
    // Setup DMA length (in word here)
    *((vu16*) VDP_CTRL_PORT) = 0x9300 | ((TITAN_256C_COLORS_PER_STRIP/3) & 0xff);
    //*((vu16*) VDP_CTRL_PORT) = 0x9400 | (((TITAN_256C_COLORS_PER_STRIP/3) >> 8) & 0xff);
    // Setup DMA address
    *((vu16*) VDP_CTRL_PORT) = 0x9500 | (fromAddrForDMA & 0xff);
    *((vu16*) VDP_CTRL_PORT) = 0x9600 | ((fromAddrForDMA >> 8) & 0xff);
    //*((vu16*) VDP_CTRL_PORT) = 0x9700 | ((fromAddrForDMA >> 16) & 0x7f);
    waitHCounter(154);
    turnOffVDP(116);
    *((vu32*) VDP_CTRL_PORT) = palCmdForDMA; // trigger DMA transfer
    turnOnVDP(116);

    fromAddrForDMA = (u32) titan256cPalsPtr >> 1;
    titan256cPalsPtr += TITAN_256C_COLORS_PER_STRIP/3 + TITAN_256C_COLORS_PER_STRIP_REMAINDER(3);
    palCmdForDMA = palIdx == 0 ? 0xC0280080 : 0xC0680080;
MEMORY_BARRIER();
    waitHCounter(154);
    // set GB color before setup DMA
    *((vu32*) VDP_CTRL_PORT) = 0xC0000000; // VDP_WRITE_CRAM_ADDR(0): write to CRAM color index 0 multiplied by 2
    *((vu16*) VDP_DATA_PORT) = bgColor3;
    //setupDMAForPals(TITAN_256C_COLORS_PER_STRIP/3 + TITAN_256C_COLORS_PER_STRIP_REMAINDER(3), fromAddrForDMA);
    // Setup DMA length (in word here)
    *((vu16*) VDP_CTRL_PORT) = 0x9300 | ((TITAN_256C_COLORS_PER_STRIP/3 + TITAN_256C_COLORS_PER_STRIP_REMAINDER(3)) & 0xff);
    //*((vu16*) VDP_CTRL_PORT) = 0x9400 | (((TITAN_256C_COLORS_PER_STRIP/3 + TITAN_256C_COLORS_PER_STRIP_REMAINDER(3)) >> 8) & 0xff);
    // Setup DMA address
    *((vu16*) VDP_CTRL_PORT) = 0x9500 | (fromAddrForDMA & 0xff);
    *((vu16*) VDP_CTRL_PORT) = 0x9600 | ((fromAddrForDMA >> 8) & 0xff);
    //*((vu16*) VDP_CTRL_PORT) = 0x9700 | ((fromAddrForDMA >> 16) & 0x7f);
    waitHCounter(154);
    turnOffVDP(116);
    *((vu32*) VDP_CTRL_PORT) = palCmdForDMA; // trigger DMA transfer
    turnOnVDP(116);

    vcounterManual += TITAN_256C_STRIP_HEIGHT;
    currGradPtr += 4 * setGradColorForText; // advance 4 colors if condition is met
    //titan256cPalsPtr += TITAN_256C_COLORS_PER_STRIP; // advance to next strip's palettes (if pointer wasn't incremented previously)
    palIdx ^= TITAN_256C_COLORS_PER_STRIP; // cycles between 0 and 32
    //palIdx = palIdx == 0 ? 32 : 0;
    //palIdx = (palIdx + 32) & 63; // (palIdx + 32) % 64 => x mod y = x & (y-1) when y is power of 2
    
    waitHCounter(154);
    // set GB color before setup DMA
    *((vu32*) VDP_CTRL_PORT) = 0xC0000000; // VDP_WRITE_CRAM_ADDR(0): write to CRAM color index 0 multiplied by 2
    *((vu16*) VDP_DATA_PORT) = bgColor4;

    if (vcounterManual >= applyBlackPalPosY)
        titan256cPalsPtr = (u16*) palette_black;
}

HINTERRUPT_CALLBACK horizIntOnTitan256cCallback_DMA_OneTime () {
    // instead of VDP_setHIntCounter(0xFF) due to additionals read and write from/to internal regValues[]
    *((u16*) VDP_CTRL_PORT) = 0x8A00 | 0xFF;

    // Giving the nature of SGDK hint callback it will be invoked several times until the VDP_setHIntCounter(0xFF) makes effect.
    // So we need to cancel out the subsequent invocations which are those having VCOUNTER > 0 (is 0 based).
    if (GET_VCOUNTER > 0) {
        return;
    }

    // Value under current conditions is always 116
    //u8 reg01 = VDP_getReg(0x01); // Holds current VDP register 1 value (it holds other bits than VDP ON/OFF status)
    // NOTE: here is OK to call VDP_getReg(0x01) only if we didn't previously change the the VDP's reg 1 using direct access without VDP_setReg()

    // Simulates waiting the first call to Simulates VDP_setHIntCounter(TITAN_256C_STRIP_HEIGHT - 1)
    waitVCounterReg(TITAN_256C_STRIP_HEIGHT - 1 - 1);
    // at this point GET_VCOUNTER value is TITAN_256C_STRIP_HEIGHT - 1
    u16 vcounter = TITAN_256C_STRIP_HEIGHT - 1;

    for (;;) {
        // SCANLINE 1 starts right here

        // exits when we reach last image strip which it doesn't need any palette to be loaded
        if (vcounter > (TITAN_256C_HEIGHT - TITAN_256C_STRIP_HEIGHT - 1)) { // valid for NTSC and PAL since titan image size is fixed
            return;
        }

        bool setGradColorForText = vcounter >= textRampEffectLimitTop && vcounter <= textRampEffectLimitBottom;

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
        u16 bgColor1=0, bgColor2=0, bgColor3=0, bgColor4=0;

        if (setGradColorForText) {
            bgColor1 = *(currGradPtr + 0);
            bgColor2 = *(currGradPtr + 1);
            bgColor3 = *(currGradPtr + 2);
            bgColor4 = *(currGradPtr + 3);
        }

        fromAddrForDMA = (u32) titan256cPalsPtr >> 1;
        titan256cPalsPtr += TITAN_256C_COLORS_PER_STRIP/3;
        palCmdForDMA = palIdx == 0 ? 0xC0000080 : 0xC0400080;
MEMORY_BARRIER();
        waitHCounter(154);
        // SCANLINE 2 starts here (few pixels ahead)

        // set GB color before setup DMA
        *((vu32*) VDP_CTRL_PORT) = 0xC0000000; // VDP_WRITE_CRAM_ADDR(0): write to CRAM color index 0 multiplied by 2
        *((vu16*) VDP_DATA_PORT) = bgColor1;
        //setupDMAForPals(TITAN_256C_COLORS_PER_STRIP/3, fromAddrForDMA);
        // Setup DMA length (in word here)
        *((vu16*) VDP_CTRL_PORT) = 0x9300 | ((TITAN_256C_COLORS_PER_STRIP/3) & 0xff);
        //*((vu16*) VDP_CTRL_PORT) = 0x9400 | (((TITAN_256C_COLORS_PER_STRIP/3) >> 8) & 0xff);
        // Setup DMA address
        *((vu16*) VDP_CTRL_PORT) = 0x9500 | (fromAddrForDMA & 0xff);
        *((vu16*) VDP_CTRL_PORT) = 0x9600 | ((fromAddrForDMA >> 8) & 0xff);
        //*((vu16*) VDP_CTRL_PORT) = 0x9700 | ((fromAddrForDMA >> 16) & 0x7f);

        waitHCounter(154);
        // SCANLINE 3 starts here (few pixels ahead)

        turnOffVDP(116);
        *((vu32*) VDP_CTRL_PORT) = palCmdForDMA; // trigger DMA transfer
        turnOnVDP(116);

        fromAddrForDMA = (u32) titan256cPalsPtr >> 1;
        titan256cPalsPtr += TITAN_256C_COLORS_PER_STRIP/3;
        palCmdForDMA = palIdx == 0 ? 0xC0140080 : 0xC0540080;
MEMORY_BARRIER();
        waitHCounter(154);
        // SCANLINE 4 starts here (few pixels ahead)

        // set GB color before setup DMA
        *((vu32*) VDP_CTRL_PORT) = 0xC0000000; // VDP_WRITE_CRAM_ADDR(0): write to CRAM color index 0 multiplied by 2
        *((vu16*) VDP_DATA_PORT) = bgColor2;
        //setupDMAForPals(TITAN_256C_COLORS_PER_STRIP/3, fromAddrForDMA);
        // Setup DMA length (in word here)
        *((vu16*) VDP_CTRL_PORT) = 0x9300 | ((TITAN_256C_COLORS_PER_STRIP/3) & 0xff);
        //*((vu16*) VDP_CTRL_PORT) = 0x9400 | (((TITAN_256C_COLORS_PER_STRIP/3) >> 8) & 0xff);
        // Setup DMA address
        *((vu16*) VDP_CTRL_PORT) = 0x9500 | (fromAddrForDMA & 0xff);
        *((vu16*) VDP_CTRL_PORT) = 0x9600 | ((fromAddrForDMA >> 8) & 0xff);
        //*((vu16*) VDP_CTRL_PORT) = 0x9700 | ((fromAddrForDMA >> 16) & 0x7f);

        waitHCounter(154);
        // SCANLINE 5 starts here (few pixels ahead)

        turnOffVDP(116);
        *((vu32*) VDP_CTRL_PORT) = palCmdForDMA; // trigger DMA transfer
        turnOnVDP(116);

        fromAddrForDMA = (u32) titan256cPalsPtr >> 1;
        titan256cPalsPtr += TITAN_256C_COLORS_PER_STRIP/3 + TITAN_256C_COLORS_PER_STRIP_REMAINDER(3);
        palCmdForDMA = palIdx == 0 ? 0xC0280080 : 0xC0680080;
MEMORY_BARRIER();
        waitHCounter(154);
        // SCANLINE 6 starts here (few pixels ahead)

        // set GB color before setup DMA
        *((vu32*) VDP_CTRL_PORT) = 0xC0000000; // VDP_WRITE_CRAM_ADDR(0): write to CRAM color index 0 multiplied by 2
        *((vu16*) VDP_DATA_PORT) = bgColor3;
        //setupDMAForPals(TITAN_256C_COLORS_PER_STRIP/3 + TITAN_256C_COLORS_PER_STRIP_REMAINDER(3), fromAddrForDMA);
        // Setup DMA length (in word here)
        *((vu16*) VDP_CTRL_PORT) = 0x9300 | ((TITAN_256C_COLORS_PER_STRIP/3 + TITAN_256C_COLORS_PER_STRIP_REMAINDER(3)) & 0xff);
        //*((vu16*) VDP_CTRL_PORT) = 0x9400 | (((TITAN_256C_COLORS_PER_STRIP/3 + TITAN_256C_COLORS_PER_STRIP_REMAINDER(3)) >> 8) & 0xff);
        // Setup DMA address
        *((vu16*) VDP_CTRL_PORT) = 0x9500 | (fromAddrForDMA & 0xff);
        *((vu16*) VDP_CTRL_PORT) = 0x9600 | ((fromAddrForDMA >> 8) & 0xff);
        //*((vu16*) VDP_CTRL_PORT) = 0x9700 | ((fromAddrForDMA >> 16) & 0x7f);

        waitHCounter(154);
        // SCANLINE 7 starts here (few pixels ahead)

        turnOffVDP(116);
        *((vu32*) VDP_CTRL_PORT) = palCmdForDMA; // trigger DMA transfer
        turnOnVDP(116);

        currGradPtr += 4 * setGradColorForText; // advance 4 colors if condition is met
        //titan256cPalsPtr += TITAN_256C_COLORS_PER_STRIP; // advance to next strip's palettes (if pointer wasn't incremented previously)
        palIdx ^= TITAN_256C_COLORS_PER_STRIP; // cycles between 0 and 32
        //palIdx = palIdx == 0 ? 32 : 0;
        //palIdx = (palIdx + 32) & 63; // (palIdx + 32) % 64 => x mod y = x & (y-1) when y is power of 2
MEMORY_BARRIER();
        waitHCounter(154);
        // SCANLINE 8 starts here (few pixels ahead)

        // set GB color before setup DMA
        *((vu32*) VDP_CTRL_PORT) = 0xC0000000; // VDP_WRITE_CRAM_ADDR(0): write to CRAM color index 0 multiplied by 2
        *((vu16*) VDP_DATA_PORT) = bgColor4;

        if (vcounter >= applyBlackPalPosY)
            titan256cPalsPtr = (u16*) palette_black;

        vcounter += TITAN_256C_STRIP_HEIGHT;
        waitVCounterReg(vcounter);
    }
}

static u16 textColor = 0xE00; // initial color is the same than titanCharsGradientColors[0]
static u8 textColorIndex = 0;
static s8 textColorDirection = 1;

void vertIntOnDrawTextCallback () {
    // reset text color to white
    PAL_setColor(FONT_COLOR_TILE_INDEX, 0xEEE);

    if ((vtimer % 4) == 0) {
        // get next text color value for the HInt
        textColor = *(getTitanCharsGradientColors() + textColorIndex);
        textColorIndex += textColorDirection;
        if (textColorIndex == TITAN_CHARS_GRADIENT_MAX_COLORS) {
            textColorIndex -= 2;
            textColorDirection = -1;
        }
        else if (textColorIndex == 0) {
            textColorDirection = 1;
        }
    }
}

HINTERRUPT_CALLBACK horizIntOnDrawTextCallback () {
    waitHCounter(154);
    PAL_setColor(FONT_COLOR_TILE_INDEX, textColor);
}