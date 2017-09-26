#include "gamestates/game/map.h"
#define JSMN_STRICT
#include "jsmn.h"
#include <ace/config.h>
#include <ace/managers/blit.h>
#include <ace/managers/viewport/simplebuffer.h>
#include <ace/utils/extview.h>
#include "gamestates/game/team.h"
#include "gamestates/game/building.h"
#include "gamestates/game/turret.h"
#include "gamestates/game/control.h"

tTile **g_pMap;
FUBYTE g_fubMapTileWidth, g_fubMapTileHeight;
char g_szMapName[256];

tTileCoord g_pTilesToRedraw[9] = {{0, 0}};
UBYTE g_ubPendingTileCount;

tBitMap *s_pTileset;
tBitMap *s_pBuffer;

// https://alisdair.mcdiarmid.org/jsmn-example/

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
		ubMapTile == MAP_LOGIC_SENTRY0 ||
		ubMapTile == MAP_LOGIC_SENTRY1 ||
		ubMapTile == MAP_LOGIC_SENTRY2 ||
		0
	);
}

UBYTE mapIsRoadFriend(UBYTE ubMapTile) {
	return (
		ubMapTile == MAP_LOGIC_ROAD     ||
		ubMapTile == MAP_LOGIC_SPAWN0   ||
		ubMapTile == MAP_LOGIC_SPAWN1   ||
		ubMapTile == MAP_LOGIC_SPAWN2   ||
		ubMapTile == MAP_LOGIC_CAPTURE0 ||
		ubMapTile == MAP_LOGIC_CAPTURE1 ||
		ubMapTile == MAP_LOGIC_CAPTURE2 ||
		ubMapTile == MAP_LOGIC_SPAWN2   ||
		ubMapTile == MAP_LOGIC_SPAWN2   ||
		ubMapTile == MAP_LOGIC_GATE1    ||
		ubMapTile == MAP_LOGIC_GATE2    ||
		ubMapTile == MAP_LOGIC_FLAG1    ||
		ubMapTile == MAP_LOGIC_FLAG2    ||
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

void mapJsonGetMeta(
	char *szJson, jsmntok_t *pTokens, FUWORD fuwTokenCnt,
	FUBYTE *pWidth, FUBYTE *pHeight, FUBYTE *pControlPointCount
) {
	for(FUWORD i = 1; i < fuwTokenCnt; ++i) { // Skip first gigantic obj
		// Read property name
		FUWORD fuwWidth = pTokens[i].end - pTokens[i].start;
		if(pTokens[i].type != JSMN_STRING)
			continue;
		if(!memcmp(szJson + pTokens[i].start, "width", fuwWidth))
			*pWidth = strtoul(szJson + pTokens[++i].start, 0, 10);
		else if(!memcmp(szJson + pTokens[i].start, "height", fuwWidth))
			*pHeight = strtoul(szJson + pTokens[++i].start, 0, 10);
		else if(!memcmp(szJson + pTokens[i].start, "controlPoints", fuwWidth))
			*pControlPointCount = pTokens[++i].size;
	}
}

void mapJsonReadTiles(
	char *szJson, jsmntok_t *pTokens, FUWORD fuwTokenCnt,
	FUBYTE *pSpawnCount
) {
	// Find 'tiles' in JSON
	FUBYTE fubFound = 0;
	FUWORD i;
	for(i = 1; i < fuwTokenCnt && !fubFound; ++i) { // Skip first obj
		FUWORD fuwWidth = pTokens[i].end - pTokens[i].start;
		if(pTokens[i].type != JSMN_STRING)
			continue;
		if(memcmp(szJson + pTokens[i].start, "tiles", fuwWidth))
			continue;
		fubFound = 1;
	}
	if(!fubFound) {
		logWrite("JSON 'tiles' array not found!");
	}

	// Tiles found - check row count
	if(pTokens[i].size != g_fubMapTileHeight) {
		logWrite(
			"Only %d rows provided, %"PRI_FUBYTE"expected\n",
			pTokens[i].size, g_fubMapTileHeight
		);
		return;
	}

	// Do some reading
	*pSpawnCount = 0;
	for(FUBYTE y = 0; y < g_fubMapTileHeight; ++y) {
		++i;
		FUWORD fuwWidth = pTokens[i].end - pTokens[i].start;

		// Sanity check
		if(pTokens[i].type != JSMN_STRING) {
			logWrite("Unexpected row type @y %"PRI_FUBYTE": %d", y, pTokens[i].type);
			return;
		}
		if(fuwWidth != g_fubMapTileWidth) {
			logWrite(
				"Row @y %"PRI_FUBYTE" is too short: %"PRI_FUWORD", expected %"PRI_FUBYTE"\n",
				y, fuwWidth, g_fubMapTileWidth
			);
			return;
		}

		// Read row
		for(FUBYTE x = 0; x < fuwWidth; ++x) {
			g_pMap[x][y].ubIdx = (UBYTE)szJson[pTokens[i].start + x];
			g_pMap[x][y].ubData = BUILDING_IDX_INVALID;
			if(
				g_pMap[x][y].ubIdx == MAP_LOGIC_SPAWN0 ||
				g_pMap[x][y].ubIdx == MAP_LOGIC_SPAWN1 ||
				g_pMap[x][y].ubIdx == MAP_LOGIC_SPAWN2
			)
				++*pSpawnCount;
		}
	}
}

void mapCreate(char *szPath) {
	UBYTE ubTileIdx;
	tSimpleBufferManager *pManager;

	logBlockBegin("mapCreate(szPath: %s)", szPath);
	g_ubPendingTileCount = 0;

	// Read whole file to string
	FILE *pMapFile = fopen(szPath, "rb");
	if(!pMapFile)
		logWrite("ERR: File doesn't exist: %s\n", szPath);
	fseek(pMapFile, 0, SEEK_END);
	ULONG ulFileSize = ftell(pMapFile);
	fseek(pMapFile, 0, SEEK_SET);
	UBYTE *szMapFileContent = memAllocFast(ulFileSize+1);
	fread(szMapFileContent, 1, ulFileSize, pMapFile);
	szMapFileContent[ulFileSize] = '\0';
	fclose(pMapFile);

	jsmn_parser sJsonParser;
	jsmn_init(&sJsonParser);

	// Count tokens & alloc
	FWORD fwTokenCount = jsmn_parse(&sJsonParser, szMapFileContent, ulFileSize+1, 0, 0);
	if(fwTokenCount < 0) {
		logWrite("ERR: JSON token counting: %"PRI_FWORD"\n", fwTokenCount);
		return;
	}
	jsmntok_t *pJsonTokens = memAllocFast(fwTokenCount * sizeof(jsmntok_t));

	// Read tokens
	jsmn_init(&sJsonParser);
	FWORD fwResult = jsmn_parse(
		&sJsonParser, szMapFileContent, ulFileSize+1, pJsonTokens, fwTokenCount
	);
	if(fwResult < 0) {
		logWrite("ERR: JSON tokenize: %"PRI_FWORD"\n", fwResult);
		return;
	}

	// Objects may have properties passed in random order
	// so 1st pass will extract only general data
	FUBYTE fubControlPointCount;
	mapJsonGetMeta(
		szMapFileContent, pJsonTokens, fwTokenCount,
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
	mapJsonReadTiles(szMapFileContent, pJsonTokens, fwTokenCount, &fubSpawnCount);
	spawnManagerCreate(fubSpawnCount);

	memFree(pJsonTokens, fwTokenCount * sizeof(jsmntok_t));
	memFree(szMapFileContent, ulFileSize+1);
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
							:MAP_LOGIC_SENTRY1? TEAM_GREEN
							:TEAM_BROWN
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
	FUBYTE x, y;
	UBYTE ubTileIdx, ubOutTile;

	for(x = 0; x != g_fubMapTileWidth; ++x) {
		for(y = 0; y != g_fubMapTileHeight; ++y) {
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
			return MAP_TILE_SPAWN0;
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
		case MAP_LOGIC_SENTRY0:
		case MAP_LOGIC_SENTRY1:
		case MAP_LOGIC_SENTRY2:
			return MAP_TILE_WALL;
		case MAP_LOGIC_CAPTURE0:
			return MAP_TILE_CAPTURE0;
		case MAP_LOGIC_CAPTURE1:
			return MAP_TILE_CAPTURE1;
		case MAP_LOGIC_CAPTURE2:
			return MAP_TILE_CAPTURE2;
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
