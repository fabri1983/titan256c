#ifndef _RES_TITAN256C_RES_H_
#define _RES_TITAN256C_RES_H_

#define COMPER 5
#define COMPERX 6
#define KOSINSKI 7
#define KOSINSKI_PLUS 8
#define LZKN1 9
#define ROCKET 10
#define SAXMAN 11
#define SAXMAN2 12
#define SNKRLE 13
#define UFTC 14
#define UFTC_15 15

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
extern const Palette32AllStrips palTitanRGB;

#endif // _RES_TITAN256C_RES_H_
