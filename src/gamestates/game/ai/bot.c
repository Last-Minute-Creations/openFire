#include "gamestates/game/ai/bot.h"
#include <fixmath/fix16.h>
#include "gamestates/game/spawn.h"
#include "gamestates/game/ai/astar.h"

#define AI_BOT_STATE_IDLE           0
#define AI_BOT_STATE_MOVING_TO_NODE 1
#define AI_BOT_STATE_NODE_REACHED   2
#define AI_BOT_STATE_HOLDING_POS    3
#define AI_BOT_STATE_BLOCKED        4

static tBot *s_pBots;
static FUBYTE s_fubBotCount;
static FUBYTE s_fubBotLimit;

static void botSay(tBot *pBot, char *szFmt, ...) {
#ifdef AI_BOT_DEBUG
	char szBfr[100];
	va_list vArgs;
	va_start(vArgs, szFmt);
	vsprintf(szBfr, szFmt, vArgs);
	playerSay(pBot->pPlayer, szBfr, 0);
	va_end(vArgs);
#endif // #ifdef AI_BOT_DEBUG
}

void botManagerCreate(FUBYTE fubBotLimit) {
	logBlockBegin("botManagerCreate(fubBotLimit: %"PRI_FUBYTE")", fubBotLimit);
	s_fubBotCount = 0;
	s_pBots = memAllocFastClear(sizeof(tBot) * fubBotLimit);
	s_fubBotLimit = fubBotLimit;
	logBlockEnd("botManagerCreate()");
}

void botManagerDestroy(void) {
	logBlockBegin("botManagerDestroy()");
	for(UBYTE i = s_fubBotCount; i--;)
		astarDestroy(s_pBots[i].pNavData);
	memFree(s_pBots, sizeof(tBot) * s_fubBotLimit);
	logBlockEnd("botManagerDestroy()");
}

void botAdd(const char *szName, UBYTE ubTeam) {
	tBot *pBot = &s_pBots[s_fubBotCount];
	pBot->pPlayer = playerAdd(szName, ubTeam);
	if(!pBot->pPlayer) {
		logWrite("ERR: No more room for bots\n");
		return;
	}
	pBot->ubState = AI_BOT_STATE_IDLE;
	pBot->ubTick = 0;
	pBot->uwNextX = 0;
	pBot->uwNextY = 0;
	pBot->ubNextAngle = 0;
	++s_fubBotCount;
	pBot->pNavData = astarCreate();
	pBot->pNavData->sRoute.ubCurrNode = 0;

	botSay(pBot, "Ich bin ein computer");
}

void botRemoveByPtr(tBot *pBot) {
	astarDestroy(pBot->pNavData);
}

void botRemoveByName(const char *szName) {
	for(UBYTE i = 0; i < s_fubBotCount; ++i) {
		if(!strcmp(s_pBots[i].pPlayer->szName, szName)) {
			botRemoveByPtr(&s_pBots[i]);
			return;
		}
	}
}

/**
 * Finds next destination for AI's vehicle and sets up pathfinding data.
 * @param pBot: Pointer to bot.
 * @param pDestToEvade: Node which shouldn't be used as next destination.
 * @return Pointer to first node on new route.
 */
static tAiNode *botFindNewTarget(tBot *pBot, tAiNode *pDestToEvade) {
	// Find capture point which is neutral or needs defending or being
	// attacked or nearest to attack
	// TODO remaining variants, prioritize
	for(FUBYTE i = 0; i != g_fubCaptureNodeCount; ++i) {
		if(
			g_pCaptureNodes[i]->pControlPoint->fubTeam != pBot->pPlayer->ubTeam &&
			g_pCaptureNodes[i] != pDestToEvade
		) {
			tAiNode *pRouteEnd = g_pCaptureNodes[i];
			botSay(
				pBot, "New target at %"PRI_FUBYTE",%"PRI_FUBYTE,
				pRouteEnd->fubX, pRouteEnd->fubY
			);
			tAiNode *pRouteStart = aiFindClosestNode(
				pBot->pPlayer->sVehicle.uwX >> MAP_TILE_SIZE,
				pBot->pPlayer->sVehicle.uwY >> MAP_TILE_SIZE
			);
			astarStart(pBot->pNavData, pRouteStart, pRouteEnd);
			return pRouteStart;
		}
	}
	return 0;
}

