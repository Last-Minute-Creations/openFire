#ifndef GUARD_OF_GAMESTATES_GAME_PLAYER_H
#define GUARD_OF_GAMESTATES_GAME_PLAYER_H

#include <ace/config.h>
#include "gamestates/game/vehicle.h"

#define PLAYER_DEATH_COOLDOWN 150

#define PLAYER_STATE_OFF       0
#define PLAYER_STATE_DEAD      1
#define PLAYER_STATE_BUNKERED  2
#define PLAYER_STATE_SURFACING 3
#define PLAYER_STATE_DRIVING   4

typedef struct _tPlayer {
	// General
	char szName[20];
	UBYTE ubTeam;
	UBYTE ubCurrentVehicleType;
	UBYTE ubState;
	UWORD uwCooldown;
	tVehicle sVehicle;
	tSteerRequest sSteerRequest;

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

void playerDamageVehicle(
	IN tPlayer *pPlayer,
	IN UBYTE ubDamage
);

void playerLoseVehicle(
	IN tPlayer *pPlayer
);

void playerSteerVehicle(
	IN tPlayer *pPlayer
);

extern tPlayer *g_pPlayers;
extern tPlayer *g_pLocalPlayer;
extern UBYTE g_ubPlayerLimit; /// Defined by current server
extern UBYTE g_ubPlayerCount;

#endif
