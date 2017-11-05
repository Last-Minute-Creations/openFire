#ifndef GUARD_OF_GAMESTATES_GAME_AI_ROUTE_H
#define GUARD_OF_GAMESTATES_GAME_AI_ROUTE_H

#define AI_ROUTE_NODE_MAX 20

#include "gamestates/game/ai/ai.h"

typedef struct _tRoute {
	UBYTE ubNodeCount;
	UBYTE ubCurrNode;
	tAiNode *pNodes[AI_ROUTE_NODE_MAX];
	UWORD uwCost;
} tRoute;

void routeInit(
	IN tRoute *pRoute,
	IN tAiNode *pNodeStart,
	IN tAiNode *pNodeEnd
);

void routeCopy(
	IN tRoute *pSrcRoute,
	IN tRoute *pDstRoute
);

UBYTE routePushNode(
	IN tRoute *pRoute,
	IN tAiNode *pNode
);

/**
 * Get route's total cost with given node added just before end.
 */
UWORD routeGetCostWithNode(
	IN tRoute *pRoute,
	IN tAiNode *pNode
);

UBYTE routeHasNode(
	IN tRoute *pRoute,
	IN tAiNode *pNode
);

#endif // GUARD_OF_GAMESTATES_GAME_AI_ROUTE_H