UWORD botTargetNearbyTurret(tBot *pBot) {
	// Octants: E, SE, S, SW, W, NW, N, NE
	UWORD uwBotTileX = pBot->pPlayer->sVehicle.uwX >> MAP_TILE_SIZE;
	UWORD uwBotTileY = pBot->pPlayer->sVehicle.uwY >> MAP_TILE_SIZE;
	UBYTE ubOctant = ((pBot->pPlayer->sVehicle.ubTurretAngle+4) & 63) >> 3;


	return TURRET_INVALID;
}

void botTarget(tBot *pBot) {
	UBYTE ubEnemyTeam = pBot->pPlayer->ubTeam == TEAM_BLUE ? TEAM_RED : TEAM_BLUE;

	// Player, turret or wall
	tPlayer *pTargetPlayer = playerGetClosestInRange(
		pBot->pPlayer->sVehicle.uwX, pBot->pPlayer->sVehicle.uwY,
		PROJECTILE_RANGE, ubEnemyTeam
	);
	if(pTargetPlayer) {
		return;
	}

	UWORD uwTurretTargetIdx = botTargetNearbyTurret(pBot);

	// if(/* wall on course*/) {
	// 	return;
	// }
}

void botProcessDriving(tBot *pBot) {
	static UWORD uwCnt = 0;
	switch(pBot->ubState) {
		case AI_BOT_STATE_IDLE:
			if(pBot->pNavData->ubState == ASTAR_STATE_OFF) {
				uwCnt = 0;
				botFindNewTarget(pBot, pBot->pNavData->sRoute.pNodes[0]);
			}
			else {
				if(!astarProcess(pBot->pNavData))
					break;
				tAiNode *pNextNode = pBot->pNavData->sRoute.pNodes[pBot->pNavData->sRoute.ubCurrNode];
				pBot->uwNextX = (pNextNode->fubX << MAP_TILE_SIZE) + MAP_HALF_TILE;
				pBot->uwNextY = (pNextNode->fubY << MAP_TILE_SIZE) + MAP_HALF_TILE;
				botSay(pBot, "Going to %hu,%hu\n", pNextNode->fubX, pNextNode->fubY);
				pBot->ubTick = 10;
				pBot->ubState = AI_BOT_STATE_MOVING_TO_NODE;
			}
			break;
		case AI_BOT_STATE_MOVING_TO_NODE:
			// Update target angle
			if(pBot->ubTick == 10) {
				pBot->ubTick = 0;
				pBot->ubNextAngle = getAngleBetweenPoints(
					pBot->pPlayer->sVehicle.uwX, pBot->pPlayer->sVehicle.uwY,
					pBot->uwNextX, pBot->uwNextY
				);
			}
			else
				++pBot->ubTick;
			// Check if in destination
			if(
				ABS(pBot->uwNextX - pBot->pPlayer->sVehicle.uwX) < (MAP_HALF_TILE/2) &&
				ABS(pBot->uwNextY - pBot->pPlayer->sVehicle.uwY) < (MAP_HALF_TILE/2)
			) {
				pBot->pPlayer->sSteerRequest.ubForward = 0;
				pBot->pPlayer->sSteerRequest.ubBackward = 0;
				pBot->pPlayer->sSteerRequest.ubLeft = 0;
				pBot->pPlayer->sSteerRequest.ubRight = 0;
				pBot->ubState = AI_BOT_STATE_NODE_REACHED;
				break;
			}

			// Steer to proper angle
			BYTE bDir = getDeltaAngleDirection(pBot->pPlayer->sVehicle.ubBodyAngle, pBot->ubNextAngle, 1);
			if(bDir) {
				if(bDir > 0) {
					pBot->pPlayer->sSteerRequest.ubLeft = 0;
					pBot->pPlayer->sSteerRequest.ubRight = 1;
				}
				else if(bDir < 0) {
					pBot->pPlayer->sSteerRequest.ubLeft = 1;
					pBot->pPlayer->sSteerRequest.ubRight = 0;
				}
				break;
			}

			// Check field in front for collisions
			pBot->pPlayer->sSteerRequest.ubLeft = 0;
			pBot->pPlayer->sSteerRequest.ubRight = 0;
			UWORD uwChkX = pBot->pPlayer->sVehicle.uwX + fix16_to_int(
				(3*MAP_FULL_TILE)/2 * ccos(pBot->pPlayer->sVehicle.ubBodyAngle)
			);
			UWORD uwChkY = pBot->pPlayer->sVehicle.uwY + fix16_to_int(
				(3*MAP_FULL_TILE)/2 * csin(pBot->pPlayer->sVehicle.ubBodyAngle)
			);
			if(playerAnyNearPoint(uwChkX, uwChkY, MAP_FULL_TILE)) {
				pBot->ubState = AI_BOT_STATE_BLOCKED;
				pBot->ubTick = 0;
				pBot->pPlayer->sSteerRequest.ubForward = 0;
				break;
			}

			// Move until close to node
			pBot->pPlayer->sSteerRequest.ubForward = 1;
			break;
		case AI_BOT_STATE_NODE_REACHED:
			if(!pBot->pNavData->sRoute.ubCurrNode) {
				// Last node from route - hold pos
				pBot->ubTick = 0;
				pBot->ubState = AI_BOT_STATE_HOLDING_POS;
				botSay(pBot, "Destination reached - holding pos");
			}
			else {
				// Get next node from route
				--pBot->pNavData->sRoute.ubCurrNode;
				tAiNode *pNextNode = pBot->pNavData->sRoute.pNodes[pBot->pNavData->sRoute.ubCurrNode];
				pBot->uwNextX = (pNextNode->fubX << MAP_TILE_SIZE) + MAP_HALF_TILE;
				pBot->uwNextY = (pNextNode->fubY << MAP_TILE_SIZE) + MAP_HALF_TILE;
				pBot->ubTick = 10;
				pBot->ubState = AI_BOT_STATE_MOVING_TO_NODE;
				botSay(
					pBot, "Moving to next pos: %hu, %hu\n",
					pNextNode->fubX, pNextNode->fubY
				);
			}
			break;
		case AI_BOT_STATE_HOLDING_POS:
			// Move to tile next to it to prevent blocking other players/bots
			// If point has been captured start measuring ticks
			if(pBot->ubTick >= 200) {
				tAiNode *pLastNode = pBot->pNavData->sRoute.pNodes[0];
				// After some ticks check if work is done on this point
				if(pLastNode->pControlPoint->fubTeam == pBot->pPlayer->ubTeam) {
					// If so, go to next point
					botFindNewTarget(pBot, 0);
					pBot->ubState = AI_BOT_STATE_IDLE;
				}
				else
					botSay(pBot, "capturing...");
				pBot->ubTick = 0;
			}
			else
				++pBot->ubTick;
			break;
		case AI_BOT_STATE_BLOCKED: {
			UWORD uwChkX = pBot->pPlayer->sVehicle.uwX + fix16_to_int(
				(3*MAP_FULL_TILE)/2 * ccos(pBot->pPlayer->sVehicle.ubBodyAngle)
			);
			UWORD uwChkY = pBot->pPlayer->sVehicle.uwY + fix16_to_int(
				(3*MAP_FULL_TILE)/2 * csin(pBot->pPlayer->sVehicle.ubBodyAngle)
			);
			if(playerAnyNearPoint(uwChkX, uwChkY, MAP_FULL_TILE)) {
				if(pBot->ubTick == 50) {
					// Change target & route
					botFindNewTarget(pBot, pBot->pNavData->sRoute.pNodes[0]);
					pBot->ubState = AI_BOT_STATE_IDLE;
				}
				else
					++pBot->ubTick;
			}
			else {
				// Continue your route
				pBot->ubState = AI_BOT_STATE_MOVING_TO_NODE;
				pBot->ubTick = 0;
			}
		} break;
	}
}

