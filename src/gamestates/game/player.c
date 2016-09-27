#include "gamestates/game/player.h"
#include <string.h>
#include <ace/config.h>
#include <ace/managers/memory.h>
#include <ace/managers/log.h>
#include "gamestates/game/vehicle.h"

void playerListCreate(UBYTE ubPlayerLimit) {
	g_ubPlayerLimit = ubPlayerLimit;
	g_pPlayers = memAllocFastClear(ubPlayerLimit * sizeof(tPlayer));
}

void playerListDestroy() {
	UBYTE i;
	
	for(i = 0; i != g_ubPlayerLimit; ++i)
		if(g_pPlayers[i].szName[0])
			playerRemoveByPtr(&g_pPlayers[i]);
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
		memset(pPlayer, 0, sizeof(tPlayer));
		strcpy(pPlayer->szName, szName);
		pPlayer->ubTeam = ubTeam;
		pPlayer->pVehiclesLeft[VEHICLE_TYPE_TANK] = 4;
		pPlayer->pVehiclesLeft[VEHICLE_TYPE_JEEP] = 10;
		++g_ubPlayerCount;
		return pPlayer;
	}
	logWrite("Can't add player %s - no more slots\n", szName);
	return 0;
}

void playerRemoveByIdx(UBYTE ubPlayerIdx) {	
	if(ubPlayerIdx > g_ubPlayerLimit) {
		logWrite(
			"ERR: Tried to remove player %hhu, player limit %hhu",
			ubPlayerIdx,
			g_ubPlayerLimit
		);
		return;
	}
	playerRemoveByPtr(&g_pPlayers[ubPlayerIdx]);
}

void playerRemoveByPtr(tPlayer *pPlayer) {
	if(pPlayer->pCurrentVehicle)
		vehicleDestroy(pPlayer->pCurrentVehicle);
	if(!pPlayer->szName[0]) {
		logWrite("ERR: Tried to remove non-existing player: %p", pPlayer);
		return;
	}
	memset(pPlayer, 0, sizeof(tPlayer));
	--g_ubPlayerCount;
}

void playerSelectVehicle(tPlayer *pPlayer, UBYTE ubVehicleType) {
	pPlayer->ubCurrentVehicleType = ubVehicleType;
	pPlayer->pCurrentVehicle = vehicleCreate(ubVehicleType);
}

void playerHideInBunker(tPlayer *pPlayer) {
	vehicleDestroy(pPlayer->pCurrentVehicle);
	pPlayer->pCurrentVehicle = 0;
}

void playerLoseVehicle(tPlayer *pPlayer) {
	vehicleDestroy(pPlayer->pCurrentVehicle);
	pPlayer->pCurrentVehicle = 0;
	if(pPlayer->pVehiclesLeft[pPlayer->ubCurrentVehicleType])
		--pPlayer->pVehiclesLeft[pPlayer->ubCurrentVehicleType];
}

tPlayer *g_pPlayers;
UBYTE g_ubPlayerLimit;
UBYTE g_ubPlayerCount;
tPlayer *g_pLocalPlayer;