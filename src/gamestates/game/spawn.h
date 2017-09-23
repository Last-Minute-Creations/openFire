#ifndef GUARD_OF_GAMESTATES_GAME_SPAWN_H
#define GUARD_OF_GAMESTATES_GAME_SPAWN_H

#include <ace/types.h>

#define SPAWN_BUSY_NOT 0
#define SPAWN_BUSY_CAPTURING 1 /* Final capturing stage - noone may spawn there */
#define SPAWN_BUSY_SURFACING 2
#define SPAWN_BUSY_BUNKERING 3

#define SPAWN_INVALID 0xFF

typedef struct _tSpawn {
	UBYTE ubTileY;
	UBYTE ubTileX;
	UBYTE ubTeam;
	UBYTE ubBusy;
	UBYTE ubFrame;
	UBYTE ubVehicleType;
} tSpawn;

void spawnManagerCreate(
	IN uint_fast8_t fubMaxCount
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

UBYTE spawnGetNearest(
	IN UBYTE ubTileX,
	IN UBYTE ubTileY,
	IN UBYTE ubTeam
);

UBYTE spawnGetAt(
	IN UBYTE ubTileX,
	IN UBYTE ubTileY
);

void spawnSetBusy(
	IN uint_fast8_t fubSpawnIdx,
	IN uint_fast8_t fubBusyType,
	IN uint_fast8_t fubVehicleType
);

void spawnAnimate(
	IN UBYTE ubSpawnIdx
);

void spawnSim(void);

extern tSpawn *g_pSpawns;

#endif // GUARD_OF_GAMESTATES_GAME_SPAWN_H