void botProcessLimbo(tBot *pBot) {
	if(pBot->pNavData->ubState == ASTAR_STATE_OFF) {
		// Find some place to go - e.g. capture point
		tAiNode *pFirstNode = botFindNewTarget(pBot, 0);
		// Find nearest spawn point
		pBot->pPlayer->ubSpawnIdx = spawnGetNearest(
			pFirstNode->fubX, pFirstNode->fubY,
			pBot->pPlayer->ubTeam
		);
		// After arriving at surface, recalculate where bot is going
		pBot->ubState = AI_BOT_STATE_IDLE;
	}
	// Make sure noone from own team stands at spawn
	// TODO: spawn kill ppl from other team
	if(!pBot->pPlayer->uwCooldown && !spawnIsCoveredByAnyPlayer(pBot->pPlayer->ubSpawnIdx)) {
		playerSelectVehicle(pBot->pPlayer, VEHICLE_TYPE_TANK);
		botSay(pBot, "Surfacing...");
	}
}

void botProcess(void) {
	for(FUBYTE i = 0; i != s_fubBotCount; ++i) {
		tBot *pBot = &s_pBots[i];
		switch(pBot->pPlayer->ubState) {
			case PLAYER_STATE_DRIVING:
				botProcessDriving(pBot);
				break;
			case PLAYER_STATE_LIMBO:
				botProcessLimbo(pBot);
				break;
		}
	}
}
