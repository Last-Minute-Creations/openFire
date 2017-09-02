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
		TAG_VPORT_BPP, WORLD_BPP,
		TAG_VPORT_OFFSET_TOP, 1,
		TAG_DONE
	);
	s_pHudBfr = simpleBufferCreate(0,
		TAG_SIMPLEBUFFER_VPORT, s_pHudVPort,
		TAG_SIMPLEBUFFER_COPLIST_OFFSET, WORLD_COP_VPHUD_POS,
		TAG_DONE
	);

	tCopCmd *pCopList = g_pWorldView->pCopList->pBackBfr->pList;
	copSetWait(
		&pCopList[WORLD_COP_VPHUD_DMAOFF_POS+0].sWait,
		0x04,
		pCopList[WORLD_COP_VPHUD_POS].sWait.bfWaitY
	);
	copSetMove(
		&pCopList[WORLD_COP_VPHUD_DMAOFF_POS+1].sMove,
		&custom.dmacon, BITCLR|DMAF_RASTER
	);
	copSetMove(
		&pCopList[WORLD_COP_VPHUD_DMAON_POS].sMove,
		&custom.dmacon, BITSET|DMAF_RASTER
	);
	// Same for front bfr
	CopyMemQuick(
		&g_pWorldView->pCopList->pBackBfr->pList[WORLD_COP_VPHUD_DMAOFF_POS],
		&g_pWorldView->pCopList->pFrontBfr->pList[WORLD_COP_VPHUD_DMAOFF_POS],
		2*sizeof(tCopCmd)
	);
	CopyMemQuick(
		&g_pWorldView->pCopList->pBackBfr->pList[WORLD_COP_VPHUD_DMAON_POS],
		&g_pWorldView->pCopList->pFrontBfr->pList[WORLD_COP_VPHUD_DMAON_POS],
		1*sizeof(tCopCmd)
	);

	// Initial draw on buffer
	bitmapLoadFromFile(s_pHudBfr->pBuffer, "data/hud_bg.bm", 0, 0);
}

void hudDestroy(void) {
	// VPort & simpleBuffer are destroyed by g_pGameView
}
