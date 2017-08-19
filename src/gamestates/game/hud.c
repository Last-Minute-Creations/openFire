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
	s_pHudVPort = vPortCreate(0,
		TAG_VPORT_VIEW,	g_pWorldView,
		TAG_VPORT_BPP, HUD_BPP,
		TAG_DONE
	);
	s_pHudBfr = simpleBufferCreate(0,
		TAG_SIMPLEBUFFER_VPORT, s_pHudVPort,
		TAG_SIMPLEBUFFER_COPLIST_OFFSET, 14+0,
		TAG_DONE
	);
	
	// Initial draw on buffer
	bitmapLoadFromFile(s_pHudBfr->pBuffer, "data/hud_bg.bm", 0, 0);	
}

void hudDestroy(void) {
	// VPort & simpleBuffer are destroyed by g_pGameView
}
