#include <types.h>
#include <vdp.h>
#include <pal.h>
#include <sys.h>
#include <tools.h> // KLog methods
#include "hvInterrupts.h"
#include "titan256c.h"

// static u8 reg01; // Holds current VDP register 1 whole value (it holds other bits than VDP ON/OFF status)

// static FORCE_INLINE void copyReg01 () {
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
 * Wait until HCounter 0xC00009 reaches nth position (in fact (n*2)th pixel since the VDP counts by 2)
*/
static FORCE_INLINE void waitHCounter (u16 n) {
    ASM_STATEMENT volatile (
        " .LoopHC%=:"
        "    cmpi.b  %[hcLimit], 0xC00009.l;"  // we only interested in comparing byte since n won't be > 160 for our practical cases
        "    blo     .LoopHC%=;"
        :
        : [hcLimit] "i" (n)
        : "cc" // Clobbers: condition codes
    );
}

/**
 * Shannon Birt version. Loads HCounter into register for faster comparison.
*/
static FORCE_INLINE void waitHCounter_ShannonBirt (u16 n) {
    // vu32* regA=0; // placeholder used to indicate the use of an A register
    // vu16* regD=0; // placeholder used to indicate the use of a D register
    // ASM_STATEMENT volatile (
    //     " move.l    #0xC00009, %0;"     // Load H Counter address into an A register
    //     " move.w    #158, %1;"          // Load 158 into a D register
    //     " .hcLimit%=:" 
    //     "    cmp.b   (%0), %1;"         // Compares H Counter with 158. '()' specifies memory indirection or dereferencing
    //     "    blo     .hcLimit%=;"
    //     : "+a" (regA), "+d" (regD)
    //     :
    //     : "cc"                          // Clobbers: condition codes
    // );
    ASM_STATEMENT volatile (
        " move.l    #0xC00009, %%a0;"    // Load H Counter address into a0 register
        " move.w    %[hcLimit], %%d1;"   // Load hcLimit into d1 register
        " .hcLimit%=:" 
        "    cmp.b   (%%a0), %%d1;"      // Compares H Counter with hcLimit. '()' specifies memory indirection or dereferencing
        "    blo     .hcLimit%=;"
        :
        : [hcLimit] "i" (n)
        : "a0", "d1", "cc"               // Clobbers: register d1, address register a0, condition codes
    );
}

/**
 * Wait until VCounter 0xC00008 reaches nth scanline position. Only valid with constants.
*/
static FORCE_INLINE void waitVCounter (u16 n) {
    ASM_STATEMENT volatile (
        " .LoopVC%=:"
        "    CMPI.w  %[vcLimit], 0xC00008.l;"  // we only interested in comparing word since n won't be > 255 (then shifted right) for our practical cases
        "    BLO     .LoopVC%=;"
        :
        : [vcLimit] "i" (n << 8) // (n << 8) | 0xFF
        : "cc" // Clobbers: condition codes
    );
}

/**
 * Wait until VCounter 0xC00008 reaches nth scanline position.
*/
static FORCE_INLINE void waitVCounterReg (u16 n) {
    ASM_STATEMENT volatile (
        " .LoopVC%=:"
        "    CMP.w   0xC00008.l, %0;"  // we only interested in comparing word since n won't be > 255 (then shifted right) for our practical cases
        "    BHI     .LoopVC%=;"
        :
        : "r" (n << 8) // (n << 8) | 0xFF
        : "cc" // Clobbers: condition codes
    );
}

