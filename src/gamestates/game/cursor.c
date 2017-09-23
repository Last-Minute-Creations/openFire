#include "gamestates/game/cursor.h"
#include <ace/macros.h>
#include <ace/managers/mouse.h>
#include <ace/utils/bitmap.h>
#include "gamestates/game/game.h"

UWORD g_uwMouseX, g_uwMouseY;
static tBitMap *s_pCrosshair;

static s_uwXLo, s_uwYLo, s_uwXHi, s_uwYHi;

void cursorSetConstraints(UWORD uwXLo, UWORD uwYLo, UWORD uwXHi, UWORD uwYHi) {
	s_uwXLo = uwXLo;
	s_uwYLo = uwYLo;
	s_uwXHi = uwXHi;
	s_uwYHi = uwYHi;
}

void cursorUpdate(void) {
	// Destination angle from mouse
	g_uwMouseX = CLAMP(mouseGetX(), s_uwXLo, s_uwXHi);
	g_uwMouseY = CLAMP(mouseGetY(), s_uwYLo, s_uwYHi);
	mouseSetPosition(g_uwMouseX, g_uwMouseY);
	const UWORD uwCrossHeight = 11;
	UWORD uwVStart =0x2B-4 + g_uwMouseY;
	UWORD uwVStop = uwVStart + uwCrossHeight;

	UWORD *pSpriteBfr = (UWORD*)s_pCrosshair->Planes[0];
	pSpriteBfr[0] = (uwVStart << 8) | (64-(4>>1) + (g_uwMouseX >> 1));
	pSpriteBfr[1] = (uwVStop << 8)
		| (((uwVStart & (1 << 8)) >> 8) << 2)
		| (((uwVStop  & (1 << 8)) >> 8) << 1)
		| (g_uwMouseX & 1);
}

void cursorCreate(void) {
	cursorSetConstraints(0, 0, 320, 255);
	s_pCrosshair = bitmapCreateFromFile("data/crosshair.bm");
	UWORD *pSpriteBfr = (UWORD*)s_pCrosshair->Planes[0];
	tCopCmd *pCrossList = &g_pWorldView->pCopList->pBackBfr->pList[WORLD_COP_CROSS_POS];
	cursorUpdate();
	copSetMove(&pCrossList[0].sMove, &pSprPtrs[2].uwHi, (ULONG)((UBYTE*)pSpriteBfr) >> 16);
	copSetMove(&pCrossList[1].sMove, &pSprPtrs[2].uwLo, (ULONG)((UBYTE*)pSpriteBfr) & 0xFFFF);
	CopyMemQuick(
		&g_pWorldView->pCopList->pBackBfr->pList[WORLD_COP_CROSS_POS],
		&g_pWorldView->pCopList->pFrontBfr->pList[WORLD_COP_CROSS_POS],
		2*sizeof(tCopCmd)
	);
}

void cursorDestroy(void) {
	bitmapDestroy(s_pCrosshair);
}