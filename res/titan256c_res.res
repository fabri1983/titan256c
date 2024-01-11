HEADER_APPENDER  headerCustomTypes1  "typedef struct\n{\n\tTileSet *tileset;\n\tTileMap *tilemap;\n} ImageNoPals;\n"
HEADER_APPENDER  headerCustomTypes2  "typedef struct\n{\n\tTileSet *tileset1;\n\tTileSet *tileset2;\n\tTileMap *tilemap1;\n\tTileMap *tilemap2;\n} ImageNoPalsTilesetSplit2;\n"
HEADER_APPENDER  headerCustomTypes3  "typedef struct\n{\n\tTileSet *tileset1;\n\tTileSet *tileset2;\n\tTileSet *tileset3;\n\tTileMap *tilemap1;\n\tTileMap *tilemap2;\n\tTileMap *tilemap3;\n} ImageNoPalsTilesetSplit3;\n"
HEADER_APPENDER  headerCustomTypes4  "typedef struct\n{\n\tu16* data;\n} Palette32AllStrips;\n"

IMAGE_STRIPS_NO_PALS  titanRGB  "titan256c/titan_0_0_RGB.png"  28  1  -1  FALSE  FAST
PALETTE_32_COLORS_ALL_STRIPS  palTitanRGB  "titan256c/titan_0_0_RGB.png"  28  1 PAL0PAL1  TRUE  FAST