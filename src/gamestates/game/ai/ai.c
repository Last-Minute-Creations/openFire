#include "gamestates/game/ai/ai.h"
#include "gamestates/game/worldmap.h"
#include "gamestates/game/turret.h"
#include "gamestates/game/player.h"
#include "gamestates/game/gamemath.h"
#include "gamestates/game/ai/bot.h"

// Cost is almost wall/turret hp
#define TURRET_COST 5
#define WALL_COST 5

// Costs
static UWORD **s_pNodeConnectionCosts;
static UBYTE **s_pTileCosts;

// Nodes
tAiNode g_pNodes[AI_MAX_NODES];
tAiNode *g_pCaptureNodes[AI_MAX_CAPTURE_NODES];
FUBYTE g_fubNodeCount;
FUBYTE g_fubCaptureNodeCount;

void aiGraphAddNode(FUBYTE fubX, FUBYTE fubY, FUBYTE fubNodeType) {
	// Check for overflow
	if(g_fubNodeCount >= AI_MAX_NODES) {
		logWrite("ERR: No more room for nodes\n");
		return;
	}

	// Check for duplicates
	for(FUBYTE i = 0; i != g_fubNodeCount; ++i)
		if(g_pNodes[i].fubX == fubX && g_pNodes[i].fubY == fubY)
			return;

	// Add node
	g_pNodes[g_fubNodeCount].fubX = fubX;
	g_pNodes[g_fubNodeCount].fubY = fubY;
	g_pNodes[g_fubNodeCount].fubType = fubNodeType;
	g_pNodes[g_fubNodeCount].fubIdx = g_fubNodeCount;

	// Add to capture list?
	if(fubNodeType == AI_NODE_TYPE_CAPTURE) {
		if(g_fubCaptureNodeCount >= AI_MAX_CAPTURE_NODES)
			logWrite("ERR: No more room for capture nodes\n");
		else {
			g_pCaptureNodes[g_fubCaptureNodeCount] = &g_pNodes[g_fubNodeCount];
			g_pNodes[g_fubNodeCount].pControlPoint = controlPointGetAt(fubX, fubY);
			++g_fubCaptureNodeCount;
		}
	}
	++g_fubNodeCount;
}

static FUBYTE aiGraphGenerateMapNodes(void) {
	// Get all nodes on map
	for(FUBYTE x = 0; x < g_sMap.fubWidth; ++x) {
		for(FUBYTE y = 0; y < g_sMap.fubHeight; ++y) {
			if(
				g_sMap.pData[x][y].ubIdx == MAP_LOGIC_CAPTURE0 ||
				g_sMap.pData[x][y].ubIdx == MAP_LOGIC_CAPTURE1 ||
				g_sMap.pData[x][y].ubIdx == MAP_LOGIC_CAPTURE2
			) {
				// Capture points
				aiGraphAddNode(x,y, AI_NODE_TYPE_CAPTURE);
			}
			else if(
				g_sMap.pData[x][y].ubIdx == MAP_LOGIC_SPAWN0 ||
				g_sMap.pData[x][y].ubIdx == MAP_LOGIC_SPAWN1 ||
				g_sMap.pData[x][y].ubIdx == MAP_LOGIC_SPAWN2
			) {
				// Spawn points
				aiGraphAddNode(x,y, AI_NODE_TYPE_SPAWN);
			}
			else if(
				g_sMap.pData[x][y].ubIdx == MAP_LOGIC_ROAD &&
				worldMapIsWall(g_sMap.pData[x-1][y].ubIdx) &&
				worldMapIsWall(g_sMap.pData[x+1][y].ubIdx)
			) {
				// Gate with horizontal walls
				if(!worldMapIsWall(g_sMap.pData[x-1][y-1].ubIdx) && !worldMapIsWall(g_sMap.pData[x+1][y-1].ubIdx))
					aiGraphAddNode(x,y-1, AI_NODE_TYPE_ROAD);
				if(!worldMapIsWall(g_sMap.pData[x-1][y+1].ubIdx) && !worldMapIsWall(g_sMap.pData[x+1][y+1].ubIdx))
					aiGraphAddNode(x,y+1, AI_NODE_TYPE_ROAD);
			}
			else if(
				g_sMap.pData[x][y].ubIdx == MAP_LOGIC_ROAD &&
				worldMapIsWall(g_sMap.pData[x][y-1].ubIdx) &&
				worldMapIsWall(g_sMap.pData[x][y+1].ubIdx)
			) {
				// Gate with vertical walls
				if(!worldMapIsWall(g_sMap.pData[x-1][y-1].ubIdx) && !worldMapIsWall(g_sMap.pData[x-1][y+1].ubIdx))
					aiGraphAddNode(x-1,y, AI_NODE_TYPE_ROAD);
				if(!worldMapIsWall(g_sMap.pData[x+1][y-1].ubIdx) && !worldMapIsWall(g_sMap.pData[x+1][y+1].ubIdx))
					aiGraphAddNode(x+1,y, AI_NODE_TYPE_ROAD);
			}
			// TODO this won't work if e.g. horizontal gate is adjacent to vertical wall
		}
	}
	logWrite(
		"Created %"PRI_FUBYTE" nodes (capture pts: %"PRI_FUBYTE")\n",
		g_fubNodeCount, g_fubCaptureNodeCount
	);
	return g_fubNodeCount;
}

