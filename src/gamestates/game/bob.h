#ifndef GUARD_OF_GAMESTATES_GAME_BOB_H
#define GUARD_OF_GAMESTATES_GAME_BOB_H

#include <ace/config.h>
#include <ace/utils/bitmap.h>
#include <ace/utils/bitmapmask.h>

/// Used when bob is inactive - no undraw, no draw
#define BOB_FLAG_NODRAW        0
/// Used when bob is going to be inactive - undraw, no draw
#define BOB_FLAG_STOP_DRAWING  1
/// Used when bob is going to be active - no undraw, draw
#define BOB_FLAG_START_DRAWING 2
/// Used when bob is active - undraw, draw
#define BOB_FLAG_DRAW          4

typedef struct _tBobSource {
	tBitMap *pBitmap;
	tBitmapMask *pMask;	
} tBobSource;

typedef struct _tBob {
	tBobSource sSource;
	tBitMap* pBg;
	tUwCoordYX sPrevCoord;
	UWORD uwOffsY;
	UWORD uwHeight;
	UBYTE ubFlags;
} tBob;

tBob *bobCreate(
	IN tBitMap *pBitmap,
	IN tBitmapMask *pMask,
	IN UWORD uwFrameHeight,
	IN UWORD uwFrameIdx
);

void bobDestroy(
	IN tBob *pBob
);

void bobSetSource(
	IN tBob *pBob,
	IN tBobSource *pSource
);

tBob *bobUniqueCreate(
	char *szBitmapPath,
	char *szMaskPath,
	UWORD uwFrameHeight,
	UWORD uwFrameIdx
);

void bobUniqueDestroy(
	IN tBob *pBob
);

void bobChangeFrame(
	IN tBob *pBob,
	IN UWORD uwFrameIdx
);

void bobUndraw(
	IN tBob *pBob,
	IN tBitMap *pDest
);

void bobDraw(
	IN tBob *pBob,
	IN tBitMap *pDest,
	IN UWORD uwX,
	IN UWORD uwY
);

#endif // GUARD_OF_GAMESTATES_GAME_BOB_H
