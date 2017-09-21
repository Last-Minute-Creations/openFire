#ifndef GUARD_OF_GAMESTATES_GAME_TEAM_H
#define GUARD_OF_GAMESTATES_GAME_TEAM_H

#include <ace/config.h>
#include "gamestates/game/spawn.h"

#define TEAM_COUNT 2
#define TEAM_GREEN 0
#define TEAM_BROWN 1
#define TEAM_NONE  2

typedef struct _tTeam {
	UWORD uwTicketsLeft; ///< In conquest mode
} tTeam;

void teamsInit(void);

extern tTeam g_pTeams[TEAM_COUNT];

#endif // GUARD_OF_GAMESTATES_GAME_TEAM_H
