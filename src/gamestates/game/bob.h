#ifndef GUARD_OF_GAMESTATES_GAME_BOB_H
#define GUARD_OF_GAMESTATES_GAME_BOB_H

#include <ace/utils/bitmap.h>
#include <ace/managers/viewport/simplebuffer.h>

/// Used when bob is inactive - no undraw, no draw
#define BOB_STATE_NODRAW        0
/// Used when bob is going to be inactive - undraw, no draw
#define BOB_STATE_STOP_DRAWING  1
/// Used when bob is going to be active - no undraw, draw
#define BOB_STATE_START_DRAWING 2
/// Used when bob is active - undraw, draw
#define BOB_STATE_DRAW          3

typedef struct _tBobOffset {
	UWORD uwDy;
	UWORD uwHeight;
} tBobFrameOffset;

typedef struct _tBobData {
	tBitMap *pBitmap;
	tBitMap *pMask;
	tBobFrameOffset *pFrameOffsets;
} tBobData;

typedef struct _tBob {
	tBobData sData;
	tBitMap* pBg;
	tUwCoordYX sPrevCoord;
	tBobFrameOffset sBgDrawOffset;
	FUBYTE fubCurrFrame;
	FUBYTE fubMaxFrameHeight;
	UWORD uwOffsY;
	UBYTE ubState;
	UBYTE isDrawn;
} tBob;

tBob *bobCreate(
	tBitMap *pBitmap, tBitMap *pMask, tBobFrameOffset *pFrameOffsets,
	FUBYTE fubMaxFrameHeight, FUBYTE fubFrameIdx
);

void bobDestroy(tBob *pBob);

void bobSetData(
	tBob *pBob, tBitMap *pFrames, tBitMap *pMask, tBobFrameOffset *pOffsets
);

tBob *bobUniqueCreate(
	char *szBitmapPath, char *szMaskPath, tBobFrameOffset *pFrameOffsets,
	FUBYTE fubMaxFrameHeight, FUBYTE fubFrameIdx
);

void bobUniqueDestroy(tBob *pBob);

void bobChangeFrame(tBob *pBob, FUBYTE fubFrameIdx);

UWORD bobUndraw(tBob *pBob, tSimpleBufferManager *pDest);

UWORD bobDraw(
	tBob *pBob, tSimpleBufferManager *pDest, UWORD uwX, UWORD uwY,
	UBYTE ubBgDy, UBYTE ubBgHeight
);

void bobEnable(tBob *pBob);

void bobDisable(tBob *pBob);

#endif // GUARD_OF_GAMESTATES_GAME_BOB_H