UWORD aiCalcCostBetweenNodes(tAiNode *pFrom, tAiNode *pTo) {
	BYTE bDeltaX = pTo->fubX - pFrom->fubX;
	BYTE bDeltaY = pTo->fubY - pFrom->fubY;
	UWORD uwCost = 0;
	if(ABS(bDeltaX) > ABS(bDeltaY)) {
		BYTE bDirX = SGN(bDeltaX);
		FUBYTE fubPrevY = pFrom->fubY;
		for(FUBYTE x = pFrom->fubX; x != pTo->fubX; x += bDirX) {
			FUBYTE y = pFrom->fubY + ((bDeltaY * (x - pFrom->fubX) + bDeltaX/2) / bDeltaX);
			if(x != pFrom->fubX) {
				uwCost += s_pTileCosts[x][y];
				if(y != fubPrevY) {
					uwCost += s_pTileCosts[x][fubPrevY];
					uwCost += s_pTileCosts[x-bDirX][y];
				}
			}
			fubPrevY = y;
		}
	}
	else {
		BYTE bDirY = SGN(bDeltaY);
		FUBYTE fubPrevX = pFrom->fubX;
		for(FUBYTE y = pFrom->fubY; y != pTo->fubY; y += bDirY) {
			FUBYTE x = pFrom->fubX + ((bDeltaX * (y - pFrom->fubY) + bDeltaY/2) / bDeltaY);
			if(y != pFrom->fubY) {
				uwCost += s_pTileCosts[x][y];
				if(x != fubPrevX) {
					uwCost += s_pTileCosts[x][y-bDirY];
					uwCost += s_pTileCosts[fubPrevX][y];
				}
			}
			fubPrevX = x;
		}
	}
	return uwCost;
}

void aiGraphCreate(void) {
	logBlockBegin("aiGraphCreate()");
	if(!aiGraphGenerateMapNodes()) {
		logWrite("WARN: No AI nodes on map!\n");
		logBlockEnd("aiGraphCreate()");
		return;
	}

	// Create array for connections & calculate costs between nodes
	logWrite("     ");
	for(FUBYTE fubTo = g_fubNodeCount; fubTo--;)
		logWrite("%5hu ", fubTo);
	logWrite("\n");
	s_pNodeConnectionCosts = memAllocFast(sizeof(UWORD*) * g_fubNodeCount);
	for(FUBYTE fubFrom = g_fubNodeCount; fubFrom--;) {
		s_pNodeConnectionCosts[fubFrom] = memAllocFastClear(sizeof(UWORD) * g_fubNodeCount);
		logWrite("%3hu ", fubFrom);
		for(FUBYTE fubTo = g_fubNodeCount; fubTo--;) {
			s_pNodeConnectionCosts[fubFrom][fubTo] = aiCalcCostBetweenNodes(
				&g_pNodes[fubFrom], &g_pNodes[fubTo]
			);
			logWrite("%5hu ", s_pNodeConnectionCosts[fubFrom][fubTo]);
		}
		logWrite("\n");
	}
	logBlockEnd("aiGraphCreate()");
}

