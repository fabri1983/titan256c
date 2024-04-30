#include <types.h>
#include <vdp_bg.h>
#include <sprite_eng.h>
#include <tools.h>
#include <mapper.h>
#include <memory.h>
#include "titan256c.h"
#include "decomp/unpackSelector.h"

static TileSet* allocateTileSetInternal (VOID_OR_CHAR* adr) {
    TileSet *result = (TileSet*) adr;
    if (result != NULL) {
        result->compression = COMPRESSION_NONE;
        result->tiles = (u32*) (adr + sizeof(TileSet));
    }
    return result;
}

static TileSet* unpackTileSet_custom(const TileSet* src, TileSet *dest) {
    TileSet *result;
    if (dest) result = dest;
    else result = allocateTileSetInternal(MEM_alloc(src->numTile * 32 + sizeof(TileSet)));

    if (result != NULL) {
        result->numTile = src->numTile;
        result->compression = COMPRESSION_NONE;
        if (src->compression != COMPRESSION_NONE) {
            unpackSelector(src->compression, (u8*) FAR_SAFE(src->tiles, src->numTile * 32), (u8*) result->tiles, src->numTile * 32);
        }
        else if (src->tiles != result->tiles) {
            const u16 size = src->numTile * 32;
            memcpy((u8*) result->tiles, FAR_SAFE(src->tiles, size), size);
        }
    }

    return result;
}

void loadTitan256cTileSet (u16 currTileIndex) {
    TileSet* tileset = titanRGB.tileset;
    if (tileset->compression != COMPRESSION_NONE) {
        TileSet *unpacked = unpackTileSet_custom(tileset, NULL);
        VDP_loadTileData(unpacked->tiles, currTileIndex, unpacked->numTile, DMA);
        // be careful, we are releasing buffer here so DMA_QUEUE transfer isn't safe here, use DMA_QUEUE_COPY instead for safe operation
        MEM_free(unpacked);
    }
    else
        VDP_loadTileData(FAR_SAFE(tileset->tiles, tileset->numTile * 32), currTileIndex, tileset->numTile, DMA);
}

static TileMap* allocateTileMapInternal (VOID_OR_CHAR* adr) {
    TileMap *result = (TileMap*) adr;
    if (result != NULL) {
        result->compression = COMPRESSION_NONE;
        result->tilemap = (u16*) (adr + sizeof(TileMap));
    }
    return result;
}

static TileMap* unpackTileMap_custom(const TileMap *src, TileMap *dest) {
    TileMap *result;
    if (dest) result = dest;
    else result = allocateTileMapInternal(MEM_alloc((src->w * src->h * 2) + sizeof(TileMap)));

    if (result != NULL) {
        result->w = src->w;
        result->h = src->h;
        result->compression = COMPRESSION_NONE;
        if (src->compression != COMPRESSION_NONE)
            unpackSelector(src->compression, (u8*) FAR_SAFE(src->tilemap, (src->w * src->h) * 2), (u8*) result->tilemap, src->w * src->h * 2);
        else if (src->tilemap != result->tilemap) {
            const u16 size = (src->w * src->h) * 2;
            memcpy((u8*) result->tilemap, FAR_SAFE(src->tilemap, size), size);
        }
    }

    return result;
}

