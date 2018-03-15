#include "gamestates/game/worldmap.h"
#include <ace/managers/log.h>
#include <ace/managers/blit.h>
#include <ace/managers/viewport/simplebuffer.h>
#include <ace/utils/extview.h>
#include "map.h"
#include "mapjson.h"
#include "gamestates/game/team.h"
#include "gamestates/game/building.h"
#include "gamestates/game/turret.h"
#include "gamestates/game/control.h"

static tTileCoord s_pTilesToRedraw[9] = {{0, 0}};
static UBYTE s_ubPendingTileCount;

tBitMap *g_pMapTileset;
static tBitMap *s_pBuffer;

void worldMapSetSrcDst(tBitMap *pTileset, tBitMap *pDst) {
	g_pMapTileset = pTileset;
	s_pBuffer = pDst;
}

static UBYTE worldMapIsWater(UBYTE ubMapTile) {
	return ubMapTile == MAP_LOGIC_WATER;
}

UBYTE worldMapIsWall(UBYTE ubMapTile) {
	return (
		ubMapTile == MAP_LOGIC_WALL    ||
		ubMapTile == MAP_LOGIC_SENTRY0 ||
		ubMapTile == MAP_LOGIC_SENTRY1 ||
		ubMapTile == MAP_LOGIC_SENTRY2 ||
		0
	);
}

