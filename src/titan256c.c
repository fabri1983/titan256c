#include <types.h>
#include <vdp_bg.h>
#include <sprite_eng.h>
#include <tools.h>
#include <memory.h>
#include "titan256c.h"

u16 titanCharsGradientColors[TITAN_CHARS_GRADIENT_MAX_COLORS] = {
    0xE00, 0xE02, 0xE04, 0xE06, 0xE08, 0xE0A, 0xE0C, // this values are in VDP format: BGR
    0xE0E, 0xC0E, 0xA0E, 0x80E, 0x60E, 0x40E, 0x20E, 
    0x00E, 0x02E, 0x04E, 0x06E, 0x08E, 0x0AE, 0x0CE, 
    0x0EE, 0x0EC, 0x0EA, 0x0E8, 0x0E6, 0x0E4, 0x0E2,
    0x0E0, 0x2E0, 0x4E0, 0x6E0, 0x8E0, 0xAE0, 0xCE0, 
    0xEE0, 0xEC0, 0xEA0, 0xE80, 0xE60, 0xE40, 0xE20
};

void resetGradientColors () {
    u16* p = titanCharsGradientColors;
    *p++ = 0xE00; *p++ = 0xE02; *p++ = 0xE04; *p++ = 0xE06; *p++ = 0xE08; *p++ = 0xE0A; *p++ = 0xE0C; // this values are in VDP format: BGR
    *p++ = 0xE0E; *p++ = 0xC0E; *p++ = 0xA0E; *p++ = 0x80E; *p++ = 0x60E; *p++ = 0x40E; *p++ = 0x20E; 
    *p++ = 0x00E; *p++ = 0x02E; *p++ = 0x04E; *p++ = 0x06E; *p++ = 0x08E; *p++ = 0x0AE; *p++ = 0x0CE; 
    *p++ = 0x0EE; *p++ = 0x0EC; *p++ = 0x0EA; *p++ = 0x0E8; *p++ = 0x0E6; *p++ = 0x0E4; *p++ = 0x0E2;
    *p++ = 0x0E0; *p++ = 0x2E0; *p++ = 0x4E0; *p++ = 0x6E0; *p++ = 0x8E0; *p++ = 0xAE0; *p++ = 0xCE0; 
    *p++ = 0xEE0; *p++ = 0xEC0; *p++ = 0xEA0; *p++ = 0xE80; *p++ = 0xE60; *p++ = 0xE40; *p++ = 0xE20;
}

static u16* unpackedData;

void NO_INLINE fadingStepToBlack (const u16 stripN) {
    u16 colorDst = 0x0;
    // fade the palettes
    u16* palsPtr = unpackedData + stripN*32;
    for (u16 i=4; i > 0; --i) {
        *palsPtr++ = colorDst;
        *palsPtr++ = colorDst;
        *palsPtr++ = colorDst;
        *palsPtr++ = colorDst;
        *palsPtr++ = colorDst;
        *palsPtr++ = colorDst;
        *palsPtr++ = colorDst;
        *palsPtr++ = colorDst;
    }
    // fade the gradient colors
    if (stripN >= 21 && stripN <= 25) {
        u16* rampPtr = titanCharsGradientColors;
        for (u16 i=0; i < TITAN_CHARS_GRADIENT_MAX_COLORS; ++i) {
            *rampPtr++ = colorDst;
        }
    }
}

void unpackPalettes (const Palette32AllStrips* pals32) {
    if (pals32->compression != COMPRESSION_NONE) {
        unpackedData = (u16*) MEM_alloc(TITAN_256C_STRIPS_COUNT * TITAN_256C_COLORS_PER_STRIP * sizeof(u16));
        // No need to use FAR_SAFE() macro here because palette data is always stored near
        unpack(pals32->compression, (u8*) pals32->data, (u8*) unpackedData);
    }
    else {
        unpackedData = pals32->data;
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