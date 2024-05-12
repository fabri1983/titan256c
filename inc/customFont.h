#ifndef _CUSTOM_FONT_H
#define _CUSTOM_FONT_H

#include <types.h>

/// @brief Replacement for VDP_drawText(). This version uses the custom font.
/// @param str 
/// @param x (in tiles)
/// @param y (in tiles)
void drawText (const char *str, u16 x, u16 y);

#endif // _CUSTOM_FONT_H