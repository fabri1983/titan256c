/**
 * Credits go to 
 * https://github.com/a-dietrich/SEGA-Genesis-Demos/tree/main/sega-logo
*/
#include <genesis.h>
#include "segaLogo.h"

#ifdef __GNUC__
#define INTERRUPT_ATTRIBUTE HINTERRUPT_CALLBACK
#else
#define INTERRUPT_ATTRIBUTE void
#endif

#ifdef __GNUC__
#define ASM_STATEMENT __asm__
#elif defined(_MSC_VER)
#define ASM_STATEMENT __asm
#endif

#define MAX_FRAMES_EFFECT 100

// *****************************************************************************
//
//  Global variables
//
// *****************************************************************************

static u16 tileIndexNext = TILE_USER_INDEX;

static u16 paletteBuffer[4*16];
static s16 vScrollBuffer[32*16];

static u16 colorsLogoGradient[26] =
{
    // Blue/cyan cycling gradient
    0x200, 0x400, 0x600, 0x800, 0xA00, 0xC00, 0xE00, 0xE20, 0xE40, 0xE60, 0xE80, 0xEA0, 0xEC0, 0xEE0, // blue to cyan
    0xEC0, 0xEA0, 0xE80, 0xE60, 0xE40, 0xE20, 0xE00, 0xC00, 0xA00, 0x800, 0x600, 0x400 // cyan to blue
};

// *****************************************************************************
//
//  Subroutines
//
// *****************************************************************************

static void fillVerticalScrollTile (VDPPlane plan, u16 tile, s16 value, u16 len, TransferMethod tm)
{
    s16 buffer[32];
    memsetU16((u16*)buffer, (u16)value, len & 0x1F);

    VDP_setVerticalScrollTile(plan, tile, buffer, len, tm);
}

// -----------------------------------------------------------------------------

static u16 drawImage (VDPPlane plane, const Image *image, u16 basetile, u16 x, u16 y, u16 loadpal, bool dma)
{
    const u16 tileIndex = tileIndexNext;
    basetile |= tileIndex;

    VDP_drawImageEx(plane, image, basetile, x, y, loadpal, dma);
    tileIndexNext += image->tileset->numTile;  

    return tileIndex;
}

// -----------------------------------------------------------------------------

static void changePaletteBrightness (u16 *dstPal, u16 *srcPal, s16 val, u16 count)
{
    for (u16 i=0; i<count; i++)
    {
        const u16 color = srcPal[i];

        s16 b = color & 0xF00;
        s16 g = color & 0x0F0;
        s16 r = color & 0x00F;

        const s16 db = val * 256;
        const s16 dg = val *  16;
        const s16 dr = val;

        b = CLAMP(b + db, 0, 0xF00);
        g = CLAMP(g + dg, 0, 0x0F0);
        r = CLAMP(r + dr, 0, 0x00F);

        dstPal[i] = b | g | r;
    }
}

// -----------------------------------------------------------------------------

static void cyclePaletteUp(u16 *pal, u16 index, u16 count)
{
    if (count < 2)
        return;

    u16 tmp = pal[index+count-1];
    for (u16 i=index+count-1, n=index; i>n; i--)
        pal[i] = pal[i-1];
    pal[index] = tmp;
}

// -------------------------------------------------------------------------
//  Interrupt handlers
// -------------------------------------------------------------------------

static INTERRUPT_ATTRIBUTE HIntLogoHandler ()
{
    vu16 *data;
    vu32 *ctrl;

    // Check if we are outside the logo
    const u16 y = GET_VCOUNTER - 96;
    if (y >= 32)
    {
        // Prime control and data ports
        ASM_STATEMENT volatile (
            "   lea     0xC00004,%0;"      // Load the Effective Address of the memory location 0xC00004 into the %0 register
            "   move.w  #0x8F04,(%0);"     // Set auto-increment to 4. '()' specifies memory indirection or dereferencing.
            "   move.l  #0x401E0010,(%0);" // Set VSRAM address (two-tile column 7)
            : "=>a" (ctrl)                 // Output operands. Ctrl will be placed in a general-purpose register: a0, a1, etc.
            : "0" (ctrl)                   // Input operands. 0 indicates that the input operand should use the same operand number as the corresponding output operand (in this case, operand 0 corresponds to =>a)
        );

        return;
    }

    // Change vscroll values if inside the logo
    s16 *addr = vScrollBuffer+(y<<3);
    ASM_STATEMENT volatile (
        "   lea     0xC00000,%1;"  // Load the Effective Address of the memory location 0xC00000 (the VDP_DATA_PORT) into the %1 register
        "   move.l  (%0)+,(%1);"
        "   move.l  (%0)+,(%1);"
        "   move.l  (%0)+,(%1);"
        "   lea     4(%1),%2;"    // Immediate offset of 4 bytes from %1 memory address loaded into the %2 register
        "   move.l  #0x401E0010,(%2);" // Reset VSRAM address (two-tile column 7)
        : "=>a" (addr), "=>a" (data), "=>a" (ctrl)
        : "0" (addr), "1" (data), "2" (ctrl)
    );
}

static inline u16 attr (u16 pal)
{
    return TILE_ATTR(pal, 0, FALSE, FALSE);
}

