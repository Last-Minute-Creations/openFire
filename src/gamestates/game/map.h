#ifndef GUARD_OF_GAMESTATES_GAME_MAP_H
#define GUARD_OF_GAMESTATES_GAME_MAP_H

#define MAP_TILE_SIZE 5

#define MAP_LOGIC_WATER   '.'
#define MAP_LOGIC_DIRT    ' '
#define MAP_LOGIC_ROAD    '#'
#define MAP_LOGIC_WALL    '-'
#define MAP_LOGIC_SPAWN1  '1'
#define MAP_LOGIC_SPAWN2  '2'
#define MAP_LOGIC_SENTRY1 's'
#define MAP_LOGIC_SENTRY2 'S'
#define MAP_LOGIC_FLAG1   'f'
#define MAP_LOGIC_FLAG2   'F'
#define MAP_LOGIC_GATE1   'g'
#define MAP_LOGIC_GATE2   'G'

#include <ace/utils/extview.h>
#include <ace/utils/bitmap.h>

extern UWORD g_uwMapWidth, g_uwMapHeight;
extern UWORD g_uwMapTileWidth, g_uwMapTileHeight;
extern char g_szMapName[256];

void mapCreate(
	IN tVPort *pVPort,
	IN tBitMap *pTileset,
	IN char *szPath
);

void mapDestroy(void);

extern UBYTE **g_pMap;

#endif