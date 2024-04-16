#ifndef TITAN_256C_CONSTS_H
#define TITAN_256C_CONSTS_H

#define TITAN_256C_STRIPS_COUNT 28
#define TITAN_256C_WIDTH 320 // In pixels
#define TITAN_256C_HEIGHT 224 // In pixels
#define TITAN_256C_STRIP_HEIGHT 8
#define TITAN_256C_COLORS_PER_STRIP 32
// in case you were to split any calculation over the colors of strip by an odd divisor
#define TITAN_256C_COLORS_PER_STRIP_REMAINDER(n) (TITAN_256C_COLORS_PER_STRIP % n)
#define TITAN_256C_TEXT_STARTING_STRIP 21
#define TITAN_256C_TEXT_ENDING_STRIP 25
#define TITAN_256C_TEXT_OFFSET_TOP 3
#define TITAN_256C_TEXT_OFFSET_BOTTOM 3

#define TITAN_CHARS_GRADIENT_SCROLL_FREQ 4
#define TITAN_CHARS_GRADIENT_MAX_COLORS 42
#define TITAN_CHARS_CURR_GRADIENT_ELEMS 16

// Sphere tilemap width in tiles
#define TITAN_SPHERE_TILEMAP_WIDTH 22
// Sphere tilemap height in tiles
#define TITAN_SPHERE_TILEMAP_HEIGHT 13
// Sphere tilemap starting y position (in tiles)
#define TITAN_SPHERE_TILEMAP_START_Y_POS 4
// Sphere tilemap starting x position (in tiles)
#define TITAN_SPHERE_TILEMAP_START_X_POS 10

#endif // TITAN_256C_CONSTS_H