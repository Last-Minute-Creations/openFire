#ifndef GUARD_OF_GAMESTATES_GAME_WORLD_H
#define GUARD_OF_GAMESTATES_GAME_WORLD_H

#include <ace/config.h>
#include <ace/utils/extview.h>
#include <ace/managers/viewport/simplebuffer.h>

extern tView *g_pWorldView;
extern tSimpleBufferManager *g_pWorldMainBfr;

UBYTE worldCreate(void);
void worldDestroy(void);
void worldProcess(void);

void worldDraw(void);

void worldShow(void);
void worldHide(void);

UBYTE worldProcessInput(void);

#endif
