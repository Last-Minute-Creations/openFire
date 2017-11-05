#ifndef GUARD_OF_GAMESTATES_GAME_AI_BOT_H
#define GUARD_OF_GAMESTATES_GAME_AI_BOT_H

#include <ace/types.h>
#include "gamestates/game/player.h"
#include "gamestates/game/ai/ai.h"

#define AI_BOT_ROUTE_NODE_MAX 20

#define AI_BOT_DEBUG

typedef struct _tBotRoute {
	UBYTE ubNodeCount;
	UBYTE ubCurrNode;
	tAiNode pNodes[AI_BOT_ROUTE_NODE_MAX];
} tBotRoute;

typedef struct _tBot {
	tPlayer *pPlayer;
	tBotRoute sRoute;
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
