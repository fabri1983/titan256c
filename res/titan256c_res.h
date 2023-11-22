#ifndef _RES_TITAN256C_RES_H_
#define _RES_TITAN256C_RES_H_

typedef struct
{
	TileSet *tileset;
	TileMap *tilemap;
} ImageNoPals;

typedef struct
{
	u16 compression;
	u16* data;
} Palette32AllStrips;

extern const ImageNoPals titanRGB;
extern const Palette32AllStrips palTitanRGB;

#endif // _RES_TITAN256C_RES_H_
