#ifndef OF_GAMESTATES_GAME_AI_ASTAR_H
#define OF_GAMESTATES_GAME_AI_ASTAR_H

#include <ace/types.h>
#include "gamestates/game/ai/heap.h"
#include "gamestates/game/ai/ai.h"

#define ASTAR_STATE_OFF 0
#define ASTAR_STATE_LOOPING 1
#define ASTAR_STATE_DONE 2

#define ASTAR_ROUTE_NODE_MAX 20

typedef struct _tRoute {
	UBYTE ubNodeCount;
	UBYTE ubCurrNode;
	tAiNode *pNodes[ASTAR_ROUTE_NODE_MAX];
	UWORD uwCost;
} tRoute;

typedef struct {
	UBYTE ubState; ///< See ASTAR_STATE_* defines
	tHeap *pFrontier;
	tAiNode *pCameFrom[AI_MAX_NODES];
	UWORD pCostSoFar[AI_MAX_NODES];
	tAiNode *pNodeSrc;
	tAiNode *pNodeDst;
	tAiNode *pNodeCurr;
	UWORD uwCurrNeighbour;
	tRoute sRoute;
} tAstarData;

tAstarData *astarCreate(void);

void astarDestroy(
	IN tAstarData *pNav
);

void astarStart(
	IN tAstarData *pNav,
	IN tAiNode *pNodeSrc,
	IN tAiNode *pNodeDst
);

UBYTE astarProcess(
	IN tAstarData *pNav
);

#endif // OF_GAMESTATES_GAME_AI_ASTAR_H
