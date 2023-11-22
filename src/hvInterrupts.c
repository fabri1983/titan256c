#include <types.h>
#include <vdp.h>
#include <pal.h>
#include <sys.h>
#include <tools.h> // KLog methods
#include "hvInterrupts.h"
#include "titan256c.h"

#ifdef __GNUC__
#define ASM_STATEMENT __asm__
#elif defined(_MSC_VER)
#define ASM_STATEMENT __asm
#endif

// static u8 reg01; // Holds current VDP register 1 value (it holds other bits than VDP ON/OFF status)

// static FORCE_INLINE void copyReg01 () {
//     reg01 = VDP_getReg(0x01);
// }

static FORCE_INLINE void turnOffVDP (u8 reg01) {
    //reg01 &= ~0x40;
    //*(vu16*) VDP_CTRL_PORT = 0x8100 | reg01;
    *(vu16*) VDP_CTRL_PORT = 0x8100 | (reg01 & ~0x40);
}

static FORCE_INLINE void turnOnVDP (u8 reg01) {
    //reg01 |= 0x40;
    //*(vu16*) VDP_CTRL_PORT = 0x8100 | reg01;
    *(vu16*) VDP_CTRL_PORT = 0x8100 | (reg01 | 0x40);
}

/**
 * Wait until HCounter 0xC00009 reaches n position (n*2 th pixel, the VDP counts by 2)
*/
static FORCE_INLINE void waitHCounter (u16 n) {
    ASM_STATEMENT volatile (
        " .LoopHBlank%=:"
        "    cmpi.b  %[hbLimit], 0xC00009.l;"
        "    blo     .LoopHBlank%=;"
        :
        : [hbLimit] "i" (n)
        : "cc" // Clobbers: condition codes
    );
}

/**
 * Shannon Birt version (seems slower when tested on Exodus because black lines appears earlier)
*/
static FORCE_INLINE void waitHCounter_other (u16 n) {
    // vu32* regA=0; // placeholder used to indicate the use of an A register
    // vu16* regD=0; // placeholder used to indicate the use of a D register
    // ASM_STATEMENT volatile (
    //     " move.l    #0xC00009, %0;"     // Load H Counter address into an A register
    //     " move.w    #158, %1;"          // Load 158 into a D register
    //     " .LoopHBlank%=:" 
    //     "    cmp.b   (%0), %1;"         // Compares H Counter with 158. '()' specifies memory indirection or dereferencing
    //     "    bhs     .LoopHBlank%=;"
    //     : "+a" (regA), "+d" (regD)
    //     :
    // );
    ASM_STATEMENT volatile (
        " move.l    #0xC00009, %%a0;"    // Load H Counter address into a0 register
        " move.w    %[hbLimit], %%d1;"   // Load hbLimit into d1 register
        " .LoopHBlank%=:" 
        "    cmp.b   (%%a0), %%d1;"      // Compares H Counter with hbLimit. '()' specifies memory indirection or dereferencing
        "    blo     .LoopHBlank%=;"
        :
        : [hbLimit] "i" (n)
        : "a0", "d1"                     // Clobbers: register d1, address register a0
    );
}

/**
 * \brief
 * \param len How many colors to move.
 * \param fromAddr Must be >> 1.
*/
static FORCE_INLINE void setupDMAForPals (u16 len, u32 fromAddr) {
    // Uncomment if you previously change it to 1 (CPU access to VRAM is 1 byte lenght, and 2 bytes length for CRAM and VSRAM)
    //VDP_setAutoInc(2);

    vu16* palDmaPtr = (vu16*) VDP_CTRL_PORT;

    // Setup DMA length (in word here)
    *palDmaPtr = 0x9300 + (len & 0xff);
    *palDmaPtr = 0x9400 + ((len >> 8) & 0xff);

    // Setup DMA address
    *palDmaPtr = 0x9500 + (fromAddr & 0xff);
    *palDmaPtr = 0x9600 + ((fromAddr >> 8) & 0xff);
    *palDmaPtr = 0x9700 + ((fromAddr >> 16) & 0x7f);
}

