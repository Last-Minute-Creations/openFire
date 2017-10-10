#include "gamestates/game/map.h"
#include <ace/config.h>
#include <ace/managers/log.h>
#include <ace/managers/blit.h>
#include <ace/managers/viewport/simplebuffer.h>
#include <ace/utils/extview.h>
#include "gamestates/game/team.h"
#include "gamestates/game/building.h"
#include "gamestates/game/turret.h"
#include "gamestates/game/control.h"
#include "gamestates/game/mapjson.h"

tTile **g_pMap;
FUBYTE g_fubMapTileWidth, g_fubMapTileHeight;
char g_szMapName[256];

tTileCoord g_pTilesToRedraw[9] = {{0, 0}};
UBYTE g_ubPendingTileCount;

tBitMap *g_pMapTileset;
tBitMap *s_pBuffer;

void mapSetSrcDst(tBitMap *pTileset, tBitMap *pDst) {
	g_pMapTileset = pTileset;
	s_pBuffer = pDst;
}

UBYTE mapIsWater(UBYTE ubMapTile) {
	return ubMapTile == MAP_LOGIC_WATER;
}

UBYTE mapIsWall(UBYTE ubMapTile) {
	return (
		ubMapTile == MAP_LOGIC_WALL    ||
		ubMapTile == MAP_LOGIC_SENTRY0 ||
		ubMapTile == MAP_LOGIC_SENTRY1 ||
		ubMapTile == MAP_LOGIC_SENTRY2 ||
		0
	);
}

UBYTE mapIsRoadFriend(UBYTE ubMapTile) {
	return (
		ubMapTile == MAP_LOGIC_ROAD     ||
		// ubMapTile == MAP_LOGIC_SPAWN0   ||
		// ubMapTile == MAP_LOGIC_SPAWN1   ||
		// ubMapTile == MAP_LOGIC_SPAWN2   ||
		// ubMapTile == MAP_LOGIC_CAPTURE0 ||
		// ubMapTile == MAP_LOGIC_CAPTURE1 ||
		// ubMapTile == MAP_LOGIC_CAPTURE2 ||
		// ubMapTile == MAP_LOGIC_SPAWN2   ||
		// ubMapTile == MAP_LOGIC_SPAWN2   ||
		// ubMapTile == MAP_LOGIC_GATE1    ||
		// ubMapTile == MAP_LOGIC_GATE2    ||
		// ubMapTile == MAP_LOGIC_FLAG1    ||
		// ubMapTile == MAP_LOGIC_FLAG2    ||
		0
	);
}

