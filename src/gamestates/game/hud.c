#include "gamestates/game/hud.h"
#include <ace/config.h>
#include <ace/managers/blit.h>
#include <ace/managers/viewport/simplebuffer.h>
#include "gamestates/game/game.h"

tVPort *s_pHudVPort;
tSimpleBufferManager *s_pHudBfr;

void hudCreate(void) {
	s_pHudVPort = vPortCreate(g_pGameView, WINDOW_SCREEN_WIDTH, 64, GAME_BPP, 0);
	s_pHudBfr = simpleBufferCreate(s_pHudVPort, WINDOW_SCREEN_WIDTH, 64);
	
	// Initial draw on buffer
	blitRect(s_pHudBfr->pBuffer, 0, 0, 320, 1, 31);
	blitRect(s_pHudBfr->pBuffer, 0, 63, 320, 1, 31);
	blitRect(s_pHudBfr->pBuffer, 0, 0, 1, 64, 31);
	blitRect(s_pHudBfr->pBuffer, 0, 319, 1, 64, 31);
}

void hudDestroy(void) {
	// VPort & simpleBuffer are destroyed by g_pGameView
}
