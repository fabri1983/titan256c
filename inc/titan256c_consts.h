#ifndef TITAN_256C_CONSTS_H
#define TITAN_256C_CONSTS_H

#define TITAN_256C_STRIPS_COUNT 28
#define TITAN_256C_WIDTH 320 // In pixels
#define TITAN_256C_HEIGHT 224 // In pixels
#define TITAN_256C_STRIP_HEIGHT 8
#define TITAN_256C_COLORS_PER_STRIP 32
// In case you were to split any calculation over the colors of strip by an odd divisor
#define TITAN_256C_COLORS_PER_STRIP_REMAINDER(n) (TITAN_256C_COLORS_PER_STRIP % n)
#define TITAN_256C_TEXT_STARTING_STRIP 21
#define TITAN_256C_TEXT_ENDING_STRIP 25
#define TITAN_256C_TEXT_OFFSET_TOP 3
#define TITAN_256C_TEXT_OFFSET_BOTTOM 3

#define TITAN_TEXT_GRADIENT_SCROLL_FREQ 4
#define TITAN_TEXT_GRADIENT_MAX_COLORS 42
#define TITAN_TEXT_GRADIENT_ELEMS 16

#define TITAN_256C_FADE_TO_BLACK_STRATEGY_U32 FALSE // TRUE: fade to black 4 bytes at a time (2 colors). FALSE: fade to black 2 bytes at a time (1 color)
#define TITAN_256C_FADE_TO_BLACK_STRATEGY 2 // 0, 1, 2. From slowest to fastest (in cpu usage)
#define TITAN_TEXT_GRADIENT_FADE_TO_BLACK_STRATEGY_U32 TRUE // TRUE: fade to black 4 bytes at a time (2 colors). FALSE: fade to black 2 bytes at a time (1 color)
#define TITAN_TEXT_GRADIENT_FADE_TO_BLACK_STRATEGY 2 // 0, 1, 2. From slowest to fastest (in cpu usage)

#define TITAN_SPHERE_TEXT_ANIMATION TRUE // Enable or disable the sphere wrapping text animation
#define TITAN_SPHERE_TEXT_ANIMATION_COMPRESSED FALSE // If Sphere Text Animation sprites are compressed then set this to TRUE
#define TITAN_SPHERE_TEXT_ANIMATION_CHANGE_COLOR FALSE
#define TITAN_SPHERE_TILEMAP_WIDTH 20 // Sphere tilemap width in tiles
#define TITAN_SPHERE_TILEMAP_HEIGHT 14 // Sphere tilemap height in tiles
#define TITAN_SPHERE_TILEMAP_START_X_POS 11 // Sphere tilemap starting x position (in tiles)
#define TITAN_SPHERE_TILEMAP_START_Y_POS 4 // Sphere tilemap starting y position (in tiles)

#endif // TITAN_256C_CONSTS_H