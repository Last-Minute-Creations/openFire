#ifndef GUARD_OF_GAMESTATES_GAME_WORLDMAP_H
#define GUARD_OF_GAMESTATES_GAME_WORLDMAP_H

#include <ace/utils/extview.h>
#include <ace/utils/bitmap.h>
#include "map.h"

#define MAP_TILE_SIZE 5
#define MAP_FULL_TILE (1 << MAP_TILE_SIZE)
#define MAP_HALF_TILE (MAP_FULL_TILE >> 1)

#define MAP_TILE_WATER      0
#define MAP_TILE_SPAWN_BLUE 1
#define MAP_TILE_SPAWN_RED  2
#define MAP_TILE_SPAWN_NONE 3

// Capture points
#define MAP_TILE_CAPTURE_BLUE 4
#define MAP_TILE_CAPTURE_RED  5
#define MAP_TILE_CAPTURE_NONE 6

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


typedef struct _tTileCoord {
	UBYTE ubX;
	UBYTE ubY;
} tTileCoord;

void worldMapCreate();

void worldMapDestroy(void);

void worldMapRedraw(void);
void worldMapGenerateLogic(void);

void worldMapChangeTile(
	IN UBYTE ubX,
	IN UBYTE ubY,
	IN UBYTE ubLogicTileIdx
);

void worldMapSetSrcDst(
	IN tBitMap *pTileset,
	IN tBitMap *pDst
);

void worldMapRequestUpdateTile(
	IN UBYTE ubX,
	IN UBYTE ubY
);

UBYTE worldMapTileFromLogic(
	IN FUBYTE ubTileX,
	IN FUBYTE ubTileY
);

UBYTE worldMapIsWall(
	IN UBYTE ubMapTile
);

void worldMapUpdateTiles(void);

extern tBitMap *g_pMapTileset;

#endif // GUARD_OF_GAMESTATES_GAME_WORLDMAP_H
