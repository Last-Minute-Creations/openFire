#ifndef GUARD_OF_GAMESTATES_GAME_PLAYER_H
#define GUARD_OF_GAMESTATES_GAME_PLAYER_H

#include <ace/config.h>
#include "gamestates/game/vehicle.h"

#define TEAM_GREEN 0
#define TEAM_BROWN 1

typedef struct _tPlayer {
	// General
	char szName[20];
	UBYTE ubTeam;
	UBYTE ubCurrentVehicleType;
	tVehicle *pCurrentVehicle;
	
	// Vehicles available
	UBYTE pVehiclesLeft[4];
	
	// Stats for score table displaying
	UBYTE ubHasFlag;
	UBYTE pVehiclesKilled[4];
} tPlayer;

void playerListCreate(
	IN UBYTE ubPlayerLimit
);

void playerListDestroy(void);

tPlayer *playerAdd(
	IN char *szName,
	IN UBYTE ubTeam
);

void playerRemoveByIdx(
	IN UBYTE ubPlayerIdx
);

void playerRemoveByPtr(
	IN tPlayer *pPlayer
);

void playerSelectVehicle(
	tPlayer *pPlayer,
	UBYTE ubVehicleType
);

void playerHideInBunker(
	IN tPlayer *pPlayer
);

void playerLoseVehicle(
	IN tPlayer *pPlayer
);

extern tPlayer *g_pPlayers;
extern tPlayer *g_pLocalPlayer;
extern UBYTE g_ubPlayerLimit; /// Defined by current server
extern UBYTE g_ubPlayerCount;

#endif