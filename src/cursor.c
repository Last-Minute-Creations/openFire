#include "cursor.h"
#include <ace/macros.h>
#include <ace/managers/mouse.h>
#include <ace/utils/bitmap.h>

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

void cursorCreate(tView *pView, FUBYTE fubSpriteIdx, char *szPath, UWORD uwRawCopPos) {
	cursorSetConstraints(0, 0, 320, 255);
	s_pCrosshair = bitmapCreateFromFile(szPath);
	UWORD *pSpriteBfr = (UWORD*)s_pCrosshair->Planes[0];
	cursorUpdate();
	ULONG ulSprAddr = (ULONG)((UBYTE*)pSpriteBfr);
	if(pView->pCopList->ubMode == COPPER_MODE_RAW) {
		tCopCmd *pCrossList = &pView->pCopList->pBackBfr->pList[uwRawCopPos];
		copSetMove(&pCrossList[0].sMove, &pSprPtrs[fubSpriteIdx].uwHi, ulSprAddr >> 16);
		copSetMove(&pCrossList[1].sMove, &pSprPtrs[fubSpriteIdx].uwLo, ulSprAddr & 0xFFFF);
		CopyMemQuick(
			&pView->pCopList->pBackBfr->pList[uwRawCopPos],
			&pView->pCopList->pFrontBfr->pList[uwRawCopPos],
			2*sizeof(tCopCmd)
		);
	}
	else {
		tCopBlock *pBlock = copBlockCreate(pView->pCopList, 2, 0, 0);
		copMove(pView->pCopList, pBlock, &pSprPtrs[fubSpriteIdx].uwHi, ulSprAddr >> 16);
		copMove(pView->pCopList, pBlock, &pSprPtrs[fubSpriteIdx].uwLo, ulSprAddr & 0xFFFF);
	}
}

void cursorDestroy(void) {
	bitmapDestroy(s_pCrosshair);
	// CopBlock will be freed with whole copperlist
}
