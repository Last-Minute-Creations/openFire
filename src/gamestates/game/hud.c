#include "gamestates/game/hud.h"
#include <ace/config.h>
#include <ace/managers/blit.h>
#include <ace/managers/viewport/simplebuffer.h>
#include <ace/utils/bitmap.h>
#include "gamestates/game/game.h"
#include "gamestates/game/player.h"
#include "vehicletypes.h"

static tVPort *s_pHudVPort;
static tSimpleBufferManager *s_pHudBfr;
static tBitMap *s_pHudDriving, *s_pHudSelecting;
static UBYTE s_ubHudState;

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
		TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_INTERLEAVED,
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
		&custom.dmacon, BITCLR | DMAF_RASTER | DMAF_SPRITE
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
	bitmapLoadFromFile(s_pHudBfr->pBuffer, "data/hud/blank.bm", 0, 0);
	s_pHudDriving = bitmapCreateFromFile("data/hud/driving.bm");
	s_pHudSelecting = bitmapCreateFromFile("data/hud/selecting.bm");
}

void hudChangeState(UBYTE ubState) {
	s_ubHudState = ubState;
	tBitMap *pHudPanels[2];
	pHudPanels[HUD_STATE_DRIVING] = s_pHudDriving;
	pHudPanels[HUD_STATE_SELECTING] = s_pHudSelecting;

	blitCopy(
		pHudPanels[ubState], 0, 0, s_pHudBfr->pBuffer, 2, 2,
		bitmapGetByteWidth(s_pHudDriving) << 3,
		s_pHudDriving->Rows, MINTERM_COOKIE, 0xFF
	);

	UWORD uwDmaCon;
	void *pCrossCtl;
	tCopCmd *pFrontList = g_pWorldView->pCopList->pFrontBfr->pList;
	tCopCmd *pBackList = g_pWorldView->pCopList->pBackBfr->pList;
	if(ubState == HUD_STATE_DRIVING) {
		pCrossCtl = &custom.spr[2].ctl;
		uwDmaCon = BITCLR | DMAF_RASTER | DMAF_SPRITE;
	}
	else {
		pCrossCtl = &custom.spr[0].ctl;
		uwDmaCon = BITCLR | DMAF_RASTER;
	}
	// Disable/enable sprite DMA during hud
	copSetMove(&pBackList[WORLD_COP_VPHUD_DMAOFF_POS+1].sMove, &custom.dmacon, uwDmaCon);
	copSetMove(&pFrontList[WORLD_COP_VPHUD_DMAOFF_POS+1].sMove, &custom.dmacon, uwDmaCon);
	// Disable/enable crosshair sprite during hud
	copSetMove(&pBackList[WORLD_COP_CLEANUP_POS+2].sMove, pCrossCtl, 0);
	copSetMove(&pFrontList[WORLD_COP_CLEANUP_POS+2].sMove, pCrossCtl, 0);
}

void drawHudBar(
	const UWORD uwBarY, const UWORD uwValue, const UWORD uwMaxValue, const UBYTE ubColor
) {
	// TODO draw rects on only one bitplane
	const UWORD uwBarHeight = 4;
	const UWORD uwBarX = 71;
	const UWORD uwMaxBarWidth = 32;

	// Calculate color length
	UWORD uwCurrBarWidth = (uwMaxBarWidth*uwValue)/ uwMaxValue;

	// Black part of bar
	if(uwCurrBarWidth != uwMaxBarWidth)
		blitRect(
			s_pHudBfr->pBuffer,	uwBarX + uwCurrBarWidth, uwBarY,
			uwMaxBarWidth - uwCurrBarWidth, uwBarHeight, 0
		);

	// Colored part of bar
	if(uwCurrBarWidth)
		blitRect(
			s_pHudBfr->pBuffer, uwBarX, uwBarY,
			uwCurrBarWidth, uwBarHeight, ubColor
		);
}

void hudUpdate(void) {
	if(s_ubHudState == HUD_STATE_DRIVING) {
		// TODO one thing per frame, HP - always
		tVehicle *pVehicle = &g_pLocalPlayer->sVehicle;
		tVehicleType *pType = &g_pVehicleTypes[g_pLocalPlayer->ubCurrentVehicleType];

		// Draw bars
		const UWORD uwBarLifeY = 5;
		const UWORD uwBarAmmoY = 13;
		const UWORD uwBarFuelY = 21;
		drawHudBar(uwBarLifeY, pVehicle->ubLife, pType->ubMaxLife, 4);
		drawHudBar(uwBarAmmoY, pVehicle->ubBaseAmmo, pType->ubMaxBaseAmmo, 8);
		drawHudBar(uwBarFuelY, pVehicle->ubFuel, pType->ubMaxFuel, 11);
	}
}

void hudDestroy(void) {
	bitmapDestroy(s_pHudDriving);
	bitmapDestroy(s_pHudSelecting);
	// VPort & simpleBuffer are destroyed by g_pGameView
}