void loadTitan256cTileMap (VDPPlane plane, u16 currTileIndex) {
    u16 baseTileAttribs = TILE_ATTR_FULL(PAL0, 0, FALSE, FALSE, currTileIndex);
    const TileMap* tilemap = titanRGB.tilemap;

    const u32 offset = 0;//mulu(y, tilemap->w) + x;
    if (tilemap->compression != COMPRESSION_NONE) {
        // unpack first
        TileMap *unpacked = unpackTileMap_custom(tilemap, NULL);
        VDP_setTileMapDataRectEx(plane, unpacked->tilemap + offset, baseTileAttribs, 0, 0, TITAN_256C_WIDTH/8, TITAN_256C_HEIGHT/8, 
            TITAN_256C_WIDTH/8, DMA_QUEUE_COPY);
        // be careful, we are releasing buffer here so DMA_QUEUE transfer isn't safe here, use DMA_QUEUE_COPY instead for safe operation
        MEM_free(unpacked);
    }
    else
        VDP_setTileMapDataRectEx(plane, (u16*) FAR_SAFE(tilemap->tilemap + offset, (TITAN_256C_WIDTH/8) * (TITAN_256C_HEIGHT/8) * 2), 
            baseTileAttribs, 0, 0, TITAN_256C_WIDTH/8, TITAN_256C_HEIGHT/8, TITAN_256C_WIDTH/8, DMA_QUEUE_COPY);
}

static u16* palettesData;

void unpackPalettes () {
    palettesData = (u16*) MEM_alloc(TITAN_256C_STRIPS_COUNT * TITAN_256C_COLORS_PER_STRIP * sizeof(u16));
    if (palTitanRGB.compression != COMPRESSION_NONE) {
        // No FAR_SAFE() macro needed here. Palette data is always stored at near region.
        unpackSelector(palTitanRGB.compression, (u8*) palTitanRGB.data, (u8*) palettesData, TITAN_256C_STRIPS_COUNT * TITAN_256C_COLORS_PER_STRIP);
    }
    else {
        // Copy the palette data. It is modified later on fading out effect.
        const u16 size = (TITAN_256C_STRIPS_COUNT * TITAN_256C_COLORS_PER_STRIP) * 2;
        // No FAR_SAFE() macro needed here. Palette data is always stored at near region.
        memcpy((u8*) palettesData, palTitanRGB.data, size);
    }
}

void freePalettes () {
    MEM_free((void*) palettesData);
    palettesData = NULL;
}

FORCE_INLINE u16* getPalettesData () {
    return palettesData;
}

static u16 yPosFalling = 0;

FORCE_INLINE void setYPosFalling (u16 value) {
    yPosFalling = value;
}

FORCE_INLINE u16 getYPosFalling () {
    return yPosFalling;
}

FORCE_INLINE void enqueue2Pals (u16 startingScreenStrip) {
    // if current starting strip is at the top of the screen (0 based) we only need to load the palette of that strip
    if (startingScreenStrip >= (TITAN_256C_HEIGHT/TITAN_256C_STRIP_HEIGHT - 1))
        PAL_setColors(TITAN_256C_COLORS_PER_STRIP, palettesData + TITAN_256C_COLORS_PER_STRIP * (TITAN_256C_HEIGHT/TITAN_256C_STRIP_HEIGHT - 1), TITAN_256C_COLORS_PER_STRIP, DMA_QUEUE);
    // if current strip is even then next strip is odd, so we can just load PAL0,PAL1 followed by PAL2,PAL3
    else if ((startingScreenStrip % 2) == 0)
        PAL_setColors(0, palettesData + TITAN_256C_COLORS_PER_STRIP * startingScreenStrip, TITAN_256C_COLORS_PER_STRIP * 2, DMA_QUEUE);
    // if current strip is odd then its palettes go into PAL2,PAL3 and next strip's palettes go into PAL0,PAL1
    else {
        PAL_setColors(TITAN_256C_COLORS_PER_STRIP, palettesData + TITAN_256C_COLORS_PER_STRIP * startingScreenStrip, TITAN_256C_COLORS_PER_STRIP, DMA_QUEUE);
        PAL_setColors(0, palettesData + TITAN_256C_COLORS_PER_STRIP * (startingScreenStrip + 1), TITAN_256C_COLORS_PER_STRIP, DMA_QUEUE);
    }
}

