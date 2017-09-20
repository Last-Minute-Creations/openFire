#ifndef GUARD_OF_GAMESTATES_GAME_PLAYER_H
#define GUARD_OF_GAMESTATES_GAME_PLAYER_H

#include <ace/config.h>
#include "gamestates/game/vehicle.h"

#define PLAYER_NAME_MAX        20

#define PLAYER_DEATH_COOLDOWN 150

#define PLAYER_STATE_OFF       0 /* Offline */
#define PLAYER_STATE_LIMBO     1 /* Dead / in bunker */
#define PLAYER_STATE_SURFACING 2 /* Animation out of bunker */
#define PLAYER_STATE_DRIVING   3 /* On map */
#define PLAYER_STATE_PARKING   4 /* Changing angle to facing south */
#define PLAYER_STATE_HIDING    5 /* Animating to bunker */

typedef struct _tPlayer {
	// General
	char szName[PLAYER_NAME_MAX];
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

void playerLocalProcessInput(void);

extern tPlayer *g_pPlayers;
extern tPlayer *g_pLocalPlayer;
extern UBYTE g_ubPlayerLimit; /// Defined by current server
extern UBYTE g_ubPlayerCount;

#endif
