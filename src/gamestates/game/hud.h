#ifndef GUARD_OF_GAMESTATES_GAME_HUD_H
#define GUARD_OF_GAMESTATES_GAME_HUD_H

#include <ace/types.h>
#include <ace/managers/viewport/simplebuffer.h>

#define HUD_HEIGHT 64

#define HUD_STATE_SELECTING 0
#define HUD_STATE_DRIVING 1

void hudChangeState(
	IN FUBYTE fubState
);

void hudCreate(void);
void hudDestroy(void);
void hudUpdate(void);

extern tSimpleBufferManager *g_pHudBfr;

#endif // GUARD_OF_GAMESTATES_GAME_HUD_H
