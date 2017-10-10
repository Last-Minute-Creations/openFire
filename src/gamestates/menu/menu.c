#include "gamestates/menu/menu.h"
#include <clib/dos_protos.h>
#include <ace/utils/extview.h>
#include <ace/utils/palette.h>
#include <ace/utils/font.h>
#include <ace/managers/viewport/simplebuffer.h>
#include <ace/managers/key.h>
#include <ace/managers/mouse.h>
#include <ace/managers/game.h>
#include "config.h"
#include "cursor.h"
#include "gamestates/menu/button.h"
#include "gamestates/game/game.h"

#define MENU_BPP 4

static tView *s_pView;
static tVPort *s_pVPort;
static tSimpleBufferManager *s_pBuffer;
static tFont *s_pFont;

void menuOnStartGame(void) {
	gameChangeState(gsGameCreate, gsGameLoop, gsGameDestroy);
}

void menuOnQuit(void) {
	gamePopState();
}

void menuCreate(void) {
	// Create View & VPort
	s_pView = viewCreate(0,
		TAG_VIEW_GLOBAL_CLUT, 1,
		TAG_DONE
	);
	s_pVPort = vPortCreate(0,
		TAG_VPORT_VIEW, s_pView,
		TAG_VPORT_BPP, MENU_BPP,
		TAG_DONE
	);
	s_pBuffer = simpleBufferCreate(0,
		TAG_SIMPLEBUFFER_VPORT, s_pVPort,
		TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_CLEAR,
		TAG_DONE
	);
	copBlockDisableSprites(s_pView->pCopList, 0xFE);
	cursorCreate(s_pView, 0, "data/crosshair.bm", 0);
	paletteLoad("data/game.plt", s_pVPort->pPalette, 1 << MENU_BPP);
	paletteLoad("data/openfire_sprites.plt", &s_pVPort->pPalette[16], 1 << MENU_BPP);
	s_pFont = fontCreate("data/silkscreen5.fnt");
	bitmapLoadFromFile(s_pBuffer->pBuffer, "data/menu/logo.bm", 80, 16);
	buttonListCreate(10, s_pBuffer->pBuffer, s_pFont);

	buttonAdd(64, 96, 320-128, 32, "PLAY GAME", menuOnStartGame);
	buttonAdd(64, 96+32+16, 320-128, 32, "EXIT", menuOnQuit);

	buttonDrawAll();

	viewLoad(s_pView);
}

void menuDestroy(void) {
	viewLoad(0);
	cursorDestroy();
	buttonListDestroy();
	fontDestroy(s_pFont);
	viewDestroy(s_pView);
}

void menuLoop() {
	if(keyUse(KEY_ESCAPE)) {
		gamePopState();
		return;
	}
	cursorUpdate();
	if(mouseUse(MOUSE_LMB))
		buttonProcessClick(g_uwMouseX, g_uwMouseY);

	viewProcessManagers(s_pView);
	copProcessBlocks();
}
