#include <types.h>
#include <vdp_bg.h>
#include <sprite_eng.h>
#include <tools.h>
#include <memory.h>
#include "titan256c.h"

static u16* unpackedData;

void unpackPalettes () {
    unpackedData = (u16*) MEM_alloc(TITAN_256C_STRIPS_COUNT * TITAN_256C_COLORS_PER_STRIP * sizeof(u16));
    if (palTitanRGB.compression != COMPRESSION_NONE) {
        // No need to use FAR_SAFE() macro here because palette data is always stored near
        unpack(palTitanRGB.compression, (u8*) palTitanRGB.data, (u8*) unpackedData);
    }
    else {
        // Copy the palette data. It is modified later on fading out effect. No FAR_SAFE() needed here, palette data is always at near region.
        const u16 size = (TITAN_256C_STRIPS_COUNT * TITAN_256C_COLORS_PER_STRIP) * 2;
        memcpy((u8*) unpackedData, palTitanRGB.data, size);
    }
}

void freePalettes () {
    if (palTitanRGB.compression != COMPRESSION_NONE) {
        MEM_free((void*) unpackedData);
        unpackedData = NULL;
    }
}

FORCE_INLINE u16* getUnpackedPtr () {
    return unpackedData;
}

FORCE_INLINE void set2FirstStripsPals () {
    PAL_setColors(0, unpackedData, TITAN_256C_COLORS_PER_STRIP * 2, DMA_QUEUE);
}

static const u16 titanCharsGradientColors[TITAN_CHARS_GRADIENT_MAX_COLORS] = {
    0xE00, 0xE02, 0xE04, 0xE06, 0xE08, 0xE0A, 0xE0C, // this values are in VDP format: BGR
    0xE0E, 0xC0E, 0xA0E, 0x80E, 0x60E, 0x40E, 0x20E, 
    0x00E, 0x02E, 0x04E, 0x06E, 0x08E, 0x0AE, 0x0CE, 
    0x0EE, 0x0EC, 0x0EA, 0x0E8, 0x0E6, 0x0E4, 0x0E2,
    0x0E0, 0x2E0, 0x4E0, 0x6E0, 0x8E0, 0xAE0, 0xCE0, 
    0xEE0, 0xEC0, 0xEA0, 0xE80, 0xE60, 0xE40, 0xE20
};

static u16 gradColorsBuffer[TITAN_CURR_GRADIENT_ELEMS];

static u16 titanCharsCycleCnt = 0;

void updateCharsGradientColors () {
    // Strips [21,25] (0 based) renders the letters using transparent color, and we want to use a gradient scrolling over time.
    // So 5 strips. However each strip is 8 scanlines meaning we need to render every 4 scanlines inside the HInt.

    s16 shift = divu(titanCharsCycleCnt, TITAN_CHARS_GRADIENT_SCROLL_FREQ); // advance ramp color every N frames
    u16* gradPtr = gradColorsBuffer;
    for (s16 i=0; i < TITAN_CURR_GRADIENT_ELEMS; ++i) {
        u16 colorIdx = modu(TITAN_CHARS_GRADIENT_MAX_COLORS + i - shift, TITAN_CHARS_GRADIENT_MAX_COLORS);
        u16 c = *(titanCharsGradientColors + colorIdx);
        *gradPtr++ = c;
    }

    ++titanCharsCycleCnt;
    if (titanCharsCycleCnt == TITAN_CHARS_GRADIENT_SCROLL_FREQ * TITAN_CHARS_GRADIENT_MAX_COLORS)
        titanCharsCycleCnt = 0;
}

FORCE_INLINE u16* getGradientColorsBuffer () {
    return (u16*) gradColorsBuffer;
}

