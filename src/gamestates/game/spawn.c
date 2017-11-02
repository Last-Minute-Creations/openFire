#include "gamestates/game/spawn.h"
#include <ace/macros.h>
#include <ace/managers/log.h>
#include <ace/managers/blit.h>
#include "gamestates/game/game.h"
#include "gamestates/game/player.h"
#include "gamestates/game/team.h"

tSpawn *g_pSpawns;
UBYTE g_ubSpawnCount;
static UBYTE s_ubSpawnMaxCount;
static tBitMap *s_pGreenAnims;
static tBitMap *s_pBrownAnims;

void spawnManagerCreate(FUBYTE fubMaxCount) {
	logBlockBegin("spawnManagerCreate(fubMaxCount: %"PRI_FUBYTE")", fubMaxCount);
	s_ubSpawnMaxCount = fubMaxCount;
	g_ubSpawnCount = 0;
	g_pSpawns = memAllocFastClear(sizeof(tSpawn) * fubMaxCount);
	s_pGreenAnims = bitmapCreateFromFile("data/vehicles/bunkering_blue.bm");
	// TODO: bunkering_brown.bm
	s_pBrownAnims = bitmapCreateFromFile("data/vehicles/bunkering_red.bm");
	logBlockEnd("spawnManagerCreate()");
}

void spawnManagerDestroy(void) {
	logBlockBegin("spawnManagerDestroy()");
	memFree(g_pSpawns, sizeof(tSpawn) * s_ubSpawnMaxCount);
	bitmapDestroy(s_pGreenAnims);
	bitmapDestroy(s_pBrownAnims);
	logBlockEnd("spawnManagerDestroy()");
}

UBYTE spawnAdd(UBYTE ubTileX, UBYTE ubTileY, UBYTE ubTeam) {
	if(g_ubSpawnCount == s_ubSpawnMaxCount) {
		logWrite("ERR: No more room for spawns");
		return 0;
	}
	tSpawn *pSpawn = &g_pSpawns[g_ubSpawnCount];

	pSpawn->ubBusy = SPAWN_BUSY_NOT;
	pSpawn->ubFrame = 0;
	pSpawn->ubTeam = ubTeam;
	pSpawn->ubTileX = ubTileX;
	pSpawn->ubTileY = ubTileY;

	return g_ubSpawnCount++;
}

void spawnCapture(UBYTE ubSpawnIdx, UBYTE ubTeam) {
	g_pSpawns[ubSpawnIdx].ubTeam = ubTeam;
	mapChangeTile(
		g_pSpawns[ubSpawnIdx].ubTileX, g_pSpawns[ubSpawnIdx].ubTileY,
		ubTeam == TEAM_BLUE ? MAP_LOGIC_SPAWN1
		: ubTeam == TEAM_RED ? MAP_LOGIC_SPAWN2
		: MAP_LOGIC_SPAWN0
	);
}

UBYTE spawnGetNearest(UBYTE ubTileX, UBYTE ubTileY, UBYTE ubTeam) {
	UBYTE ubNearestIdx = SPAWN_INVALID;
	UWORD uwNearestDist = 0xFF;
	for(FUBYTE i = 0; i != g_ubSpawnCount; ++i) {
		if(g_pSpawns[i].ubTeam != ubTeam)
			continue;
		// Maybe this will suffice
		// If not, sum of squares - no need for sqrting since actual range is irrelevant
		UWORD uwDist = ABS(g_pSpawns[i].ubTileX - ubTileX) + ABS(g_pSpawns[i].ubTileY - ubTileY);
		if(uwDist < uwNearestDist) {
			uwNearestDist = uwDist;
			ubNearestIdx = i;
		}
	}
	return ubNearestIdx;
}

UBYTE spawnGetAt(UBYTE ubTileX, UBYTE ubTileY) {
	if(
		g_pMap[ubTileX][ubTileY].ubIdx != MAP_LOGIC_SPAWN0 &&
		g_pMap[ubTileX][ubTileY].ubIdx != MAP_LOGIC_SPAWN1 &&
		g_pMap[ubTileX][ubTileY].ubIdx != MAP_LOGIC_SPAWN2
	)
		return SPAWN_INVALID;
	for(FUBYTE i = g_ubSpawnCount; i--;) {
		if(g_pSpawns[i].ubTileX == ubTileX && g_pSpawns[i].ubTileY == ubTileY)
			return i;
	}
	return SPAWN_INVALID;
}

