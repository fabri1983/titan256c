HEADER_APPENDER  headerDefineCompressionCustom  "#define COMPER 4\n#define KOSINSKI 5\n#define KOSINSKI_PLUS 6\n"

HEADER_APPENDER  headerCustomTypes1  "typedef struct\n{\n\tu16 *tilemap;\n} TileMapCustom;\n"
HEADER_APPENDER  headerCustomTypes2  "typedef struct\n{\n\tTileSet *tileset;\n\tTileMap *tilemap;\n} ImageNoPals;\n"
HEADER_APPENDER  headerCustomTypes3  "typedef struct\n{\n\tTileSet *tileset1;\n\tTileSet *tileset2;\n\tTileMapCustom *tilemap1;\n\tTileMapCustom *tilemap2;\n} ImageNoPalsTilesetSplit2;\n"
HEADER_APPENDER  headerCustomTypes4  "typedef struct\n{\n\tTileSet *tileset1;\n\tTileSet *tileset2;\n\tTileSet *tileset3;\n\tTileMapCustom *tilemap1;\n\tTileMapCustom *tilemap2;\n\tTileMapCustom *tilemap3;\n} ImageNoPalsTilesetSplit3;\n"
HEADER_APPENDER  headerCustomTypes5  "typedef struct\n{\n\tu16* data;\n} Palette32AllStrips;\n"
HEADER_APPENDER  headerCustomTypes6  "typedef struct\n{\n\tu16* data1;\n\tu16* data2;\n} Palette32AllStripsSplit2;\n"
HEADER_APPENDER  headerCustomTypes7  "typedef struct\n{\n\tu16* data1;\n\tu16* data2;\n\tu16* data3;\n} Palette32AllStripsSplit3;\n"

IMAGE_STRIPS_NO_PALS  titanRGB  "titan256c/titan_0_0_RGB.png"  28  1  -1  FALSE  FAST  NONE  ALL
PALETTE_32_COLORS_ALL_STRIPS  palTitanRGB  "titan256c/titan_0_0_RGB.png"  28  1  PAL0PAL1  TRUE  FAST  NONE