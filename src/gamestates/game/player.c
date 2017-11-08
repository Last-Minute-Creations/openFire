#include "gamestates/game/player.h"
#include <string.h>
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
#include "gamestates/game/console.h"

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
			g_pVehicleTypes[VEHICLE_TYPE_TANK].pMainFrames[TEAM_BLUE],
			g_pVehicleTypes[VEHICLE_TYPE_TANK].pMainMask,
			g_pVehicleTypes[VEHICLE_TYPE_TANK].pMainFrameOffsets,
			VEHICLE_BODY_HEIGHT, angleToFrame(ANGLE_90)
		);
		g_pPlayers[i].sVehicle.pBob->ubState = BOB_STATE_NODRAW;

		g_pPlayers[i].sVehicle.pAuxBob = bobCreate(
			g_pVehicleTypes[VEHICLE_TYPE_TANK].pAuxFrames[TEAM_BLUE],
			g_pVehicleTypes[VEHICLE_TYPE_TANK].pAuxMask,
			g_pVehicleTypes[VEHICLE_TYPE_TANK].pAuxFrameOffsets,
			VEHICLE_TURRET_HEIGHT, angleToFrame(ANGLE_90)
		);
		g_pPlayers[i].sVehicle.pAuxBob->ubState = BOB_STATE_NODRAW;
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

void playerMoveToLimbo(tPlayer *pPlayer, FUBYTE fubSpawnIdx) {
	if(fubSpawnIdx == SPAWN_INVALID)
		pPlayer->ubSpawnIdx = spawnGetNearest(
			pPlayer->sVehicle.uwX >> MAP_TILE_SIZE,
			pPlayer->sVehicle.uwY >> MAP_TILE_SIZE,
			pPlayer->ubTeam
		);
	else
		pPlayer->ubSpawnIdx = fubSpawnIdx;

	pPlayer->sVehicle.uwX = (g_pSpawns[fubSpawnIdx].ubTileX << MAP_TILE_SIZE) + MAP_HALF_TILE;
	pPlayer->sVehicle.uwY = (g_pSpawns[fubSpawnIdx].ubTileY << MAP_TILE_SIZE) + MAP_HALF_TILE;
	pPlayer->sVehicle.fX = fix16_from_int(pPlayer->sVehicle.uwX);
	pPlayer->sVehicle.fY = fix16_from_int(pPlayer->sVehicle.uwY);
	if(pPlayer == g_pLocalPlayer)
		displayPrepareLimbo();
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
		char szMessage[35];
		if(ubTeam == TEAM_BLUE)
			sprintf(szMessage, "%s joined BLUE", szName);
		else
			sprintf(szMessage, "%s joined RED", szName);
		consoleWrite(szMessage, CONSOLE_COLOR_GENERAL);

		// Set vehicle coordinates 'cuz playerMoveToLimbo finds closest spawn
		pPlayer->sVehicle.uwX = 0;
		pPlayer->sVehicle.uwY = 0;
		playerMoveToLimbo(pPlayer, SPAWN_INVALID);
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
	if(pPlayer->uwCooldown)
		return;
	pPlayer->ubCurrentVehicleType = ubVehicleType;
	vehicleInit(&pPlayer->sVehicle, ubVehicleType, pPlayer->ubSpawnIdx);
	pPlayer->ubState = PLAYER_STATE_SURFACING;
	pPlayer->uwCooldown = PLAYER_SURFACING_COOLDOWN;
	if(pPlayer == g_pLocalPlayer)
		displayPrepareDriving();
}

void playerHideInBunker(tPlayer *pPlayer, FUBYTE fubSpawnIdx) {
	vehicleUnset(&pPlayer->sVehicle);
	pPlayer->ubState = PLAYER_STATE_BUNKERING;
	pPlayer->uwCooldown = PLAYER_SURFACING_COOLDOWN;
	spawnSetBusy(fubSpawnIdx, SPAWN_BUSY_BUNKERING, VEHICLE_TYPE_TANK);
	playerMoveToLimbo(pPlayer, fubSpawnIdx);
}

