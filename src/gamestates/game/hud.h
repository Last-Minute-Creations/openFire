#ifndef GUARD_OF_GAMESTATES_GAME_HUD_H
#define GUARD_OF_GAMESTATES_GAME_HUD_H

#define HUD_HEIGHT 64

#define HUD_STATE_SELECTING 0
#define HUD_STATE_DRIVING 1

#include <ace/types.h>

void hudChangeState(
	IN UBYTE ubState
);

void hudCreate(void);
void hudDestroy(void);
void hudUpdate(void);

#endif // GUARD_OF_GAMESTATES_GAME_HUD_H
