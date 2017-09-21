#include "gamestates/game/player.h"
#include <string.h>
#include <ace/config.h>
#include <ace/managers/memory.h>
#include <ace/managers/log.h>
#include <ace/managers/mouse.h>
#include <ace/managers/key.h>
#include "gamestates/game/vehicle.h"
#include "gamestates/game/explosions.h"
#include "gamestates/game/game.h"

// Steer requests
#define OF_KEY_FORWARD      KEY_W
#define OF_KEY_BACKWARD     KEY_S
#define OF_KEY_LEFT         KEY_A
#define OF_KEY_RIGHT        KEY_D
#define OF_KEY_ACTION1      KEY_F
#define OF_KEY_ACTION2      KEY_R
#define OF_KEY_ACTION3      KEY_V

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
		pPlayer->ubState = PLAYER_STATE_LIMBO;
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
	pPlayer->ubState = PLAYER_STATE_HIDING;
	if(pPlayer == g_pLocalPlayer) {
		// TODO something
	}
}

void playerDamageVehicle(tPlayer *pPlayer, UBYTE ubDamage) {
	if(pPlayer->sVehicle.ubLife <= ubDamage) {
		explosionsAdd(
			pPlayer->sVehicle.fX - (VEHICLE_BODY_WIDTH >> 1),
			pPlayer->sVehicle.fY - (VEHICLE_BODY_HEIGHT >> 1)
		);
		playerLoseVehicle(pPlayer);
	}
	else
		pPlayer->sVehicle.ubLife -= ubDamage;
}

void playerLoseVehicle(tPlayer *pPlayer) {
	pPlayer->sVehicle.ubLife = 0;
	vehicleUnset(&pPlayer->sVehicle);
	if(pPlayer->pVehiclesLeft[pPlayer->ubCurrentVehicleType])
		--pPlayer->pVehiclesLeft[pPlayer->ubCurrentVehicleType];
	pPlayer->ubState = PLAYER_STATE_LIMBO;
	if(pPlayer == g_pLocalPlayer) {
		pPlayer->uwCooldown = PLAYER_DEATH_COOLDOWN;
		gameEnterLimbo();
	}
}

void playerLocalProcessInput(void) {
	switch(g_pLocalPlayer->ubState) {
		case PLAYER_STATE_DRIVING: {
			// Receive player's steer request
			tSteerRequest *pReq = &g_pLocalPlayer->sSteerRequest;
			pReq->ubForward     = keyCheck(OF_KEY_FORWARD);
			pReq->ubBackward    = keyCheck(OF_KEY_BACKWARD);
			pReq->ubLeft        = keyCheck(OF_KEY_LEFT);
			pReq->ubRight       = keyCheck(OF_KEY_RIGHT);
			pReq->ubAction1     = mouseCheck(MOUSE_LMB);
			pReq->ubAction2     = mouseCheck(MOUSE_RMB);
			pReq->ubAction3     = keyCheck(OF_KEY_ACTION3);

			pReq->ubDestAngle = getAngleBetweenPoints(
				g_pLocalPlayer->sVehicle.fX, g_pLocalPlayer->sVehicle.fY,
				g_pWorldCamera->uPos.sUwCoord.uwX + g_uwMouseX,
				g_pWorldCamera->uPos.sUwCoord.uwY + g_uwMouseY
			);
		} break;
		case PLAYER_STATE_LIMBO: {
			if(mouseUse(MOUSE_LMB)) {
				const UWORD uwHudOffs = 192 + 1 + 2; // + black line + border
				tUwRect sTankRect = {
					.uwX = 2 + 5, .uwY = uwHudOffs + 5, .uwWidth = 28, .uwHeight = 20
				};
				UWORD uwMouseX = mouseGetX(), uwMouseY = mouseGetY();
				#define inRect(x, y, r) (                \
					x >= r.uwX && x <= r.uwX + r.uwWidth   \
					&& y >= r.uwY && y <= r.uwY+r.uwHeight \
				)
				if(inRect(uwMouseX, uwMouseY, sTankRect)) {
					playerSelectVehicle(g_pLocalPlayer, VEHICLE_TYPE_TANK);
					gameEnterDriving();
				}
			}
		} break;
	}
}

tPlayer *g_pPlayers;
UBYTE g_ubPlayerLimit;
UBYTE g_ubPlayerCount;
tPlayer *g_pLocalPlayer;
