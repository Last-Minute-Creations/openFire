#ifndef GUARD_OF_GAMESTATES_GAME_SPAWN_H
#define GUARD_OF_GAMESTATES_GAME_SPAWN_H

#include <ace/types.h>

#define SPAWN_BUSY_NOT 0
#define SPAWN_BUSY_CAPTURING 1 /* Final capturing stage - noone may spawn there */
#define SPAWN_BUSY_SURFACING 2
#define SPAWN_BUSY_BUNKERING 3

typedef struct _tSpawn {
	UBYTE ubTileY;
	UBYTE ubTileX;
	UBYTE ubTeam;
	UBYTE ubBusy;
	UBYTE ubFrame;
	UBYTE ubVehicleType;
} tSpawn;

void spawnManagerCreate(
	IN UBYTE ubMaxCount
);

void spawnManagerDestroy(void);

UBYTE spawnAdd(
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

void spawnSetBusy(
	IN UBYTE ubSpawnIdx,
	IN UBYTE ubBusyType,
	IN UBYTE ubVehicleType
);

void spawnAnimate(
	IN UBYTE ubSpawnIdx
);

extern tSpawn *g_pSpawns;

#endif // GUARD_OF_GAMESTATES_GAME_SPAWN_H
