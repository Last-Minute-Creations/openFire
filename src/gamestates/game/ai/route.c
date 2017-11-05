#include "gamestates/game/ai/route.h"
#include <string.h>

void routeInit(tRoute *pRoute, tAiNode *pNodeStart, tAiNode *pNodeEnd) {
	pRoute->ubNodeCount = 2;
	pRoute->pNodes[0] = pNodeStart;
	pRoute->pNodes[1] = pNodeEnd;
	pRoute->uwCost = aiGetCostBetweenNodes(pNodeStart, pNodeEnd);
}

void routeCopy(tRoute *pSrcRoute, tRoute *pDstRoute) {
	memcpy(pDstRoute, pSrcRoute, sizeof(tRoute));
}

UBYTE routePushNode(tRoute *pRoute, tAiNode *pNode) {
	if(pRoute->ubNodeCount >= AI_ROUTE_NODE_MAX)
		return 0;
	pRoute->uwCost = routeGetCostWithNode(pRoute, pNode);

	// Move destination one slot further
	pRoute->pNodes[pRoute->ubNodeCount] = pRoute->pNodes[pRoute->ubNodeCount-1];

	// Insert new node before destination
	pRoute->pNodes[pRoute->ubNodeCount-1] = pNode;

	++pRoute->ubNodeCount;
	return 1;
}

UWORD routeGetCostWithNode(tRoute *pRoute, tAiNode *pNode) {
	// Connection between last before destination & destination is broken.
	// Add costs to new node & from new node to destination
	return pRoute->uwCost - aiGetCostBetweenNodes(
		pRoute->pNodes[pRoute->ubNodeCount-2],
		pRoute->pNodes[pRoute->ubNodeCount-1]
	) +	pRoute->uwCost + aiGetCostBetweenNodes(
		pRoute->pNodes[pRoute->ubNodeCount-2], pNode
	) + aiGetCostBetweenNodes(
		pNode, pRoute->pNodes[pRoute->ubNodeCount]
	);
}

UBYTE routeHasNode(tRoute *pRoute, tAiNode *pNode) {
	for(FUBYTE i = pRoute->ubNodeCount; i--;)
		if(pRoute->pNodes[i] == pNode)
			return 1;
	return 0;
}
