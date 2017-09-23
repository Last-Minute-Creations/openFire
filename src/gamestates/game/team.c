#include "gamestates/game/team.h"
#include <ace/managers/log.h>

void teamsInit(void) {
	g_pTeams[TEAM_GREEN].uwTicketsLeft = 40;
	g_pTeams[TEAM_BROWN].uwTicketsLeft = 40;
}

tTeam g_pTeams[TEAM_COUNT];