FUBYTE playerDamageVehicle(tPlayer *pPlayer, UBYTE ubDamage) {
	if(pPlayer->sVehicle.ubLife <= ubDamage) {
		explosionsAdd(
			pPlayer->sVehicle.uwX - (VEHICLE_BODY_HEIGHT >> 1),
			pPlayer->sVehicle.uwY - (VEHICLE_BODY_WIDTH >> 1)
		);
		playerLoseVehicle(pPlayer);
		return 1;
	}
	else
		pPlayer->sVehicle.ubLife -= ubDamage;
	return 0;
}

void playerLoseVehicle(tPlayer *pPlayer) {
	pPlayer->sVehicle.ubLife = 0;
	if(g_pTeams[pPlayer->ubTeam].uwTicketsLeft)
		--g_pTeams[pPlayer->ubTeam].uwTicketsLeft;
	vehicleUnset(&pPlayer->sVehicle);
	if(pPlayer->pVehiclesLeft[pPlayer->ubCurrentVehicleType])
		--pPlayer->pVehiclesLeft[pPlayer->ubCurrentVehicleType];
	pPlayer->ubState = PLAYER_STATE_LIMBO;
	pPlayer->uwCooldown = PLAYER_DEATH_COOLDOWN;
	playerMoveToLimbo(pPlayer, SPAWN_INVALID);
}

void playerLocalProcessInput(void) {
	switch(g_pLocalPlayer->ubState) {
		case PLAYER_STATE_DRIVING: {
			// Receive player's steer request
			tSteerRequest *pReq = &g_pLocalPlayer->sSteerRequest;
			if(g_isChatting) {
				if(keyUse(g_sKeyManager.ubLastKey))
					g_isChatting = consoleChatProcessChar(g_sKeyManager.ubLastKey);
				pReq->ubForward  = 0;
				pReq->ubBackward = 0;
				pReq->ubLeft     = 0;
				pReq->ubRight    = 0;
				pReq->ubAction3  = 0;
			}
			else {
				pReq->ubForward  = keyCheck(OF_KEY_FORWARD);
				pReq->ubBackward = keyCheck(OF_KEY_BACKWARD);
				pReq->ubLeft     = keyCheck(OF_KEY_LEFT);
				pReq->ubRight    = keyCheck(OF_KEY_RIGHT);
				pReq->ubAction3  = keyCheck(OF_KEY_ACTION3);
			}
			pReq->ubAction1 = mouseCheck(MOUSE_LMB);
			pReq->ubAction2 = mouseCheck(MOUSE_RMB);

			pReq->ubDestAngle = getAngleBetweenPoints(
				g_pLocalPlayer->sVehicle.uwX,
				g_pLocalPlayer->sVehicle.uwY,
				g_pWorldCamera->uPos.sUwCoord.uwX + g_uwMouseX,
				g_pWorldCamera->uPos.sUwCoord.uwY + g_uwMouseY
			);
		} break;
		case PLAYER_STATE_LIMBO: {
			if(g_isChatting) {
				if(keyUse(g_sKeyManager.ubLastKey))
					g_isChatting = consoleChatProcessChar(g_sKeyManager.ubLastKey);
			}
			else if(mouseUse(MOUSE_LMB)) {
				const UWORD uwHudOffs = 192 + 1 + 2; // + black line + border
				tUwRect sTankRect = {
					.uwX = 2 + 4, .uwY = uwHudOffs +1+4, .uwWidth = 28, .uwHeight = 20
				};
				tUwRect sJeepRect = {
					.uwX = 2 + 38, .uwY = uwHudOffs +1+35, .uwWidth = 28, .uwHeight = 20
				};
				UWORD uwMouseX = mouseGetX(), uwMouseY = mouseGetY();
				if(inRect(uwMouseX, uwMouseY, sTankRect))
					playerSelectVehicle(g_pLocalPlayer, VEHICLE_TYPE_TANK);
				else if(inRect(uwMouseX, uwMouseY, sJeepRect))
					playerSelectVehicle(g_pLocalPlayer, VEHICLE_TYPE_JEEP);
			}
		} break;
	}
}

