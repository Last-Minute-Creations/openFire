#include "gamestates/game/bob.h"
#include <ace/managers/blit.h>

tBob *bobCreate(tBitMap *pBitmap, tBitmapMask *pMask, UWORD uwFrameHeight, UWORD uwFrameIdx) {
	tBob *pBob;
	
	logBlockBegin(
		"bobCreate(pBitmap: %p, pMask: %p, uwFrameHeight: %u, uwFrameIdx: %u)",
		pBitmap, pMask, uwFrameHeight, uwFrameIdx
	);
	
	pBob = memAllocFastClear(sizeof(tBob));
	if(!pBob) {
		logWrite("ERR: Couldn't alloc bob struct!\n");
		return 0;
	}
	logWrite("Addr: %p\n", pBob);
	pBob->sSource.pBitmap = pBitmap;
	pBob->sSource.pMask = pMask;
	pBob->uwOffsY = uwFrameIdx*uwFrameHeight;
	pBob->uwHeight = uwFrameHeight;
	pBob->ubFlags = BOB_FLAG_START_DRAWING;
	
	// BG buffer
	pBob->pBg = bitmapCreate(
		bitmapGetByteWidth(pBitmap) << 3,
		uwFrameHeight,
		pBitmap->Depth,
		0
	);
	
	// TODO init bg buf, so first draw won't corrupt dest

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
}

tBob *bobUniqueCreate(char *szBitmapPath, char *szMaskPath, UWORD uwFrameHeight, UWORD uwFrameIdx) {
	tBitMap *pBitmap;
	tBitmapMask *pMask;
	tBob *pBob;
	
	logBlockBegin(
		"bobUniqueCreate(szBitmapPath: %s, szMaskPath: %s, uwFrameHeight: %hu, uwFrameIdx: %hu)",
		szBitmapPath, szMaskPath, uwFrameHeight, uwFrameIdx
	);
	pBob = 0;
	pBitmap = bitmapCreateFromFile(szBitmapPath);
	if(!pBitmap) {
		logWrite("ERR: Couldn't read bitmap file!\n");
		logBlockEnd("bobUniqueCreate()");
		return 0;
	}
	if(uwFrameHeight == 0)
		uwFrameHeight = pBitmap->Rows;
	pMask = bitmapMaskCreateFromFile(szMaskPath);
	if(!pMask) {
		logWrite("ERR: Couldn't read mask file!\n");
		bitmapDestroy(pBitmap);
		logBlockEnd("bobUniqueCreate()");
		return 0;
	}
	pBob = bobCreate(pBitmap, pMask, uwFrameHeight, uwFrameIdx);
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

void bobChangeFrame(tBob *pBob, UWORD uwFrameIdx) {
	pBob->uwOffsY = uwFrameIdx * pBob->uwHeight;
}

/**
 *  @todo Change to blitCopy when it gets stable
 */
void bobUndraw(tBob *pBob, tBitMap *pDest) {
	if(pBob->ubFlags == BOB_FLAG_NODRAW || pBob->ubFlags == BOB_FLAG_START_DRAWING)
		return;
	BltBitMap(
		pBob->pBg, 0, 0,
		pDest, pBob->sPrevCoord.sUwCoord.uwX, pBob->sPrevCoord.sUwCoord.uwY,
		pBob->pBg->BytesPerRow<<3, pBob->pBg->Rows,
		0xC0, 0xFF, 0
	);
	if(pBob->ubFlags == BOB_FLAG_STOP_DRAWING)
		pBob->ubFlags = BOB_FLAG_NODRAW;
}

/**
 *  @todo Change to blitCopy when it gets stable
 */
void bobDraw(tBob *pBob, tBitMap *pDest, UWORD uwX, UWORD uwY) { 
	// Save BG
	if(pBob->ubFlags == BOB_FLAG_NODRAW || pBob->ubFlags == BOB_FLAG_STOP_DRAWING)
		return;
	BltBitMap(
		pDest, uwX, uwY,
		pBob->pBg, 0, 0,
		pBob->pBg->BytesPerRow<<3, pBob->pBg->Rows,
		0xC0, 0xFF, 0
	);
	
	// Redraw bob
	blitCopyMask(
		pBob->sSource.pBitmap, 0, pBob->uwOffsY,
		pDest, uwX, uwY,
		pBob->pBg->BytesPerRow<<3, pBob->pBg->Rows,
		pBob->sSource.pMask->pData
	);
	
	// Update bob position
	pBob->sPrevCoord.sUwCoord.uwX = uwX;
	pBob->sPrevCoord.sUwCoord.uwY = uwY;

	if(pBob->ubFlags == BOB_FLAG_START_DRAWING)
		pBob->ubFlags = BOB_FLAG_DRAW;
}
