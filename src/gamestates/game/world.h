#ifndef GUARD_OF_GAMESTATES_GAME_WORLD_H
#define GUARD_OF_GAMESTATES_GAME_WORLD_H

#include <ace/config.h>
#include <ace/utils/extview.h>
#include <ace/managers/viewport/simplebuffer.h>

#define WORLD_BPP 4

/**
 * Viewport dimensions.
 * WORLD_VPORT_* refer to main world's VPort size.
 * WORLD_VPORT_BEGIN_* refer to actual PAL pixels with overscan taken
 * into account.
 */
#define WORLD_VPORT_WIDTH     320
#define WORLD_VPORT_HEIGHT    (256-64)
#define WORLD_VPORT_BEGIN_X 126
#define WORLD_VPORT_BEGIN_Y 44

extern tView *g_pWorldView;
extern tSimpleBufferManager *g_pWorldMainBfr;

extern UBYTE g_ubDoSiloHighlight;
extern UWORD g_uwSiloHighlightTileY;
extern UWORD g_uwSiloHighlightTileX;
extern tCameraManager *g_pWorldCamera;

UBYTE worldCreate(void);
void worldDestroy(void);
void worldProcess(void);

void worldDraw(void);

void worldShow(void);
void worldHide(void);

void worldProcessInput(void);

#endif