/**
 * \brief
 * \param len How many colors to move.
 * \param fromAddr Must be >> 1.
*/
static void NO_INLINE setupDMAForPals (u16 len, u32 fromAddr) {
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
static u16 applyBlackPalPos = TITAN_256C_HEIGHT - 1;
static u16 vcounterManual;

void blackCurrentGradientPtr () {
    currGradPtr = (u16*) palette_black;
}

static FORCE_INLINE void varsSetup () {
    VDP_setAutoInc(2); // Needed for DMA of colors in u32 type, and it seems is neeed for CPU too (had some black screen flickering if not set)
    u16 stripN = min(TITAN_256C_HEIGHT/TITAN_256C_STRIP_HEIGHT - 1, getYPosFalling() / TITAN_256C_STRIP_HEIGHT + 2);
    titan256cPalsPtr = getUnpackedPtr() + stripN * TITAN_256C_COLORS_PER_STRIP;
    applyBlackPalPos = (TITAN_256C_HEIGHT - 1) - getYPosFalling();
    palIdx = ((getYPosFalling() / TITAN_256C_STRIP_HEIGHT) % 2) == 0 ? 0 : 32;
    currGradPtr = getGradientColorsBuffer();
}

void vertIntOnTitan256cCallback_HIntEveryN () {
    varsSetup();
    vcounterManual = TITAN_256C_STRIP_HEIGHT - 1;
}

void vertIntOnTitan256cCallback_HIntOneTime () {
    varsSetup();
    VDP_setHIntCounter(0);
}

HINTERRUPT_CALLBACK horizIntOnTitan256cCallback_CPU_EveryN () {

    bool setGradColorForText = vcounterManual >= TITAN_256C_TEXT_STARTING_STRIP * TITAN_256C_STRIP_HEIGHT 
        && vcounterManual <= TITAN_256C_TEXT_ENDING_STRIP * TITAN_256C_TEXT_ENDING_STRIP;

    if (vcounterManual >= applyBlackPalPos) {
        titan256cPalsPtr = (u16*) palette_black;
        setGradColorForText = FALSE;
    }

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

    //u32 offset = 0x80000; // in case we use cmdAddress += offset
    u32 cmdAddress;
    u16 bgColor;
    u32 colors2_A, colors2_B, colors2_C, colors2_D;

    // Value under current conditions is always 116
    //u8 reg01 = VDP_getReg(0x01); // Holds current VDP register 1 value (it holds other bits than VDP ON/OFF status)

    colors2_A = *((u32*) (titan256cPalsPtr + 0)); // 2 colors
    colors2_B = *((u32*) (titan256cPalsPtr + 2)); // next 2 colors
    cmdAddress = palIdx == 0 ? 0xC0000000 : 0xC0400000;
    bgColor = *(currGradPtr + 0) * setGradColorForText;
    waitHCounter(145);
    turnOffVDP(116);
    *((vu32*) VDP_CTRL_PORT) = cmdAddress;
    *((vu32*) VDP_DATA_PORT) = colors2_A;
    *((vu32*) VDP_DATA_PORT) = colors2_B;
        *((vu32*) VDP_CTRL_PORT) = 0xC0000000; // VDP_WRITE_CRAM_ADDR(0): write to CRAM color index 0 multiplied by 2
        *((vu16*) VDP_DATA_PORT) = bgColor;
    turnOnVDP(116);

    colors2_A = *((u32*) (titan256cPalsPtr + 4)); // 2 colors
    colors2_B = *((u32*) (titan256cPalsPtr + 6)); // next 2 colors
    colors2_C = *((u32*) (titan256cPalsPtr + 8)); // next2 colors
    colors2_D = *((u32*) (titan256cPalsPtr + 10)); // next 2 colors
    cmdAddress = palIdx == 0 ? 0xC0080000 : 0xC0480000;
    waitHCounter(145);
    turnOffVDP(116);
    *((vu32*) VDP_CTRL_PORT) = cmdAddress;
    *((vu32*) VDP_DATA_PORT) = colors2_A;
    *((vu32*) VDP_DATA_PORT) = colors2_B;
    *((vu32*) VDP_DATA_PORT) = colors2_C;
    *((vu32*) VDP_DATA_PORT) = colors2_D;
    turnOnVDP(116);

    colors2_A = *((u32*) (titan256cPalsPtr + 12)); // 2 colors
    colors2_B = *((u32*) (titan256cPalsPtr + 14)); // next 2 colors
    cmdAddress = palIdx == 0 ? 0xC0180000 : 0xC0580000;
    bgColor = *(currGradPtr + 1) * setGradColorForText;
    waitHCounter(145);
    turnOffVDP(116);
    *((vu32*) VDP_CTRL_PORT) = cmdAddress;
    *((vu32*) VDP_DATA_PORT) = colors2_A;
    *((vu32*) VDP_DATA_PORT) = colors2_B;
        *((vu32*) VDP_CTRL_PORT) = 0xC0000000; // VDP_WRITE_CRAM_ADDR(0): write to CRAM color index 0 multiplied by 2
        *((vu16*) VDP_DATA_PORT) = bgColor;
    turnOnVDP(116);

    colors2_A = *((u32*) (titan256cPalsPtr + 16)); // 2 colors
    colors2_B = *((u32*) (titan256cPalsPtr + 18)); // next 2 colors
    colors2_C = *((u32*) (titan256cPalsPtr + 20)); // next colors
    colors2_D = *((u32*) (titan256cPalsPtr + 22)); // next 2 colors
    cmdAddress = palIdx == 0 ? 0xC0200000 : 0xC0600000;
    waitHCounter(145);
    turnOffVDP(116);
    *((vu32*) VDP_CTRL_PORT) = cmdAddress;
    *((vu32*) VDP_DATA_PORT) = colors2_A;
    *((vu32*) VDP_DATA_PORT) = colors2_B;
    *((vu32*) VDP_DATA_PORT) = colors2_C;
    *((vu32*) VDP_DATA_PORT) = colors2_D;
    turnOnVDP(116);

    colors2_A = *((u32*) (titan256cPalsPtr + 24)); // 2 colors
    colors2_B = *((u32*) (titan256cPalsPtr + 26)); // next 2 colors
    cmdAddress = palIdx == 0 ? 0xC0300000 : 0xC0700000;
    bgColor = *(currGradPtr + 2) * setGradColorForText;
    waitHCounter(145);
    turnOffVDP(116);
    *((vu32*) VDP_CTRL_PORT) = cmdAddress;
    *((vu32*) VDP_DATA_PORT) = colors2_A;
    *((vu32*) VDP_DATA_PORT) = colors2_B;
        *((vu32*) VDP_CTRL_PORT) = 0xC0000000; // VDP_WRITE_CRAM_ADDR(0): write to CRAM color index 0 multiplied by 2
        *((vu16*) VDP_DATA_PORT) = bgColor;
    turnOnVDP(116);

    colors2_A = *((u32*) (titan256cPalsPtr + 28)); // 2 colors
    colors2_B = *((u32*) (titan256cPalsPtr + 30)); // next 2 colors
    cmdAddress = palIdx == 0 ? 0xC0380000 : 0xC0780000;
    waitHCounter(145);
    turnOffVDP(116);
    *((vu32*) VDP_CTRL_PORT) = cmdAddress;
    *((vu32*) VDP_DATA_PORT) = colors2_A;
    *((vu32*) VDP_DATA_PORT) = colors2_B;
    turnOnVDP(116);

    bgColor = *(currGradPtr + 3) * setGradColorForText;
    waitHCounter(150);
    turnOffVDP(116);
        *((vu32*) VDP_CTRL_PORT) = 0xC0000000; // VDP_WRITE_CRAM_ADDR(0): write to CRAM color index 0 multiplied by 2
        *((vu16*) VDP_DATA_PORT) = bgColor;
    turnOnVDP(116);

    vcounterManual += TITAN_256C_STRIP_HEIGHT;
    currGradPtr += 4 * setGradColorForText; // advance 3 colors if condition is met
    titan256cPalsPtr += 32; // advance to next strip's palette
    palIdx ^= 32; // cycles between 0 and 32
    //palIdx = palIdx == 0 ? 32 : 0;
    //palIdx = (palIdx + 32) & 63; // (palIdx + 32) % 64 => x mod y = x & (y-1) when y is power of 2
}

HINTERRUPT_CALLBACK horizIntOnTitan256cCallback_DMA_EveryN () {

    bool setGradColorForText = vcounterManual >= TITAN_256C_TEXT_STARTING_STRIP * TITAN_256C_STRIP_HEIGHT 
        && vcounterManual <= TITAN_256C_TEXT_ENDING_STRIP * TITAN_256C_TEXT_ENDING_STRIP;

    if (vcounterManual >= applyBlackPalPos) {
        titan256cPalsPtr = (u16*) palette_black;
        setGradColorForText = FALSE;
    }

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
    u16 bgColor;

    // Value under current conditions is always 116
    //u8 reg01 = VDP_getReg(0x01); // Holds current VDP register 1 value (it holds other bits than VDP ON/OFF status)

    fromAddrForDMA = (u32) titan256cPalsPtr >> 1;
    titan256cPalsPtr += TITAN_256C_COLORS_PER_STRIP/3;
    palCmdForDMA = palIdx == 0 ? 0xC0000080 : 0xC0400080;
    bgColor = *(currGradPtr + 0) * setGradColorForText;
    waitHCounter(152);
        // set GB color before setup DMA
        *((vu32*) VDP_CTRL_PORT) = 0xC0000000; // VDP_WRITE_CRAM_ADDR(0): write to CRAM color index 0 multiplied by 2
        *((vu16*) VDP_DATA_PORT) = bgColor;
    setupDMAForPals(TITAN_256C_COLORS_PER_STRIP/3, fromAddrForDMA);
    waitHCounter(152);
    turnOffVDP(116);
    *((vu32*) VDP_CTRL_PORT) = palCmdForDMA; // trigger DMA transfer
    turnOnVDP(116);

    fromAddrForDMA = (u32) titan256cPalsPtr >> 1;
    titan256cPalsPtr += TITAN_256C_COLORS_PER_STRIP/3;
    palCmdForDMA = palIdx == 0 ? 0xC0140080 : 0xC0540080;
    bgColor = *(currGradPtr + 1) * setGradColorForText;
    waitHCounter(152);
        // set GB color before setup DMA
        *((vu32*) VDP_CTRL_PORT) = 0xC0000000; // VDP_WRITE_CRAM_ADDR(0): write to CRAM color index 0 multiplied by 2
        *((vu16*) VDP_DATA_PORT) = bgColor;
    setupDMAForPals(TITAN_256C_COLORS_PER_STRIP/3, fromAddrForDMA);
    waitHCounter(152);
    turnOffVDP(116);
    *((vu32*) VDP_CTRL_PORT) = palCmdForDMA; // trigger DMA transfer
    turnOnVDP(116);

    fromAddrForDMA = (u32) titan256cPalsPtr >> 1;
    titan256cPalsPtr += TITAN_256C_COLORS_PER_STRIP/3 + TITAN_256C_COLORS_PER_STRIP_REMINDER(3);
    palCmdForDMA = palIdx == 0 ? 0xC0280080 : 0xC0680080;
    bgColor = *(currGradPtr + 2) * setGradColorForText;
    waitHCounter(152);
        // set GB color before setup DMA
        *((vu32*) VDP_CTRL_PORT) = 0xC0000000; // VDP_WRITE_CRAM_ADDR(0): write to CRAM color index 0 multiplied by 2
        *((vu16*) VDP_DATA_PORT) = bgColor;
    setupDMAForPals(TITAN_256C_COLORS_PER_STRIP/3 + TITAN_256C_COLORS_PER_STRIP_REMINDER(3), fromAddrForDMA);
    waitHCounter(152);
    turnOffVDP(116);
    *((vu32*) VDP_CTRL_PORT) = palCmdForDMA; // trigger DMA transfer
    turnOnVDP(116);

    vcounterManual += TITAN_256C_STRIP_HEIGHT;
    currGradPtr += 3 * setGradColorForText; // advance 3 colors if condition is met
    //titan256cPalsPtr += TITAN_256C_COLORS_PER_STRIP; // advance to next strip's palettes (if pointer wasn't incremented previously)
    palIdx ^= 32; // cycles between 0 and 32
    //palIdx = palIdx == 0 ? 32 : 0;
    //palIdx = (palIdx + 32) & 63; // (palIdx + 32) % 64 => x mod y = x & (y-1) when y is power of 2
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

    // Simulates waiting the first call to Simulates VDP_setHIntCounter(TITAN_256C_STRIP_HEIGHT - 1)
    waitVCounter(TITAN_256C_STRIP_HEIGHT - 1 - 1);
    // at this point GET_VCOUNTER value is TITAN_256C_STRIP_HEIGHT - 1
    u16 vcounter = TITAN_256C_STRIP_HEIGHT - 1;

    for (;;) {
        // SCANLINE 1 starts right here

        if (vcounter > (TITAN_256C_HEIGHT - TITAN_256C_STRIP_HEIGHT - 1)) { // valid for NTSC and PAL since titan image size is fixed
            return;
        }

        bool setGradColorForText = vcounter >= TITAN_256C_TEXT_STARTING_STRIP * TITAN_256C_STRIP_HEIGHT 
            && vcounter <= TITAN_256C_TEXT_ENDING_STRIP * TITAN_256C_STRIP_HEIGHT;

        if (vcounter >= applyBlackPalPos) {
            titan256cPalsPtr = (u16*) palette_black;
            setGradColorForText = FALSE;
        }

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
        u16 bgColor;

        fromAddrForDMA = (u32) titan256cPalsPtr >> 1;
        titan256cPalsPtr += TITAN_256C_COLORS_PER_STRIP/3;
        palCmdForDMA = palIdx == 0 ? 0xC0000080 : 0xC0400080;
        bgColor = *(currGradPtr + 0) * setGradColorForText;

        waitHCounter(152);
        // SCANLINE 2 starts (few pixels ahead)
        
            // set GB color before setup DMA
            *((vu32*) VDP_CTRL_PORT) = 0xC0000000; // VDP_WRITE_CRAM_ADDR(0): write to CRAM color index 0 multiplied by 2
            *((vu16*) VDP_DATA_PORT) = bgColor;
        setupDMAForPals(TITAN_256C_COLORS_PER_STRIP/3, fromAddrForDMA);

        waitHCounter(152);
        // SCANLINE 3 starts (few pixels ahead)

        turnOffVDP(116);
        *((vu32*) VDP_CTRL_PORT) = palCmdForDMA; // trigger DMA transfer
        turnOnVDP(116);

        fromAddrForDMA = (u32) titan256cPalsPtr >> 1;
        titan256cPalsPtr += TITAN_256C_COLORS_PER_STRIP/3;
        palCmdForDMA = palIdx == 0 ? 0xC0140080 : 0xC0540080;
        bgColor = *(currGradPtr + 1) * setGradColorForText;

        waitHCounter(152);
        // SCANLINE 4 starts (few pixels ahead)

            // set GB color before setup DMA
            *((vu32*) VDP_CTRL_PORT) = 0xC0000000; // VDP_WRITE_CRAM_ADDR(0): write to CRAM color index 0 multiplied by 2
            *((vu16*) VDP_DATA_PORT) = bgColor;
        setupDMAForPals(TITAN_256C_COLORS_PER_STRIP/3, fromAddrForDMA);

        waitHCounter(152);
        // SCANLINE 5 starts (few pixels ahead)

        turnOffVDP(116);
        *((vu32*) VDP_CTRL_PORT) = palCmdForDMA; // trigger DMA transfer
        turnOnVDP(116);

        fromAddrForDMA = (u32) titan256cPalsPtr >> 1;
        titan256cPalsPtr += TITAN_256C_COLORS_PER_STRIP/3 + TITAN_256C_COLORS_PER_STRIP_REMINDER(3);
        palCmdForDMA = palIdx == 0 ? 0xC0280080 : 0xC0680080;
        bgColor = *(currGradPtr + 2) * setGradColorForText;

        waitHCounter(152);
        // SCANLINE 6 starts (few pixels ahead)

            // set GB color before setup DMA
            *((vu32*) VDP_CTRL_PORT) = 0xC0000000; // VDP_WRITE_CRAM_ADDR(0): write to CRAM color index 0 multiplied by 2
            *((vu16*) VDP_DATA_PORT) = bgColor;
        setupDMAForPals(TITAN_256C_COLORS_PER_STRIP/3 + TITAN_256C_COLORS_PER_STRIP_REMINDER(3), fromAddrForDMA);

        waitHCounter(152);
        // SCANLINE 7 starts (few pixels ahead)

        turnOffVDP(116);
        *((vu32*) VDP_CTRL_PORT) = palCmdForDMA; // trigger DMA transfer
        turnOnVDP(116);

        currGradPtr += 3 * setGradColorForText; // advance 3 colors if condition is met
        //titan256cPalsPtr += TITAN_256C_COLORS_PER_STRIP; // advance to next strip's palettes (if pointer wasn't incremented previously)
        palIdx ^= 32; // cycles between 0 and 32
        //palIdx = palIdx == 0 ? 32 : 0;
        //palIdx = (palIdx + 32) & 63; // (palIdx + 32) % 64 => x mod y = x & (y-1) when y is power of 2

        vcounter += TITAN_256C_STRIP_HEIGHT;
        waitVCounterReg(vcounter);
    }
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