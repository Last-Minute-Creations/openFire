#ifndef GUARD_OF_GAMESTATES_GAME_BUILDING_H
#define GUARD_OF_GAMESTATES_GAME_BUILDING_H

#include "gamestates/game/turret.h"

#define BUILDING_IDX_INVALID 0

#define BUILDING_TYPE_WALL   0
#define BUILDING_TYPE_AMMO   1
#define BUILDING_TYPE_FUEL   2
#define BUILDING_TYPE_FLAG   3
#define BUILDING_TYPE_TURRET 4

#define BUILDING_DAMAGED   1
#define BUILDING_DESTROYED 0

#define BUILDING_MAX_COUNT 256

typedef struct {
	UWORD uwTurretIdx;
	UBYTE ubHp;
} tBuilding;

typedef struct {
	tBuilding pBuildings[BUILDING_MAX_COUNT];
	UBYTE ubLastIdx;
} tBuildingManager;

void buildingManagerReset(void);

UBYTE buildingAdd(
	IN UBYTE ubX,
	IN UBYTE ubY,
	IN UBYTE ubType,
	IN UBYTE ubTeam
);

UBYTE buildingDamage(
	IN UBYTE ubIdx,
	IN UBYTE ubDamage
);

#endif // GUARD_OF_GAMESTATES_GAME_BUILDING_H
