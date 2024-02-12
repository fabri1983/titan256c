#ifndef _RES_TITAN256C_RES_H_
#define _RES_TITAN256C_RES_H_

typedef struct
{
    u16 compression;
    u16* data;
} Palette32AllStripsCompField;

typedef struct
{
    u16* data;
} Palette32AllStrips;

typedef struct
{
    TileSet *tileset;
    TileMap *tilemap;
} ImageNoPals;

extern const ImageNoPals titanRGB;
extern const Palette32AllStripsCompField palTitanRGB;

#endif // _RES_TITAN256C_RES_H_