static u16* titan256cPalsPtr; // 1st and 2nd strip's palette are loaded at the beginning of the display loop, so this ptr starts at 3rd strip
static u16 palIdx; // 3rd strip starts with palettes at [PAL0,PAL1]
static u16* currGradPtr;

void vertIntOnTitan256cCallback_HIntEveryN () {
    titan256cPalsPtr = getUnpackedPtr() + 2 * TITAN_256C_COLORS_PER_STRIP;
    palIdx = 0;
}

void vertIntOnTitan256cCallback_HIntOneTime () {
    VDP_setHIntCounter(0);
    titan256cPalsPtr = getUnpackedPtr() + 2 * TITAN_256C_COLORS_PER_STRIP;
    palIdx = 0;
}

HINTERRUPT_CALLBACK horizIntOnTitan256cCallback_EveryN_CPU () {

    u16 vcounter = GET_VCOUNTER;
    // At strip 21 (0 based) titan chars appears. Chars color will be updated every 4 scanlines inside HInt.
    bool setPalBGGradientForChars = vcounter >= 21*TITAN_256C_STRIP_HEIGHT && vcounter <= 25*TITAN_256C_STRIP_HEIGHT;

    /*
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

    u32 cmdBaseAddress = (palIdx == 0) ? 0xC0000000 : 0xC0400000;
    const u32 offset = 0x80000;
    u32 cmdAddress = cmdBaseAddress;
    u16 gradColor_A = *currGradPtr * setPalBGGradientForChars;
    u16 gradColor_B = *(currGradPtr + 1) * setPalBGGradientForChars;
    u32 colors2_A;
    u32 colors2_B;

    palIdx ^= 32; // cycles between 0 and 32
    //palIdx = palIdx == 0 ? 32 : 0;
    //palIdx = (palIdx + 32) & 63; // (palIdx + 32) % 64 => x mod y = x & (y-1) when y is power of 2

    currGradPtr += setPalBGGradientForChars * 2; // advance 2 colors if condition is met

    //VDP_setAutoInc(2); // needed for moving of colors in u32 type (but likely is already set)

    u8 reg01 = VDP_getReg(0x01); // Holds current VDP register 1 value (it holds other bits than VDP ON/OFF status)

    colors2_A = *((u32*) (titan256cPalsPtr + 0)); // 2 colors
    colors2_B = *((u32*) (titan256cPalsPtr + 2)); // next 2 colors
    waitHCounter(157);
    turnOffVDP(reg01);
    *((vu32*) VDP_CTRL_PORT) = cmdAddress;
    *((vu32*) VDP_DATA_PORT) = colors2_A;
    *((vu32*) VDP_DATA_PORT) = colors2_B;
        *((vu32*) VDP_CTRL_PORT) = 0xC0000000; // VDP_WRITE_CRAM_ADDR(0): write to CRAM color index 0 multiplied by 2
        *((vu16*) VDP_DATA_PORT) = gradColor_A;
    turnOnVDP(reg01);

    colors2_A = *((u32*) (titan256cPalsPtr + 4)); // 2 colors
    colors2_B = *((u32*) (titan256cPalsPtr + 6)); // next 2 colors
    cmdAddress += offset;
    waitHCounter(157);
    turnOffVDP(reg01);
    *((vu32*) VDP_CTRL_PORT) = cmdAddress;
    *((vu32*) VDP_DATA_PORT) = colors2_A;
    *((vu32*) VDP_DATA_PORT) = colors2_B;
    turnOnVDP(reg01);

    colors2_A = *((u32*) (titan256cPalsPtr + 8)); // 2 colors
    colors2_B = *((u32*) (titan256cPalsPtr + 10)); // next 2 colors
    cmdAddress += offset;
    waitHCounter(157);
    turnOffVDP(reg01);
    *((vu32*) VDP_CTRL_PORT) = cmdAddress;
    *((vu32*) VDP_DATA_PORT) = colors2_A;
    *((vu32*) VDP_DATA_PORT) = colors2_B;
    turnOnVDP(reg01);

    colors2_A = *((u32*) (titan256cPalsPtr + 12)); // 2 colors
    colors2_B = *((u32*) (titan256cPalsPtr + 14)); // next 2 colors
    cmdAddress += offset;
    waitHCounter(157);
    turnOffVDP(reg01);
    *((vu32*) VDP_CTRL_PORT) = cmdAddress;
    *((vu32*) VDP_DATA_PORT) = colors2_A;
    *((vu32*) VDP_DATA_PORT) = colors2_B;
    turnOnVDP(reg01);

    colors2_A = *((u32*) (titan256cPalsPtr + 16)); // 2 colors
    colors2_B = *((u32*) (titan256cPalsPtr + 18)); // next 2 colors
    cmdAddress += offset;
    waitHCounter(157);
    turnOffVDP(reg01);
    *((vu32*) VDP_CTRL_PORT) = cmdAddress;
    *((vu32*) VDP_DATA_PORT) = colors2_A;
    *((vu32*) VDP_DATA_PORT) = colors2_B;
    turnOnVDP(reg01);

    colors2_A = *((u32*) (titan256cPalsPtr + 20)); // 2 colors
    colors2_B = *((u32*) (titan256cPalsPtr + 22)); // next 2 colors
    cmdAddress += offset;
    waitHCounter(157);
    turnOffVDP(reg01);
    *((vu32*) VDP_CTRL_PORT) = cmdAddress;
    *((vu32*) VDP_DATA_PORT) = colors2_A;
    *((vu32*) VDP_DATA_PORT) = colors2_B;
        *((vu32*) VDP_CTRL_PORT) = 0xC0000000; // VDP_WRITE_CRAM_ADDR(0): write to CRAM color index 0 multiplied by 2
        *((vu16*) VDP_DATA_PORT) = gradColor_B;
    turnOnVDP(reg01);

    colors2_A = *((u32*) (titan256cPalsPtr + 24)); // 2 colors
    colors2_B = *((u32*) (titan256cPalsPtr + 26)); // next 2 colors
    cmdAddress += offset;
    waitHCounter(157);
    turnOffVDP(reg01);
    *((vu32*) VDP_CTRL_PORT) = cmdAddress;
    *((vu32*) VDP_DATA_PORT) = colors2_A;
    *((vu32*) VDP_DATA_PORT) = colors2_B;
    turnOnVDP(reg01);

    colors2_A = *((u32*) (titan256cPalsPtr + 28)); // 2 colors
    colors2_B = *((u32*) (titan256cPalsPtr + 30)); // next 2 colors
    cmdAddress += offset;
    waitHCounter(157);
    turnOffVDP(reg01);
    *((vu32*) VDP_CTRL_PORT) = cmdAddress;
    *((vu32*) VDP_DATA_PORT) = colors2_A;
    *((vu32*) VDP_DATA_PORT) = colors2_B;
    turnOnVDP(reg01);

    titan256cPalsPtr += 32; // advance to next strip's palette
}

HINTERRUPT_CALLBACK horizIntOnTitan256cCallback_EveryN_DMA () {

    // At strip 21 (0 based) titan chars appears. Chars color will be updated every 4 scanlines inside HInt.
    u16 vcounter = GET_VCOUNTER;
    bool setPalBGGradientForChars = vcounter >= 21*TITAN_256C_STRIP_HEIGHT && vcounter <= 25*TITAN_256C_STRIP_HEIGHT;

    /*
        u32 palCmdForDMA_A = VDP_DMA_CRAM_ADDR((u32)palIdx * 2);
        u32 palCmdForDMA_B = VDP_DMA_CRAM_ADDR(((u32)palIdx + TITAN_256C_COLORS_PER_STRIP/3) * 2);
        u32 palCmdForDMA_C = VDP_DMA_CRAM_ADDR(((u32)palIdx + 2*(TITAN_256C_COLORS_PER_STRIP/3)) * 2);
        cmd     palIdx = 0      palIdx = 32
        A       0xC0000080      0xC0400080
        B       0xC0140080      0xC0540080
        C       0xC0280080      0xC0680080
    */

    u32 palCmdForDMA;
    u32 fromAddrForDMA;
    u16 gradColor_A = *currGradPtr;
    u16 gradColor_B = *(currGradPtr + 1);
    currGradPtr += setPalBGGradientForChars * 2; // advance 2 colors if condition is met

    VDP_setAutoInc(2); // Needed for DMA of colors in u32 type

    u8 reg01 = VDP_getReg(0x01); // Holds current VDP register 1 value (it holds other bits than VDP ON/OFF status)

    fromAddrForDMA = (u32) titan256cPalsPtr >> 1;
    titan256cPalsPtr += TITAN_256C_COLORS_PER_STRIP/3;
    palCmdForDMA = palIdx == 0 ? 0xC0000080 : 0xC0400080;

    waitHCounter(132);

    // set GB color before setup DMA
    if (setPalBGGradientForChars) {
        *((vu32*) VDP_CTRL_PORT) = 0xC0000000; // VDP_WRITE_CRAM_ADDR(0): write to CRAM color index 0 multiplied by 2
        *((vu16*) VDP_DATA_PORT) = gradColor_A;
    }
    setupDMAForPals(TITAN_256C_COLORS_PER_STRIP/3, fromAddrForDMA);

    waitHCounter(158);

    turnOffVDP(reg01);
    *((vu32*) VDP_CTRL_PORT) = palCmdForDMA; // trigger DMA transfer
    turnOnVDP(reg01);

    fromAddrForDMA = (u32) titan256cPalsPtr >> 1;
    titan256cPalsPtr += TITAN_256C_COLORS_PER_STRIP/3;
    palCmdForDMA = palIdx == 0 ? 0xC0140080 : 0xC0540080;

    waitHCounter(134);

    setupDMAForPals(TITAN_256C_COLORS_PER_STRIP/3, fromAddrForDMA);

    waitHCounter(158);

    turnOffVDP(reg01);
    *((vu32*) VDP_CTRL_PORT) = palCmdForDMA; // trigger DMA transfer
    turnOnVDP(reg01);

    fromAddrForDMA = (u32) titan256cPalsPtr >> 1;
    titan256cPalsPtr += TITAN_256C_COLORS_PER_STRIP/3 + TITAN_256C_COLORS_PER_STRIP_REMINDER(3);
    palCmdForDMA = palIdx == 0 ? 0xC0280080 : 0xC0680080;

    waitHCounter(132);

    // set GB color before setup DMA
    if (setPalBGGradientForChars) {
        *((vu32*) VDP_CTRL_PORT) = 0xC0000000; // VDP_WRITE_CRAM_ADDR(0): write to CRAM color index 0 multiplied by 2
        *((vu16*) VDP_DATA_PORT) = gradColor_B;
    }
    setupDMAForPals(TITAN_256C_COLORS_PER_STRIP/3 + TITAN_256C_COLORS_PER_STRIP_REMINDER(3), fromAddrForDMA);

    waitHCounter(158);

    turnOffVDP(reg01);
    *((vu32*) VDP_CTRL_PORT) = palCmdForDMA; // trigger DMA transfer
    turnOnVDP(reg01);

    //titan256cPalsPtr += TITAN_256C_COLORS_PER_STRIP; // advance to next strip's palettes (if pointer wasn't incremented previously)
    palIdx ^= 32; // cycles between 0 and 32
    //palIdx = palIdx == 0 ? 32 : 0;
    //palIdx = (palIdx + 32) & 63; // (palIdx + 32) % 64 => x mod y = x & (y-1) when y is power of 2
}

