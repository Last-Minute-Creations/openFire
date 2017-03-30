#include "gamestates/initloading/loadingscreen.h"
#include <ace/utils/extview.h>
#include <ace/utils/palette.h>
#include <ace/managers/viewport/simplebuffer.h>
#include <ace/managers/blit.h>
#include "config.h"

#define LOADINGSCREEN_PROGRESS_WIDTH 256
#define LOADINGSCREEN_PROGRESS_HEIGHT 32
#define LOADINGSCREEN_PROGRESS_OUTLINE_COLOR 1
#define LOADINGSCREEN_PROGRESS_FILL_COLOR 15

static tView *s_pView;
static tVPort *s_pVPort;
static tSimpleBufferManager *s_pBuffer;

void loadingScreenCreate(void) {
	// Create View & VPort
	s_pView = viewCreate(V_GLOBAL_CLUT);
	s_pVPort = vPortCreate(
		s_pView,	WINDOW_SCREEN_WIDTH, WINDOW_SCREEN_HEIGHT, WINDOW_SCREEN_BPP, 0
	);
	s_pBuffer = simpleBufferCreate(s_pVPort, WINDOW_SCREEN_WIDTH, WINDOW_SCREEN_HEIGHT, BMF_CLEAR);
	paletteLoad("data/amidb32.plt", s_pVPort->pPalette, 1 << WINDOW_SCREEN_BPP);
	viewLoad(s_pView);

	blitRect(
		s_pBuffer->pBuffer,
		(WINDOW_SCREEN_WIDTH - LOADINGSCREEN_PROGRESS_WIDTH)/2 - 1,
		(WINDOW_SCREEN_HEIGHT - LOADINGSCREEN_PROGRESS_HEIGHT)/2 - 1,
		LOADINGSCREEN_PROGRESS_WIDTH+2,
		LOADINGSCREEN_PROGRESS_HEIGHT+2,
		LOADINGSCREEN_PROGRESS_OUTLINE_COLOR
	);
}

void loadingScreenDestroy() {
	viewLoad(0);
	viewDestroy(s_pView);
}

void loadingScreenSetProgress(UBYTE ubProgress) {
	logBlockBegin("loadingScreenSetProgress(ubProgress: %hu)", ubProgress);
	blitRect(
		s_pBuffer->pBuffer,
		(WINDOW_SCREEN_WIDTH - LOADINGSCREEN_PROGRESS_WIDTH)/2,
		(WINDOW_SCREEN_HEIGHT - LOADINGSCREEN_PROGRESS_HEIGHT)/2,
		(ubProgress*LOADINGSCREEN_PROGRESS_WIDTH)/100,
		LOADINGSCREEN_PROGRESS_HEIGHT,
		LOADINGSCREEN_PROGRESS_FILL_COLOR
	);
	logBlockEnd("loadingScreenSetProgress()");
}
