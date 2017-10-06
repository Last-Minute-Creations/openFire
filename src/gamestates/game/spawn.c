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
	s_pGreenAnims = bitmapCreateFromFile("data/vehicles/blue/bunkering.bm");
	// TODO: bunkering_brown.bm
	s_pBrownAnims = bitmapCreateFromFile("data/vehicles/red/bunkering.bm");
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
	for(FUBYTE i = g_ubSpawnCount; i--;) {
		if(g_pSpawns[i].ubTileX == ubTileX && g_pSpawns[i].ubTileY == ubTileY)
			return i;
	}
	return SPAWN_INVALID;
}

void spawnSetBusy(FUBYTE fubSpawnIdx, FUBYTE fubBusyType, FUBYTE fubVehicleType) {
	tSpawn *pSpawn = &g_pSpawns[fubSpawnIdx];
	pSpawn->ubBusy = fubBusyType;
	if(fubBusyType == SPAWN_BUSY_BUNKERING || fubBusyType == SPAWN_BUSY_SURFACING) {
		pSpawn->ubFrame = 0;
		pSpawn->ubVehicleType = fubVehicleType;
	}
	// TODO capturing
}

void spawnSim(void) {
	for(FUBYTE i = 0; i != g_ubSpawnCount; ++i) {
		tSpawn *pSpawn = &g_pSpawns[i];
		if(pSpawn->ubBusy == SPAWN_BUSY_BUNKERING || pSpawn->ubBusy == SPAWN_BUSY_SURFACING) {
			if(pSpawn->ubFrame <= PLAYER_SURFACING_COOLDOWN)
				++pSpawn->ubFrame;
			else
				pSpawn->ubBusy = SPAWN_BUSY_NOT;
		}
		// TODO capturing
	}
}

void spawnAnimate(UBYTE ubSpawnIdx) {
	tSpawn *pSpawn = &g_pSpawns[ubSpawnIdx];
	UBYTE ubFrameIdx;
	if(pSpawn->ubBusy == SPAWN_BUSY_NOT)
		return; // Most likely
	if(pSpawn->ubBusy == SPAWN_BUSY_BUNKERING || pSpawn->ubBusy == SPAWN_BUSY_SURFACING) {
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
	// TODO capturing
}
