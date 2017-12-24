#ifndef GUARD_OF_GAMESTATES_GAME_PLAYER_H
#define GUARD_OF_GAMESTATES_GAME_PLAYER_H

#include "gamestates/game/vehicle.h"

#define PLAYER_NAME_MAX        20

#define PLAYER_DEATH_COOLDOWN 150
#define PLAYER_SURFACING_COOLDOWN 60

#define PLAYER_STATE_OFF       0 /* Offline */
#define PLAYER_STATE_LIMBO     1 /* Dead / in bunker */
#define PLAYER_STATE_SURFACING 2 /* Animation out of bunker */
#define PLAYER_STATE_DRIVING   3 /* On map */
#define PLAYER_STATE_PARKING   4 /* Changing angle to facing south */
#define PLAYER_STATE_BUNKERING 5 /* Animating to bunker */

typedef struct _tPlayer {
	// General
	char szName[PLAYER_NAME_MAX];
	UBYTE ubTeam;
	UBYTE ubCurrentVehicleType;
	UBYTE ubState;
	UWORD uwCooldown;
	tVehicle sVehicle;
	tSteerRequest sSteerRequest;

	UBYTE ubSpawnIdx;
	// Stats for score table displaying - for CTF
	UBYTE ubHasFlag;
	// Vehicles available - for last man standing
	UBYTE pVehiclesLeft[4];
	// Score - kills?
	UWORD uwScore;
} tPlayer;

void playerListCreate(
	IN UBYTE ubPlayerLimit
);

void playerListDestroy(void);

tPlayer *playerAdd(
	IN const char *szName,
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
	IN tPlayer *pPlayer,
	IN FUBYTE fubSpawnIdx
);

FUBYTE playerDamageVehicle(
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

void playerSim(void);

void playerSimVehicle(
	IN tPlayer *pPlayer
);

void playerSay(
	IN tPlayer *pPlayer,
	IN char *szMsg,
	IN UBYTE isSayTeam
);

inline tPlayer *playerGetByVehicle(
	IN tVehicle *pVehicle
) {
	UBYTE *pVehicleByteAddr = (UBYTE*)pVehicle;
	tPlayer sPlayer;
	UBYTE ubDist = ((UBYTE*)&sPlayer.sVehicle) - ((UBYTE*)&sPlayer);
	return (tPlayer*)(pVehicleByteAddr - ubDist);
}

UBYTE playerAnyNearPoint(
	IN UWORD uwChkX,
	IN UWORD uwChkY,
	IN UWORD uwDist
);

tPlayer *playerGetClosestInRange(
	IN UWORD uwX,
	IN UWORD uwY,
	IN UWORD uwRange,
	IN UBYTE ubTeam
);

extern tPlayer *g_pPlayers;
extern tPlayer *g_pLocalPlayer;
extern UBYTE g_ubPlayerLimit; /// Defined by current server
extern UBYTE g_ubPlayerCount;

#endif
