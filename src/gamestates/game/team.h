#ifndef GUARD_OF_GAMESTATES_GAME_TEAM_H
#define GUARD_OF_GAMESTATES_GAME_TEAM_H

#include <ace/config.h>

#define TEAM_COUNT 2
#define TEAM_GREEN 0
#define TEAM_BROWN 1
#define TEAM_NONE  2

#define TEAM_MAX_SILOS 3

typedef struct _tSilo {
	UBYTE ubTileY;
	UBYTE ubTileX;
} tSilo;

typedef struct _tTeam {
	UBYTE ubSiloCount;
	tSilo pSilos[TEAM_MAX_SILOS];
} tTeam;

void teamsInit(void);
void teamAddSpawn(UBYTE ubTeamIdx, UBYTE ubX, UBYTE ubY);

extern tTeam g_pTeams[TEAM_COUNT];

#endif // GUARD_OF_GAMESTATES_GAME_TEAM_H
