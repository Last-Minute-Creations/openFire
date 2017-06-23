#include "gamestates/game/hud.h"
#include <ace/config.h>
#include <ace/managers/blit.h>
#include <ace/managers/viewport/simplebuffer.h>
#include <ace/utils/bitmap.h>
#include "gamestates/game/game.h"
#include "gamestates/game/world.h"

tVPort *s_pHudVPort;
tSimpleBufferManager *s_pHudBfr;

void hudCreate(void) {
	s_pHudVPort = vPortCreate(g_pWorldView, WINDOW_SCREEN_WIDTH, 64, HUD_BPP, 0);
	s_pHudBfr = simpleBufferCreate(s_pHudVPort, WINDOW_SCREEN_WIDTH, 64, 0);
	
	// Initial draw on buffer
	bitmapLoadFromFile(s_pHudBfr->pBuffer, "data/hud_bg.bm", 0, 0);	
}

void hudDestroy(void) {
	// VPort & simpleBuffer are destroyed by g_pGameView
}
