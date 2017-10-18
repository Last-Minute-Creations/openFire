#include "gamestates/game/ai/bot.h"
#include <ace/libfixmath/fix16.h>
#include "gamestates/game/spawn.h"

#define AI_BOT_STATE_IDLE           0
#define AI_BOT_STATE_MOVING_TO_NODE 1
#define AI_BOT_STATE_NODE_REACHED   2
#define AI_BOT_STATE_HOLDING_POS    3
#define AI_BOT_STATE_BLOCKED        4

static tBot *s_pBots;
static FUBYTE s_fubBotCount;
static FUBYTE s_fubBotLimit;

void botManagerCreate(FUBYTE fubBotLimit) {
	logBlockBegin("botManagerCreate(fubBotLimit: %"PRI_FUBYTE")", fubBotLimit);
	s_fubBotCount = 0;
	s_pBots = memAllocFastClear(sizeof(tBot) * fubBotLimit);
	s_fubBotLimit = fubBotLimit;
	logBlockEnd("botManagerCreate()");
}

void botManagerDestroy(void) {
	logBlockBegin("botManagerDestroy()");
	memFree(s_pBots, sizeof(tBot) * s_fubBotLimit);
	logBlockEnd("botManagerDestroy()");
}

void botAdd(char *szName, UBYTE ubTeam) {
	tPlayer *pPlayer = playerAdd(szName, ubTeam);
	if(!pPlayer) {
		logWrite("ERR: No more room for bots\n");
		return;
	}
	tBot *pBot = &s_pBots[s_fubBotCount];
	pBot->pPlayer = pPlayer;
	pBot->pNextNode = 0;
	pBot->ubState = AI_BOT_STATE_IDLE;
	pBot->ubTick = 0;
	pBot->uwNextX = 0;
	pBot->uwNextY = 0;
	pBot->ubNextAngle = 0;
	++s_fubBotCount;
}

void botRemoveByName(char *szName) {

}

void botRemoveByPtr(tBot *pBot) {

}

void botSetupRoute(tBot *pBot, tAiNode *pNodeStart, tAiNode *pNodeEnd) {
	pBot->sRoute.ubNodeCount = 0;
	pBot->sRoute.ubCurrNode = 0;

	// TODO implement
	if(pBot->sRoute.ubNodeCount >= AI_BOT_ROUTE_NODE_MAX) {
		// Route too long
		logWrite(
			"WARN: Route too long "
			"from %"PRI_FUBYTE",%"PRI_FUBYTE" to %"PRI_FUBYTE",%"PRI_FUBYTE"\n",
			pNodeStart->fubX, pNodeStart->fubY, pNodeEnd->fubX, pNodeEnd->fubY
		);
		pBot->sRoute.ubNodeCount = 0;
		pBot->sRoute.ubCurrNode = 0;
		return;
	}
}

void botFindNewTarget(tBot *pBot, tAiNode *pNodeToEvade) {
	tAiNode *pRouteEnd = 0;

	// Find capture point which is neutral or needs defending or being
	// attacked or nearest to attack
	// TODO remaining variants, prioritize
	for(FUBYTE i = 0; i != g_fubCaptureNodeCount; ++i) {
		if(
			g_pCaptureNodes[i]->pControlPoint->fubTeam != pBot->pPlayer->ubTeam &&
			g_pCaptureNodes[i] != pNodeToEvade
		) {
			pRouteEnd = g_pCaptureNodes[i];
		}
	}
	if(!pRouteEnd)
		return;

	// TODO Find shortest route to destination
	tAiNode *pRouteStart = aiFindClosestNode(
		pBot->pPlayer->sVehicle.uwX >> MAP_TILE_SIZE,
		pBot->pPlayer->sVehicle.uwY >> MAP_TILE_SIZE
	);
	botSetupRoute(pBot, pRouteStart, pRouteEnd);

	// Set first node of route as next to reach
	if(pBot->sRoute.ubNodeCount) {
		pBot->pNextNode = &pBot->sRoute.pNodes[0];
		pBot->uwNextX = (pBot->pNextNode->fubX << MAP_TILE_SIZE) + MAP_HALF_TILE;
		pBot->uwNextY = (pBot->pNextNode->fubY << MAP_TILE_SIZE) + MAP_HALF_TILE;
	}
}