// *****************************************************************************
//
//  Main
//
// *****************************************************************************

void displaySegaLogo ()
{
    //
    // Initalization
    //

    // Setup VDP
    VDP_setPlaneSize(64, 32, TRUE);
    VDP_setScrollingMode(HSCROLL_TILE, VSCROLL_COLUMN);
    PAL_setPalette(PAL0, palette_black, CPU);

    // Draw SEGA logo
    {
        // Fill top plane with solid black tiles (tile index 1 seems to be a SGDK system tile)
        VDP_fillTileMapRect(BG_A, attr(PAL0)|1, 0, 0, 40, 28);

        // Draw logo outline and letters. There are two versions using different color registers.
        drawImage(BG_A, &imgSEGAOutline, attr(PAL0), 14, 12, FALSE, TRUE);
        const u16 tileIndexLogoGradient1 = drawImage(BG_B, &imgSEGAGradient1, attr(PAL0), 14,  0, FALSE, TRUE);
        const u16 tileIndexLogoGradient0 = drawImage(BG_B, &imgSEGAGradient0, attr(PAL1), 14,  4, FALSE, TRUE);

        // Copy letters using different palettes
        VDP_setTileMapEx(BG_B, imgSEGAGradient1.tilemap, attr(PAL1)|tileIndexLogoGradient1, 14,  8, 0, 0, 12, 4, DMA);
        VDP_setTileMapEx(BG_B, imgSEGAGradient0.tilemap, attr(PAL2)|tileIndexLogoGradient0, 14, 12, 0, 0, 12, 4, DMA);
        VDP_setTileMapEx(BG_B, imgSEGAGradient1.tilemap, attr(PAL2)|tileIndexLogoGradient1, 14, 16, 0, 0, 12, 4, DMA);
        VDP_setTileMapEx(BG_B, imgSEGAGradient0.tilemap, attr(PAL3)|tileIndexLogoGradient0, 14, 20, 0, 0, 12, 4, DMA);
        VDP_setTileMapEx(BG_B, imgSEGAGradient1.tilemap, attr(PAL3)|tileIndexLogoGradient1, 14, 24, 0, 0, 12, 4, DMA);  
    }

    // Setup vertical scroll values
    fillVerticalScrollTile(BG_A, 7,     0, 6, DMA); // Logo outline position
    fillVerticalScrollTile(BG_B, 7, -32*3, 6, DMA); // Backdrop (show normal colored letters)

    // Fade to logo outline palete
    PAL_fadeTo(1, 15, imgSEGAOutline.palette->data+1, 16, FALSE);

    // Setup interrupt handlers
    SYS_disableInts();
    {
        VDP_setHIntCounter(0); // each scanline
        VDP_setHInterrupt(TRUE);
        SYS_setHIntCallback(HIntLogoHandler);
    }
    SYS_enableInts();

    //
    // Display loop
    //

    s16 highlightBarY = -14; // starting from top
    s16 frameCtr = 0;

    while (1)
    {
        const u16 joyState = JOY_readJoypad(JOY_1);

        if (joyState & BUTTON_START || frameCtr > MAX_FRAMES_EFFECT)
            break;

        if (frameCtr % 2 == 0) {
            // Cycle base palette
            cyclePaletteUp(colorsLogoGradient, 0, 26);

            // Move highlight bar
            if (++highlightBarY >= 32+6)
                highlightBarY = -14; // reset to top
        }

        // Create brightened versions of the original palette.
        for (u16 i=0; i<7; i++)
            changePaletteBrightness(paletteBuffer+(i+1)*8+1, colorsLogoGradient, i*2, 7);

        // Reset vscroll values so that the logo with base colors is shown
        memsetU16((u16*)vScrollBuffer, -3*32, 32*8);

        // Compute vscroll values for highligh bar
        for (s16 line=1; line<=12; line++)
        {
            // The bar covers 6 two-tile columns
            for (u16 column=0; column<6; column++)
            {
                const s16 tilt       = column;                                   // shift per column
                const s16 brightness = (line >= 7) ? (13 - line) : line;         // brightness per line
                const s16 index      = (highlightBarY + line - tilt)*8 + column; // buffer index of vscroll value

                // Insert vscroll values for highlight bar
                if ((index >= 0) && (index < 32*8))
                    vScrollBuffer[index] = (brightness - 3) * 32;
            }
        }

        SYS_doVBlankProcess();

        // Update all palettes
        PAL_setColors(8, paletteBuffer+8, 7*8, CPU);

        ++frameCtr;
    }

    // Clean Int handlers
    SYS_disableInts();
    {
        VDP_setHInterrupt(FALSE);
        SYS_setHIntCallback(NULL);
        SYS_setVIntCallback(NULL);
    }
    SYS_enableInts();

    // Fade out all graphics to Black since all palettes are used by the logo effects
    PAL_fadeOutAll(16, FALSE);

    VDP_clearPlane(BG_A, TRUE);
    VDP_clearPlane(BG_B, TRUE);
    VDP_setScrollingMode(HSCROLL_PLANE, VSCROLL_PLANE);
    VDP_setHorizontalScroll(BG_A, 0);
    VDP_setHorizontalScroll(BG_B, 0);
}