#ifndef _CUSTOM_FONT_CONSTS_H
#define _CUSTOM_FONT_CONSTS_H

#include <config.h> // Force the inclusion so it makes visible to gcc the switch LEGACY_FONT_LOCATION

// See custom font png to get correct color index in its palette
#define CUSTOM_FONT_COLOR_INDEX 1

#define CUSTOM_FONT_PAL_COLORS_COUNT 3

#define CUSTOM_TILE_FONT_COUNT 96

// Custom font tiles are placed before or after the SGDK's Font tileset depending on switch LEGACY_FONT_LOCATION
#if LEGACY_FONT_LOCATION
#define CUSTOM_TILE_FONT_INDEX TILE_FONT_INDEX - CUSTOM_TILE_FONT_COUNT
#else
#define CUSTOM_TILE_FONT_INDEX TILE_FONT_INDEX + FONT_LENGTH
#endif

#endif // _CUSTOM_FONT_CONSTS_H