HINTERRUPT_CALLBACK horizIntOnTitan256cCallback_OneTime_DMA () {
    VDP_setHIntCounter(255);
    VDP_setAutoInc(2); // Needed for DMA of colors in u32 type

    // Giving the nature of SGDK hint callback it will be invoked several times until the VDP_setHIntCounter(255) makes effect.
    // So we need to cancel out the first invocations and only keep the 
    u16 vcounter = GET_VCOUNTER;
    if (vcounter > 1) {
        return;
    }

    // Simulates waiting the first call to Simulates VDP_setHIntCounter(TITAN_256C_STRIP_HEIGHT-1)
    while (vcounter < TITAN_256C_STRIP_HEIGHT) {
        vcounter = GET_VCOUNTER;
    }

    // Simulates VDP_setHIntCounter(TITAN_256C_STRIP_HEIGHT-1)
    do {
        // SCANLINE 1 starts right here

        // At strip 21 (0 based) titan chars appears. Chars color will be updated every 4 scanlines inside HInt.
        bool setPalBGGradientForChars = vcounter >= 21*TITAN_256C_STRIP_HEIGHT && vcounter <= 25*TITAN_256C_STRIP_HEIGHT;

        /*
            u32 palCmdForDMA_A = VDP_DMA_CRAM_ADDR((u32)palIdx * 2);
            u32 palCmdForDMA_B = VDP_DMA_CRAM_ADDR(((u32)palIdx + TITAN_256C_COLORS_PER_STRIP/3) * 2);
            u32 palCmdForDMA_C = VDP_DMA_CRAM_ADDR(((u32)palIdx + 2*(TITAN_256C_COLORS_PER_STRIP/3)) * 2);
            cmd     palIdx = 0      palIdx = 32
            A       0xC0000080      0xC0400080
            B       0xC0140080      0xC0540080
            C       0xC0280080      0xC0680080
        */

        u32 palCmdForDMA;
        u32 fromAddrForDMA;
        u16 gradColor_A = *currGradPtr;
        u16 gradColor_B = *(currGradPtr + 1);
        currGradPtr += setPalBGGradientForChars * 2; // advance 2 colors if condition is met

        u8 reg01 = VDP_getReg(0x01); // Holds current VDP register 1 value (it holds other bits than VDP ON/OFF status)

        fromAddrForDMA = (u32) titan256cPalsPtr >> 1;
        titan256cPalsPtr += TITAN_256C_COLORS_PER_STRIP/3;
        palCmdForDMA = palIdx == 0 ? 0xC0000080 : 0xC0400080;
        
        waitHCounter(132);
        // SCANLINE 2 starts (few pixels ahead)
        
        // set GB color before setup DMA
        if (setPalBGGradientForChars) {
            *((vu32*) VDP_CTRL_PORT) = 0xC0000000; // VDP_WRITE_CRAM_ADDR(0): write to CRAM color index 0 multiplied by 2
            *((vu16*) VDP_DATA_PORT) = gradColor_A;
        }
        setupDMAForPals(TITAN_256C_COLORS_PER_STRIP/3, fromAddrForDMA);

        waitHCounter(158);
        // SCANLINE 3 starts (few pixels ahead)

        turnOffVDP(reg01);
        *((vu32*) VDP_CTRL_PORT) = palCmdForDMA; // trigger DMA transfer
        turnOnVDP(reg01);

        fromAddrForDMA = (u32) titan256cPalsPtr >> 1;
        titan256cPalsPtr += TITAN_256C_COLORS_PER_STRIP/3;
        palCmdForDMA = palIdx == 0 ? 0xC0140080 : 0xC0540080;
        
        waitHCounter(134);
        // SCANLINE 4 starts (few pixels ahead)

        setupDMAForPals(TITAN_256C_COLORS_PER_STRIP/3, fromAddrForDMA);

        waitHCounter(158);
        // SCANLINE 5 starts (few pixels ahead)

        turnOffVDP(reg01);
        *((vu32*) VDP_CTRL_PORT) = palCmdForDMA; // trigger DMA transfer
        turnOnVDP(reg01);

        fromAddrForDMA = (u32) titan256cPalsPtr >> 1;
        titan256cPalsPtr += TITAN_256C_COLORS_PER_STRIP/3 + TITAN_256C_COLORS_PER_STRIP_REMINDER(3);
        palCmdForDMA = palIdx == 0 ? 0xC0280080 : 0xC0680080;

        waitHCounter(132);
        // SCANLINE 6 starts (few pixels ahead)

        // set GB color before setup DMA
        if (setPalBGGradientForChars) {
            *((vu32*) VDP_CTRL_PORT) = 0xC0000000; // VDP_WRITE_CRAM_ADDR(0): write to CRAM color index 0 multiplied by 2
            *((vu16*) VDP_DATA_PORT) = gradColor_B;
        }
        setupDMAForPals(TITAN_256C_COLORS_PER_STRIP/3 + TITAN_256C_COLORS_PER_STRIP_REMINDER(3), fromAddrForDMA);

        waitHCounter(158);
        // SCANLINE 7 starts (few pixels ahead)

        turnOffVDP(reg01);
        *((vu32*) VDP_CTRL_PORT) = palCmdForDMA; // trigger DMA transfer
        turnOnVDP(reg01);

        //titan256cPalsPtr += TITAN_256C_COLORS_PER_STRIP; // advance to next strip's palettes (if pointer wasn't incremented previously)
        palIdx ^= 32; // cycles between 0 and 32
        //palIdx = palIdx == 0 ? 32 : 0;
        //palIdx = (palIdx + 32) & 63; // (palIdx + 32) % 64 => x mod y = x & (y-1) when y is power of 2

        waitHCounter(160);
        while ( (*((vu16*)VDP_CTRL_PORT) & 4) == 0 ) {;} // Waits until HBlank flag becomes true
        // SCANLINE 8 starts (few pixels ahead)

        waitHCounter(159);
        while ( (*((vu16*)VDP_CTRL_PORT) & 4) == 0 ) {;} // Waits until HBlank flag becomes true
        // SCANLINE 9 starts (few pixels ahead) which is wrapped around to SCANLINE 1

        waitHCounter(132);

        vcounter = GET_VCOUNTER;
    }
    while (vcounter < 224-TITAN_256C_STRIP_HEIGHT-1); // NTSC: 224. PAL: 240.
}

