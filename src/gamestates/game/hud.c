#include "gamestates/game/hud.h"
#include <ace/config.h>
#include <ace/managers/blit.h>
#include <ace/managers/viewport/simplebuffer.h>
#include <ace/utils/bitmap.h>
#include "gamestates/game/game.h"
#include "gamestates/game/world.h"
#include "gamestates/game/player.h"
#include "vehicletypes.h"

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
	bitmapLoadFromFile(s_pHudBfr->pBuffer, "data/hud_bg.bm", 0, 0);
}

void drawHudBar(
	const UWORD uwBarY, const UWORD uwValue, const UWORD uwMaxValue, const UBYTE ubColor
) {
	// TODO draw rects on only one bitplane
	const UWORD uwBarHeight = 4;
	const UWORD uwBarX = 48;
	const UWORD uwMaxBarWidth = 48;

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
	// TODO one thing per frame, HP - always
	tVehicle *pVehicle = &g_pLocalPlayer->sVehicle;
	tVehicleType *pType = &g_pVehicleTypes[g_pLocalPlayer->ubCurrentVehicleType];

	// Draw bars
	const UWORD uwBarLifeY = 14;
	const UWORD uwBarAmmoY = 30;
	const UWORD uwBarFuelY = 46;
	drawHudBar(uwBarLifeY, pVehicle->ubLife, pType->ubMaxLife, 4);
	drawHudBar(uwBarAmmoY, pVehicle->ubBaseAmmo, pType->ubMaxBaseAmmo, 8);
	drawHudBar(uwBarFuelY, pVehicle->ubFuel, pType->ubMaxFuel, 11);
}

void hudDestroy(void) {
	// VPort & simpleBuffer are destroyed by g_pGameView
}