UBYTE mapCheckWater(UBYTE ubX, UBYTE ubY) {
	UBYTE ubOut;
	if(ubX && g_pMap[ubX-1][ubY].ubIdx == MAP_LOGIC_WATER) {
		if(ubY && g_pMap[ubX][ubY-1].ubIdx == MAP_LOGIC_WATER)
			ubOut = 1;
		else if(ubY < g_fubMapTileHeight-1 && g_pMap[ubX][ubY+1].ubIdx == MAP_LOGIC_WATER)
			ubOut = 2;
		else
			ubOut = 5 + (ubY & 1);
	}
	else if(ubX < g_fubMapTileWidth-1 && g_pMap[ubX+1][ubY].ubIdx == MAP_LOGIC_WATER) {
		if(ubY && g_pMap[ubX][ubY-1].ubIdx == MAP_LOGIC_WATER)
			ubOut = 3;
		else if(ubY < g_fubMapTileHeight-1 && g_pMap[ubX][ubY+1].ubIdx == MAP_LOGIC_WATER)
			ubOut = 4;
		else
			ubOut = 7 + (ubY & 1);
	}
	else if(ubY && g_pMap[ubX][ubY-1].ubIdx == MAP_LOGIC_WATER)
		ubOut = 9 + (ubX & 1);
	else if(ubY < g_fubMapTileHeight-1 && g_pMap[ubX][ubY+1].ubIdx == MAP_LOGIC_WATER)
		ubOut = 11 + (ubX & 1);
	else
		ubOut = 0;
	return ubOut;
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
	UBYTE ubTileIdx;
	tSimpleBufferManager *pManager;

	logBlockBegin("mapCreate(szPath: %s)", szPath);
	g_ubPendingTileCount = 0;

	tJson *pMapJson = mapJsonCreate(szPath);

	// Objects may have properties passed in random order
	// so 1st pass will extract only general data
	FUBYTE fubControlPointCount;
	mapJsonGetMeta(
		pMapJson,
		&g_fubMapTileWidth, &g_fubMapTileHeight, &fubControlPointCount
	);

	logWrite(
		"Dimensions: %" PRI_FUBYTE "x%" PRI_FUBYTE ", control pts: %"PRI_FUBYTE"\n",
		g_fubMapTileWidth, g_fubMapTileHeight, fubControlPointCount
	);

	g_pMap = memAllocFast(sizeof(tTile*) * g_fubMapTileWidth);
	for(FUBYTE x = 0; x != g_fubMapTileWidth; ++x)
		g_pMap[x] = memAllocFast(sizeof(tTile) * g_fubMapTileHeight);

	buildingManagerReset();
	controlManagerCreate(fubControlPointCount);
	FUBYTE fubSpawnCount;
	mapJsonReadTiles(pMapJson, &fubSpawnCount);
	spawnManagerCreate(fubSpawnCount);
	mapGenerateLogic();
	mapJsonReadControlPoints(pMapJson);

	mapJsonDestroy(pMapJson);
	logBlockEnd("mapCreate()");
}