UBYTE playerCheckDeathFromSpawn(tPlayer *pPlayer) {
	tBCoordYX *pCollisionPts = pPlayer->sVehicle.pType->pCollisionPts[pPlayer->sVehicle.ubBodyAngle >> 1].pPts;

	// Unrolling for best results
	FUBYTE fubTileX = (pPlayer->sVehicle.uwX + pCollisionPts[0].bX) >> MAP_TILE_SIZE;
	FUBYTE fubTileY = (pPlayer->sVehicle.uwY + pCollisionPts[0].bY) >> MAP_TILE_SIZE;
	FUBYTE fubSpawnIdx = spawnGetAt(fubTileX, fubTileY);
	if(fubSpawnIdx != SPAWN_INVALID && g_pSpawns[fubSpawnIdx].ubBusy != SPAWN_BUSY_NOT)
		goto kill;

	fubTileX = (pPlayer->sVehicle.uwX + pCollisionPts[2].bX) >> MAP_TILE_SIZE;
	fubTileY = (pPlayer->sVehicle.uwY + pCollisionPts[2].bY) >> MAP_TILE_SIZE;
	fubSpawnIdx = spawnGetAt(fubTileX, fubTileY);
	if(fubSpawnIdx != SPAWN_INVALID && g_pSpawns[fubSpawnIdx].ubBusy != SPAWN_BUSY_NOT)
		goto kill;

	fubTileX = (pPlayer->sVehicle.uwX + pCollisionPts[5].bX) >> MAP_TILE_SIZE;
	fubTileY = (pPlayer->sVehicle.uwY + pCollisionPts[5].bY) >> MAP_TILE_SIZE;
	fubSpawnIdx = spawnGetAt(fubTileX, fubTileY);
	if(fubSpawnIdx != SPAWN_INVALID && g_pSpawns[fubSpawnIdx].ubBusy != SPAWN_BUSY_NOT)
		goto kill;

	fubTileX = (pPlayer->sVehicle.uwX + pCollisionPts[7].bX) >> MAP_TILE_SIZE;
	fubTileY = (pPlayer->sVehicle.uwY + pCollisionPts[7].bY) >> MAP_TILE_SIZE;
	fubSpawnIdx = spawnGetAt(fubTileX, fubTileY);
	if(fubSpawnIdx != SPAWN_INVALID && g_pSpawns[fubSpawnIdx].ubBusy != SPAWN_BUSY_NOT)
		goto kill;

	return 0;
kill:
	playerDamageVehicle(pPlayer, 200);
	return 1;
}

