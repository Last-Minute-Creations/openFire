#include "gamestates/game/map.h"
#include <ace/config.h>
#include <ace/managers/blit.h>
#include <ace/managers/viewport/simplebuffer.h>
#include <ace/utils/extview.h>
#include "gamestates/game/team.h"
#include "gamestates/game/building.h"
#include "gamestates/game/turret.h"

tTile **g_pMap;
uint_fast8_t g_fubMapTileWidth, g_fubMapTileHeight;
char g_szMapName[256];

tTileCoord g_pTilesToRedraw[9] = {{0, 0}};
UBYTE g_ubPendingTileCount;

tBitMap *s_pTileset;
tBitMap *s_pBuffer;

void mapSetSrcDst(tBitMap *pTileset, tBitMap *pDst) {
	s_pTileset = pTileset;
	s_pBuffer = pDst;
}

UBYTE mapIsWater(UBYTE ubMapTile) {
	return ubMapTile == MAP_LOGIC_WATER;
}

UBYTE mapIsWall(UBYTE ubMapTile) {
	return (
		ubMapTile == MAP_LOGIC_WALL    ||
		ubMapTile == MAP_LOGIC_SENTRY1 ||
		ubMapTile == MAP_LOGIC_SENTRY2 ||
		0
	);
}

UBYTE mapIsRoadFriend(UBYTE ubMapTile) {
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
	if(ubX-1 < g_fubMapTileWidth && checkFn(g_pMap[ubX-1][ubY].ubIdx))
		ubOut |= ubW;
	if(ubY && checkFn(g_pMap[ubX][ubY-1].ubIdx))
		ubOut |= ubN;
	if(ubY-1 < g_fubMapTileHeight && checkFn(g_pMap[ubX][ubY+1].ubIdx))
		ubOut |= ubS;
	return ubOut;
}

void mapCreate(char *szPath) {
	UBYTE x, y, ubTileIdx;
	FILE *pMapFile;
	tSimpleBufferManager *pManager;
	char szHeaderBfr[256];

	logBlockBegin("mapCreate(szPath: %s)", szPath);
	g_ubPendingTileCount = 0;

	// Header & mem alloc
	pMapFile = fopen(szPath, "rb");
	if(!pMapFile)
		logWrite("ERR: File doesn't exist: %s\n", szPath);
	fgets(g_szMapName, 256, pMapFile);
	fscanf(
		pMapFile, "%" PRIuFAST8 "x%" PRIuFAST8 "\n",
		&g_fubMapTileWidth, &g_fubMapTileHeight
	);
	logWrite(
		"Dimensions: %" PRIuFAST8 ", %" PRIuFAST8 "\n",
		g_fubMapTileWidth, g_fubMapTileHeight
	);
	g_pMap = memAllocFast(sizeof(tTile*) * g_fubMapTileWidth);
	for(x = 0; x != g_fubMapTileWidth; ++x)
		g_pMap[x] = memAllocFast(sizeof(tTile) * g_fubMapTileHeight);
	buildingManagerReset();

	// Read map data
	for(y = 0; y != g_fubMapTileHeight; ++y) {
		for(x = 0; x != g_fubMapTileWidth; ++x) {
			do
				fread(&ubTileIdx, 1, 1, pMapFile);
				while(ubTileIdx == '\n' || ubTileIdx == '\r');
			g_pMap[x][y].ubIdx = ubTileIdx;
			g_pMap[x][y].ubData = BUILDING_IDX_INVALID;
		}
	}
	fclose(pMapFile);
	logBlockEnd("mapCreate()");
}

