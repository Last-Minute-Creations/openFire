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
	if(!bDeltaX && !bDeltaY)
		return 0;
	fix16_t fFineX = fix16_from_int((pFrom->fubX << MAP_TILE_SIZE) + MAP_HALF_TILE);
	fix16_t fFineY = fix16_from_int((pFrom->fubY << MAP_TILE_SIZE) + MAP_HALF_TILE);
	UBYTE ubAngle = getAngleBetweenPoints(
		pFrom->fubX << MAP_TILE_SIZE, pFrom->fubY << MAP_TILE_SIZE,
		pTo->fubX << MAP_TILE_SIZE, pTo->fubY << MAP_TILE_SIZE
	);
	tBCoordYX sPtA, sPtB;
	sPtA.bX = fix16_to_int(10 * csin(ubAngle));
	sPtA.bY = fix16_to_int(10 * ccos(ubAngle));
	sPtB.bX = fix16_to_int(-10 * csin(ubAngle));
	sPtB.bY = fix16_to_int(-10 * ccos(ubAngle));
	FUBYTE fubStart, fubStop;
	BYTE bDir;
	fix16_t fDx, fDy;
	if(ABS(bDeltaX) > ABS(bDeltaY)) {
		bDir = SGN(bDeltaX);
		fDx = fix16_from_int(bDir*MAP_FULL_TILE);
		fDy = fix16_div(fix16_from_int(bDeltaY*MAP_FULL_TILE), fix16_from_int(ABS(bDeltaX)));
		fubStart = pFrom->fubX;
		fubStop = pTo->fubX;
	}
	else {
		bDir = SGN(bDeltaY);
		fDx = fix16_div(fix16_from_int(bDeltaX*MAP_FULL_TILE), fix16_from_int(ABS(bDeltaY)));
		fDy = fix16_from_int(bDir*MAP_FULL_TILE);
		fubStart = pFrom->fubY;
		fubStop = pTo->fubY;
	}
	UWORD uwCost = 0;
	// logWrite("Line from %hu,%hu to %hu,%hu\n",	pFrom->fubX, pFrom->fubY, pTo->fubX, pTo->fubY);
	// logWrite("dir: %hhd, dx: %hd, dy: %hd\n", bDir, fix16_to_int(fDx), fix16_to_int(fDy));
	// logWrite("Point offsets: %hhd,%hhd; %hhd,%hhd,", sPtA.bX, sPtA.bY, sPtB.bX, sPtB.bY);
	fix16_t fHalf = fix16_one>>1;
	for(FUBYTE i = fubStart+bDir; i != fubStop; i += bDir) {
		fFineX += fDx;
		fFineY += fDy;
		// Process point A
		FUBYTE fubChkAX = (fix16_to_int(fFineX+fHalf) + sPtA.bX) >> MAP_TILE_SIZE;
		FUBYTE fubChkAY = (fix16_to_int(fFineY+fHalf) + sPtA.bY) >> MAP_TILE_SIZE;
		uwCost += s_pTileCosts[fubChkAX][fubChkAY];
		// logWrite("ptA: %hu,%hu -> %hhu", fubChkAX, fubChkAY, s_pTileCosts[fubChkAX][fubChkAY]);
		// Process point B
		FUBYTE fubChkBX = (fix16_to_int(fFineX+fHalf) + sPtB.bX) >> MAP_TILE_SIZE;
		FUBYTE fubChkBY = (fix16_to_int(fFineY+fHalf) + sPtB.bY) >> MAP_TILE_SIZE;
		if(fubChkBX != fubChkAX || fubChkBY != fubChkAY)
			uwCost += s_pTileCosts[fubChkBX][fubChkBY];
		// logWrite(", ptB: %hu,%hu -> %hhu\n", fubChkBX, fubChkBY, s_pTileCosts[fubChkBX][fubChkBY]);
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
	s_pNodeConnectionCosts = memAllocFast(sizeof(UWORD*) * g_fubNodeCount);
	for(FUBYTE fubFrom = g_fubNodeCount; fubFrom--;) {
		s_pNodeConnectionCosts[fubFrom] = memAllocFastClear(sizeof(UWORD) * g_fubNodeCount);
		for(FUBYTE fubTo = g_fubNodeCount; fubTo--;) {
			logWrite("[%hu -> %hu]\n", fubFrom, fubTo);
			s_pNodeConnectionCosts[fubFrom][fubTo] = aiCalcCostBetweenNodes(
				&g_pNodes[fubFrom], &g_pNodes[fubTo]
			);
		}
	}

	// aiGraphDump();
	logBlockEnd("aiGraphCreate()");
}

void aiGraphDump(void) {
	logBlockBegin("aiGraphDump()");
	logWrite("    ");
	for(FUBYTE fubTo = 0; fubTo < g_fubNodeCount; ++fubTo)
		logWrite("%5hu ", fubTo);
	logWrite("\n");
	for(FUBYTE fubFrom = 0; fubFrom < g_fubNodeCount; ++fubFrom) {
		logWrite("%3hu ", fubFrom);
		for(FUBYTE fubTo = 0; fubTo < g_fubNodeCount; ++fubTo)
			logWrite("%5hu ", s_pNodeConnectionCosts[fubFrom][fubTo]);
		logWrite("\n");
	}
	logBlockEnd("aiGraphDump()");
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

void aiDumpTileCosts(void) {
	logBlockBegin("aiDumpTileCosts()");
	logWrite("Tile costs:\n");
	logWrite("    ");
	for(FUBYTE x = 0; x != g_sMap.fubWidth; ++x)
		logWrite("%3hu ", x);
	logWrite("\n");
	for(FUBYTE y = 0; y != g_sMap.fubHeight; ++y) {
		logWrite("%3hu ", y);
		for(FUBYTE x = 0; x != g_sMap.fubWidth; ++x)
			logWrite("%3hhu ", s_pTileCosts[x][y]);
		logWrite("\n");
	}
	logBlockEnd("aiDumpTileCosts()");
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
