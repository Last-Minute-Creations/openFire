#include "gamestates/game/spawn.h"
#include <ace/macros.h>
#include <ace/managers/log.h>

tSpawn g_pSpawns[SPAWN_MAX];
static UBYTE s_ubSpawnCount = 0;

UBYTE spawnCreate(UBYTE ubTileX, UBYTE ubTileY, UBYTE ubTeam) {
	if(s_ubSpawnCount == SPAWN_MAX) {
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

UBYTE spawnFindNearest(UBYTE ubTileX, UBYTE ubTileY, UBYTE ubTeam) {
	UBYTE ubNearestIdx = 0xFF;
	UWORD uwNearestDist = 0xFF;
	for(UBYTE i = 0; i != s_ubSpawnCount; ++i) {
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