void NO_INLINE fadingStepToBlack (s16 currFadingStrip, u16 cycle) {
    // No need to fade strip when currFadingStrip is inside the stepping
    if (cycle > 0 && currFadingStrip < (FADE_OUT_COLOR_STEPS / FADE_OUT_STRIPS_SPLIT_CYCLES)) {
        return;
    }

    currFadingStrip = max(0, currFadingStrip - cycle * (FADE_OUT_COLOR_STEPS / FADE_OUT_STRIPS_SPLIT_CYCLES));

    // No need to fade strips ahead the current limit
    if (currFadingStrip >= TITAN_256C_STRIPS_COUNT){
        return;
    }

    u16 limit = max(0, currFadingStrip - (FADE_OUT_COLOR_STEPS / FADE_OUT_STRIPS_SPLIT_CYCLES) + 1);

    for (; currFadingStrip >= limit; --currFadingStrip) {
        // fade the palettes of stripN
        u16* palsPtr = unpackedData + currFadingStrip * TITAN_256C_COLORS_PER_STRIP;
        for (s16 i=TITAN_256C_COLORS_PER_STRIP; i > 0; --i) {
            u16 s = *palsPtr;
            if (s == 0) {
                ++palsPtr;
                continue;
            };
            // VDP u16 color is represented as next:
            // F  E  D  C  B  A  9  8  7  6  5  4  3  2  1  0
            // -  -  -  -  B2 B1 B0 -  G2 G1 G0 -  R2 R1 R0 -
            u16 rs = s & VDPPALETTE_REDMASK;
            u16 gs = s & VDPPALETTE_GREENMASK;
            u16 bs = s & VDPPALETTE_BLUEMASK;
            if (rs == 0x004) rs = 0; // this done due to missing step on last cycle value due to non exactly division
            else if (rs != 0) rs -= 0x002;
            if (gs == 0x040) gs = 0; // this done due to missing step on last cycle value due to non exactly division
            else if (gs != 0) gs -= 0x020;
            if (bs == 0x400) bs = 0; // this done due to missing step on last cycle value due to non exactly division
            else if (bs != 0) bs -= 0x200;
            *palsPtr++ = rs | gs | bs;
        }
        // fade the gradient colors
        // if (currFadingStrip >= 21 && currFadingStrip <= 25) {
        //     u16* rampBufPtr = gradColorsBuffer;
        //     for (u16 i=0; i < TITAN_CURR_GRADIENT_ELEMS; ++i) {
        //         *rampBufPtr++ = 0x0;
        //     }
        // }
    }
}

// void NO_INLINE fadingStepToBlack (const u16 currFadingStrip) {
//     for (s16 stripN=currFadingStrip; stripN >= max(currFadingStrip - FADE_OUT_STEPS, 0); --stripN) {
//         // fade the palettes of stripN
//         u16* palsPtr = unpackedData + stripN * TITAN_256C_COLORS_PER_STRIP;
//         for (s16 i=TITAN_256C_COLORS_PER_STRIP; i > 0; --i) {
//             const u16 s = *palsPtr;
//             if (s == 0) {
//                 ++palsPtr;
//                 continue;
//             };
//             const u16 rs = (s & VDPPALETTE_REDMASK) >> VDPPALETTE_REDSFT;
//             const u16 gs = (s & VDPPALETTE_GREENMASK) >> VDPPALETTE_GREENSFT;
//             const u16 bs = (s & VDPPALETTE_BLUEMASK) >> VDPPALETTE_BLUESFT;
//             const u16 rd = rs == 0 ? 0 : divu(rs * (FADE_OUT_STEPS - 1), FADE_OUT_STEPS) << VDPPALETTE_REDSFT;
//             const u16 gd = gs == 0 ? 0 : divu(gs * (FADE_OUT_STEPS - 1), FADE_OUT_STEPS) << VDPPALETTE_GREENSFT;
//             const u16 bd = bs == 0 ? 0 : divu(bs * (FADE_OUT_STEPS - 1), FADE_OUT_STEPS) << VDPPALETTE_BLUESFT;
//             const u16 d = rd | gd | bd;
//             *palsPtr++ = d;
//             //kprintf("%X %X %X %X", bd, gd, rd, d);
//             kprintf("0x%03X 0x%03X", s, d);
//         }
//     }
// }
