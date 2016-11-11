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
	pBob->pBitmap = pBitmap;
	pBob->pMask = pMask;
	pBob->uwOffsY = uwFrameIdx*uwFrameHeight;
	pBob->uwHeight = uwFrameHeight;
	
	// BG buffer
	pBob->pBg = bitmapCreate(
		pBitmap->BytesPerRow << 3,
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
	if(pBitmap) {
		if(uwFrameHeight == 0)
			uwFrameHeight = pBitmap->Rows;
		pMask = bitmapMaskCreate(szMaskPath);
		if(pMask)
			pBob = bobCreate(pBitmap, pMask, uwFrameHeight, uwFrameIdx);
	}
	logBlockEnd("bobUniqueCreate()");
	return pBob;
}

void bobUniqueDestroy(tBob *pBob) {
	logBlockBegin("bobUniqueDestroy(pBob: %p)", pBob);
	bitmapDestroy(pBob->pBitmap);
	bitmapMaskDestroy(pBob->pMask);
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
	BltBitMap(
		pBob->pBg, 0, 0,
		pDest, pBob->uwPrevX, pBob->uwPrevY,
		pBob->pBg->BytesPerRow<<3, pBob->pBg->Rows,
		0xC0, 0xFF, 0
	);	
}

/**
 *  @todo Change to blitCopy when it gets stable
 */
void bobDraw(tBob *pBob, tBitMap *pDest, UWORD uwX, UWORD uwY) { 
	// Save BG
	BltBitMap(
		pDest, uwX, uwY,
		pBob->pBg, 0, 0,
		pBob->pBg->BytesPerRow<<3, pBob->pBg->Rows,
		0xC0, 0xFF, 0
	);
	
	// Redraw bob
	blitCopyMask(
		pBob->pBitmap, 0, pBob->uwOffsY,
		pDest, uwX, uwY,
		pBob->pBg->BytesPerRow<<3, pBob->pBg->Rows,
		pBob->pMask->pData
	);
	
	// Update bob position
	pBob->uwPrevX = uwX;
	pBob->uwPrevY = uwY;
}