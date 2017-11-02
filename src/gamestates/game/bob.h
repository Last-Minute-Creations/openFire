#ifndef GUARD_OF_GAMESTATES_GAME_BOB_H
#define GUARD_OF_GAMESTATES_GAME_BOB_H

#include <ace/utils/bitmap.h>
#include <ace/utils/bitmapmask.h>
#include <ace/managers/viewport/simplebuffer.h>

/// Used when bob is inactive - no undraw, no draw
#define BOB_FLAG_NODRAW        0
/// Used when bob is going to be inactive - undraw, no draw
#define BOB_FLAG_STOP_DRAWING  1
/// Used when bob is going to be active - no undraw, draw
#define BOB_FLAG_START_DRAWING 2
/// Used when bob is active - undraw, draw
#define BOB_FLAG_DRAW          3

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
	FUBYTE fubPrevFrame;
	FUBYTE fubCurrFrame;
	FUBYTE fubMaxFrameHeight;
	UWORD uwOffsY;
	UBYTE ubFlags;
	UBYTE isDrawn;
} tBob;

tBob *bobCreate(
	IN tBitMap *pBitmap,
	IN tBitMap *pMask,
	IN tBobFrameOffset *pFrameOffsets,
	IN FUBYTE fubMaxFrameHeight,
	IN FUBYTE fubFrameIdx
);

void bobDestroy(
	IN tBob *pBob
);

void bobSetData(
	IN tBob *pBob,
	IN tBitMap *pFrames,
	IN tBitMap *pMask,
	IN tBobFrameOffset *pOffsets
);

tBob *bobUniqueCreate(
	char *szBitmapPath,
	char *szMaskPath,
	IN tBobFrameOffset *pFrameOffsets,
	IN FUBYTE fubMaxFrameHeight,
	IN FUBYTE fubFrameIdx
);

void bobUniqueDestroy(
	IN tBob *pBob
);

void bobChangeFrame(
	IN tBob *pBob,
	IN FUBYTE fubFrameIdx
);

UWORD bobUndraw(
	IN tBob *pBob,
	IN tSimpleBufferManager *pDest
);

UWORD bobDraw(
	IN tBob *pBob,
	IN tSimpleBufferManager *pDest,
	IN UWORD uwX,
	IN UWORD uwY
);

void bobEnable(
	IN tBob *pBob
);

void bobDisable(
	IN tBob *pBob
);

#endif // GUARD_OF_GAMESTATES_GAME_BOB_H
