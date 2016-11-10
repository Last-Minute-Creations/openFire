#include "gamestates/game/map.h"
#include <ace/config.h>
#include <ace/managers/blit.h>
#include <ace/managers/viewport/simplebuffer.h>
#include <ace/utils/extview.h>
#include "gamestates/game/team.h"
#include "gamestates/game/building.h"

#define MAP_TILE_WATER  0
#define MAP_TILE_SPAWN1 1
#define MAP_TILE_SPAWN2 2
// Flag buildings - 'live' & destroyed
#define MAP_TILE_FLAG1L 3
#define MAP_TILE_FLAG2L 4
#define MAP_TILE_FLAGD  5
// Destroyed wall tile
#define MAP_TILE_WALLD  6
// Gates - horizontal & vertical, 'live' & destroyed
#define MAP_TILE_GATEVL 7
#define MAP_TILE_GATEVD 8
#define MAP_TILE_GATEHL 9
#define MAP_TILE_GATEHD 10
// 16-variant tiles - base codes, ends 16 positions later.
#define MAP_TILE_DIRT   16
#define MAP_TILE_ROAD   32
#define MAP_TILE_WALL   48
#define MAP_TILE_TURRET 64

tTile **g_pMap;
UWORD g_uwMapWidth, g_uwMapHeight;
UWORD g_uwMapTileWidth, g_uwMapTileHeight;
char g_szMapName[256];
tBitMap *s_pTileset;
tBitMap *s_pBuffer;

UBYTE isWater(UBYTE ubMapTile) {
	return ubMapTile == MAP_LOGIC_WATER;
}

UBYTE isWall(UBYTE ubMapTile) {
	return (
		ubMapTile == MAP_LOGIC_WALL    ||
		ubMapTile == MAP_LOGIC_SENTRY1 ||
		ubMapTile == MAP_LOGIC_SENTRY2 ||
		0
	);
}

UBYTE isRoadFriend(UBYTE ubMapTile) {
	return (
		ubMapTile == MAP_LOGIC_ROAD   ||
		ubMapTile == MAP_LOGIC_SPAWN1 ||
		ubMapTile == MAP_LOGIC_SPAWN2 ||
		ubMapTile == MAP_LOGIC_GATE1  ||
		ubMapTile == MAP_LOGIC_GATE2  ||
		ubMapTile == MAP_LOGIC_FLAG1 ||
		ubMapTile == MAP_LOGIC_FLAG2 ||
		0
	);
}

UBYTE mapCheckNeighbours(UBYTE ubX, UBYTE ubY, UBYTE (*checkFn)(UBYTE)) {
	UBYTE ubTileType;
	UBYTE ubOut;
	const UBYTE ubE = 8;
	const UBYTE ubW = 4;
	const UBYTE ubS = 2;
	const UBYTE ubN = 1;
	
	ubOut = 0;
	ubTileType = g_pMap[ubX][ubY].ubIdx;
	if(ubX && checkFn(g_pMap[ubX+1][ubY].ubIdx))
		ubOut |= ubE;
	if(ubX-1 < g_uwMapTileWidth && checkFn(g_pMap[ubX-1][ubY].ubIdx))
		ubOut |= ubW;
	if(ubY && checkFn(g_pMap[ubX][ubY-1].ubIdx))
		ubOut |= ubN;
	if(ubY-1 < g_uwMapHeight && checkFn(g_pMap[ubX][ubY+1].ubIdx))
		ubOut |= ubS;
	return ubOut;
}

