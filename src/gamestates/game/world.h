#ifndef GUARD_OF_GAMESTATES_GAME_WORLD_H
#define GUARD_OF_GAMESTATES_GAME_WORLD_H

#include <ace/config.h>
#include <ace/utils/extview.h>
#include <ace/managers/viewport/simplebuffer.h>

extern tView *g_pWorldView;
extern tSimpleBufferManager *g_pWorldMainBfr;

extern UBYTE g_ubDoSiloHighlight;
extern UWORD g_uwSiloHighlightTileY;
extern UWORD g_uwSiloHighlightTileX;

UBYTE worldCreate(void);
void worldDestroy(void);
void worldProcess(void);

void worldDraw(void);

void worldShow(void);
void worldHide(void);

void worldProcessInput(void);

#endif