static u16 titanCharsCycleCnt = 0;
static u16 titanCurrentGradient[TITAN_CURR_GRADIENT_ELEMS];

void beforeVBlankProcOnTitan256c_DMA_QUEUE () {
    // Strips [21,25] (0 based) renders the letters using transparent color, and we want to use a gradient scrolling over time.
    // So 5 strips. However each strip is 8 scanlines meaning we need to render every 4 scanlines inside the HInt.

    s16 shift = titanCharsCycleCnt / TITAN_CHARS_GRADIENT_SCROLL_FREQ; // every N frames
    currGradPtr = titanCurrentGradient;
    for (u16 i=0; i < TITAN_CURR_GRADIENT_ELEMS; ++i) {
        u16 c = titanCharsGradientColors[abs(TITAN_CHARS_GRADIENT_MAX_COLORS + i - shift) % TITAN_CHARS_GRADIENT_MAX_COLORS];
        *currGradPtr++ = c;
    }
    currGradPtr = titanCurrentGradient; // reset pointer to first element

    ++titanCharsCycleCnt;
    if (titanCharsCycleCnt == TITAN_CHARS_GRADIENT_SCROLL_FREQ * TITAN_CHARS_GRADIENT_MAX_COLORS)
        titanCharsCycleCnt = 0;
}

