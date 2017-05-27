#include "gamestates/game/player.h"
#include <string.h>
#include <ace/config.h>
#include <ace/managers/memory.h>
#include <ace/managers/log.h>
#include "gamestates/game/vehicle.h"
#include "gamestates/game/world.h"

void playerListCreate(UBYTE ubPlayerLimit) {
	UBYTE i;

	g_ubPlayerLimit = ubPlayerLimit;
	g_pPlayers = memAllocFastClear(ubPlayerLimit * sizeof(tPlayer));
	for(i = 0; i != ubPlayerLimit; ++i) {
		g_pPlayers[i].sVehicle.pBob = bobCreate(
			g_pVehicleTypes[0].sMainSource.pBitmap,
			g_pVehicleTypes[0].sMainSource.pMask,
			VEHICLE_BODY_HEIGHT, angleToFrame(ANGLE_90)
		);
		g_pPlayers[i].sVehicle.pBob->ubFlags = BOB_FLAG_NODRAW;

		g_pPlayers[i].sVehicle.pAuxBob = bobCreate(
			g_pVehicleTypes[0].sAuxSource.pBitmap,
			g_pVehicleTypes[0].sAuxSource.pMask,
			VEHICLE_TURRET_HEIGHT, angleToFrame(ANGLE_90)
		);
		g_pPlayers[i].sVehicle.pAuxBob->ubFlags = BOB_FLAG_NODRAW;
	}
}

void playerListDestroy() {
	UBYTE i;
	
	for(i = 0; i != g_ubPlayerLimit; ++i) {
		if(g_pPlayers[i].ubState != PLAYER_STATE_OFF)
			playerRemoveByPtr(&g_pPlayers[i]);
		bobDestroy(g_pPlayers[i].sVehicle.pBob);
		bobDestroy(g_pPlayers[i].sVehicle.pAuxBob);
	}
	memFree(g_pPlayers, sizeof(tPlayer));
}

/**
 *  @todo Vehicle count from server rules
 *  @todo Vehicle global for whole team
 */
tPlayer *playerAdd(char *szName, UBYTE ubTeam) {
	UBYTE i;
	tPlayer *pPlayer;
	
	for(i = 0; i != g_ubPlayerLimit; ++i) {
		if(g_pPlayers[i].szName[0])
			continue;
		pPlayer = &g_pPlayers[i];
		strcpy(pPlayer->szName, szName);
		pPlayer->ubTeam = ubTeam;
		pPlayer->ubState = PLAYER_STATE_BUNKERED;
		pPlayer->pVehiclesLeft[VEHICLE_TYPE_TANK] = 4;
		pPlayer->pVehiclesLeft[VEHICLE_TYPE_JEEP] = 10;
		++g_ubPlayerCount;
		logWrite("Player %s joined team %hu, player idx: %hu\n", szName, ubTeam, i);
		return pPlayer;
	}
	logWrite("Can't add player %s - no more slots\n", szName);
	return 0;
}

void playerRemoveByIdx(UBYTE ubPlayerIdx) {	
	if(ubPlayerIdx > g_ubPlayerLimit) {
		logWrite(
			"ERR: Tried to remove player %hhu, player limit %hhu\n",
			ubPlayerIdx,
			g_ubPlayerLimit
		);
		return;
	}
	playerRemoveByPtr(&g_pPlayers[ubPlayerIdx]);
}

void playerRemoveByPtr(tPlayer *pPlayer) {
	if(pPlayer->sVehicle.ubLife)
		vehicleUnset(&pPlayer->sVehicle);
	if(!pPlayer->ubState == PLAYER_STATE_OFF) {
		logWrite("ERR: Tried to remove offline player: %p\n", pPlayer);
		return;
	}
	memset(pPlayer, 0, sizeof(tPlayer));
	--g_ubPlayerCount;
}

void playerSelectVehicle(tPlayer *pPlayer, UBYTE ubVehicleType) {
	pPlayer->ubCurrentVehicleType = ubVehicleType;
	vehicleInit(&pPlayer->sVehicle, ubVehicleType);
}

void playerHideInBunker(tPlayer *pPlayer) {
	vehicleUnset(&pPlayer->sVehicle);
	pPlayer->ubState = PLAYER_STATE_BUNKERED;
	if(pPlayer == g_pLocalPlayer)
		worldHide();
}

void playerLoseVehicle(tPlayer *pPlayer) {
	vehicleUnset(&pPlayer->sVehicle);
	if(pPlayer->pVehiclesLeft[pPlayer->ubCurrentVehicleType])
		--pPlayer->pVehiclesLeft[pPlayer->ubCurrentVehicleType];
	pPlayer->ubState = PLAYER_STATE_DEAD;
	pPlayer->uwCooldown = PLAYER_DEATH_COOLDOWN;
}

tPlayer *g_pPlayers;
UBYTE g_ubPlayerLimit;
UBYTE g_ubPlayerCount;
tPlayer *g_pLocalPlayer;
