#ifndef _RES_TITAN256C_RES_H_
#define _RES_TITAN256C_RES_H_

typedef struct
{
    u16 compression;
    u16* data;
} Palette32AllStripsComp;

typedef struct
{
    TileSet *tileset;
    TileMap *tilemap;
} ImageNoPals;

extern const ImageNoPals titanRGB;
extern const Palette32AllStripsComp palTitanRGB;

#endif // _RES_TITAN256C_RES_H_
