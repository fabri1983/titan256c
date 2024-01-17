#ifndef _RES_TITAN256C_RES_H_
#define _RES_TITAN256C_RES_H_

#define COMPER 4
#define KOSINSKI 5
#define KOSINSKI_PLUS 6

typedef struct
{
	u16 *tilemap;
} TileMapCustom;

typedef struct
{
	TileSet *tileset;
	TileMap *tilemap;
} ImageNoPals;

typedef struct
{
	TileSet *tileset1;
	TileSet *tileset2;
	TileMapCustom *tilemap1;
	TileMapCustom *tilemap2;
} ImageNoPalsTilesetSplit2;

typedef struct
{
	TileSet *tileset1;
	TileSet *tileset2;
	TileSet *tileset3;
	TileMapCustom *tilemap1;
	TileMapCustom *tilemap2;
	TileMapCustom *tilemap3;
} ImageNoPalsTilesetSplit3;

typedef struct
{
	u16* data;
} Palette32AllStrips;

typedef struct
{
	u16* data1;
	u16* data2;
} Palette32AllStripsSplit2;

typedef struct
{
	u16* data1;
	u16* data2;
	u16* data3;
} Palette32AllStripsSplit3;

extern const ImageNoPals titanRGB;
extern const Palette32AllStrips palTitanRGB;

#endif // _RES_TITAN256C_RES_H_