void aiGraphDestroy(void) {
	logBlockBegin("aiGraphDestroy()");
	if(g_fubNodeCount) {
		for(FUBYTE fubFrom = g_fubNodeCount; fubFrom--;)
			memFree(s_pNodeConnectionCosts[fubFrom], sizeof(UWORD) * g_fubNodeCount);
		memFree(s_pNodeConnectionCosts, sizeof(UWORD*) * g_fubNodeCount);
	}
	logBlockEnd("aiGraphDestroy()");
}

void aiCalcTileCostsFrag(FUBYTE fubX1, FUBYTE fubY1, FUBYTE fubX2, FUBYTE fubY2) {
	for(FUBYTE x = fubX1; x <= fubX2; ++x) {
		for(FUBYTE y = fubY1; y <= fubY2; ++y) {
			// Check for walls
			if(g_sMap.pData[x][y].ubIdx == MAP_LOGIC_WATER) {
				s_pTileCosts[x][y] = 0xFF;
				continue;
			}
			if(worldMapIsWall(g_sMap.pData[x][y].ubIdx)) {
				s_pTileCosts[x][y] = 0xFF;
				continue;
			}
			else {
				// There should be a minimal cost of transport for finding shortest path
				s_pTileCosts[x][y] = 1;
			}
			// Check for turret in range of fire
			FUBYTE fubTileRange = TURRET_MAX_PROCESS_RANGE_Y >> MAP_TILE_SIZE;
			for(FUBYTE i = MAX(0, x - fubTileRange); i != MIN(g_sMap.fubWidth, x+fubTileRange); ++i)
				for(FUBYTE j = MAX(0, y - fubTileRange); j != MIN(g_sMap.fubHeight, y+fubTileRange); ++j)
					if(g_pTurretTiles[i][j])
						s_pTileCosts[x][y] += MIN(s_pTileCosts[x][y]+10, 255);
		}
	}
}

void aiCalcTileCosts(void) {
	logBlockBegin("aiCalcTileCosts()");
	aiCalcTileCostsFrag(0, 0, g_sMap.fubWidth-1, g_sMap.fubHeight-1);
	logBlockEnd("aiCalcTileCosts()");
}

tAiNode *aiFindClosestNode(FUBYTE fubTileX, FUBYTE fubTileY) {
	UWORD uwClosestDist = 0xFFFF;
	tAiNode *pClosest = 0;
	for(FUBYTE i = 0; i != g_fubNodeCount; ++i) {
		tAiNode *pNode = &g_pNodes[i];
		UWORD uwDist = ABS(pNode->fubX - fubTileX) + ABS(pNode->fubY - fubTileY);
		if(uwDist < uwClosestDist) {
			uwClosestDist = uwDist;
			pClosest = pNode;
		}
	}
	return pClosest;
}

UWORD aiGetCostBetweenNodes(tAiNode *pSrc, tAiNode *pDst) {
	return s_pNodeConnectionCosts[pSrc->fubIdx][pDst->fubIdx];
}

void aiManagerCreate(void) {
	logBlockBegin("aiManagerCreate()");
	g_fubNodeCount = 0;
	g_fubCaptureNodeCount = 0;
	botManagerCreate(g_ubPlayerLimit);

	// Calculate tile costs
	s_pTileCosts = memAllocFast(g_sMap.fubWidth * sizeof(UBYTE*));
	for(FUBYTE x = 0; x != g_sMap.fubWidth; ++x)
		s_pTileCosts[x] = memAllocFastClear(g_sMap.fubHeight * sizeof(UBYTE));
	aiCalcTileCosts();

	// Create node network
	aiGraphCreate();
	logBlockEnd("aiManagerCreate()");
}

void aiManagerDestroy(void) {
	logBlockBegin("aiManagerDestroy()");
	aiGraphDestroy();
	botManagerDestroy();
	for(FUBYTE x = 0; x != g_sMap.fubWidth; ++x)
		memFree(s_pTileCosts[x], g_sMap.fubHeight * sizeof(UBYTE));
	memFree(s_pTileCosts, g_sMap.fubWidth * sizeof(UBYTE*));
	logBlockEnd("aiManagerDestroy()");
}
