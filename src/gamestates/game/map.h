#ifndef GUARD_OF_GAMESTATES_GAME_MAP_H
#define GUARD_OF_GAMESTATES_GAME_MAP_H

#include <ace/utils/extview.h>
#include <ace/utils/bitmap.h>

#define MAP_TILE_SIZE 5
#define MAP_FULL_TILE (1 << MAP_TILE_SIZE)
#define MAP_HALF_TILE (MAP_FULL_TILE >> 1)

#define MAP_LOGIC_WATER   '.'
#define MAP_LOGIC_DIRT    ' '
#define MAP_LOGIC_ROAD    '#'
#define MAP_LOGIC_WALL    '-'
#define MAP_LOGIC_SPAWN0  '0'
#define MAP_LOGIC_SPAWN1  '1'
#define MAP_LOGIC_SPAWN2  '2'
#define MAP_LOGIC_SENTRY0 '$'
#define MAP_LOGIC_SENTRY1 's'
#define MAP_LOGIC_SENTRY2 'S'
#define MAP_LOGIC_FLAG1   'f'
#define MAP_LOGIC_FLAG2   'F'
#define MAP_LOGIC_GATE1   'g'
#define MAP_LOGIC_GATE2   'G'
#define MAP_LOGIC_CAPTURE0 'o'
#define MAP_LOGIC_CAPTURE1 'c'
#define MAP_LOGIC_CAPTURE2 'C'

#define MAP_TILE_WATER       0
#define MAP_TILE_SPAWN_GREEN 1
#define MAP_TILE_SPAWN_BROWN 2
#define MAP_TILE_SPAWN_NONE  3

// Capture points
#define MAP_TILE_CAPTURE_GREEN 4
#define MAP_TILE_CAPTURE_BROWN 5
#define MAP_TILE_CAPTURE_NONE  6

// Flag buildings - 'live' & destroyed
#define MAP_TILE_FLAG1L 7
#define MAP_TILE_FLAG2L 8
#define MAP_TILE_FLAGD  9

// Destroyed wall tile
#define MAP_TILE_WALLD    10

// Gates - horizontal & vertical, 'live' & destroyed
#define MAP_TILE_GATEVL   11
#define MAP_TILE_GATEVD   12
#define MAP_TILE_GATEHL   13
#define MAP_TILE_GATEHD   14

// 16-variant tiles - base codes, ends 16 positions later.
#define MAP_TILE_DIRT     16
#define MAP_TILE_ROAD     32
#define MAP_TILE_WALL     48
#define MAP_TILE_TURRET   64

typedef struct _tTile {
	UBYTE ubIdx;  ///< Tileset idx
	UBYTE ubData; ///< Data field. For buildings/gates/spawns used as array idx.
} tTile;

typedef struct _tTileCoord {
	UBYTE ubX;
	UBYTE ubY;
} tTileCoord;

extern FUBYTE g_fubMapTileWidth, g_fubMapTileHeight;
extern char g_szMapName[256];
extern tTile **g_pMap;

void mapCreate(
	IN char *szPath
);

void mapDestroy(void);

void mapRedraw(void);
void mapGenerateLogic(void);

void mapChangeTile(
	IN UBYTE ubX,
	IN UBYTE ubY,
	IN UBYTE ubLogicTileIdx
);

void mapSetSrcDst(
	IN tBitMap *pTileset,
	IN tBitMap *pDst
);

void mapRequestUpdateTile(
	IN UBYTE ubX,
	IN UBYTE ubY
);

UBYTE mapTileFromLogic(
	IN FUBYTE ubTileX,
	IN FUBYTE ubTileY
);

void mapUpdateTiles(void);

extern tTile **g_pMap;
extern tBitMap *g_pMapTileset;

#endif
