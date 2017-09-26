#include "gamestates/game/control.h"
#include <ace/macros.h>
#include "gamestates/game/map.h"
#include "gamestates/game/turret.h"

tControlPoint *s_pControlPoints;
FUBYTE s_fubControlPointMaxCount;
FUBYTE s_fubControlPointCount;

void controlManagerCreate(FUBYTE fubPointCount) {
	s_fubControlPointMaxCount = fubPointCount;
	s_pControlPoints = memAllocFastClear(sizeof(tControlPoint) * fubPointCount);
	s_fubControlPointCount = 0;
}

void controlManagerDestroy(void) {
	for(FUBYTE i = 0; i != s_fubControlPointCount; ++i) {
		tControlPoint *pPoint = &s_pControlPoints[i];
		memFree(pPoint->pSpawns, pPoint->fubSpawnCount * sizeof(UBYTE));
		memFree(pPoint->pTurrets, pPoint->fubTurretCount * sizeof(UWORD));
	}
	memFree(s_pControlPoints, sizeof(tControlPoint) * s_fubControlPointMaxCount);
}

UBYTE ** controlPolygonMaskCreate(
	tControlPoint *pPoint, FUBYTE fubPolyPtCnt, tUbCoordYX *pPolyPts,
	FUBYTE *pX1, FUBYTE *pY1, FUBYTE *pX2, FUBYTE *pY2
) {
	UBYTE **pMask;
	pMask = memAllocFast(g_fubMapTileWidth * sizeof(UBYTE*));
	for(FUBYTE x = 0; x != g_fubMapTileWidth; ++x) {
		pMask[x] = memAllocFastClear(sizeof(UBYTE) * g_fubMapTileHeight);
	}
	for(FUBYTE i = 1; i != fubPolyPtCnt; ++i) {
		FUBYTE x1 = pPolyPts[i-1].sUbCoord.ubX;
		FUBYTE y1 = pPolyPts[i-1].sUbCoord.ubY;
		FUBYTE x2 = pPolyPts[i].sUbCoord.ubX;
		FUBYTE y2 = pPolyPts[i].sUbCoord.ubY;
		// Only horizontal lines
		// TODO: remove this restriction
		if(y1 != y2)
			continue;

		// Plot on mask
		FUBYTE fubStartX = MIN(x1, x2);
		FUBYTE fubEndX = MAX(x1, x2);
		for(FUBYTE fubLineX = fubStartX; fubLineX <= fubEndX; ++fubLineX)
			pMask[fubLineX][y1] = 1;
		// Polygon bounds
		*pX1 = MIN(*pX1, fubStartX);
		*pX2 = MAX(*pX2, fubEndX);
		*pY1 = MIN(*pY1, y1);
		*pY2 = MAX(*pY2, y1);
	}
	return pMask;
}

void controlPolygonMaskDestroy(UBYTE **pMask) {
	for(FUBYTE x = 0; x != g_fubMapTileWidth; ++x)
		memFree(pMask[x], sizeof(UBYTE) * g_fubMapTileHeight);
	memFree(pMask, g_fubMapTileWidth * sizeof(UBYTE*));
}

UBYTE s_ubAllocSpawnCount;
UBYTE s_ubAllocTurretCount;

void increaseSpawnCount(tControlPoint *pPoint, FUBYTE fubSpawnIdx) {
	++s_ubAllocSpawnCount;
}

void increaseTurretCount(tControlPoint *pPoint, FUBYTE fubTurretIdx) {
	++s_ubAllocTurretCount;
}

void addSpawn(tControlPoint *pPoint, FUBYTE fubSpawnIdx) {
	pPoint->pSpawns[pPoint->fubSpawnCount++] = fubSpawnIdx;
}

void addTurret(tControlPoint *pPoint, FUBYTE fubTurretIdx) {
	pPoint->pTurrets[pPoint->fubTurretCount++] = fubTurretIdx;
}