void mapCreate(tVPort *pVPort, tBitMap *pTileset, char *szPath) {
	UBYTE x, y, ubTileIdx, ubOutTile;
	FILE *pMapFile;
	tSimpleBufferManager *pManager;
	char szHeaderBfr[256];
	
	logBlockBegin("mapCreate(pVPort: %p, pTileset: %p, szPath: %s)", pVPort, pTileset, szPath);

	pManager = (tSimpleBufferManager*)vPortGetManager(pVPort, VPM_SCROLL);
	s_pBuffer = pManager->pBuffer;
	s_pTileset = pTileset;
	
	// Header & mem alloc
	pMapFile = fopen(szPath, "rb");
	if(!pMapFile)
		logWrite("ERR: File doesn't exist: %s\n", szPath);
	fgets(g_szMapName, 256, pMapFile);
	fscanf(pMapFile, "%hux%hu\n", &g_uwMapTileWidth, &g_uwMapTileHeight);
	logWrite("Dimensions: %u, %u\n", g_uwMapTileWidth, g_uwMapTileHeight);
	g_uwMapWidth = g_uwMapTileWidth << MAP_TILE_SIZE;
	g_uwMapHeight = g_uwMapTileHeight << MAP_TILE_SIZE;
	g_pMap = memAllocFast(sizeof(tTile*) * g_uwMapTileWidth);
	for(x = 0; x != g_uwMapTileWidth; ++x)
		g_pMap[x] = memAllocFast(sizeof(tTile) * g_uwMapTileHeight);
	buildingManagerReset();
	
	// Read map data
	for(y = 0; y != g_uwMapTileHeight; ++y) {
		for(x = 0; x != g_uwMapTileWidth; ++x) {
			do
				fread(&ubTileIdx, 1, 1, pMapFile);
				while(ubTileIdx == '\n' || ubTileIdx == '\r');
			g_pMap[x][y].ubIdx = ubTileIdx;
			g_pMap[x][y].ubData = BUILDING_IDX_INVALID;
		}
	}
	fclose(pMapFile);
	
	// 2nd data pass - generate graphics based on logic tiles
	for(x = 0; x != g_uwMapTileWidth; ++x) {
		for(y = 0; y != g_uwMapTileHeight; ++y) {
			ubTileIdx = g_pMap[x][y].ubIdx;
			switch(ubTileIdx) {
				case MAP_LOGIC_WATER:
					ubOutTile = MAP_TILE_WATER;
					break;
				case MAP_LOGIC_SPAWN1:
					ubOutTile = MAP_TILE_SPAWN1;
					teamAddSpawn(TEAM_GREEN, x, y);
					break;
				case MAP_LOGIC_SPAWN2:
					ubOutTile = MAP_TILE_SPAWN2;
					teamAddSpawn(TEAM_BROWN, x, y);
					break;
				case MAP_LOGIC_ROAD:
					ubOutTile = MAP_TILE_ROAD + mapCheckNeighbours(x, y, isRoadFriend);
					break;
				case MAP_LOGIC_WALL:
					ubOutTile = MAP_TILE_WALL + mapCheckNeighbours(x, y, isWall);
					g_pMap[x][y].ubData = buildingAdd(BUILDING_TYPE_WALL);
					break;
				case MAP_LOGIC_FLAG1:
					ubOutTile = MAP_TILE_FLAG1L;
					g_pMap[x][y].ubData = buildingAdd(BUILDING_TYPE_FLAG);
					// TODO: register flag
					break;
				case MAP_LOGIC_FLAG2:
					ubOutTile = MAP_TILE_FLAG2L;
					g_pMap[x][y].ubData = buildingAdd(BUILDING_TYPE_FLAG);
					// TODO: register flag
					break;
				case MAP_LOGIC_SENTRY1:
				case MAP_LOGIC_SENTRY2:
					ubOutTile = MAP_TILE_TURRET;
					g_pMap[x][y].ubData = buildingAdd(BUILDING_TYPE_WALL);
					// TODO: register turret
					break;
				case MAP_LOGIC_DIRT:
				default:
					ubOutTile = MAP_TILE_DIRT + mapCheckNeighbours(x, y, isWater);
					break;
			}
			mapDrawTile(x, y, ubOutTile);
		}
	}
	
	logBlockEnd("mapCreate()");
}

void mapDrawTile(UBYTE ubX, UBYTE ubY, UBYTE ubTileIdx) {
	blitCopyAligned(
		s_pTileset, 0, ubTileIdx << MAP_TILE_SIZE,
		s_pBuffer, ubX << MAP_TILE_SIZE, ubY << MAP_TILE_SIZE,
		1 << MAP_TILE_SIZE, 1 << MAP_TILE_SIZE
	);	
}

void mapDestroy(void) {
	UBYTE x;
	
	logBlockBegin("mapDestroy()");
	for(x = 0; x != g_uwMapTileWidth; ++x) {
		memFree(g_pMap[x], sizeof(tTile) * g_uwMapTileHeight);	
	}
	memFree(g_pMap, sizeof(tTile*) * g_uwMapTileWidth);
	logBlockEnd("mapDestroy()");
}