// ramp color effect in VDP format: BGR
static const u16 titanCharsGradientColors[TITAN_TEXT_GRADIENT_MAX_COLORS + TITAN_TEXT_CURR_GRADIENT_ELEMS] = {
    0xE00, 0xE02, 0xE04, 0xE06, 0xE08, 0xE0A, 0xE0C,
    0xE0E, 0xC0E, 0xA0E, 0x80E, 0x60E, 0x40E, 0x20E, 
    0x00E, 0x02E, 0x04E, 0x06E, 0x08E, 0x0AE, 0x0CE, 
    0x0EE, 0x0EC, 0x0EA, 0x0E8, 0x0E6, 0x0E4, 0x0E2,
    0x0E0, 0x2E0, 0x4E0, 0x6E0, 0x8E0, 0xAE0, 0xCE0, 
    0xEE0, 0xEC0, 0xEA0, 0xE80, 0xE60, 0xE40, 0xE20,
    // next TITAN_TEXT_CURR_GRADIENT_ELEMS colors are repeated from the start of the array to help cycling the ramp colors without using mod operand
    0xE00, 0xE02, 0xE04, 0xE06, 0xE08, 0xE0A, 0xE0C,
    0xE0E, 0xC0E, 0xA0E, 0x80E, 0x60E, 0x40E, 0x20E, 
    0x00E, 0x02E
};

const u16* getTitanCharsGradientColors () {
    return titanCharsGradientColors;
}

static u16 gradColorsBuffer[TITAN_TEXT_CURR_GRADIENT_ELEMS];

FORCE_INLINE u16* getGradientColorsBuffer () {
    return gradColorsBuffer;
}

static u8 currFadingStrip = 0;

void setCurrentFadingStripForText (u8 currFadingStrip_) {
    currFadingStrip = currFadingStrip_;
}

void NO_INLINE setSphereTextColorsIntoTitanPalettes (const SpriteDefinition sprDef) {
    // The animation sprite expect color to be in PAL0, and we only use PAL0 at even strips
    u8 startingStrip = TITAN_SPHERE_TILEMAP_START_Y_POS - (TITAN_SPHERE_TILEMAP_START_Y_POS % 2);
    // Every other strip inside range [5,16] contains the color used by the sprite text surrounding the sphere.
    // at position 15th. So we are jumping every 2 pals in order to set its color.
    u16* palsPtr = palettesData + (startingStrip * TITAN_256C_COLORS_PER_STRIP);

    //u16 colorAt15 = sprDef.palette->data[14];
    u16 colorAt16 = sprDef.palette->data[15];

    for (u8 i=(TITAN_SPHERE_TILEMAP_HEIGHT+1)/2 + + (TITAN_SPHERE_TILEMAP_START_Y_POS % 2); i--; ) {
        //*(palsPtr + 14) = colorAt15;
        *(palsPtr + 15) = colorAt16;
        palsPtr += 2 * TITAN_256C_COLORS_PER_STRIP;
    }
}

void NO_INLINE updateSphereTextColor () {
    // The animation sprite expect color to be in PAL0, and we only use PAL0 at even strips
    u8 startingStrip = TITAN_SPHERE_TILEMAP_START_Y_POS - (TITAN_SPHERE_TILEMAP_START_Y_POS % 2);
    // Every other strip inside range [5,16] contains the color used by the sprite text surrounding the sphere.
    // at position 15th. So we are jumping every 2 pals in order to set its color.
    u16* palsPtr = palettesData + (startingStrip * TITAN_256C_COLORS_PER_STRIP + 15);

    u16 fadeAmount = 0;
    if (currFadingStrip >= startingStrip) {
        u16 factor = currFadingStrip - startingStrip + 1;
        fadeAmount = 0x222 * factor;
    }
    
    for (u8 i=(TITAN_SPHERE_TILEMAP_HEIGHT+1)/2 + (TITAN_SPHERE_TILEMAP_START_Y_POS % 2); i--; ) {
        u16 d = gradColorsBuffer[0] - min(0xEEE, fadeAmount);
        fadeAmount = max(0, fadeAmount - 0x222); // diminish the fade out weight

        // IMPL B:
        if (d & 0b0000000010000) d &= ~0b0000000011110; // red overflows? then zero it
        if (d & 0b0000100000000) d &= ~0b0000111100000; // green overflows? then zero it
        if (d & 0b1000000000000) d &= ~0b1111000000000; // blue overflows? then zero it

        *palsPtr = d;
        palsPtr += 2 * TITAN_256C_COLORS_PER_STRIP;
    }
}