void botProcessDriving(tBot *pBot) {
	switch(pBot->ubState) {
		case AI_BOT_STATE_IDLE:
			botFindNewTarget(pBot, pBot->pNextNode);
			pBot->ubTick = 10;
			pBot->ubState = AI_BOT_STATE_MOVING_TO_NODE;
			break;
		case AI_BOT_STATE_MOVING_TO_NODE:
			// Update target angle
			if(pBot->ubTick == 10) {
				pBot->ubTick = 0;
				pBot->ubNextAngle = getAngleBetweenPoints(
					pBot->pPlayer->sVehicle.uwX,
					pBot->pPlayer->sVehicle.uwY,
					(pBot->pNextNode->fubX << MAP_TILE_SIZE) + MAP_HALF_TILE,
					(pBot->pNextNode->fubY << MAP_TILE_SIZE) + MAP_HALF_TILE
				);
			}
			else
				++pBot->ubTick;
			// Steer to proper angle
			BYTE bDir = getDeltaAngleDirection(pBot->pPlayer->sVehicle.ubBodyAngle, pBot->ubNextAngle, 1);
			if(bDir > 0) {
				pBot->pPlayer->sSteerRequest.ubLeft = 0;
				pBot->pPlayer->sSteerRequest.ubRight = 1;
			}
			else if(bDir < 0) {
				pBot->pPlayer->sSteerRequest.ubLeft = 1;
				pBot->pPlayer->sSteerRequest.ubRight = 0;
			}
			else {
				pBot->pPlayer->sSteerRequest.ubLeft = 0;
				pBot->pPlayer->sSteerRequest.ubRight = 0;
				// Check field in front for collisions
				UWORD uwChkX = pBot->pPlayer->sVehicle.uwX + fix16_to_int(
					(3*MAP_FULL_TILE)/2 * ccos(pBot->pPlayer->sVehicle.ubBodyAngle)
				);
				UWORD uwChkY = pBot->pPlayer->sVehicle.uwY + fix16_to_int(
					(3*MAP_FULL_TILE)/2 * csin(pBot->pPlayer->sVehicle.ubBodyAngle)
				);
				if(playerAnyNearPoint(uwChkX, uwChkY, MAP_FULL_TILE)) {
					pBot->ubState = AI_BOT_STATE_BLOCKED;
					pBot->ubTick = 0;
					break;
				}
				// Move until close to node
				if(
					ABS(pBot->uwNextX - pBot->pPlayer->sVehicle.uwX) > MAP_HALF_TILE ||
					ABS(pBot->uwNextY - pBot->pPlayer->sVehicle.uwY) > MAP_HALF_TILE
				)
					pBot->pPlayer->sSteerRequest.ubForward = 1;
				else {
					pBot->pPlayer->sSteerRequest.ubForward = 0;
					pBot->ubState = AI_BOT_STATE_NODE_REACHED;
				}
			}
			break;
		case AI_BOT_STATE_NODE_REACHED:
			++pBot->sRoute.ubCurrNode;
			if(pBot->sRoute.ubCurrNode == pBot->sRoute.ubNodeCount) {
				// Last node from route - hold pos
				pBot->ubTick = 0;
				pBot->ubState = AI_BOT_STATE_HOLDING_POS;
			}
			else {
				// Get next node from route
				pBot->pNextNode = &pBot->sRoute.pNodes[pBot->sRoute.ubCurrNode];
				pBot->ubTick = 10;
				pBot->ubState = AI_BOT_STATE_MOVING_TO_NODE;
			}
			break;
		case AI_BOT_STATE_HOLDING_POS:
			// Move to tile next to it to prevent blocking other players/bots
			// If point has been captured start measuring ticks
			if(pBot->ubTick >= 200) {
				// After some ticks go to next point
				botFindNewTarget(pBot, 0);
				if(pBot->pNextNode)
					pBot->ubState = AI_BOT_STATE_MOVING_TO_NODE;
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
					botFindNewTarget(pBot, pBot->pNextNode);
					if(!pBot->pNextNode)
						pBot->ubState = AI_BOT_STATE_IDLE;
					else
						pBot->ubState = AI_BOT_STATE_MOVING_TO_NODE;
					pBot->ubTick = 0;
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
	// Find some place to go - e.g. capture point
	botFindNewTarget(pBot, 0);
	// Find nearest spawn point
	pBot->pPlayer->ubSpawnIdx = spawnGetNearest(
		pBot->pNextNode->fubX, pBot->pNextNode->fubY,
		pBot->pPlayer->ubTeam
	);
	// After arriving at surface, recalculate where bot is going
	pBot->ubState = AI_BOT_STATE_IDLE;
	// Make sure noone from own team stands at spawn
	// TODO: spawn kill ppl from other team
	if(!spawnIsCoveredByAnyPlayer(pBot->pPlayer->ubSpawnIdx))
		playerSelectVehicle(pBot->pPlayer, VEHICLE_TYPE_TANK);
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
