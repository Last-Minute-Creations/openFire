#include "gamestates/game/ai.h"
#include "gamestates/game/map.h"
#include "gamestates/game/turret.h"

// Cost is almost wall/turret hp
#define TURRET_COST 5
#define WALL_COST 5

UBYTE **ubCosts;

void aiManagerCreate(void) {
	ubCosts = memAllocFast(g_fubMapTileWidth * sizeof(UBYTE*));
	for(FUBYTE x = 0; x != g_fubMapTileWidth; ++x)
		ubCosts[x] = memAllocFastClear(g_fubMapTileHeight * sizeof(UBYTE));
}

void aiCalculateCosts(void) {
	aiCalculateCostFrag(0, 0, g_fubMapTileWidth-1, g_fubMapTileHeight-1);
}

void aiCalculateCostFrag(FUBYTE fubX1, FUBYTE fubY1, FUBYTE fubX2, FUBYTE fubY2) {
	for(FUBYTE x = fubX1; x <= fubX2; ++x) {
		for(FUBYTE y = fubY1; y <= fubY2; ++y) {
			// check for turret in range of fire
			FUBYTE fubTileRange = TURRET_MAX_PROCESS_RANGE_Y >> MAP_TILE_SIZE;
			ubCosts[x][y] = 0;
			for(FUBYTE i = MAX(0, x - fubTileRange); i != MIN(g_fubMapTileWidth, x+fubTileRange); ++i)
				for(FUBYTE j = MAX(0, y - fubTileRange); j != MIN(g_fubMapTileHeight, y+fubTileRange); ++j)
					if(g_pTurretTiles[i][j])
						ubCosts[x][y] += 2;
			// Check for walls
			if(g_pMap[x][y].ubIdx == MAP_LOGIC_WATER)
				ubCosts[x][y] = 0xFF;
			else if(g_pMap[x][y].ubIdx == MAP_LOGIC_WALL)
				ubCosts[x][y] += 5;
		}
	}
}

void aiGraphCreate(void) {

}

void aiGraphDestroy(void) {

}

void aiGraphAddNode(FUBYTE fubX, FUBYTE fubY) {

}

void aiGraphRecalcAllConnections(void) {

}

void aiManagerDestroy(void) {

}

void addBot(char *szName, UBYTE ubTeam) {

}