void mapGenerateLogic(void) {
	FUBYTE x, y, ubTileIdx;
	logBlockBegin("mapGenerateLogic()");
	turretListCreate();
	// 2nd data pass - generate additional logic
	for(x = 0; x != g_fubMapTileWidth; ++x) {
		for(y = 0; y != g_fubMapTileHeight; ++y) {
			ubTileIdx = g_pMap[x][y].ubIdx;
			switch(ubTileIdx) {
				case MAP_LOGIC_WATER:
					break;
				case MAP_LOGIC_SPAWN0:
					spawnAdd(x, y, TEAM_NONE);
					break;
				case MAP_LOGIC_SPAWN1:
					spawnAdd(x, y, TEAM_BLUE);
					break;
				case MAP_LOGIC_SPAWN2:
					spawnAdd(x, y, TEAM_RED);
					break;
				case MAP_LOGIC_ROAD:
					break;
				case MAP_LOGIC_WALL_VERTICAL:
					g_pMap[x][y].ubIdx = MAP_LOGIC_WALL;
				case MAP_LOGIC_WALL:
					g_pMap[x][y].ubData = buildingAdd(x, y, BUILDING_TYPE_WALL, TEAM_NONE);
					break;
				case MAP_LOGIC_FLAG1:
				case MAP_LOGIC_FLAG2:
					g_pMap[x][y].ubData = buildingAdd(
						x, y,
						BUILDING_TYPE_FLAG,
						ubTileIdx == MAP_LOGIC_FLAG1 ? TEAM_BLUE : TEAM_RED
					);
					break;
				case MAP_LOGIC_SENTRY0:
				case MAP_LOGIC_SENTRY1:
				case MAP_LOGIC_SENTRY2:
					// Change logic type so that projectiles will threat turret walls
					// in same way as any other
					g_pMap[x][y].ubIdx = MAP_LOGIC_WALL;
					g_pMap[x][y].ubData = buildingAdd(
						x, y,
						BUILDING_TYPE_TURRET,
						ubTileIdx == MAP_LOGIC_SENTRY0? TEAM_NONE
							: ubTileIdx == MAP_LOGIC_SENTRY1? TEAM_BLUE
							:TEAM_RED
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

void mapDrawTile(UBYTE ubX, UBYTE ubY, UBYTE ubTileIdx) {
	blitCopyAligned(
		g_pMapTileset, 0, ubTileIdx << MAP_TILE_SIZE,
		s_pBuffer, ubX << MAP_TILE_SIZE, ubY << MAP_TILE_SIZE,
		MAP_FULL_TILE, MAP_FULL_TILE
	);
}

void mapRedraw(void) {
	UBYTE ubTileIdx, ubOutTile;

	for(FUBYTE x = 0; x != g_fubMapTileWidth; ++x) {
		for(FUBYTE y = 0; y != g_fubMapTileHeight; ++y) {
			ubTileIdx = g_pMap[x][y].ubIdx;
			mapDrawTile(x, y, mapTileFromLogic(x, y));
		}
	}
}

UBYTE mapTileFromLogic(FUBYTE fubTileX, FUBYTE fubTileY) {
	FUBYTE fubLogicIdx = g_pMap[fubTileX][fubTileY].ubIdx;
	switch(fubLogicIdx) {
		case MAP_LOGIC_WATER:
			return MAP_TILE_WATER;
		case MAP_LOGIC_SPAWN0:
			return MAP_TILE_SPAWN_NONE;
		case MAP_LOGIC_SPAWN1:
			return MAP_TILE_SPAWN_BLUE;
		case MAP_LOGIC_SPAWN2:
			return MAP_TILE_SPAWN_RED;
		case MAP_LOGIC_ROAD:
			return MAP_TILE_ROAD + mapCheckNeighbours(fubTileX, fubTileY, mapIsRoadFriend);
		case MAP_LOGIC_WALL:
			return MAP_TILE_WALL + mapCheckNeighbours(fubTileX, fubTileY, mapIsWall);
		case MAP_LOGIC_FLAG1:
			return MAP_TILE_FLAG1L;
		case MAP_LOGIC_FLAG2:
			return MAP_TILE_FLAG2L;
		case MAP_LOGIC_SENTRY0:
		case MAP_LOGIC_SENTRY1:
		case MAP_LOGIC_SENTRY2:
			return MAP_TILE_WALL;
		case MAP_LOGIC_CAPTURE0:
			return MAP_TILE_CAPTURE_NONE;
		case MAP_LOGIC_CAPTURE1:
			return MAP_TILE_CAPTURE_BLUE;
		case MAP_LOGIC_CAPTURE2:
			return MAP_TILE_CAPTURE_RED;
		case MAP_LOGIC_DIRT:
		default:
			return MAP_TILE_DIRT + mapCheckWater(fubTileX, fubTileY);
	}
}

void mapDestroy(void) {
	logBlockBegin("mapDestroy()");
	controlManagerDestroy();
	spawnManagerDestroy();
	FUBYTE x;

	for(x = 0; x != g_fubMapTileWidth; ++x) {
		memFree(g_pMap[x], sizeof(tTile) * g_fubMapTileHeight);
	}
	memFree(g_pMap, sizeof(tTile*) * g_fubMapTileWidth);
	turretListDestroy();
	logBlockEnd("mapDestroy()");

}

void mapRequestUpdateTile(UBYTE ubTileX, UBYTE ubTileY) {
	// TODO omit if not visible
	// TODO when scrolling trick: omit if not yet drawn on redraw margin
	++g_ubPendingTileCount;
	g_pTilesToRedraw[g_ubPendingTileCount].ubX = ubTileX;
	g_pTilesToRedraw[g_ubPendingTileCount].ubY = ubTileY;
}

/**
 * @todo Redraw proper tile type.
 */
void mapUpdateTiles(void) {
	while(g_ubPendingTileCount) {
		FUBYTE fubTileX = g_pTilesToRedraw[g_ubPendingTileCount].ubX;
		FUBYTE fubTileY = g_pTilesToRedraw[g_ubPendingTileCount].ubY;
		mapDrawTile(fubTileX, fubTileY, mapTileFromLogic(fubTileX, fubTileY));
		--g_ubPendingTileCount;
	}
}

void mapChangeTile(UBYTE ubX,	UBYTE ubY, UBYTE ubLogicTileIdx) {
	g_pMap[ubX][ubY].ubIdx = ubLogicTileIdx;
	mapRequestUpdateTile(ubX, ubY);
}