void mapGenerateLogic(void) {
	UBYTE x, y, ubTileIdx;
	logBlockBegin("mapGenerateLogic()");
	turretListCreate();
	// 2nd data pass - generate additional logic
	for(x = 0; x != g_fubMapTileWidth; ++x) {
		for(y = 0; y != g_fubMapTileHeight; ++y) {
			ubTileIdx = g_pMap[x][y].ubIdx;
			switch(ubTileIdx) {
				case MAP_LOGIC_WATER:
					break;
				case MAP_LOGIC_SPAWN1:
					spawnAdd(x, y, TEAM_GREEN);
					break;
				case MAP_LOGIC_SPAWN2:
					spawnAdd(x, y, TEAM_BROWN);
					break;
				case MAP_LOGIC_ROAD:
					break;
				case MAP_LOGIC_WALL:
					g_pMap[x][y].ubData = buildingAdd(x, y, BUILDING_TYPE_WALL, TEAM_NONE);
					break;
				case MAP_LOGIC_FLAG1:
				case MAP_LOGIC_FLAG2:
					g_pMap[x][y].ubData = buildingAdd(
						x, y,
						BUILDING_TYPE_FLAG,
						ubTileIdx == MAP_LOGIC_FLAG1 ? TEAM_GREEN : TEAM_BROWN
					);
					break;
				case MAP_LOGIC_SENTRY1:
				case MAP_LOGIC_SENTRY2:
					// Change logic type so that projectiles will threat turret walls
					// in same way as any other
					g_pMap[x][y].ubIdx = MAP_LOGIC_WALL;
					g_pMap[x][y].ubData = buildingAdd(
						x, y,
						BUILDING_TYPE_TURRET,
						ubTileIdx == MAP_LOGIC_SENTRY1 ? TEAM_GREEN : TEAM_BROWN
					);
					break;
				case MAP_LOGIC_DIRT:
				default:
					break;
			}
		}
	}
	logBlockEnd("mapGenerateLogic()");
}

void mapRedraw(void) {
	uint_fast8_t x, y;
	UBYTE ubTileIdx, ubOutTile;

	for(x = 0; x != g_fubMapTileWidth; ++x) {
		for(y = 0; y != g_fubMapTileHeight; ++y) {
			ubTileIdx = g_pMap[x][y].ubIdx;
			mapDrawTile(x, y, mapTileFromLogic(x, y));
		}
	}
}

UBYTE mapTileFromLogic(uint_fast8_t fubTileX, uint_fast8_t fubTileY) {
	uint_fast8_t fubLogicIdx = g_pMap[fubTileX][fubTileY].ubIdx;
	switch(fubLogicIdx) {
		case MAP_LOGIC_WATER:
			return MAP_TILE_WATER;
		case MAP_LOGIC_SPAWN1:
			return MAP_TILE_SPAWN1;
		case MAP_LOGIC_SPAWN2:
			return MAP_TILE_SPAWN2;
		case MAP_LOGIC_ROAD:
			return MAP_TILE_ROAD + mapCheckNeighbours(fubTileX, fubTileY, mapIsRoadFriend);
		case MAP_LOGIC_WALL:
			return MAP_TILE_WALL + mapCheckNeighbours(fubTileX, fubTileY, mapIsWall);
		case MAP_LOGIC_FLAG1:
			return MAP_TILE_FLAG1L;
		case MAP_LOGIC_FLAG2:
			return MAP_TILE_FLAG2L;
		case MAP_LOGIC_SENTRY1:
		case MAP_LOGIC_SENTRY2:
			return MAP_TILE_WALL;
		case MAP_LOGIC_DIRT:
		default:
			return MAP_TILE_DIRT + mapCheckNeighbours(fubTileX, fubTileY, mapIsWater);
	}
}

void mapDrawTile(UBYTE ubX, UBYTE ubY, UBYTE ubTileIdx) {
	blitCopyAligned(
		s_pTileset, 0, ubTileIdx << MAP_TILE_SIZE,
		s_pBuffer, ubX << MAP_TILE_SIZE, ubY << MAP_TILE_SIZE,
		MAP_FULL_TILE, MAP_FULL_TILE
	);
}

void mapDestroy(void) {
	uint_fast8_t x;

	logBlockBegin("mapDestroy()");
	for(x = 0; x != g_fubMapTileWidth; ++x) {
		memFree(g_pMap[x], sizeof(tTile) * g_fubMapTileHeight);
	}
	memFree(g_pMap, sizeof(tTile*) * g_fubMapTileWidth);
	turretListDestroy();
	logBlockEnd("mapDestroy()");

}

void mapRequestUpdateTile(UBYTE ubTileX, UBYTE ubTileY) {
	++g_ubPendingTileCount;
	g_pTilesToRedraw[g_ubPendingTileCount].ubX = ubTileX;
	g_pTilesToRedraw[g_ubPendingTileCount].ubY = ubTileY;
}

/**
 * @todo Redraw proper tile type.
 */
void mapUpdateTiles(void) {
	while(g_ubPendingTileCount) {
		uint_fast8_t fubTileX = g_pTilesToRedraw[g_ubPendingTileCount].ubX;
		uint_fast8_t fubTileY = g_pTilesToRedraw[g_ubPendingTileCount].ubY;
		mapDrawTile(fubTileX, fubTileY, mapTileFromLogic(fubTileX, fubTileY));
		--g_ubPendingTileCount;
	}
}
