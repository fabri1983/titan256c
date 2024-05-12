#include <genesis.h>
#include "customFont.h"
#include "customFont_consts.h"

void drawText (const char *str, u16 x, u16 y) {
    u16 basetile = TILE_ATTR_FULL(PAL0, 0, FALSE, FALSE, CUSTOM_TILE_FONT_INDEX);
    VDPPlane plane = VDP_getTextPlane();

    u16 data[128];
    const u8 *s;
    u16 *d;
    u16 i, pw, ph, len;

    // get the horizontal plane size (in cell)
    pw = (plane == WINDOW)?windowWidth:planeWidth;
    ph = (plane == WINDOW)?32:planeHeight;

    // string outside plane --> exit
    if ((x >= pw) || (y >= ph))
        return;

    // get string len
    len = strlen(str);
    // if string don't fit in plane, we cut it
    if (len > (pw - x))
        len = pw - x;

    // prepare the data
    s = (const u8*) str;
    d = data;
    i = len;
    while(i--)
        *d++ = (*s++ - 32);

    // VDP_setTileMapDataRowEx(..) take care of using temporary buffer to build the data so we are ok here
    VDP_setTileMapDataRowEx(plane, data, basetile, y, x, len, CPU);
}