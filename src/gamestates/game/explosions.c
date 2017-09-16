#include "gamestates/game/explosions.h"
#include "gamestates/game/bob.h"

#define EXPLOSION_FRAME_COUNT 6
#define EXPLOSION_FRAME_LENGTH 4
#define EXPLOSION_DURATION (EXPLOSION_FRAME_COUNT * EXPLOSION_FRAME_LENGTH)
#define EXPLOSION_SIZE 32

typedef struct _tExplosion {
	UWORD uwX;
	UWORD uwY;
	UWORD uwDuration;
	tBob *pBob;
} tExplosion;

tExplosion s_pExplosions[EXPLOSIONS_MAX];
static tBitMap *s_pBitmap;
static tBitmapMask *s_pMask;

void explosionsAdd(const IN UWORD uwX, const IN UWORD uwY) {
	// Find free explosion slot
	for(UWORD i = EXPLOSIONS_MAX; i--;) {
		logWrite("Explosion %hu (%p) has flags: %hhu\n", i, s_pExplosions[i].pBob, s_pExplosions[i].pBob->ubFlags);
		if(s_pExplosions[i].pBob->ubFlags == BOB_FLAG_NODRAW) {
			// Free slot found - setup explosion
			s_pExplosions[i].uwX = uwX;
			s_pExplosions[i].uwY = uwY;
			s_pExplosions[i].pBob->ubFlags = BOB_FLAG_START_DRAWING;
			return;
		}
	}
}

void explosionsCreate(void) {
	logBlockBegin("explosionsCreate()");
	s_pBitmap = bitmapCreateFromFile("data/explosion.bm");
	s_pMask = bitmapMaskCreateFromFile("data/explosion.msk");

	for(UWORD i = EXPLOSIONS_MAX; i--;) {
		s_pExplosions[i].pBob = bobCreate(s_pBitmap, s_pMask, EXPLOSION_SIZE, 0);
		s_pExplosions[i].pBob->ubFlags = BOB_FLAG_NODRAW;
		s_pExplosions[i].uwDuration = 0;
	}
	logBlockEnd("explosionsCreate()");
}

void explosionsDestroy(void) {
	logBlockBegin("explosionsDestroy()");
	for(UWORD i = EXPLOSIONS_MAX; i--;)
		bobDestroy(s_pExplosions[i].pBob);

	bitmapDestroy(s_pBitmap);
	bitmapMaskDestroy(s_pMask);
	logBlockEnd("explosionsDestroy()");
}

void explosionsUndraw(tBitMap *pBfr) {
	for(UWORD i = EXPLOSIONS_MAX; i--;) {
		if(!bobUndraw(s_pExplosions[i].pBob, pBfr))
			continue;
		if(s_pExplosions[i].uwDuration >= EXPLOSION_DURATION) {
			s_pExplosions[i].pBob->ubFlags = BOB_FLAG_STOP_DRAWING;
			s_pExplosions[i].uwDuration = 0;
		}
		else {
			bobChangeFrame(
				s_pExplosions[i].pBob,
				s_pExplosions[i].uwDuration/EXPLOSION_FRAME_LENGTH
			);
			++s_pExplosions[i].uwDuration;
		}
	}
}

void explosionsDraw(tBitMap *pBfr) {
	for(UWORD i = 0; i != EXPLOSIONS_MAX; ++i) {
		bobDraw(
			s_pExplosions[i].pBob, pBfr,
			s_pExplosions[i].uwX, s_pExplosions[i].uwY
		);
	}
}
