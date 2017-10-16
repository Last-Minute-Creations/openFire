#include "gamestates/game/control.h"
#include <ace/macros.h>
#include <ace/managers/blit.h>
#include "gamestates/game/map.h"
#include "gamestates/game/turret.h"
#include "gamestates/game/game.h"
#include "gamestates/game/console.h"

#define CONTROL_POINT_LIFE 250 /* 15s */
#define CONTROL_POINT_LIFE_RED   0
#define CONTROL_POINT_LIFE_NEUTRAL (CONTROL_POINT_LIFE)
#define CONTROL_POINT_LIFE_BLUE   (CONTROL_POINT_LIFE*2)

tControlPoint *g_pControlPoints;
FUBYTE g_fubControlPointCount;
FUBYTE s_fubControlPointMaxCount;
static UWORD s_uwFrameCounter;

void controlManagerCreate(FUBYTE fubPointCount) {
	logBlockBegin(
		"controlManagerCreate(fubPointCount: %"PRI_FUBYTE")", fubPointCount
	);
	s_fubControlPointMaxCount = fubPointCount;
	g_pControlPoints = memAllocFastClear(sizeof(tControlPoint) * fubPointCount);
	g_fubControlPointCount = 0;
	s_uwFrameCounter = 0;
	logBlockEnd("controlManagerCreate()");
}

void controlManagerDestroy(void) {
	logBlockBegin("controlManagerDestroy()");
	for(FUBYTE i = 0; i != g_fubControlPointCount; ++i) {
		tControlPoint *pPoint = &g_pControlPoints[i];
		memFree(pPoint->pSpawns, pPoint->fubSpawnCount * sizeof(FUBYTE));
		memFree(pPoint->pTurrets, pPoint->fubTurretCount * sizeof(UWORD));
	}
	memFree(g_pControlPoints, sizeof(tControlPoint) * s_fubControlPointMaxCount);
	logBlockEnd("controlManagerDestroy()");
}

