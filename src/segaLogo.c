/**
 * Credits go to 
 * https://github.com/a-dietrich/SEGA-Genesis-Demos/tree/main/sega-logo
*/
#include <types.h>
#include <vdp.h>
#include <vdp_bg.h>
#include <vdp_tile.h>
#include <memory.h>
#include <joy.h>
#include <sys.h>
#include <sprite_eng.h>
#include "segaLogo.h"
#include "utils.h"
#include "logo_res.h"

#define LOGO_MAX_FRAMES_EFFECT 100

// *****************************************************************************
//
//  Global variables
//
// *****************************************************************************

static u16 tileIndexNext = TILE_USER_INDEX;

static s16* vScrollBuffer_ptr;
static s16* vScrollTileBuffer_ptr;

// *****************************************************************************
//
//  Subroutines
//
// *****************************************************************************

static void fillVerticalScrollTile (VDPPlane plan, u16 tile, s16 value, u16 len, TransferMethod tm)
{
    memsetU16((u16*)vScrollTileBuffer_ptr, (u16)value, len & 0x1F);
    VDP_setVerticalScrollTile(plan, tile, vScrollTileBuffer_ptr, len, tm);
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

static void cyclePaletteUp(u16 *pal)
{
    // Next causes compilation error: <artificial>:(.text+0xc2ce): undefined reference to `memmove'
    /*u16 temp = pal[index];
    // Shift colors up starting from the index
    for (u16 i = index; i < count - 1; i++) {
        pal[i] = pal[i + 1];
    }
    // Place the original first color at the end
    pal[count - 1] = pal[0];
    // Shift colors up from 0 to index
    for (u16 i = 0; i < index; i++) {
        pal[i] = pal[i + 1];
    }
    // Place the original first color at the beginning
    pal[index] = temp;*/

    // Shift colors up starting from the beginning
    u16 temp = pal[0];
    pal[0] = pal[1];
    pal[1] = pal[2];
    pal[2] = pal[3];
    pal[3] = pal[4];
    pal[4] = pal[5];
    pal[5] = pal[6];
    pal[6] = pal[7];
    pal[7] = pal[8];
    pal[8] = pal[9];
    pal[9] = pal[10];
    pal[10] = pal[11];
    pal[11] = pal[12];
    pal[12] = pal[13];
    pal[13] = pal[14];
    pal[14] = pal[15];
    pal[15] = pal[16];
    pal[16] = pal[17];
    pal[17] = pal[18];
    pal[18] = pal[19];
    pal[19] = pal[20];
    pal[20] = pal[21];
    pal[21] = pal[22];
    pal[22] = pal[23];
    pal[23] = pal[24];
    pal[24] = pal[25];
    pal[25] = temp;
}

// -------------------------------------------------------------------------
//  Interrupt handlers
// -------------------------------------------------------------------------

static u8 vcounterManual = 0;

static void VIntHandler ()
{
    vcounterManual = 0;
}

static HINTERRUPT_CALLBACK HIntLogoHandler ()
{
    vu16 *data = 0; // place holder to help inline asm use an Ax register
    vu32 *ctrl = 0; // place holder to help inline asm use an Ax register

    // Check if we are outside the logo
    const u16 y = vcounterManual++ - 96;
    if (y >= 32)
    {
        // Prime control and data ports
        __asm volatile (
            "   lea     0xC00004,%0\n"      // Load the Effective Address of the memory location 0xC00004 (the VDP_CTRL_PORT) into the %0 register.
            "   move.w  #0x8F04,(%0)\n"     // Set auto-increment to 4. '()' specifies memory indirection or dereferencing.
            "   move.l  #0x401E0010,(%0)\n" // Set VSRAM address (two-tile column 7).
            : "=>a" (ctrl)                  // Output operands. Ctrl will be placed in a general-purpose register: a0, a1, etc.
            : "0" (ctrl)                    // Input operands. 0 indicates that the input operand should use the same operand number as the corresponding output operand (in this case, operand 0 corresponds to =>a)
            : "cc", "memory"
        );
        return;
    }

    // Change vscroll values if inside the logo
    s16 *addr = vScrollBuffer_ptr+(y<<3);
    __asm volatile (
        "   lea     0xC00000,%1\n"      // Load the Effective Address of the memory location 0xC00000 (the VDP_DATA_PORT) into the %1 register.
        "   move.l  (%0)+,(%1)\n"
        "   move.l  (%0)+,(%1)\n"
        "   move.l  (%0)+,(%1)\n"
        "   lea     4(%1),%2\n"         // Immediate offset of 4 bytes from %1 memory address loaded into the %2 register.
        "   move.l  #0x401E0010,(%2)\n" // Reset VSRAM address (two-tile column 7).
        : "=>a" (addr), "=>a" (data), "=>a" (ctrl)   // = is write-only, > applies change immediately, addr is param 0, data is 1, ctr is 2.
        : "0" (addr), "1" (data), "2" (ctrl)         // addr goes in same location than param 0, etc.
        : "cc", "memory"
    );
}

static FORCE_INLINE u16 attr (u16 pal)
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

    // Blue/cyan cycling gradient
    // u16 colorsLogoGradient[26] = {
    //     0x200, 0x400, 0x600, 0x800, 0xA00, 0xC00, 0xE00, 0xE20, 0xE40, 0xE60, 0xE80, 0xEA0, 0xEC0, 0xEE0, // blue to cyan
    //     0xEC0, 0xEA0, 0xE80, 0xE60, 0xE40, 0xE20, 0xE00, 0xC00, 0xA00, 0x800, 0x600, 0x400 // cyan to blue
    // };
    u16* colorsLogoGradient = (u16*) MEM_alloc(26 * sizeof(u16));
    u16* p = colorsLogoGradient;
    *p++ = 0x200; *p++ = 0x400; *p++ = 0x600; *p++ = 0x800; *p++ = 0xA00; *p++ = 0xC00; *p++ = 0xE00;
    *p++ = 0xE20; *p++ = 0xE40; *p++ = 0xE60; *p++ = 0xE80; *p++ = 0xEA0; *p++ = 0xEC0; *p++ = 0xEE0;
    *p++ = 0xEC0; *p++ = 0xEA0; *p++ = 0xE80; *p++ = 0xE60; *p++ = 0xE40; *p++ = 0xE20; *p++ = 0xE00;
    *p++ = 0xC00; *p++ = 0xA00; *p++ = 0x800; *p++ = 0x600; *p++ = 0x400;

    // u16 paletteBuffer[4*16] = {0};
    // s16 vScrollBuffer[32*16] = {0};
    u16* paletteBuffer = (u16*) MEM_alloc(4*16 * sizeof(u16));
    s16* vScrollBuffer = (s16*) MEM_alloc(32*16 * sizeof(s16));
    vScrollBuffer_ptr = vScrollBuffer;
    vScrollTileBuffer_ptr = (s16*) MEM_alloc(32 * sizeof(s16));

    // Setup VDP
    VDP_setPlaneSize(64, 32, TRUE);
    VDP_setScrollingMode(HSCROLL_TILE, VSCROLL_COLUMN);
    PAL_setPalette(PAL0, palette_black, CPU);

    // Draw SEGA logo
    {
        // Fill top plane with solid black tiles (tile index 1 its a SGDK system tile)
        VDP_fillTileMapRect(BG_A, attr(PAL0)|1, 0, 0, screenWidth/8, screenHeight/8);

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
        SYS_setVBlankCallback(VIntHandler);
        VDP_setHIntCounter(0); // each scanline
        VDP_setHInterrupt(TRUE);
        SYS_setHIntCallback(HIntLogoHandler);
    SYS_enableInts();

    //
    // Display loop
    //

    s16 highlightBarY = -14; // starting from top
    s16 frameCtr = 0;

    for (;;)
    {
        const u16 joyState = JOY_readJoypad(JOY_1);

        if (joyState & BUTTON_START)
            break;
        if (frameCtr > LOGO_MAX_FRAMES_EFFECT) {
            // Leave some time the last animation frame in the screen
            waitMs_(400);
            break;
        }

        if (frameCtr % 2 == 0) {
            // Cycle base palette
            cyclePaletteUp(colorsLogoGradient);

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
        SYS_setVBlankCallback(NULL);
        VDP_setHInterrupt(FALSE);
        SYS_setHIntCallback(NULL);
        SYS_setVIntCallback(NULL);
    SYS_enableInts();

    // Fade out all graphics to Black since all palettes are used by the logo effects
    PAL_fadeOutAll(16, FALSE);
    SYS_doVBlankProcess();

    // restore planes and settings
    VDP_clearPlane(BG_A, TRUE);
    VDP_clearPlane(BG_B, TRUE);
    VDP_setScrollingMode(HSCROLL_PLANE, VSCROLL_PLANE);
    fillVerticalScrollTile(BG_A, 7, 0, 6, DMA); // restore VScroll Tile values
    fillVerticalScrollTile(BG_B, 7, 0, 6, DMA); // restore VScroll Tile values

    // restore SGDK's default palettes
    PAL_setPalette(PAL0, palette_grey, DMA);
    PAL_setPalette(PAL1, palette_red, DMA);
    PAL_setPalette(PAL2, palette_green, DMA);
    PAL_setPalette(PAL3, palette_blue, DMA);

    MEM_free(colorsLogoGradient);
    MEM_free(paletteBuffer);
    MEM_free(vScrollBuffer);
    MEM_free(vScrollTileBuffer_ptr);
}