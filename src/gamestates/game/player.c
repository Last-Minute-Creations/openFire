#include "gamestates/game/player.h"
#include <string.h>
#include <ace/config.h>
#include <ace/macros.h>
#include <ace/managers/memory.h>
#include <ace/managers/log.h>
#include <ace/managers/mouse.h>
#include <ace/managers/key.h>
#include "gamestates/game/vehicle.h"
#include "gamestates/game/explosions.h"
#include "gamestates/game/game.h"
#include "gamestates/game/spawn.h"
#include "gamestates/game/team.h"
#include "gamestates/game/control.h"

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
	memFree(g_pPlayers, g_ubPlayerLimit * sizeof(tPlayer));
}

/**
 *  @todo Vehicle count from server rules
 *  @todo Vehicle global for whole team
 */
tPlayer *playerAdd(char *szName, UBYTE ubTeam) {
	for(FUBYTE i = 0; i != g_ubPlayerLimit; ++i) {
		if(g_pPlayers[i].szName[0])
			continue;
		tPlayer *pPlayer = &g_pPlayers[i];
		strcpy(pPlayer->szName, szName);
		pPlayer->ubTeam = ubTeam;
		pPlayer->ubState = PLAYER_STATE_LIMBO;
		pPlayer->ubCurrentVehicleType = 0xFF;
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
	vehicleInit(&pPlayer->sVehicle, ubVehicleType, pPlayer->ubSpawnIdx);
}

void playerHideInBunker(tPlayer *pPlayer, FUBYTE fubSpawnIdx) {
	vehicleUnset(&pPlayer->sVehicle);
	pPlayer->ubState = PLAYER_STATE_BUNKERING;
	pPlayer->uwCooldown = PLAYER_SURFACING_COOLDOWN;
	spawnSetBusy(fubSpawnIdx, SPAWN_BUSY_BUNKERING, VEHICLE_TYPE_TANK);
	if(pPlayer == g_pLocalPlayer)
		displayPrepareLimbo(fubSpawnIdx);
}

void playerDamageVehicle(tPlayer *pPlayer, UBYTE ubDamage) {
	if(pPlayer->sVehicle.ubLife <= ubDamage) {
		explosionsAdd(
			fix16_to_int(pPlayer->sVehicle.fX) - (VEHICLE_BODY_WIDTH >> 1),
			fix16_to_int(pPlayer->sVehicle.fY) - (VEHICLE_BODY_HEIGHT >> 1)
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
		displayPrepareLimbo(SPAWN_INVALID);
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
				fix16_to_int(g_pLocalPlayer->sVehicle.fX),
				fix16_to_int(g_pLocalPlayer->sVehicle.fY),
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
				if(inRect(uwMouseX, uwMouseY, sTankRect)) {
					playerSelectVehicle(g_pLocalPlayer, VEHICLE_TYPE_TANK);
					g_pLocalPlayer->ubState = PLAYER_STATE_SURFACING;
					g_pLocalPlayer->uwCooldown = PLAYER_SURFACING_COOLDOWN;
					displayPrepareDriving();
				}
			}
		} break;
	}
}

