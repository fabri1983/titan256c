#ifndef _RES_TITAN256C_RES_H_
#define _RES_TITAN256C_RES_H_

typedef struct {
    u16 numTile;
    u32* tiles;
} TileSetOriginalCustom;

typedef struct {
    u16 w;
    u16 h;
    u16* tilemap;
} TileMapOriginalCustom;

typedef struct {
    u16* tilemap;
} TileMapCustom;

typedef struct {
    u16 compression;
    u16* tilemap;
} TileMapCustomCompField;

typedef struct {
    TileSetOriginalCustom* tileset;
    TileMapOriginalCustom* tilemap;
} ImageNoPals;

typedef struct {
    TileSet* tileset;
    TileMap* tilemap;
} ImageNoPalsCompField;

typedef struct {
    TileSetOriginalCustom* tileset1;
    TileSetOriginalCustom* tileset2;
    TileMapCustom* tilemap1;
} ImageNoPalsTilesetSplit21;

typedef struct {
    TileSet* tileset1;
    TileSet* tileset2;
    TileMapCustom* tilemap1;
    TileMapCustom* tilemap2;
} ImageNoPalsTilesetSplit22;

typedef struct {
    TileSetOriginalCustom* tileset1;
    TileSetOriginalCustom* tileset2;
    TileSetOriginalCustom* tileset3;
    TileMapCustom* tilemap1;
} ImageNoPalsTilesetSplit31;

typedef struct {
    TileSetOriginalCustom* tileset1;
    TileSetOriginalCustom* tileset2;
    TileSetOriginalCustom* tileset3;
    TileMapCustom* tilemap1;
    TileMapCustom* tilemap2;
} ImageNoPalsTilesetSplit32;

typedef struct {
    TileSet* tileset1;
    TileSet* tileset2;
    TileSet* tileset3;
    TileMapCustom* tilemap1;
    TileMapCustom* tilemap2;
    TileMapCustom* tilemap3;
} ImageNoPalsTilesetSplit33;

typedef struct {
    TileSet* tileset1;
    TileSet* tileset2;
    TileMapCustom* tilemap1;
} ImageNoPalsTilesetSplit21CompField;

typedef struct {
    TileSet* tileset1;
    TileSet* tileset2;
    TileMapCustomCompField* tilemap1;
    TileMapCustomCompField* tilemap2;
} ImageNoPalsTilesetSplit22CompField;

typedef struct {
    TileSet* tileset1;
    TileSet* tileset2;
    TileSet* tileset3;
    TileMapCustomCompField* tilemap1;
} ImageNoPalsTilesetSplit31CompField;

typedef struct {
    TileSet* tileset1;
    TileSet* tileset2;
    TileSet* tileset3;
    TileMapCustomCompField* tilemap1;
    TileMapCustomCompField* tilemap2;
} ImageNoPalsTilesetSplit32CompField;

typedef struct {
    TileSet* tileset1;
    TileSet* tileset2;
    TileSet* tileset3;
    TileMapCustomCompField* tilemap1;
    TileMapCustomCompField* tilemap2;
    TileMapCustomCompField* tilemap3;
} ImageNoPalsTilesetSplit33CompField;

typedef struct {
    u16* data;
} Palette16;

typedef struct {
    u16* data;
} Palette32;

typedef struct {
    u16* data;
} Palette16AllStrips;

typedef struct {
    u16* data1;
    u16* data2;
} Palette16AllStripsSplit2;

typedef struct {
    u16* data1;
    u16* data2;
    u16* data3;
} Palette16AllStripsSplit3;

typedef struct {
    u16 compression;
    u16* data;
} Palette16AllStripsCompField;

typedef struct {
    u16 compression;
    u16* data1;
    u16* data2;
} Palette16AllStripsSplit2CompField;

typedef struct {
    u16 compression;
    u16* data1;
    u16* data2;
    u16* data3;
} Palette16AllStripsSplit3CompField;

typedef struct {
    u16* data;
} Palette32AllStrips;

typedef struct {
    u16* data1;
    u16* data2;
} Palette32AllStripsSplit2;

typedef struct {
    u16* data1;
    u16* data2;
    u16* data3;
} Palette32AllStripsSplit3;

typedef struct {
    u16 compression;
    u16* data;
} Palette32AllStripsCompField;

typedef struct {
    u16 compression;
    u16* data1;
    u16* data2;
} Palette32AllStripsSplit2CompField;

typedef struct {
    u16 compression;
    u16* data1;
    u16* data2;
    u16* data3;
} Palette32AllStripsSplit3CompField;

extern const ImageNoPalsCompField titanRGB;
extern const Palette32AllStripsCompField palTitanRGB;

#endif // _RES_TITAN256C_RES_H_
