#include "gamestates/game/bob.h"
#include <ace/managers/blit.h>

tBob *bobCreate(
	tBitMap *pBitmap, tBitMap *pMask,
	tBobFrameOffset *pFrameOffsets, FUBYTE fubMaxFrameHeight, FUBYTE fubFrameIdx
) {
	tBob *pBob;

	logBlockBegin(
		"bobCreate(pBitmap: %p, pMask: %p, pFrameOffsets: %u, "
		"fubMaxFrameHeight: %"PRI_FUBYTE" fubFrameIdx: %"PRI_FUBYTE")",
		pBitmap, pMask, pFrameOffsets, fubMaxFrameHeight, fubFrameIdx
	);

	pBob = memAllocFastClear(sizeof(tBob));
	if(!pBob) {
		logWrite("ERR: Couldn't alloc bob struct!\n");
		return 0;
	}
	logWrite("Addr: %p\n", pBob);
	pBob->sData.pBitmap = pBitmap;
	pBob->sData.pMask = pMask;
	pBob->sData.pFrameOffsets = pFrameOffsets;
	pBob->fubCurrFrame = fubFrameIdx;
	pBob->ubState = BOB_STATE_START_DRAWING;
	pBob->isDrawn = 0;
	pBob->fubMaxFrameHeight = fubMaxFrameHeight;

	// BG buffer
	pBob->pBg = bitmapCreate(
		(bitmapGetByteWidth(pBitmap) << 3) + 16, fubMaxFrameHeight,
		pBitmap->Depth,	bitmapIsInterleaved(pBitmap) ? BMF_INTERLEAVED : 0
	);

	logBlockEnd("bobCreate()");
	return pBob;
}

void bobDestroy(tBob *pBob) {
	logBlockBegin("bobDestroy(pBob: %p)", pBob);
	bitmapDestroy(pBob->pBg);
	memFree(pBob, sizeof(tBob));
	logBlockEnd("bobDestroy()");
}

void bobSetData(
	tBob *pBob, tBitMap *pFrames, tBitMap *pMask, tBobFrameOffset *pOffsets
) {
	pBob->sData.pBitmap = pFrames;
	pBob->sData.pMask = pMask;
	pBob->sData.pFrameOffsets = pOffsets;
}

tBob *bobUniqueCreate(
	char *szBitmapPath, char *szMaskPath,
	tBobFrameOffset *pFrameOffsets, FUBYTE fubMaxFrameHeight, FUBYTE fubFrameIdx
)	{
	tBitMap *pBitmap;
	tBitMap *pMask;
	tBob *pBob;

	logBlockBegin(
		"bobUniqueCreate(szBitmapPath: %s, szMaskPath: %s, pFrameOffsets: %p, "
		"fubMaxFrameHeight: %"PRI_FUBYTE", fubFrameIdx: %"PRI_FUBYTE")",
		szBitmapPath, szMaskPath, pFrameOffsets, fubMaxFrameHeight, fubFrameIdx
	);
	pBob = 0;
	pBitmap = bitmapCreateFromFile(szBitmapPath);
	if(!pBitmap) {
		logWrite("ERR: Couldn't read bitmap file!\n");
		logBlockEnd("bobUniqueCreate()");
		return 0;
	}
	if(fubMaxFrameHeight == 0)
	fubMaxFrameHeight = pBitmap->Rows;
	pMask = bitmapCreateFromFile(szMaskPath);
	if(!pMask) {
		logWrite("ERR: Couldn't read mask file!\n");
		bitmapDestroy(pBitmap);
		logBlockEnd("bobUniqueCreate()");
		return 0;
	}
	pBob = bobCreate(pBitmap, pMask, pFrameOffsets, fubMaxFrameHeight, fubFrameIdx);
	logBlockEnd("bobUniqueCreate()");
	return pBob;
}