static u8 titanCharsCycleCnt = 0;

void NO_INLINE updateTextGradientColors () {
    // Strips [21,25] (0 based) renders the letters using transparent color, and we want to use a gradient scrolling over time. So 5 strips.
    u16 fadeTextAmount = 0;
    if (currFadingStrip >= TITAN_256C_TEXT_STARTING_STRIP) {
        u16 factor = currFadingStrip - TITAN_256C_TEXT_STARTING_STRIP + 1;
        fadeTextAmount = 0x222 * factor;
    }

    u16* rampBufPtr = gradColorsBuffer;
    u8 colorIdx = titanCharsCycleCnt / TITAN_TEXT_GRADIENT_SCROLL_FREQ; // advance ramp color every N frames (use divu for divisor non power of 2)
    if (colorIdx > TITAN_TEXT_GRADIENT_MAX_COLORS) colorIdx = 0;

    // Traverse titanCharsGradientColors from colorIdx position and copy the colors into gradColorsBuffer
    u16* palsPtr = (u16*)titanCharsGradientColors + colorIdx;
    for (u8 i=0; i < TITAN_TEXT_CURR_GRADIENT_ELEMS; ++i) {
        u16 d = *palsPtr++;
        d -= min(0xEEE, fadeTextAmount); // fadeTextAmount is 0 when is not in fading to black animation
        // Diminish the fade out weight every 4 colors (amount of ramp colors per strip)
        if ((i % 4) == 0)
            fadeTextAmount = max(0, fadeTextAmount - 0x222);

        #if TITAN_TEXT_GRADIENT_FADE_TO_BLACK_STRATEGY == 0
        // IMPL A:
        switch (d & 0b1000100010000) {
            case 0b0000000010000: d &= ~0b0000000011110; break; // red overflows? then zero it
            case 0b0000100010000: d &= ~0b0000111111110; break; // red and green overflow? then zero them
            case 0b0000100000000: d &= ~0b0000111100000; break; // green overflows? then zero it
            case 0b1000000010000: d &= ~0b1111000011110; break; // red and blue overflow? then zero them
            case 0b1000000000000: d &= ~0b1111000000000; break; // blue overflows? then zero it
            case 0b1000100000000: d &= ~0b1111111100000; break; // green and blue overflow? then zero them
            case 0b1000100010000: d = 0; break; // all colors overflow, then zero them
            default: break;
        }
        #elif TITAN_TEXT_GRADIENT_FADE_TO_BLACK_STRATEGY == 1
        // IMPL B:
        if (d & 0b0000000010000) d &= ~0b0000000011110; // red overflows? then zero it
        if (d & 0b0000100000000) d &= ~0b0000111100000; // green overflows? then zero it
        if (d & 0b1000000000000) d &= ~0b1111000000000; // blue overflows? then zero it
        #elif TITAN_TEXT_GRADIENT_FADE_TO_BLACK_STRATEGY == 2
        // IMPL C:
        if (d & 0b1000100010000) d = 0; // if only one color overflows then zero them all
        #endif
        
        *rampBufPtr++ = d;
    }

    ++titanCharsCycleCnt;
    if (titanCharsCycleCnt == TITAN_TEXT_GRADIENT_SCROLL_FREQ * TITAN_TEXT_GRADIENT_MAX_COLORS)
        titanCharsCycleCnt = 0;
}