static UBYTE worldMapIsRoadFriend(UBYTE ubMapTile) {
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

static UBYTE worldMapCheckWater(UBYTE ubX, UBYTE ubY) {
	UBYTE ubOut;
	if(ubX && g_sMap.pData[ubX-1][ubY].ubIdx == MAP_LOGIC_WATER) {
		if(ubY && g_sMap.pData[ubX][ubY-1].ubIdx == MAP_LOGIC_WATER)
			ubOut = 1;
		else if(ubY < g_sMap.fubHeight-1 && g_sMap.pData[ubX][ubY+1].ubIdx == MAP_LOGIC_WATER)
			ubOut = 2;
		else
			ubOut = 5 + (ubY & 1);
	}
	else if(ubX < g_sMap.fubWidth-1 && g_sMap.pData[ubX+1][ubY].ubIdx == MAP_LOGIC_WATER) {
		if(ubY && g_sMap.pData[ubX][ubY-1].ubIdx == MAP_LOGIC_WATER)
			ubOut = 3;
		else if(ubY < g_sMap.fubHeight-1 && g_sMap.pData[ubX][ubY+1].ubIdx == MAP_LOGIC_WATER)
			ubOut = 4;
		else
			ubOut = 7 + (ubY & 1);
	}
	else if(ubY && g_sMap.pData[ubX][ubY-1].ubIdx == MAP_LOGIC_WATER)
		ubOut = 9 + (ubX & 1);
	else if(ubY < g_sMap.fubHeight-1 && g_sMap.pData[ubX][ubY+1].ubIdx == MAP_LOGIC_WATER)
		ubOut = 11 + (ubX & 1);
	else
		ubOut = 0;
	return ubOut;
}

static UBYTE worldMapCheckNeighbours(UBYTE ubX, UBYTE ubY, UBYTE (*checkFn)(UBYTE)) {
	UBYTE ubTileType;
	UBYTE ubOut;
	const UBYTE ubE = 8;
	const UBYTE ubW = 4;
	const UBYTE ubS = 2;
	const UBYTE ubN = 1;

	ubOut = 0;
	ubTileType = g_sMap.pData[ubX][ubY].ubIdx;
	if(ubX && checkFn(g_sMap.pData[ubX+1][ubY].ubIdx))
		ubOut |= ubE;
	if(ubX-1 < g_sMap.fubWidth && checkFn(g_sMap.pData[ubX-1][ubY].ubIdx))
		ubOut |= ubW;
	if(ubY && checkFn(g_sMap.pData[ubX][ubY-1].ubIdx))
		ubOut |= ubN;
	if(ubY-1 < g_sMap.fubHeight && checkFn(g_sMap.pData[ubX][ubY+1].ubIdx))
		ubOut |= ubS;
	return ubOut;
}

void worldMapCreate(void) {
	logBlockBegin("worldMapCreate()");
	s_ubPendingTileCount = 0;

	buildingManagerReset();
	controlManagerCreate(g_sMap.fubControlPointCount);
	spawnManagerCreate(g_sMap.fubSpawnCount);
	turretListCreate(g_sMap.fubWidth, g_sMap.fubHeight);
	worldMapGenerateLogic();

	// Read remaining JSON data
	tJson *pMapJson = jsonCreate(g_sMap.szPath);
	mapJsonReadControlPoints(pMapJson);
	jsonDestroy(pMapJson);

	logBlockEnd("worldMapCreate()");
}

void worldMapGenerateLogic(void) {
	FUBYTE x, y, ubTileIdx;
	logBlockBegin("mapGenerateLogic()");
	// 2nd data pass - generate additional logic
	for(x = 0; x != g_sMap.fubWidth; ++x) {
		for(y = 0; y != g_sMap.fubHeight; ++y) {
			ubTileIdx = g_sMap.pData[x][y].ubIdx;
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
					g_sMap.pData[x][y].ubIdx = MAP_LOGIC_WALL;
				case MAP_LOGIC_WALL:
					g_sMap.pData[x][y].ubData = buildingAdd(x, y, BUILDING_TYPE_WALL, TEAM_NONE);
					break;
				case MAP_LOGIC_FLAG1:
				case MAP_LOGIC_FLAG2:
					g_sMap.pData[x][y].ubData = buildingAdd(
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
					g_sMap.pData[x][y].ubIdx = MAP_LOGIC_WALL;
					g_sMap.pData[x][y].ubData = buildingAdd(
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

static void worldMapDrawTile(UBYTE ubX, UBYTE ubY, UBYTE ubTileIdx) {
	blitCopyAligned(
		g_pMapTileset, 0, ubTileIdx << MAP_TILE_SIZE,
		s_pBuffer, ubX << MAP_TILE_SIZE, ubY << MAP_TILE_SIZE,
		MAP_FULL_TILE, MAP_FULL_TILE
	);
}

void worldMapRedraw(void) {
	for(FUBYTE x = 0; x != g_sMap.fubWidth; ++x) {
		for(FUBYTE y = 0; y != g_sMap.fubHeight; ++y) {
			worldMapDrawTile(x, y, worldMapTileFromLogic(x, y));
		}
	}
}

UBYTE worldMapTileFromLogic(FUBYTE fubTileX, FUBYTE fubTileY) {
	FUBYTE fubLogicIdx = g_sMap.pData[fubTileX][fubTileY].ubIdx;
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
			return MAP_TILE_ROAD + worldMapCheckNeighbours(fubTileX, fubTileY, worldMapIsRoadFriend);
		case MAP_LOGIC_WALL:
			return MAP_TILE_WALL + worldMapCheckNeighbours(fubTileX, fubTileY, worldMapIsWall);
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
			return MAP_TILE_DIRT + worldMapCheckWater(fubTileX, fubTileY);
	}
}

void worldMapDestroy(void) {
	logBlockBegin("worldMapDestroy()");
	controlManagerDestroy();
	spawnManagerDestroy();

	turretListDestroy();
	logBlockEnd("worldMapDestroy()");
}

void worldMapRequestUpdateTile(UBYTE ubTileX, UBYTE ubTileY) {
	// TODO omit if not visible
	// TODO when scrolling trick: omit if not yet drawn on redraw margin
	++s_ubPendingTileCount;
	s_pTilesToRedraw[s_ubPendingTileCount].ubX = ubTileX;
	s_pTilesToRedraw[s_ubPendingTileCount].ubY = ubTileY;
}

/**
 * @todo Redraw proper tile type.
 */
void worldMapUpdateTiles(void) {
	while(s_ubPendingTileCount) {
		FUBYTE fubTileX = s_pTilesToRedraw[s_ubPendingTileCount].ubX;
		FUBYTE fubTileY = s_pTilesToRedraw[s_ubPendingTileCount].ubY;
		worldMapDrawTile(fubTileX, fubTileY, worldMapTileFromLogic(fubTileX, fubTileY));
		--s_ubPendingTileCount;
	}
}

void worldMapChangeTile(UBYTE ubX,	UBYTE ubY, UBYTE ubLogicTileIdx) {
	g_sMap.pData[ubX][ubY].ubIdx = ubLogicTileIdx;
	worldMapRequestUpdateTile(ubX, ubY);
}
