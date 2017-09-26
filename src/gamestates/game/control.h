#ifndef GUARD_OF_GAMESTATES_GAME_CONTROL_H
#define GUARD_OF_GAMESTATES_GAME_CONTROL_H

#include <ace/types.h>
#include "gamestates/game/player.h"
#include "gamestates/game/team.h"

#define CONTROL_CAPTURE_RANGE 64
#define CONTROL_NAME_MAX 20

typedef struct _tControlPoint {
	FUBYTE fubTileX;
	FUBYTE fubTileY;
	FUBYTE pCapturerCounts[TEAM_COUNT]; ///< Number of players from given team
	                                    ///  capturing point at the moment.
	FUBYTE fubTurretCount;
	FUBYTE fubSpawnCount;
	FUWORD *pTurrets;
	FUBYTE *pSpawns;
	char szName[CONTROL_NAME_MAX];
} tControlPoint;

void controlManagerCreate(
	IN FUBYTE fubPointCount
);

void controlManagerDestroy(void);

/**
 *  This function expects all logic tiles to be initialized - spawns & turrets too.
 */
void controlAddPoint(
	IN char *szName,
	IN FUBYTE fubCaptureTileX,
	IN FUBYTE fubCaptureTileY,
	IN FUBYTE fubPolyPtCnt,
	IN tUbCoordYX *pPolyPts
);

void controlSim(void);

#endif // GUARD_OF_GAMESTATES_GAME_CONTROL_H
