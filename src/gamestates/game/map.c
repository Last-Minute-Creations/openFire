#include "gamestates/game/map.h"
#include <ace/config.h>
#include <ace/managers/blit.h>
#include <ace/managers/viewport/simplebuffer.h>
#include <ace/utils/extview.h>
#include "gamestates/game/team.h"
#include "gamestates/game/building.h"
#include "gamestates/game/turret.h"

tTile **g_pMap;
UWORD g_uwMapTileWidth, g_uwMapTileHeight;
char g_szMapName[256];

tTileCoord g_pTilesToRedraw[9] = {{0, 0}};
UBYTE g_ubPendingTileCount;

tBitMap *s_pTileset;
tBitMap *s_pBuffer;

void mapSetSrcDst(tBitMap *pTileset, tBitMap *pDst) {
	s_pTileset = pTileset;
	s_pBuffer = pDst;
}

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
	if(ubY-1 < g_uwMapTileHeight && checkFn(g_pMap[ubX][ubY+1].ubIdx))
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
	fscanf(pMapFile, "%hux%hu\n", &g_uwMapTileWidth, &g_uwMapTileHeight);
	logWrite("Dimensions: %u, %u\n", g_uwMapTileWidth, g_uwMapTileHeight);
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
	logBlockEnd("mapCreate()");
}

void mapGenerateLogic(void) {
	UBYTE x, y, ubTileIdx;
	logBlockBegin("mapGenerateLogic()");
	turretListCreate();
	// 2nd data pass - generate additional logic
	for(x = 0; x != g_uwMapTileWidth; ++x) {
		for(y = 0; y != g_uwMapTileHeight; ++y) {
			ubTileIdx = g_pMap[x][y].ubIdx;
			switch(ubTileIdx) {
				case MAP_LOGIC_WATER:
					break;
				case MAP_LOGIC_SPAWN1:
					teamAddSpawn(TEAM_GREEN, x, y);
					break;
				case MAP_LOGIC_SPAWN2:
					teamAddSpawn(TEAM_BROWN, x, y);
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
	UWORD x, y;
	UBYTE ubTileIdx, ubOutTile;

	for(x = 0; x != g_uwMapTileWidth; ++x) {
		for(y = 0; y != g_uwMapTileHeight; ++y) {
			ubTileIdx = g_pMap[x][y].ubIdx;
			switch(ubTileIdx) {
				case MAP_LOGIC_WATER:
					ubOutTile = MAP_TILE_WATER;
					break;
				case MAP_LOGIC_SPAWN1:
					ubOutTile = MAP_TILE_SPAWN1;
					break;
				case MAP_LOGIC_SPAWN2:
					ubOutTile = MAP_TILE_SPAWN2;
					break;
				case MAP_LOGIC_ROAD:
					ubOutTile = MAP_TILE_ROAD + mapCheckNeighbours(x, y, isRoadFriend);
					break;
				case MAP_LOGIC_WALL:
					ubOutTile = MAP_TILE_WALL + mapCheckNeighbours(x, y, isWall);
					break;
				case MAP_LOGIC_FLAG1:
					ubOutTile = MAP_TILE_FLAG1L;
					break;
				case MAP_LOGIC_FLAG2:
					ubOutTile = MAP_TILE_FLAG2L;
					break;
				case MAP_LOGIC_SENTRY1:
				case MAP_LOGIC_SENTRY2:
					ubOutTile = MAP_TILE_WALL;
					break;
				case MAP_LOGIC_DIRT:
				default:
					ubOutTile = MAP_TILE_DIRT + mapCheckNeighbours(x, y, isWater);
					break;
			}
			mapDrawTile(x, y, ubOutTile);
		}
	}
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
		blitCopyAligned(
			s_pTileset, 0, MAP_TILE_DIRT << MAP_TILE_SIZE,
			s_pBuffer,
			g_pTilesToRedraw[g_ubPendingTileCount].ubX << MAP_TILE_SIZE,
			g_pTilesToRedraw[g_ubPendingTileCount].ubY << MAP_TILE_SIZE,
			1 << MAP_TILE_SIZE, 1 << MAP_TILE_SIZE
		);
		--g_ubPendingTileCount;
	}
}