void spawnSetBusy(FUBYTE fubSpawnIdx, FUBYTE fubBusyType, FUBYTE fubVehicleType) {
	tSpawn *pSpawn = &g_pSpawns[fubSpawnIdx];
	pSpawn->ubBusy = fubBusyType;
	pSpawn->ubFrame = 0;
	pSpawn->ubVehicleType = fubVehicleType;
}

void spawnSim(void) {
	for(FUBYTE i = 0; i != g_ubSpawnCount; ++i) {
		tSpawn *pSpawn = &g_pSpawns[i];
		if(pSpawn->ubFrame < PLAYER_SURFACING_COOLDOWN)
			++pSpawn->ubFrame;
		else
			pSpawn->ubBusy = SPAWN_BUSY_NOT;
	}
}

void spawnAnimate(UBYTE ubSpawnIdx) {
	tSpawn *pSpawn = &g_pSpawns[ubSpawnIdx];
	UBYTE ubFrameIdx;
	if(pSpawn->ubBusy == SPAWN_BUSY_NOT)
		return; // Most likely
	if(pSpawn->ubFrame == PLAYER_SURFACING_COOLDOWN)
		mapRequestUpdateTile(pSpawn->ubTileX, pSpawn->ubTileY);
	else {
		UBYTE ubFrameIdx = pSpawn->ubFrame / 10;
		if(pSpawn->ubBusy == SPAWN_BUSY_SURFACING)
			ubFrameIdx = 5 - ubFrameIdx;
		blitCopyAligned(
			pSpawn->ubTeam == TEAM_BLUE ? s_pGreenAnims : s_pBrownAnims,
			0, ubFrameIdx << MAP_TILE_SIZE,
			g_pWorldMainBfr->pBuffer,
			pSpawn->ubTileX << MAP_TILE_SIZE, pSpawn->ubTileY << MAP_TILE_SIZE,
			MAP_FULL_TILE, MAP_FULL_TILE
		);
	}
}

UBYTE spawnIsCoveredByAnyPlayer(UBYTE ubSpawnIdx) {
	tSpawn *pSpawn = &g_pSpawns[ubSpawnIdx];
	for(FUBYTE i = 0; i != g_ubPlayerCount; ++i) {
		tPlayer *pPlayer = &g_pPlayers[i];
		if(pPlayer->ubState != PLAYER_STATE_DRIVING)
			continue;
		if(
			ABS((pPlayer->sVehicle.uwX >> MAP_TILE_SIZE) - pSpawn->ubTileX) > 1 ||
			ABS((pPlayer->sVehicle.uwY >> MAP_TILE_SIZE) - pSpawn->ubTileY) > 1
		)
			continue;

		// Unrolled for performance
		tBCoordYX *pEdges = pPlayer->sVehicle.pType->pCollisionPts[pPlayer->sVehicle.ubBodyAngle >> 1].pPts;
		if(
			((pPlayer->sVehicle.uwX + pEdges[0].bX) >> MAP_TILE_SIZE) == pSpawn->ubTileX &&
			((pPlayer->sVehicle.uwY + pEdges[0].bY) >> MAP_TILE_SIZE) == pSpawn->ubTileY
		)
			return 1;
		if(
			((pPlayer->sVehicle.uwX + pEdges[2].bX) >> MAP_TILE_SIZE) == pSpawn->ubTileX &&
			((pPlayer->sVehicle.uwY + pEdges[2].bY) >> MAP_TILE_SIZE) == pSpawn->ubTileY
		)
			return 1;
		if(
			((pPlayer->sVehicle.uwX + pEdges[5].bX) >> MAP_TILE_SIZE) == pSpawn->ubTileX &&
			((pPlayer->sVehicle.uwY + pEdges[5].bY) >> MAP_TILE_SIZE) == pSpawn->ubTileY
		)
			return 1;
		if(
			((pPlayer->sVehicle.uwX + pEdges[7].bX) >> MAP_TILE_SIZE) == pSpawn->ubTileX &&
			((pPlayer->sVehicle.uwY + pEdges[7].bY) >> MAP_TILE_SIZE) == pSpawn->ubTileY
		)
			return 1;
	}
	return 0;
}
