#ifndef _RES_TITAN256C_RES_H_
#define _RES_TITAN256C_RES_H_

typedef struct
{
	TileSet *tileset;
	TileMap *tilemap;
} ImageNoPals;

typedef struct
{
	TileSet *tileset1;
	TileSet *tileset2;
	TileMap *tilemap1;
	TileMap *tilemap2;
} ImageNoPalsTilesetSplit2;

typedef struct
{
	TileSet *tileset1;
	TileSet *tileset2;
	TileSet *tileset3;
	TileMap *tilemap1;
	TileMap *tilemap2;
	TileMap *tilemap3;
} ImageNoPalsTilesetSplit3;

typedef struct
{
	u16* data;
} Palette32AllStrips;

extern const ImageNoPals titanRGB;
extern const Palette32AllStrips palTitanRGB;

#endif // _RES_TITAN256C_RES_H_
