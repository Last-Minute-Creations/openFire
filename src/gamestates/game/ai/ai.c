#include "gamestates/game/ai/ai.h"
#include "gamestates/game/map.h"
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

	// Check for doubles
	for(FUBYTE i = 0; i != g_fubNodeCount; ++i)
		if(g_pNodes[i].fubX == fubX && g_pNodes[i].fubY == fubY)
			return;

	// Add node
	g_pNodes[g_fubNodeCount].fubY = fubY;
	g_pNodes[g_fubNodeCount].fubX = fubX;
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

void aiGraphCreate(void) {
	logBlockBegin("aiGraphCreate()");
	// Get all nodes on map
	for(FUBYTE x = 0; x < g_fubMapTileWidth; ++x) {
		for(FUBYTE y = 0; y < g_fubMapTileHeight; ++y) {
			if(
				g_pMap[x][y].ubIdx == MAP_LOGIC_CAPTURE0 ||
				g_pMap[x][y].ubIdx == MAP_LOGIC_CAPTURE1 ||
				g_pMap[x][y].ubIdx == MAP_LOGIC_CAPTURE2
			) {
				// Capture points
				aiGraphAddNode(x,y, AI_NODE_TYPE_CAPTURE);
			}
			else if(
				g_pMap[x][y].ubIdx == MAP_LOGIC_SPAWN0 ||
				g_pMap[x][y].ubIdx == MAP_LOGIC_SPAWN1 ||
				g_pMap[x][y].ubIdx == MAP_LOGIC_SPAWN2
			) {
				// Spawn points
				aiGraphAddNode(x,y, AI_NODE_TYPE_SPAWN);
			}
			else if(
				g_pMap[x][y].ubIdx == MAP_LOGIC_ROAD &&
				mapIsWall(g_pMap[x-1][y].ubIdx) &&
				mapIsWall(g_pMap[x+1][y].ubIdx)
			) {
				// Gate with horizontal walls
				if(!mapIsWall(g_pMap[x-1][y-1].ubIdx) && !mapIsWall(g_pMap[x+1][y-1].ubIdx))
					aiGraphAddNode(x,y-1, AI_NODE_TYPE_ROAD);
				if(!mapIsWall(g_pMap[x-1][y+1].ubIdx) && !mapIsWall(g_pMap[x+1][y+1].ubIdx))
					aiGraphAddNode(x,y+1, AI_NODE_TYPE_ROAD);
			}
			else if(
				g_pMap[x][y].ubIdx == MAP_LOGIC_ROAD &&
				mapIsWall(g_pMap[x][y-1].ubIdx) &&
				mapIsWall(g_pMap[x][y+1].ubIdx)
			) {
				// Gate with vertical walls
				if(!mapIsWall(g_pMap[x-1][y-1].ubIdx) && !mapIsWall(g_pMap[x-1][y+1].ubIdx))
					aiGraphAddNode(x-1,y, AI_NODE_TYPE_ROAD);
				if(!mapIsWall(g_pMap[x+1][y-1].ubIdx) && !mapIsWall(g_pMap[x+1][y+1].ubIdx))
					aiGraphAddNode(x+1,y, AI_NODE_TYPE_ROAD);
			}
			// TODO this won't work if e.g. horizontal gate is adjacent to vertical wall
		}
	}
	logWrite(
		"Created %"PRI_FUBYTE" nodes (capture pts: %"PRI_FUBYTE")\n",
		g_fubNodeCount, g_fubCaptureNodeCount
	);

	// Create array for connections & calculate costs between nodes
	if(!g_fubNodeCount) {
		logWrite("WARN: No AI nodes on map!\n");
	}
	else {
		s_pNodeConnectionCosts = memAllocFast(sizeof(UWORD*) * g_fubNodeCount);
		for(FUBYTE fubFrom = g_fubNodeCount; fubFrom--;) {
			tAiNode *pFrom = &g_pNodes[fubFrom];
			s_pNodeConnectionCosts[fubFrom] = memAllocFastClear(sizeof(UWORD) * g_fubNodeCount);
			for(FUBYTE fubTo = g_fubNodeCount; fubTo--;) {
				tAiNode *pTo = &g_pNodes[fubTo];
				BYTE bDeltaX = pTo->fubX - pFrom->fubX;
				BYTE bDeltaY = pTo->fubY - pFrom->fubY;
				if(ABS(bDeltaX) > ABS(bDeltaY)) {
					BYTE bDirX = SGN(bDeltaX);
					for(FUBYTE x = pFrom->fubX; x != pTo->fubX; x += bDirX) {
						FUBYTE y = pFrom->fubY + ((bDeltaY * (x - pFrom->fubX)) / bDeltaX);
						s_pNodeConnectionCosts[fubFrom][fubTo] += s_pTileCosts[x][y];
					}
				}
				else {
					BYTE bDirY = SGN(bDeltaY);
					for(FUBYTE y = pFrom->fubY; y != pTo->fubY; y += bDirY) {
						FUBYTE x = pFrom->fubX + ((bDeltaX * (y - pFrom->fubY)) / bDeltaY);
						s_pNodeConnectionCosts[fubFrom][fubTo] += s_pTileCosts[x][y];
					}
				}
			}
		}
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

void aiManagerCreate(void) {
	logBlockBegin("aiManagerCreate()");
	g_fubNodeCount = 0;
	g_fubCaptureNodeCount = 0;
	botManagerCreate(g_ubPlayerLimit);

	// Calculate tile costs
	s_pTileCosts = memAllocFast(g_fubMapTileWidth * sizeof(UBYTE*));
	for(FUBYTE x = 0; x != g_fubMapTileWidth; ++x)
		s_pTileCosts[x] = memAllocFastClear(g_fubMapTileHeight * sizeof(UBYTE));
	aiCalculateTileCosts();

	// Create node network
	aiGraphCreate();
	logBlockEnd("aiManagerCreate()");
}

void aiManagerDestroy(void) {
	logBlockBegin("aiManagerDestroy()");
	aiGraphDestroy();
	botManagerDestroy();
	for(FUBYTE x = 0; x != g_fubMapTileWidth; ++x)
		memFree(s_pTileCosts[x], g_fubMapTileHeight * sizeof(UBYTE));
	memFree(s_pTileCosts, g_fubMapTileWidth * sizeof(UBYTE*));
	logBlockEnd("aiManagerDestroy()");
}

void aiCalculateTileCosts(void) {
	logBlockBegin("aiCalculateTileCosts()");
	aiCalculateTileCostsFrag(0, 0, g_fubMapTileWidth-1, g_fubMapTileHeight-1);
	logBlockEnd("aiCalculateTileCosts()");
}

void aiCalculateTileCostsFrag(FUBYTE fubX1, FUBYTE fubY1, FUBYTE fubX2, FUBYTE fubY2) {
	for(FUBYTE x = fubX1; x <= fubX2; ++x) {
		for(FUBYTE y = fubY1; y <= fubY2; ++y) {
			// Check for walls
			if(g_pMap[x][y].ubIdx == MAP_LOGIC_WATER) {
				s_pTileCosts[x][y] = 0xFF;
				continue;
			}
			if(mapIsWall(g_pMap[x][y].ubIdx)) {
				s_pTileCosts[x][y] = 0xFF;
				continue;
			}
			else {
				// There should be a minimal cost of transport for finding shortest path
				s_pTileCosts[x][y] = 1;
			}
			// Check for turret in range of fire
			FUBYTE fubTileRange = TURRET_MAX_PROCESS_RANGE_Y >> MAP_TILE_SIZE;
			for(FUBYTE i = MAX(0, x - fubTileRange); i != MIN(g_fubMapTileWidth, x+fubTileRange); ++i)
				for(FUBYTE j = MAX(0, y - fubTileRange); j != MIN(g_fubMapTileHeight, y+fubTileRange); ++j)
					if(g_pTurretTiles[i][j])
						s_pTileCosts[x][y] += MIN(s_pTileCosts[x][y]+10, 255);
		}
	}
}

tAiNode *aiFindClosestNode(FUBYTE fubTileX, FUBYTE fubTileY) {
	UWORD uwClosestDist = 0xFFFF;
	tAiNode *pClosest = 0;
	for(FUBYTE i = 0; i != g_fubNodeCount; ++i) {
		tAiNode *pNode = &g_pNodes[i];
		UWORD uwDist = ABS(pNode->fubX - fubTileX) + ABS(pNode->fubY + fubTileY);
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
