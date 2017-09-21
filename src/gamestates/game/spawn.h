#ifndef GUARD_OF_GAMESTATES_GAME_SPAWN_H
#define GUARD_OF_GAMESTATES_GAME_SPAWN_H

#include <ace/types.h>

#define SPAWN_MAX 10

#define SPAWN_BUSY_NOT 0
#define SPAWN_BUSY_SURFACING 1
#define SPAWN_BUSY_BUNKERING 2
#define SPAWN_BUSY_CAPTURING 3 /* Final capturing stage - noone may spawn there */

typedef struct _tSpawn {
	UBYTE ubTileY;
	UBYTE ubTileX;
	UBYTE ubTeam;
	UBYTE ubBusy;
	UBYTE ubFrame;
} tSpawn;

UBYTE spawnCreate(
	IN UBYTE ubTileX,
	IN UBYTE ubTileY,
	IN UBYTE ubTeam
);

void spawnCapture(
	IN UBYTE ubSpawnIdx,
	IN UBYTE ubTeam
);

UBYTE spawnFindNearest(
	IN UBYTE ubTileX,
	IN UBYTE ubTileY,
	IN UBYTE ubTeam
);

extern tSpawn g_pSpawns[SPAWN_MAX];

#endif // GUARD_OF_GAMESTATES_GAME_SPAWN_H