void controlMaskIterateSpawns(
	UBYTE **pMask, tControlPoint *pPoint,
	FUBYTE fubX1, FUBYTE fubY1, FUBYTE fubX2, FUBYTE fubY2,
	void (*onFound)(tControlPoint *pPoint, FUBYTE fubSpawnIdx)
) {
	for(FUBYTE i = 0; i != g_ubSpawnCount; ++i) {
		FUBYTE fubX = g_pSpawns[i].ubTileX;
		FUBYTE fubY = g_pSpawns[i].ubTileY;
		FUBYTE isInPoly = 0, isProcessed;
		for(FUBYTE y = fubY1; y <= fubY2; ++y) {
			isProcessed = 0;
			if(!isInPoly && pMask[fubX][y]) {
				isInPoly = !isInPoly;
				isProcessed = 1;
			}

			if(y == fubY && isInPoly) {
				onFound(pPoint, i);
				break;
			}

			if(isInPoly && pMask[fubX][y] && !isProcessed)
				isInPoly = !isInPoly;
		}
	}
}

void controlMaskIterateTurrets(
	UBYTE **pMask, tControlPoint *pPoint,
	FUBYTE fubX1, FUBYTE fubY1, FUBYTE fubX2, FUBYTE fubY2,
	void (*onFound)(tControlPoint *pPoint, FUBYTE fubTurretIdx)
) {
	for(FUBYTE i = 0; i != g_uwTurretCount; ++i) {
		FUBYTE fubX = g_pTurrets[i].uwX >> MAP_TILE_SIZE;
		FUBYTE fubY = g_pTurrets[i].uwY >> MAP_TILE_SIZE;
		FUBYTE isInPoly = 0, isProcessed;
		for(FUBYTE y = fubY1; y <= fubY2; ++y) {
			isProcessed = 0;
			if(!isInPoly && pMask[fubX][y]) {
				isInPoly = !isInPoly;
				isProcessed = 1;
			}

			if(y == fubY && isInPoly) {
				onFound(pPoint, i);
				break;
			}

			if(isInPoly && pMask[fubX][y] && !isProcessed)
				isInPoly = !isInPoly;
		}
	}
}

void controlAddPoint(
	char *szName, FUBYTE fubCaptureTileX, FUBYTE fubCaptureTileY,
	FUBYTE fubPolyPtCnt, tUbCoordYX *pPolyPts
) {
	tControlPoint *pPoint = &s_pControlPoints[s_fubControlPointCount];
	strcpy(pPoint->szName, szName);

	// Plot polygon on mask
	FUBYTE fubPolyX1, fubPolyY1, fubPolyX2, fubPolyY2;
	UBYTE **pMask = controlPolygonMaskCreate(
		pPoint, fubPolyPtCnt, pPolyPts, &fubPolyX1, &fubPolyY1, &fubPolyX2, &fubPolyY2
	);

	// Count & add spawns
	s_ubAllocSpawnCount = 0;
	pPoint->fubSpawnCount = 0;
	controlMaskIterateSpawns(pMask, pPoint, fubPolyX1, fubPolyY2, fubPolyX2, fubPolyY2, increaseSpawnCount);
	pPoint->pSpawns = memAllocFast(pPoint->fubSpawnCount * sizeof(UBYTE));
	controlMaskIterateSpawns(pMask, pPoint, fubPolyX1, fubPolyY2, fubPolyX2, fubPolyY2, addSpawn);

	// Count & add turrets
	s_ubAllocTurretCount = 0;
	pPoint->fubTurretCount = 0;
	controlMaskIterateTurrets(pMask, pPoint, fubPolyX1, fubPolyY2, fubPolyX2, fubPolyY2, increaseTurretCount);
	pPoint->pTurrets = memAllocFast(pPoint->fubTurretCount * sizeof(UWORD));
	controlMaskIterateSpawns(pMask, pPoint, fubPolyX1, fubPolyY2, fubPolyX2, fubPolyY2, addTurret);


	// Free polygon mask
	controlPolygonMaskDestroy(pMask);
	++s_fubControlPointCount;
}

void controlSim(void) {
	//solo:
	// bring to neutral: 15s (IGN: 20s)
	// take ownership:  +15s (IGN: +20s)
}