void afterVBlankProcOnTitan256c_VDP_or_DMA () {
}

// void horizIntOnMainMenuCallback_ASM () {
//     u16 vcounter = GET_VCOUNTER;
//     u16 bgColorScanRange = (HELLO_TEXT_MAX_Y * 8 + 8); // adding +8 due to tile's height
//     u16 bgGradientColor = 0x666;//gradientColorsA[vcounter >> BG_GRADIENT_SHIFT_A];
//     u16 helloTextColor = 0xFFF;//textHello.color;

//     /* https://github.com/0xAX/linux-insides/blob/master/Theory/linux-theory-3.md
//     __asm__ [volatile] [goto] (AssemblerTemplate
//                         [ : OutputOperands ]
//                         [ : InputOperands  ]
//                         [ : Clobbers       ]
//                         [ : GotoLabels     ]);*/

    // VDP_setReg(u16 reg, u16 value)
    //     "move.w  %0, %%d0\n"  // put value into register d0
    //     "move.w  %1, %%d1\n"  // put reg into register d1
    //     "move.w  %%d1, %2\n"  // put register d1 content into VDP_CTRL_PORT
    //     "move.l  %3, %%a0\n"  // put VDP_DATA_PORT into register a0
    //     "move.w  %%d0, (%2, %%a0)\n" // put register d0 (value) content into a0 (VDP_CTRL_PORT)
    //     :
    //     : "g" (value), "g" (reg), "g" (VDP_DATA_PORT), "g" (VDP_CTRL_PORT)
    //     : "d0", "d1", "a0"