UBYTE ** controlPolygonMaskCreate(
	tControlPoint *pPoint, FUBYTE fubPolyPtCnt, tUbCoordYX *pPolyPts,
	FUBYTE *pX1, FUBYTE *pY1, FUBYTE *pX2, FUBYTE *pY2
) {
	logBlockBegin(
		"controlPolygonMaskCreate(pPoint: %p, fubPolyPtCnt: %"PRI_FUBYTE", "
		"pPolyPts: %p, pX1: %p, pY1: %p, pX2: %p, pY2: %p)",
		pPoint, fubPolyPtCnt, pPolyPts, pX1, pY1, pX2, pY2
	);
	UBYTE **pMask;
	pMask = memAllocFast(g_fubMapTileWidth * sizeof(UBYTE*));
	for(FUBYTE x = 0; x != g_fubMapTileWidth; ++x) {
		pMask[x] = memAllocFastClear(sizeof(UBYTE) * g_fubMapTileHeight);
	}
	*pX1 = 0xFF; *pY1 = 0xFF; *pX2 = 0; *pY2 = 0;
	for(FUBYTE i = 1; i != fubPolyPtCnt; ++i) {
		FUBYTE x1 = pPolyPts[i-1].sUbCoord.ubX;
		FUBYTE y1 = pPolyPts[i-1].sUbCoord.ubY;
		FUBYTE x2 = pPolyPts[i].sUbCoord.ubX;
		FUBYTE y2 = pPolyPts[i].sUbCoord.ubY;

		FUBYTE fubStartX = MIN(x1, x2);
		FUBYTE fubEndX = MAX(x1, x2);

		// Only horizontal lines
		// TODO: remove this restriction
		if(y1 == y2) {
			// Plot on mask
			for(FUBYTE fubLineX = fubStartX; fubLineX <= fubEndX; ++fubLineX)
				pMask[fubLineX][y1] = 1;
		}

		// Polygon bounds
		*pX1 = MIN(*pX1, fubStartX);
		*pX2 = MAX(*pX2, fubEndX);
		*pY1 = MIN(*pY1, y1);
		*pY2 = MAX(*pY2, y1);
	}
	logWrite(
		"Bounds: %"PRI_FUBYTE",%"PRI_FUBYTE" ; %"PRI_FUBYTE",%"PRI_FUBYTE"\n",
		*pX1, *pY1, *pX2, *pY2
	);
	logBlockEnd("controlPolygonMaskCreate");
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
		if(fubX < fubX1 || fubX > fubX2)
			continue;
		FUBYTE isInPoly = 0, isEdgeProcessed;
		for(FUBYTE y = fubY1; y <= fubY2; ++y) {
			isEdgeProcessed = 0;
			if(!isInPoly && pMask[fubX][y]) {
				isInPoly = !isInPoly;
				isEdgeProcessed = 1;
			}

			if(y == fubY && isInPoly) {
				onFound(pPoint, i);
				break;
			}

			if(isInPoly && pMask[fubX][y] && !isEdgeProcessed)
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
		if(fubX < fubX1 || fubX > fubX2)
			continue;
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
	logBlockBegin(
		"controlAddPoint(szName: %s, fubCaptureTileX: %"PRI_FUBYTE", "
		"fubCaptureTileY: %"PRI_FUBYTE", fubPolyPtCnt: %"PRI_FUBYTE", pPolyPts: %p)",
		szName, fubCaptureTileX, fubCaptureTileY, fubPolyPtCnt, pPolyPts
	);
	tControlPoint *pPoint = &g_pControlPoints[g_fubControlPointCount];
	strcpy(pPoint->szName, szName);
	pPoint->fubTileX = fubCaptureTileX;
	pPoint->fubTileY = fubCaptureTileY;

	// Plot polygon on mask
	FUBYTE fubPolyX1, fubPolyY1, fubPolyX2, fubPolyY2;
	UBYTE **pMask = controlPolygonMaskCreate(
		pPoint, fubPolyPtCnt, pPolyPts, &fubPolyX1, &fubPolyY1, &fubPolyX2, &fubPolyY2
	);

	// Count & add spawns
	s_ubAllocSpawnCount = 0;
	pPoint->fubSpawnCount = 0;
	controlMaskIterateSpawns(pMask, pPoint, fubPolyX1, fubPolyY1, fubPolyX2, fubPolyY2, increaseSpawnCount);
	if(s_ubAllocSpawnCount) {
		pPoint->pSpawns = memAllocFast(s_ubAllocSpawnCount * sizeof(FUBYTE));
		controlMaskIterateSpawns(pMask, pPoint, fubPolyX1, fubPolyY1, fubPolyX2, fubPolyY2, addSpawn);
	}

	// Count & add turrets
	s_ubAllocTurretCount = 0;
	pPoint->fubTurretCount = 0;
	controlMaskIterateTurrets(pMask, pPoint, fubPolyX1, fubPolyY1, fubPolyX2, fubPolyY2, increaseTurretCount);
	if(s_ubAllocTurretCount) {
		pPoint->pTurrets = memAllocFast(s_ubAllocTurretCount * sizeof(UWORD));
		controlMaskIterateTurrets(pMask, pPoint, fubPolyX1, fubPolyY1, fubPolyX2, fubPolyY2, addTurret);
	}

	// Determine team
	if(pPoint->fubSpawnCount)
		pPoint->fubTeam = g_pSpawns[pPoint->pSpawns[0]].ubTeam;
	else if(pPoint->fubTurretCount)
		pPoint->fubTeam = g_pTurrets[pPoint->pTurrets[0]].ubTeam;
	else
		pPoint->fubTeam = TEAM_NONE;

	if(pPoint->fubTeam == TEAM_BLUE)
		pPoint->fuwLife = CONTROL_POINT_LIFE_BLUE;
	else if(pPoint->fubTeam == TEAM_RED)
		pPoint->fuwLife = CONTROL_POINT_LIFE_RED;
	else
		pPoint->fuwLife = CONTROL_POINT_LIFE_NEUTRAL;

	pPoint->fubDestTeam = pPoint->fubTeam;
	pPoint->fubBrownCount = 0;
	pPoint->fubGreenCount = 0;

	// Free polygon mask
	controlPolygonMaskDestroy(pMask);
	++g_fubControlPointCount;
	logWrite(
		"Spawns: %"PRI_FUBYTE", turrets: %"PRI_FUBYTE"\n",
		pPoint->fubSpawnCount, pPoint->fubTurretCount
	);
	logBlockEnd("controlAddPoint()");
}

void controlCapturePoint(tControlPoint *pPoint, FUBYTE fubTeam) {
	if(pPoint->fubTeam == fubTeam)
		return;
	char szLog[50];
	FUBYTE fubColor;
	if(fubTeam == TEAM_BLUE) {
		sprintf(szLog, "Control point %s is now BLUE", pPoint->szName);
		fubColor = CONSOLE_COLOR_BLUE;
	}
	else if(fubTeam == TEAM_RED) {
		sprintf(szLog, "Control point %s is now RED", pPoint->szName);
		fubColor = CONSOLE_COLOR_RED;
	}
	else {
		sprintf(szLog, "Control point %s is now NEUTRAL", pPoint->szName);
		fubColor = CONSOLE_COLOR_GENERAL;
	}
	consoleWrite(szLog, fubColor);

	pPoint->fubTeam = fubTeam;
	for(FUBYTE i = 0; i != pPoint->fubSpawnCount; ++i)
		spawnCapture(pPoint->pSpawns[i], fubTeam);
	for(FUBYTE i = 0; i != pPoint->fubTurretCount; ++i)
		turretCapture(pPoint->pTurrets[i], fubTeam);
}

// 50 spawns per team
// ticket drop each 5s, so 250s of game (4min)
// max drop speed: each
// 5 control points in total
// starting: 2xR, 2xB, 1xN - no loss
void controlSim(void) {
	FUBYTE fubControlledByRed = 0;
	FUBYTE fubControlledByBlue = 0;
	for(FUBYTE i = 0; i != g_fubControlPointCount; ++i) {
		tControlPoint *pPoint = &g_pControlPoints[i];
		FBYTE fbCaptureDir;

		// Determine destination team
		if(pPoint->fubBrownCount == pPoint->fubGreenCount) {
			if(!pPoint->fubBrownCount && pPoint->fubTeam == TEAM_NONE && pPoint->fuwLife != CONTROL_POINT_LIFE_NEUTRAL) {
				// Abandoned neutral point
				fbCaptureDir = SGN(CONTROL_POINT_LIFE_NEUTRAL - pPoint->fuwLife);
			}
			else {
				if(pPoint->fubTeam == TEAM_BLUE)
					++fubControlledByBlue;
				else if(pPoint->fubTeam == TEAM_RED)
					++fubControlledByRed;
				// Reset for next counting during player process
				pPoint->fubBrownCount = 0;
				pPoint->fubGreenCount = 0;
				continue;
			}
		}
		else if(pPoint->fubBrownCount > pPoint->fubGreenCount) {
			// Brown taking over green
			fbCaptureDir = -1;
			if(pPoint->fuwLife > CONTROL_POINT_LIFE_NEUTRAL)
				pPoint->fubDestTeam = TEAM_NONE;
			else
				pPoint->fubDestTeam = TEAM_RED;
		}
		else {
			// Green taking over brown
			fbCaptureDir = 1;
			if(pPoint->fuwLife < CONTROL_POINT_LIFE_NEUTRAL)
				pPoint->fubDestTeam = TEAM_NONE;
			else
				pPoint->fubDestTeam = TEAM_BLUE;
		}

		// Process takeover
		pPoint->fuwLife = CLAMP(
			pPoint->fuwLife+fbCaptureDir,
			CONTROL_POINT_LIFE_RED,	CONTROL_POINT_LIFE_BLUE
		);

		if(pPoint->fuwLife == CONTROL_POINT_LIFE_RED)
			controlCapturePoint(pPoint, TEAM_RED);
		else if(pPoint->fuwLife == CONTROL_POINT_LIFE_BLUE)
			controlCapturePoint(pPoint, TEAM_BLUE);
		else if(pPoint->fuwLife == CONTROL_POINT_LIFE_NEUTRAL)
			controlCapturePoint(pPoint, TEAM_NONE);

		if(pPoint->fubTeam == TEAM_BLUE)
			++fubControlledByBlue;
		else if(pPoint->fubTeam == TEAM_RED)
			++fubControlledByRed;
		// Reset for next counting during player process
		pPoint->fubBrownCount = 0;
		pPoint->fubGreenCount = 0;
	}
	if(s_uwFrameCounter >= 50*5) {
		s_uwFrameCounter -= 50*5;
		FUBYTE fubDelta = (ABS(fubControlledByRed - fubControlledByBlue)+1) >> 1;
		if(fubControlledByRed > fubControlledByBlue)
			g_pTeams[TEAM_BLUE].uwTicketsLeft = MAX(0, g_pTeams[TEAM_BLUE].uwTicketsLeft - fubDelta);
		else if(fubControlledByRed < fubControlledByBlue)
			g_pTeams[TEAM_RED].uwTicketsLeft = MAX(0, g_pTeams[TEAM_RED].uwTicketsLeft - fubDelta);
	}
	else
		++s_uwFrameCounter;
}

void controlRedrawPoints(void) {
	for(FUBYTE i = 0; i != g_fubControlPointCount; ++i) {
		tControlPoint *pPoint = &g_pControlPoints[i];
		// Omit drawing if not visible
		if(!simpleBufferIsRectVisible(
			g_pWorldMainBfr,
			pPoint->fubTileX << MAP_TILE_SIZE, pPoint->fubTileY << MAP_TILE_SIZE,
			MAP_FULL_TILE, MAP_FULL_TILE
		))
			continue;
		// TODO could be drawn only on fubTileLife change
		UWORD uwX = pPoint->fubTileX << MAP_TILE_SIZE;
		UWORD uwY = pPoint->fubTileY << MAP_TILE_SIZE;
		FUWORD fuwTileProgress = ABS(CONTROL_POINT_LIFE_NEUTRAL - pPoint->fuwLife);
		if(pPoint->fubDestTeam == TEAM_NONE)
			fuwTileProgress = CONTROL_POINT_LIFE - fuwTileProgress;
		fuwTileProgress = (MAP_FULL_TILE * fuwTileProgress) / CONTROL_POINT_LIFE;
		FUWORD fuwAntiProgress = MAP_FULL_TILE - fuwTileProgress;

		if(fuwTileProgress != MAP_FULL_TILE) {
			blitCopyAligned(
				g_pMapTileset, 0,
				(MAP_TILE_CAPTURE_BLUE + pPoint->fubTeam) << MAP_TILE_SIZE,
				g_pWorldMainBfr->pBuffer, uwX, uwY,
				MAP_FULL_TILE, fuwAntiProgress
			);
		}
		if(fuwTileProgress)
			blitCopyAligned(
				g_pMapTileset, 0,
				((MAP_TILE_CAPTURE_BLUE + pPoint->fubDestTeam) << MAP_TILE_SIZE) + fuwAntiProgress,
				g_pWorldMainBfr->pBuffer, uwX, uwY + fuwAntiProgress,
				MAP_FULL_TILE, fuwTileProgress
			);
	}
}
