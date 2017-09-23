#include "gamestates/game/spawn.h"
#include <ace/macros.h>
#include <ace/managers/log.h>
#include <ace/managers/blit.h>
#include "gamestates/game/game.h"
#include "gamestates/game/player.h"
#include "gamestates/game/team.h"

tSpawn *g_pSpawns;
static UBYTE s_ubSpawnCount;
static UBYTE s_ubSpawnMaxCount;
static tBitMap *s_pGreenAnims;
static tBitMap *s_pBrownAnims;

void spawnManagerCreate(UBYTE ubMaxCount) {
	s_ubSpawnMaxCount = ubMaxCount;
	s_ubSpawnCount = 0;
	g_pSpawns = memAllocFastClear(sizeof(tSpawn) * ubMaxCount);
	s_pGreenAnims = bitmapCreateFromFile("data/vehicles/bunkering_green.bm");
	// TODO: bunkering_brown.bm
	s_pBrownAnims = bitmapCreateFromFile("data/vehicles/bunkering_green.bm");
}

void spawnManagerDestroy(void) {
	memFree(g_pSpawns, sizeof(tSpawn) * s_ubSpawnMaxCount);
	bitmapDestroy(s_pGreenAnims);
	bitmapDestroy(s_pBrownAnims);
}

UBYTE spawnAdd(UBYTE ubTileX, UBYTE ubTileY, UBYTE ubTeam) {
	if(s_ubSpawnCount == s_ubSpawnMaxCount) {
		logWrite("ERR: No more room for spawns");
		return 0;
	}
	tSpawn *pSpawn = &g_pSpawns[s_ubSpawnCount];

	pSpawn->ubBusy = SPAWN_BUSY_NOT;
	pSpawn->ubFrame = 0;
	pSpawn->ubTeam = ubTeam;
	pSpawn->ubTileX = ubTileX;
	pSpawn->ubTileY = ubTileY;

	return s_ubSpawnCount++;
}

void spawnCapture(UBYTE ubSpawnIdx, UBYTE ubTeam) {
	g_pSpawns[ubSpawnIdx].ubTeam = ubTeam;
}

UBYTE spawnGetNearest(UBYTE ubTileX, UBYTE ubTileY, UBYTE ubTeam) {
	UBYTE ubNearestIdx = SPAWN_INVALID;
	UWORD uwNearestDist = 0xFF;
	for(uint_fast8_t i = 0; i != s_ubSpawnCount; ++i) {
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
	for(uint_fast8_t i = 0; i != s_ubSpawnCount; ++i) {
		if(g_pSpawns[i].ubTileX == ubTileX || g_pSpawns[i].ubTileY == ubTileY)
			return i;
	}
	return SPAWN_INVALID;
}

void spawnSetBusy(UBYTE ubSpawnIdx, UBYTE ubBusyType, UBYTE ubVehicleType) {
	tSpawn *pSpawn = &g_pSpawns[ubSpawnIdx];
	pSpawn->ubBusy = ubBusyType;
	if(ubBusyType == SPAWN_BUSY_BUNKERING || ubBusyType == SPAWN_BUSY_SURFACING)
		pSpawn->ubFrame = 0;
	// TODO capturing
}

void spawnSim(void) {
	for(uint_fast8_t i = 0; i != s_ubSpawnCount; ++i) {
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
				pSpawn->ubTeam == TEAM_GREEN ? s_pGreenAnims : s_pBrownAnims,
				0, ubFrameIdx << MAP_TILE_SIZE,
				g_pWorldMainBfr->pBuffer,
				pSpawn->ubTileX << MAP_TILE_SIZE, pSpawn->ubTileY << MAP_TILE_SIZE,
				MAP_FULL_TILE, MAP_FULL_TILE
			);
		}
	}
	// TODO capturing
}
