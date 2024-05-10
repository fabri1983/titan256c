#ifndef _CUSTOM_FONT_CONSTS_H
#define _CUSTOM_FONT_CONSTS_H

// See custom font png to get correct color index in its palette
#define CUSTOM_FONT_COLOR_INDEX 1

#define CUSTOM_FONT_PAL_COLORS_COUNT 3

#define CUSTOM_TILE_FONT_COUNT 96

// Custom font tiles are placed before the SGDK's tile
#define CUSTOM_TILE_FONT_INDEX TILE_FONT_INDEX - CUSTOM_TILE_FONT_COUNT

#endif // _CUSTOM_FONT_CONSTS_H