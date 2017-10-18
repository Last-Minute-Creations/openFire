#ifndef GUARD_OF_GAMESTATES_GAME_AI_AI_H
#define GUARD_OF_GAMESTATES_GAME_AI_AI_H

#include <ace/types.h>
#include "gamestates/game/control.h"

#define AI_MAX_NODES 50
#define AI_MAX_CAPTURE_NODES 10

#define AI_NODE_TYPE_ROAD 0
#define AI_NODE_TYPE_CAPTURE 1
#define AI_NODE_TYPE_SPAWN 2

typedef struct _tAiNode {
	FUBYTE fubY;
	FUBYTE fubX;
	FUBYTE fubType;
	tControlPoint *pControlPoint;
} tAiNode;

void aiManagerCreate(void);

void aiManagerDestroy(void);

void aiCalculateCosts(void);

void aiCalculateCostFrag(
	IN FUBYTE fubX1,
	IN FUBYTE fubY1,
	IN FUBYTE fubX2,
	IN FUBYTE fubY2
);

/**
 * Finds closest node to specified tile coordinates.
 * This function doesn't take into account costs to get to given node as it's
 * too costly.
 *
 * @param  fubTileX X-coordinate of tile near which node is to be found.
 * @param  fubTileY Ditto, Y.
 * @return If found, pointer to closest node, otherwise false.
 */
tAiNode *aiFindClosestNode(
	IN FUBYTE fubTileX,
	IN FUBYTE fubTileY
);


extern tAiNode g_pNodes[AI_MAX_NODES];
extern tAiNode *g_pCaptureNodes[AI_MAX_CAPTURE_NODES];
extern FUBYTE g_fubNodeCount;
extern FUBYTE g_fubCaptureNodeCount;

#endif // GUARD_OF_GAMESTATES_GAME_AI_AI_H
