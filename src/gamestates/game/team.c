#include "gamestates/game/team.h"
#include <ace/managers/log.h>

void teamsInit(void) {
	g_pTeams[TEAM_GREEN].ubSiloCount = 0;
	g_pTeams[TEAM_BROWN].ubSiloCount = 0;
}

void teamAddSpawn(UBYTE ubTeamIdx, UBYTE ubX, UBYTE ubY) {
	UBYTE ubSiloIdx;
	
	logBlockBegin(
		"teamAddSpawn(ubTeamIdx: %hu, ubX: %hu, ubY: %hu)",
		ubTeamIdx, ubX, ubY
	);
	ubSiloIdx = g_pTeams[ubTeamIdx].ubSiloCount;
	g_pTeams[ubTeamIdx].pSilos[ubSiloIdx].ubTileX = ubX;
	g_pTeams[ubTeamIdx].pSilos[ubSiloIdx].ubTileY = ubY;
	++g_pTeams[ubTeamIdx].ubSiloCount;

	logBlockEnd("teamAddSpawn");
}

tTeam g_pTeams[TEAM_COUNT];
