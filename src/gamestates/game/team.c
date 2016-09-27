#include "gamestates/game/team.h"

void teamsInit(void) {
	g_pTeams[TEAM_GREEN].ubSpawnX = 0;
	g_pTeams[TEAM_GREEN].ubSpawnY = 0;
	
	g_pTeams[TEAM_BROWN].ubSpawnX = 0;
	g_pTeams[TEAM_BROWN].ubSpawnY = 0;
}

void teamAddSpawn(UBYTE ubTeamIdx, UBYTE ubX, UBYTE ubY) {
	g_pTeams[ubTeamIdx].ubSpawnX = ubX;
	g_pTeams[ubTeamIdx].ubSpawnY = ubY;
}

tTeam g_pTeams[TEAM_COUNT];