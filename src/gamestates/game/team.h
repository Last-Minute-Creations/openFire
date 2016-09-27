#ifndef GUARD_OF_GAMESTATES_GAME_TEAM_H
#define GUARD_OF_GAMESTATES_GAME_TEAM_H

#include <ace/config.h>

#define TEAM_COUNT 2
#define TEAM_GREEN 0
#define TEAM_BROWN 1

typedef struct {
	UBYTE ubSpawnX;
	UBYTE ubSpawnY;
} tTeam;

void teamsInit(void);
void teamAddSpawn(UBYTE ubTeamIdx, UBYTE ubX, UBYTE ubY);

extern tTeam g_pTeams[TEAM_COUNT];

#endif // GUARD_OF_GAMESTATES_GAME_TEAM_H
