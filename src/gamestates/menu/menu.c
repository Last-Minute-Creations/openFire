#include "gamestates/menu/menu.h"
#include <clib/dos_protos.h>
#include <ace/utils/extview.h>
#include <ace/utils/palette.h>
#include <ace/managers/key.h>
#include <ace/managers/mouse.h>
#include <ace/managers/game.h>
#include "config.h"
#include "cursor.h"
#include "map.h"
#include "gamestates/menu/button.h"
#include "gamestates/menu/maplist.h"
#include "gamestates/game/game.h"

static tView *s_pView;
static tVPort *s_pVPort;

tSimpleBufferManager *g_pMenuBuffer;
tFont *g_pMenuFont;

void menuMainOnStartGame(void) {
	gameChangeState(mapListCreate, mapListLoop, mapListDestroy);
}

void menuMainOnQuit(void) {
	gameClose();
}

void menuMainOnDemo(void) {
	g_isLocalBot = 1;
	mapInit("fubar.json");
	gamePopState(); // From current menu substate
	gameChangeState(gsGameCreate, gsGameLoop, gsGameDestroy);
}

#define MENU_BUTTON_WIDTH 80
#define MENU_BUTTON_HEIGHT 16
#define MENU_BUTTON_OFFS_X 32

void menuMainCreate(void) {
	// Display logo
	blitRect(
		g_pMenuBuffer->pBuffer, 0, 0,
		bitmapGetByteWidth(g_pMenuBuffer->pBuffer) << 3, g_pMenuBuffer->pBuffer->Rows,
		0
	);
	WaitBlit();
	bitmapLoadFromFile(g_pMenuBuffer->pBuffer, "data/menu/logo.bm", 80, 16);

	// Create buttons
	buttonListCreate(10, g_pMenuBuffer->pBuffer, g_pMenuFont);
	buttonAdd(
		MENU_BUTTON_OFFS_X, 64   , MENU_BUTTON_WIDTH, MENU_BUTTON_HEIGHT,
		"PLAY GAME", menuMainOnStartGame
	);
	buttonAdd(
		MENU_BUTTON_OFFS_X, 64+20, MENU_BUTTON_WIDTH, MENU_BUTTON_HEIGHT,
		"DEMO", menuMainOnDemo
	);
	buttonAdd(
		MENU_BUTTON_OFFS_X, 64+100, MENU_BUTTON_WIDTH, MENU_BUTTON_HEIGHT,
		"EXIT", menuMainOnQuit
	);
	buttonDrawAll();

	// Add notice
	const UWORD uwColorNotice = 14;
	fontDrawStr(
		g_pMenuBuffer->pBuffer, g_pMenuFont, 320/2, 236,
		"Founded by KaiN, Selur and Softiron",
		uwColorNotice, FONT_HCENTER | FONT_TOP | FONT_LAZY
	);
	fontDrawStr(
		g_pMenuBuffer->pBuffer, g_pMenuFont, 320/2, 243,
		"as a RetroKomp 2017 Gamedev Compo entry.",
		uwColorNotice, FONT_HCENTER | FONT_TOP | FONT_LAZY
	);
	fontDrawStr(
		g_pMenuBuffer->pBuffer, g_pMenuFont, 320/2, 250,
		"Remaining authors are listed on project page.",
		uwColorNotice, FONT_HCENTER | FONT_TOP | FONT_LAZY
	);
}

void menuMainDestroy(void) {
	buttonListDestroy();
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
	g_pMenuBuffer = simpleBufferCreate(0,
		TAG_SIMPLEBUFFER_VPORT, s_pVPort,
		TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_CLEAR | BMF_INTERLEAVED,
		TAG_DONE
	);
	copBlockDisableSprites(s_pView->pCopList, 0xFE);
	cursorCreate(s_pView, 0, "data/crosshair.bm", 0);
	paletteLoad("data/game.plt", s_pVPort->pPalette, 1 << MENU_BPP);
	paletteLoad("data/sprites.plt", &s_pVPort->pPalette[16], 1 << MENU_BPP);
	g_pMenuFont = fontCreate("data/silkscreen5.fnt");

	gamePushState(menuMainCreate, menuLoop, menuMainDestroy);

	viewLoad(s_pView);
}

void menuDestroy(void) {
	viewLoad(0);
	cursorDestroy();
	buttonListDestroy();
	fontDestroy(g_pMenuFont);
	viewDestroy(s_pView);
}

void menuLoop() {
	if(keyUse(KEY_ESCAPE)) {
		gamePopState(); // From whichever menu state game is in
		gamePopState(); // From menu
		return;
	}
	if(mouseUse(MOUSE_LMB))
		buttonProcessClick(g_uwMouseX, g_uwMouseY);

	menuProcess();
}

void menuProcess(void) {
	cursorUpdate();
	viewProcessManagers(s_pView);
	copProcessBlocks();
}