void playerSimVehicle(tPlayer *pPlayer) {
	tVehicle *pVehicle;
	UWORD uwVx, uwVy, uwVTileX, uwVTileY;
	UWORD uwSiloDx, uwSiloDy;
	UBYTE ubTileType;

	pVehicle = &pPlayer->sVehicle;

	uwVx = fix16_to_int(pVehicle->fX);
	uwVy = fix16_to_int(pVehicle->fY);
	uwVTileX = uwVx >> MAP_TILE_SIZE;
	uwVTileY = uwVy >> MAP_TILE_SIZE;
	ubTileType = g_pMap[uwVTileX][uwVTileY].ubIdx;

	// Drowning
	if(ubTileType == MAP_LOGIC_WATER) {
		playerLoseVehicle(pPlayer);
		return;
	}

	// Standing on silos
	g_ubDoSiloHighlight = 0;
	if(ubTileType == MAP_LOGIC_SPAWN1 || ubTileType == MAP_LOGIC_SPAWN2) {
		UBYTE ubSpawnIdx = spawnGetAt(uwVTileX, uwVTileY);
		if(
			ubSpawnIdx != SPAWN_INVALID && (
				g_pSpawns[ubSpawnIdx].ubBusy == SPAWN_BUSY_BUNKERING ||
				g_pSpawns[ubSpawnIdx].ubBusy == SPAWN_BUSY_SURFACING
			)
		) {
			// If one of them is standing on moving platform, destroy vehicle
			playerDamageVehicle(pPlayer, 200);
		}
		else if(
			(pPlayer->ubTeam == TEAM_GREEN && ubTileType == MAP_LOGIC_SPAWN1) ||
			(pPlayer->ubTeam == TEAM_BROWN && ubTileType == MAP_LOGIC_SPAWN2)
		) {
			// Standing on own, unoccupied silo
			uwSiloDx = uwVx & (MAP_FULL_TILE - 1);
			uwSiloDy = uwVy & (MAP_FULL_TILE - 1);
			if(uwSiloDx > 12 && uwSiloDx < 18 && uwSiloDy > 12 && uwSiloDy < 18) {
				// Standing on bunkerable position
				if(pPlayer == g_pLocalPlayer) {
					// If one of them is local player, save data for highlight draw
					g_ubDoSiloHighlight = 1;
					g_uwSiloHighlightTileX = uwVTileX;
					g_uwSiloHighlightTileY = uwVTileY;
					// Hide in bunker
					if(pPlayer->sSteerRequest.ubAction1) {
						playerHideInBunker(pPlayer, ubSpawnIdx);
						return;
					}
				}
			}
		}
	}

	// Increase counters for control point domination
	for(FUBYTE i = 0; i != g_fubControlPointCount; ++i) {
		// Calc distance
		// Increase vehicle count near control point for given team
		if(
			ABS(uwVTileX - g_pControlPoints[i].fubTileX) <= 2 &&
			ABS(uwVTileY - g_pControlPoints[i].fubTileY) <= 2
		) {
			if(pPlayer->ubTeam == TEAM_GREEN)
				++g_pControlPoints[i].fubGreenCount;
			else
				++g_pControlPoints[i].fubBrownCount;
			break; // Player can't be in two bases at same time
		}
	}

	// Calculate vehicle positions based on steer requests
	switch(pPlayer->ubCurrentVehicleType) {
		case VEHICLE_TYPE_TANK:
			vehicleSteerTank(pVehicle, &pPlayer->sSteerRequest);
			break;
		case VEHICLE_TYPE_JEEP:
			vehicleSteerJeep(pVehicle, &pPlayer->sSteerRequest);
			break;
	}
}

/**
 * Updates players' state machine.
 */
void playerSim(void) {
	UBYTE ubPlayer;
	tPlayer *pPlayer;

	for(ubPlayer = 0; ubPlayer != g_ubPlayerLimit; ++ubPlayer) {
		pPlayer = &g_pPlayers[ubPlayer];
		switch(pPlayer->ubState) {
			case PLAYER_STATE_OFF:
				continue;
			case PLAYER_STATE_SURFACING:
				if(pPlayer->uwCooldown)
					--pPlayer->uwCooldown;
				else {
					pPlayer->ubState = PLAYER_STATE_DRIVING;
					// TODO: somewhere else?
					pPlayer->sVehicle.pBob->ubFlags = BOB_FLAG_START_DRAWING;
					if(pPlayer->ubCurrentVehicleType == VEHICLE_TYPE_TANK)
						pPlayer->sVehicle.pAuxBob->ubFlags = BOB_FLAG_START_DRAWING;
					else
						pPlayer->sVehicle.pAuxBob->ubFlags = BOB_FLAG_NODRAW;
				}
				continue;
			case PLAYER_STATE_BUNKERING:
				if(pPlayer->uwCooldown)
					--pPlayer->uwCooldown;
				else
					pPlayer->ubState = PLAYER_STATE_LIMBO;
				continue;
			case PLAYER_STATE_DRIVING:
				playerSimVehicle(pPlayer);
				continue;
		}
	}
}

tPlayer *g_pPlayers;
UBYTE g_ubPlayerLimit;
UBYTE g_ubPlayerCount;
tPlayer *g_pLocalPlayer;