void bobUniqueDestroy(tBob *pBob) {
	logBlockBegin("bobUniqueDestroy(pBob: %p)", pBob);
	bitmapDestroy(pBob->sData.pBitmap);
	bitmapDestroy(pBob->sData.pMask);
	bobDestroy(pBob);
	logBlockEnd("bobUniqueDestroy()");
}

void bobChangeFrame(tBob *pBob, FUBYTE fubFrameIdx) {
	pBob->fubCurrFrame = fubFrameIdx;
	pBob->uwOffsY = fubFrameIdx * pBob->fubMaxFrameHeight;
}

UWORD bobUndraw(tBob *pBob, tSimpleBufferManager *pDest) {
	if(pBob->ubState == BOB_STATE_NODRAW || pBob->ubState == BOB_STATE_START_DRAWING || !pBob->isDrawn)
		return 0;
	blitCopyAligned(
		pBob->pBg, 0, 0,
		pDest->pBuffer, pBob->sPrevCoord.sUwCoord.uwX & 0xFFF0,
		pBob->sPrevCoord.sUwCoord.uwY + pBob->sBgDrawOffset.uwDy,
		bitmapGetByteWidth(pBob->pBg) << 3, pBob->sBgDrawOffset.uwHeight
	);
	pBob->isDrawn = 0;
	if(pBob->ubState == BOB_STATE_STOP_DRAWING)
		pBob->ubState = BOB_STATE_NODRAW;
	return 1;
}

UWORD bobDraw(
	tBob *pBob, tSimpleBufferManager *pDest, UWORD uwX, UWORD uwY,
	UBYTE ubBgDy, UBYTE ubBgHeight
) {
	// Check if needs blit
	if(pBob->ubState <= BOB_STATE_STOP_DRAWING)
		return 0;
	if(!simpleBufferIsRectVisible(
		pDest, uwX, uwY, bitmapGetByteWidth(pBob->pBg) << 3, pBob->pBg->Rows
	)) {
		return 0;
	}

	// Save BG
	UWORD uwFrameDy, uwFrameHeight;
	if(pBob->sData.pFrameOffsets) {
		uwFrameDy = pBob->sData.pFrameOffsets[pBob->fubCurrFrame].uwDy;
		uwFrameHeight = pBob->sData.pFrameOffsets[pBob->fubCurrFrame].uwHeight;
		pBob->sBgDrawOffset.uwDy = ubBgDy;
		pBob->sBgDrawOffset.uwHeight = ubBgHeight;
	}
	else {
		uwFrameDy = pBob->sBgDrawOffset.uwDy = 0;
		uwFrameHeight = pBob->sBgDrawOffset.uwHeight = pBob->fubMaxFrameHeight;
	}
	blitCopyAligned(
		pDest->pBuffer, uwX & 0xFFF0, uwY + pBob->sBgDrawOffset.uwDy,
		pBob->pBg, 0, 0,
		bitmapGetByteWidth(pBob->pBg) << 3, pBob->sBgDrawOffset.uwHeight
	);

	// Redraw bob
	blitCopyMask(
		pBob->sData.pBitmap, 0, pBob->uwOffsY + uwFrameDy,
		pDest->pBuffer, uwX, uwY + uwFrameDy,
		bitmapGetByteWidth(pBob->sData.pBitmap)<<3, uwFrameHeight,
		(UWORD*)pBob->sData.pMask->Planes[0]
	);

	// Update bob position
	pBob->sPrevCoord.sUwCoord.uwX = uwX;
	pBob->sPrevCoord.sUwCoord.uwY = uwY;
	pBob->isDrawn = 1;

	if(pBob->ubState == BOB_STATE_START_DRAWING)
		pBob->ubState = BOB_STATE_DRAW;
	return 1;
}

void bobEnable(tBob *pBob) {
	if(pBob->ubState < BOB_STATE_START_DRAWING)
		pBob->ubState = BOB_STATE_START_DRAWING;
}

void bobDisable(tBob *pBob) {
	if(pBob->ubState > BOB_STATE_STOP_DRAWING)
		pBob->ubState = BOB_STATE_STOP_DRAWING;
}
