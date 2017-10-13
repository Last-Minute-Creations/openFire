#ifndef GUARD_OF_GAMESTATES_GAME_AI_H
#define GUARD_OF_GAMESTATES_GAME_AI_H

#include <ace/types.h>

void aiManagerCreate(void);

void aiManagerDestroy(void);

void addBot(
	IN char *szName,
	IN UBYTE ubTeam
);

void aiCalculateCosts(void);

void aiCalculateCostFrag(
	IN FUBYTE fubX1,
	IN FUBYTE fubY1,
	IN FUBYTE fubX2,
	IN FUBYTE fubY2
);

#endif // GUARD_OF_GAMESTATES_GAME_AI_H
