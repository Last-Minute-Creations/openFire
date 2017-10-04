#include "gamestates/initloading/menu.h"
#include <clib/dos_protos.h>
#include <ace/utils/extview.h>
#include <ace/utils/palette.h>
#include <ace/managers/viewport/simplebuffer.h>
#include <ace/managers/blit.h>
#include "config.h"
#include "gamestates/initloading/worker.h"
#include "gamestates/game/turret.h"

#define LOADINGSCREEN_PROGRESS_WIDTH 256
#define LOADINGSCREEN_PROGRESS_HEIGHT 32
#define LOADINGSCREEN_COLOR_PROGRESS_OUTLINE 14
#define LOADINGSCREEN_COLOR_PROGRESS_FILL 8
#define LOADINGSCREEN_COLOR_BG 0

#define LOADINGSCREEN_BPP 4

static tView *s_pView;
static tVPort *s_pVPort;
static tSimpleBufferManager *s_pBuffer;
BYTE g_pLoadProgress[LOADINGSCREEN_BOBSOURCE_COUNT] = {-1};

void menuCreate(void) {
	// Create View & VPort
	s_pView = viewCreate(0,
		TAG_VIEW_GLOBAL_CLUT, 1,
		TAG_DONE
	);
	s_pVPort = vPortCreate(0,
		TAG_VPORT_VIEW, s_pView,
		TAG_VPORT_BPP, WINDOW_SCREEN_BPP,
		TAG_DONE
	);
	s_pBuffer = simpleBufferCreate(0,
		TAG_SIMPLEBUFFER_VPORT, s_pVPort,
		TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_CLEAR,
		TAG_DONE
	);
	copBlockDisableSprites(s_pView->pCopList, 0xFF);
	paletteLoad("data/softiron.plt", s_pVPort->pPalette, 1 << WINDOW_SCREEN_BPP);
	bitmapLoadFromFile(s_pBuffer->pBuffer, "data/menu/logo.bm", 80, 16);

	menuDrawButton(64, 96, 320-128, 32, "PLAY GAME", 0);
	menuDrawButton(64, 96+32+16, 320-128, 32, "EXIT", 0);

	viewLoad(s_pView);
}

void menuDrawButton(UWORD uwX, UWORD uwY, UWORD uwWidth, UWORD uwHeight, char *szText, UBYTE isSelected) {
	// Draw border
	const UBYTE ubColorLight = 1;
	const UBYTE ubColorDark = 14;
	const UBYTE ubColorFill = 11;

	blitRect(
		s_pBuffer->pBuffer, uwX, uwY,
		uwWidth, uwHeight, ubColorFill
	);

	// Ridge
	blitRect(
		s_pBuffer->pBuffer, uwX, uwY,
		uwWidth, 2, ubColorLight
	);
	blitRect(
		s_pBuffer->pBuffer, uwX, uwY,
		2, uwHeight, ubColorLight
	);

	// Grove
	blitRect(
		s_pBuffer->pBuffer, uwX + 2, uwY + uwHeight - 1,
		uwWidth - 2, 1, ubColorDark
	);
	blitRect(
		s_pBuffer->pBuffer, uwX + uwWidth - 1, uwY + 2,
		1, uwHeight - 2, ubColorDark
	);

	// TODO text
}

void menuDrawProgress(UWORD uwProgress) {
	// BG + outline
	blitRect(
		s_pBuffer->pBuffer,
		60 - 1,
		208 - 1,
		200 + 2,
		16 + 2,
		LOADINGSCREEN_COLOR_PROGRESS_OUTLINE
	);

	// Progress
	blitRect(
		s_pBuffer->pBuffer,
		60,
		208,
		(uwProgress*200)/100,
		16,
		LOADINGSCREEN_COLOR_PROGRESS_FILL
	);

	// TODO text
}

void menuDestroy(void) {
	viewLoad(0);
	viewDestroy(s_pView);
}

UBYTE s_ubPrevWorkerStep = -1;

void menuLoop() {
	if(s_ubPrevWorkerStep != g_ubWorkerStep) {
		menuDrawProgress(g_ubWorkerStep);
		s_ubPrevWorkerStep = g_ubWorkerStep;
	}
}
