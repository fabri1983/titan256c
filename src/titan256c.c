#include <types.h>
#include <vdp_bg.h>
#include <sprite_eng.h>
#include <tools.h>
#include <memory.h>
#include "titan256c.h"

static u16* unpackedData;

void unpackPalettes (const Palette32AllStrips* pals32) {
    unpackedData = (u16*) MEM_alloc(TITAN_256C_STRIPS_COUNT * TITAN_256C_COLORS_PER_STRIP * sizeof(u16));
    if (pals32->compression != COMPRESSION_NONE) {
        // No need to use FAR_SAFE() macro here because palette data is always stored near
        unpack(pals32->compression, (u8*) pals32->data, (u8*) unpackedData);
    }
    else {
        // Copy the palette data. It is modified later on fading out effect. No FAR_SAFE() needed here, palette data is always at near region.
        const u16 size = (TITAN_256C_STRIPS_COUNT * TITAN_256C_COLORS_PER_STRIP) * 2;
        memcpy((u8*) unpackedData, pals32->data, size);
    }
}

void freePalettes (const Palette32AllStrips* pals32) {
    if (pals32->compression != COMPRESSION_NONE) {
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
        u16 colorIdx = modu(abs(TITAN_CHARS_GRADIENT_MAX_COLORS + i - shift), TITAN_CHARS_GRADIENT_MAX_COLORS);
        u16 c = titanCharsGradientColors[colorIdx];
        *gradPtr++ = c;
    }

    ++titanCharsCycleCnt;
    if (titanCharsCycleCnt == TITAN_CHARS_GRADIENT_SCROLL_FREQ * TITAN_CHARS_GRADIENT_MAX_COLORS)
        titanCharsCycleCnt = 0;
}

FORCE_INLINE u16* getGradientColorsBuffer () {
    return (u16*) gradColorsBuffer;
}

void NO_INLINE fadingStepToBlack (const u16 currFadingStrip) {
    for (s16 stripN=currFadingStrip; stripN >= max(currFadingStrip - FADE_OUT_STEPS, 0); --stripN) {
        // fade the palettes of stripN
        u16* palsPtr = unpackedData + stripN * TITAN_256C_COLORS_PER_STRIP;
        for (s16 i=TITAN_256C_COLORS_PER_STRIP; i > 0; --i) {
            // const u16 s = *palsPtr;
            // u16 d;
            // *palsPtr++ = d;
            *palsPtr++ = 0x0;
        }
        // fade the gradient colors
        // if (stripN >= 21 && stripN <= 25) {
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
//             const u16 rs = (s & VDPPALETTE_REDMASK) >> VDPPALETTE_REDSFT;
//             const u16 gs = (s & VDPPALETTE_GREENMASK) >> VDPPALETTE_GREENSFT;
//             const u16 bs = (s & VDPPALETTE_BLUEMASK) >> VDPPALETTE_BLUESFT;
//             const u16 rd = divu(rs * (FADE_OUT_STEPS - 1), FADE_OUT_STEPS) << VDPPALETTE_REDSFT;
//             const u16 gd = divu(gs * (FADE_OUT_STEPS - 1), FADE_OUT_STEPS) << VDPPALETTE_GREENSFT;
//             const u16 bd = divu(bs * (FADE_OUT_STEPS - 1), FADE_OUT_STEPS) << VDPPALETTE_BLUESFT;
//             const u16 d = rd | gd | bd;
//             *palsPtr++ = d;
//             //kprintf("%X %X %X %X", bd, gd, rd, d);
//             kprintf("0x%03X %d", d, s);
//         }
//     }
// }