//     ASM_STATEMENT volatile (
//         " .loopWaitHBlank:"
//         "    move.w  %sr, %d0;"         // Move the contents of the SR register into d0
//         "    btst    #4, %d0;"          // Test the 4th bit of the SR register (horizontal blank flag)
//         "    beq     .loopWaitHBlank;"  // If the flag is not set then loop back
//         " .updBgGradientColor:"
//         "    moveq   #0, %%d0;"         // Set color index to update to 0
//         "    move.b  %(0), %0;"         // Load new color value for index 15 into register
//         "    move.w  %%d0, %%a0;"       // Move color index into address register
//         "    move.b  %0, (%%a0);"       // Store new color value at palette index
//         " .cmpVCounter:"                // Compare V Counter value to know current vertical scanline
//         "    move.w  %(1), %0;"         // Move V Counter value into register
//         "    cmpi.w  %(2), %0;"         // Compare counter value with bgScanRange
//         "    bhi.b   .updTextColor;"    // If V Counter is greater than bgScanRange, branch to .updAllTextColor label
//         " .updHelloTextColor:"
//         "    moveq   #15, %%d0;"        // Set color index to update to 15
//         "    move.b  %(3), %0;"         // Load new color value for index 15 into register
//         "    bra.b   .storeTextColor;"  // Jump to .storeTextColor
//         " .updAllTextColor:"
//         "    moveq   #15, %%d0;"        // Set color index to update to 15
//         "    move.b  %(4), %0;"         // Load new color value for index 15 into register
//         "    bra.b   .storeTextColor;"  // Jump to .storeTextColor
//         " .storeTextColor:"
//         "    move.w  %%d0, %%a0;"       // Move color index into address register
//         "    move.b  %0, (%%a0);"       // Store new color value at palette index
//         :                               // Output: none
//         : "0" (bgGradientColor), "1" (vcounter), "2" (bgColorScanRange), "3" (helloTextColor), "4" (ALL_TEXT_COLOR)   // Input operands
//         : "%d0", "%a0", "cc"            // Clobbers: register d0, address register a0, condition codes
//     );
// }