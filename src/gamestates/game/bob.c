#include "gamestates/game/bob.h"
#include <ace/managers/blit.h>

tBob *bobCreate(
	tBitMap *pBitmap, tBitmapMask *pMask,
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
	pBob->sSource.pBitmap = pBitmap;
	pBob->sSource.pMask = pMask;
	pBob->fubCurrFrame = fubFrameIdx;
	pBob->fubPrevFrame = fubFrameIdx;
	pBob->ubFlags = BOB_FLAG_START_DRAWING;
	pBob->isDrawn = 0;
	pBob->fubMaxFrameHeight = fubMaxFrameHeight;
	pBob->sSource.pFrameOffsets = pFrameOffsets;

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

void bobSetSource(tBob *pBob, tBobSource *pSource) {
	pBob->sSource.pBitmap = pSource->pBitmap;
	pBob->sSource.pMask = pSource->pMask;
	pBob->sSource.pFrameOffsets = pSource->pFrameOffsets;
}

tBob *bobUniqueCreate(
	char *szBitmapPath, char *szMaskPath,
	tBobFrameOffset *pFrameOffsets, FUBYTE fubMaxFrameHeight, FUBYTE fubFrameIdx
)	{
	tBitMap *pBitmap;
	tBitmapMask *pMask;
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
	pMask = bitmapMaskCreateFromFile(szMaskPath);
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
	bitmapDestroy(pBob->sSource.pBitmap);
	bitmapMaskDestroy(pBob->sSource.pMask);
	bobDestroy(pBob);
	logBlockEnd("bobUniqueDestroy()");
}

void bobChangeFrame(tBob *pBob, FUBYTE fubFrameIdx) {
	pBob->fubCurrFrame = fubFrameIdx;
	pBob->uwOffsY = fubFrameIdx * pBob->fubMaxFrameHeight;
}

UWORD bobUndraw(tBob *pBob, tSimpleBufferManager *pDest) {
	if(pBob->ubFlags == BOB_FLAG_NODRAW || pBob->ubFlags == BOB_FLAG_START_DRAWING || !pBob->isDrawn)
		return 0;
	UWORD uwFrameDy, uwFrameHeight;
	if(pBob->sSource.pFrameOffsets) {
		uwFrameDy = pBob->sSource.pFrameOffsets[pBob->fubPrevFrame].uwDy;
		uwFrameHeight = pBob->sSource.pFrameOffsets[pBob->fubPrevFrame].uwHeight;
	}
	else {
		uwFrameDy = 0;
		uwFrameHeight = pBob->fubMaxFrameHeight;
	}
	blitCopyAligned(
		pBob->pBg, 0, 0,
		pDest->pBuffer, pBob->sPrevCoord.sUwCoord.uwX & 0xFFF0,
		pBob->sPrevCoord.sUwCoord.uwY + uwFrameDy,
		bitmapGetByteWidth(pBob->pBg) << 3, uwFrameHeight
	);
	pBob->isDrawn = 0;
	if(pBob->ubFlags == BOB_FLAG_STOP_DRAWING)
		pBob->ubFlags = BOB_FLAG_NODRAW;
	return 1;
}

UWORD bobDraw(tBob *pBob, tSimpleBufferManager *pDest, UWORD uwX, UWORD uwY) {
	// Save BG
	if(pBob->ubFlags == BOB_FLAG_NODRAW || pBob->ubFlags == BOB_FLAG_STOP_DRAWING)
		return 0;
	if(!simpleBufferIsRectVisible(
		pDest, uwX, uwY, bitmapGetByteWidth(pBob->pBg) << 3, pBob->pBg->Rows
	)) {
		return 0;
	}
	UWORD uwFrameDy, uwFrameHeight;
	if(pBob->sSource.pFrameOffsets) {
		uwFrameDy = pBob->sSource.pFrameOffsets[pBob->fubCurrFrame].uwDy;
		uwFrameHeight = pBob->sSource.pFrameOffsets[pBob->fubCurrFrame].uwHeight;
	}
	else {
		uwFrameDy = 0;
		uwFrameHeight = pBob->fubMaxFrameHeight;
	}
	blitCopyAligned(
		pDest->pBuffer, uwX & 0xFFF0, uwY + uwFrameDy,
		pBob->pBg, 0, 0,
		bitmapGetByteWidth(pBob->pBg) << 3, uwFrameHeight
	);

	// Redraw bob
	blitCopyMask(
		pBob->sSource.pBitmap, 0, pBob->uwOffsY + uwFrameDy,
		pDest->pBuffer, uwX, uwY + uwFrameDy,
		bitmapGetByteWidth(pBob->sSource.pBitmap)<<3, uwFrameHeight,
		pBob->sSource.pMask->pData
	);

	// Update bob position
	pBob->sPrevCoord.sUwCoord.uwX = uwX;
	pBob->sPrevCoord.sUwCoord.uwY = uwY;
	pBob->isDrawn = 1;
	pBob->fubPrevFrame = pBob->fubCurrFrame;

	if(pBob->ubFlags == BOB_FLAG_START_DRAWING)
		pBob->ubFlags = BOB_FLAG_DRAW;
	return 1;
}

void bobEnable(tBob *pBob) {
	if(pBob->ubFlags < BOB_FLAG_START_DRAWING)
		pBob->ubFlags = BOB_FLAG_START_DRAWING;
}

void bobDisable(tBob *pBob) {
	if(pBob->ubFlags > BOB_FLAG_STOP_DRAWING)
		pBob->ubFlags = BOB_FLAG_STOP_DRAWING;
}
