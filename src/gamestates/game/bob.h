#ifndef GUARD_OF_GAMESTATES_GAME_BOB_H
#define GUARD_OF_GAMESTATES_GAME_BOB_H

#include <ace/config.h>
#include <ace/utils/bitmap.h>
#include <ace/utils/bitmapmask.h>

typedef struct _tBob {
	tBitMap *pBitmap;
	tBitmapMask *pMask;
	tBitMap* pBg;
	UWORD uwPrevX;
	UWORD uwPrevY;
	UWORD uwOffsY;
	UWORD uwHeight;
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
