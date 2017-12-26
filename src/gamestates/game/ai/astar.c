#include "gamestates/game/ai/astar.h"

tAstarData *astarCreate(void) {
	tAstarData *pNav = memAllocFast(sizeof(tAstarData));
	pNav->pFrontier = heapCreate(AI_MAX_NODES*AI_MAX_NODES);
	pNav->ubState = ASTAR_STATE_OFF;
	return pNav;
}

void astarDestroy(tAstarData *pNav) {
	heapDestroy(pNav->pFrontier);
	memFree(pNav, sizeof(tAstarData));
}

void astarStart(tAstarData *pNav, tAiNode *pNodeSrc, tAiNode *pNodeDst) {
	tAiNode *pCameFrom[AI_MAX_NODES] = {0};
	memset(pNav->pCostSoFar, 0xFF, sizeof(UWORD)*AI_MAX_NODES);
	pNav->pCameFrom[pNodeSrc->fubIdx] = 0;
	pNav->pCostSoFar[pNodeSrc->fubIdx] = 0;
	pNav->pNodeDst = pNodeDst;
	pNav->pNodeSrc = pNodeSrc;
	heapPush(pNav->pFrontier, pNav->pNodeSrc, 0);
	pNav->ubState = ASTAR_STATE_LOOPING;
	pNav->uwCurrNeighbour = g_fubNodeCount;
}

UBYTE astarProcess(tAstarData *pNav) {
	const ULONG ulMaxTime = 2500; // PAL: 1 = 0.4us => 10000 = 4ms => 2500 = 1ms
	if(pNav->ubState == ASTAR_STATE_LOOPING) {
		ULONG ulStart = timerGetPrec();
		do {
			if(pNav->uwCurrNeighbour >= g_fubNodeCount) {
				if(!pNav->pFrontier->uwCount) {
					// TODO What then?
					return 0;
				}
				pNav->pNodeCurr = heapPop(pNav->pFrontier);
				if(pNav->pNodeCurr == pNav->pNodeDst) {
					pNav->ubState = ASTAR_STATE_DONE;
					return 0;
				}
				pNav->uwCurrNeighbour = 0;
			}

			tAiNode *pNextNode = &g_pNodes[pNav->uwCurrNeighbour];
			if(pNextNode != pNav->pNodeCurr) {
				UWORD uwCost = pNav->pCostSoFar[pNav->pNodeCurr - g_pNodes]
					+ aiGetCostBetweenNodes(pNav->pNodeCurr, pNextNode);
				if(uwCost < pNav->pCostSoFar[pNextNode - g_pNodes]) {
					pNav->pCostSoFar[pNextNode - g_pNodes] = uwCost;
					UWORD uwPriority = uwCost
						+ ABS(pNextNode->fubX - pNav->pNodeDst->fubX)
						+ ABS(pNextNode->fubY - pNav->pNodeDst->fubY);
					heapPush(pNav->pFrontier, pNextNode, uwPriority);
					pNav->pCameFrom[pNextNode - g_pNodes] = pNav->pNodeCurr;
				}
			}
			++pNav->uwCurrNeighbour;
		} while(timerGetDelta(ulStart, timerGetPrec()) >= ulMaxTime);
	}
	else {
		// ASTAR_STATE_DONE
		pNav->sRoute.uwCost = pNav->pCostSoFar[pNav->pNodeDst->fubIdx];
		pNav->sRoute.pNodes[0] = pNav->pNodeDst;
		pNav->sRoute.ubNodeCount = 1;
		tAiNode *pPrev = pNav->pCameFrom[pNav->pNodeDst->fubIdx];
		while(pPrev) {
			pNav->sRoute.pNodes[pNav->sRoute.ubNodeCount] = pPrev;
			++pNav->sRoute.ubNodeCount;
			pPrev = pNav->pCameFrom[pPrev->fubIdx];
		}
		pNav->sRoute.ubCurrNode = pNav->sRoute.ubNodeCount-1;

		heapClear(pNav->pFrontier);
		pNav->ubState = ASTAR_STATE_OFF;
		return 1;
	}
	return 0;
}