void playerSimVehicle(tPlayer *pPlayer) {
	tVehicle *pVehicle;
	UWORD uwVx, uwVy, uwVTileX, uwVTileY;
	UWORD uwSiloDx, uwSiloDy;
	UBYTE ubTileType;

	pVehicle = &pPlayer->sVehicle;

	uwVx = pVehicle->uwX;
	uwVy = pVehicle->uwY;
	uwVTileX = uwVx >> MAP_TILE_SIZE;
	uwVTileY = uwVy >> MAP_TILE_SIZE;
	ubTileType = g_pMap[uwVTileX][uwVTileY].ubIdx;
	g_ubDoSiloHighlight = 0;

	// Drowning
	if(ubTileType == MAP_LOGIC_WATER) {
		playerLoseVehicle(pPlayer);
		char szBfr[CONSOLE_MESSAGE_MAX];
		sprintf(szBfr, "%s has drowned", pPlayer->szName);
		consoleWrite(szBfr, CONSOLE_COLOR_GENERAL);
		return;
	}

	// Death from moving spawn
	if(playerCheckDeathFromSpawn(pPlayer)) {
		return;
	}

	// Standing on own, unoccupied spawn
	if(ubTileType == MAP_LOGIC_SPAWN1 || ubTileType == MAP_LOGIC_SPAWN2) {
		if(
			(pPlayer->ubTeam == TEAM_BLUE && ubTileType == MAP_LOGIC_SPAWN1) ||
			(pPlayer->ubTeam == TEAM_RED && ubTileType == MAP_LOGIC_SPAWN2)
		) {
			uwSiloDx = uwVx & (MAP_FULL_TILE - 1);
			uwSiloDy = uwVy & (MAP_FULL_TILE - 1);
			if(
				pPlayer == g_pLocalPlayer &&
				uwSiloDx > 12 && uwSiloDx < 18 && uwSiloDy > 12 && uwSiloDy < 18
			) {
				// Standing on bunkerable position
				// If one of them is local player, save data for highlight draw
				g_ubDoSiloHighlight = 1;
				g_uwSiloHighlightX = uwVTileX << MAP_TILE_SIZE;
				g_uwSiloHighlightY = uwVTileY << MAP_TILE_SIZE;
				// Hide in bunker
				if(pPlayer->sSteerRequest.ubAction1) {
					playerHideInBunker(pPlayer, spawnGetAt(uwVTileX, uwVTileY));
					return;
				}
			}
		}
	}

	// Increase counters for control point domination
	for(FUBYTE i = g_fubControlPointCount; i--;) {
		// Calc distance
		// Increase vehicle count near control point for given team
		if(
			ABS(uwVTileX - g_pControlPoints[i].fubTileX) <= 2 &&
			ABS(uwVTileY - g_pControlPoints[i].fubTileY) <= 2
		) {
			if(pPlayer->ubTeam == TEAM_BLUE)
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

	for(ubPlayer = g_ubPlayerLimit; ubPlayer--;) {
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
					pPlayer->sVehicle.pBob->ubState = BOB_STATE_START_DRAWING;
					if(pPlayer->ubCurrentVehicleType == VEHICLE_TYPE_TANK)
						pPlayer->sVehicle.pAuxBob->ubState = BOB_STATE_START_DRAWING;
					else
						pPlayer->sVehicle.pAuxBob->ubState = BOB_STATE_NODRAW;
				}
				spawnAnimate(pPlayer->ubSpawnIdx);
				continue;
			case PLAYER_STATE_BUNKERING:
				if(pPlayer->uwCooldown)
					--pPlayer->uwCooldown;
				else
					pPlayer->ubState = PLAYER_STATE_LIMBO;
				spawnAnimate(pPlayer->ubSpawnIdx);
				continue;
			case PLAYER_STATE_DRIVING:
				playerSimVehicle(pPlayer);
				vehicleDraw(&pPlayer->sVehicle);
				continue;
			case PLAYER_STATE_LIMBO:
				if(pPlayer->uwCooldown)
					--pPlayer->uwCooldown;
				continue;
		}
	}
}

UBYTE playerAnyNearPoint(UWORD uwChkX, UWORD uwChkY, UWORD uwDist) {
	for(FUBYTE i = g_ubPlayerCount; i--;) {
		if(g_pPlayers[i].ubState != PLAYER_STATE_DRIVING)
			continue;
		if(
			ABS(g_pPlayers[i].sVehicle.uwX - uwChkX) < uwDist &&
			ABS(g_pPlayers[i].sVehicle.uwY - uwChkY) < uwDist
		)
			return 1;
	}
	return 0;
}

void playerSay(tPlayer *pPlayer, char *szMsg, UBYTE isSayTeam) {
	char szBfr[PLAYER_NAME_MAX + CONSOLE_MESSAGE_MAX + 2+1];
	strcpy(szBfr, pPlayer->szName);
	strcat(szBfr, ": ");
	strcat(szBfr, szMsg);
	UBYTE ubColor;
	if(isSayTeam)
		ubColor = pPlayer->ubTeam == TEAM_BLUE ? CONSOLE_COLOR_BLUE : CONSOLE_COLOR_RED;
	else
		ubColor = CONSOLE_COLOR_SAY;
	consoleWrite(szBfr,	ubColor);

	// TODO send to server
}

tPlayer *g_pPlayers;
UBYTE g_ubPlayerLimit;
UBYTE g_ubPlayerCount;
tPlayer *g_pLocalPlayer;