void NO_INLINE fadingStepToBlack_pals (u8 currFadingStrip, u8 cycle) {
    // depending on the split cycle value we update the starting strip 
    currFadingStrip = max(0, currFadingStrip - cycle * (FADE_OUT_COLOR_STEPS / FADE_OUT_STRIPS_SPLIT_CYCLES));
    // starting strip is above the current strip
    u8 startingStrip = max(0, currFadingStrip - (FADE_OUT_COLOR_STEPS / FADE_OUT_STRIPS_SPLIT_CYCLES) + 1);

    // fade the palettes starting at currFadingStrip and moving backwards
    u32* palsPtr = (u32*)(palettesData + (startingStrip * TITAN_256C_COLORS_PER_STRIP));
    for (; startingStrip <= currFadingStrip; ++startingStrip) {

        // VDP u16 color is represented as next:
        // F  E  D  C  B  A  9  8  7  6  5  4  3  2  1  0
        // -  -  -  -  B2 B1 B0 -  G2 G1 G0 -  R2 R1 R0 -
        // Fading to black in 7 steps is just decrementing a color by 1 unit until reaching 0 for each color component
        // F  E  D  C  B  A  9  8  7  6  5  4  3  2  1  0
        // -  -  -  -  0  0  1  -  0  0  1  -  0  0  1  -
        // Which is the same than substracting 0x222 (0b001000100010) in 16 bits (1 color)
        // or 0x2220222 (0b00000010001000100000001000100010) in 32 bits (2 colors)

        for (u8 i=TITAN_256C_COLORS_PER_STRIP/2; i--;) {
            // NOTE: here we decrement 2 colors at a time, hence the u32 type
            u32 d = *palsPtr - 0b00000010001000100000001000100010; // decrement 1 unit in every component

            #if TITAN_256C_FADE_TO_BLACK_STRATEGY == 0
            // IMPL A:
            switch (d & 0b0001000100010000) {
                case 0b00000000000100000000000000010000: d &= ~0b00000000000111100000000000011110; break; // red overflows? then zero it
                case 0b00000001000100000000000100010000: d &= ~0b00000001111111100000000111111110; break; // red and green overflow? then zero them
                case 0b00000001000000000000000100000000: d &= ~0b00000001111000000000000111100000; break; // green overflows? then zero it
                case 0b00010000000100000001000000010000: d &= ~0b00011110000111100001111000011110; break; // red and blue overflow? then zero them
                case 0b00010000000000000001000000000000: d &= ~0b00011110000000000001111000000000; break; // blue overflows? then zero it
                case 0b00010001000000000001000100000000: d &= ~0b00011111111000000001111111100000; break; // green and blue overflow? then zero them
                case 0b00010001000100000001000100010000: d = 0; break; // all colors overflow, then zero them
                default: break;
            }
            #elif TITAN_256C_FADE_TO_BLACK_STRATEGY == 1
            // IMPL B:
            if (d & 0b00000000000100000000000000010000) d &= ~0b00000000000111100000000000011110; // red overflows? then zero it
            if (d & 0b00000001000000000000000100000000) d &= ~0b00000001111000000000000111100000; // green overflows? then zero it
            if (d & 0b00010000000000000001000000000000) d &= ~0b00011110000000000001111000000000; // blue overflows? then zero it
            #elif TITAN_256C_FADE_TO_BLACK_STRATEGY == 2
            // IMPL C:
            if (d & 0b00010001000100000001000100010000) d = 0; // if only one color overflows then zero them all
            #endif

            *palsPtr++ = d;
        }
    }
}

// void NO_INLINE fadingStepToBlack_pals (u16 currFadingStrip, u16 cycle, u16 titan256cHIntMode) {
//     for (s16 stripN=currFadingStrip; stripN >= max(currFadingStrip - FADE_OUT_STEPS, 0); --stripN) {
//         // fade the palettes of stripN
//         u16* palsPtr = palettesData + stripN * TITAN_256C_COLORS_PER_STRIP;
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
