#ifndef GUARD_OF_GAMESTATES_GAME_AI_BOT_H
#define GUARD_OF_GAMESTATES_GAME_AI_BOT_H

#include <ace/types.h>
#include "gamestates/game/player.h"
#include "gamestates/game/ai/ai.h"

#define AI_BOT_DEBUG

#define AI_BOT_ROUTE_CANDIDATE_COUNT 5
#define AI_BOT_ROUTE_SUBCANDIDATE_COUNT 5

#define AI_ROUTE_NODE_MAX 20

typedef struct _tRoute {
	UBYTE ubNodeCount;
	UBYTE ubCurrNode;
	tAiNode *pNodes[AI_ROUTE_NODE_MAX];
	UWORD uwCost;
} tRoute;

typedef struct _tBot {
	tPlayer *pPlayer;
	tRoute sRoute;
	tAiNode *pNextNode;
	UBYTE ubState;
	UBYTE ubTick;
	// Node-related fields
	UBYTE ubNextAngle;
	UWORD uwNextX;
	UWORD uwNextY;
} tBot;

void botManagerCreate(
	IN FUBYTE fubBotLimit
);

void botManagerDestroy(void);

void botAdd(char *szName, UBYTE ubTeam);

void botProcess(void);

#endif // GUARD_OF_GAMESTATES_GAME_AI_BOT_H
