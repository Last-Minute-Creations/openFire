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

static tExplosion s_pExplosions[EXPLOSIONS_MAX];
static tBitMap *s_pBitmap;
static tBitMap *s_pMask;

void explosionsAdd(const IN UWORD uwX, const IN UWORD uwY) {
	// Find free explosion slot
	for(UWORD i = EXPLOSIONS_MAX; i--;) {
		if(s_pExplosions[i].pBob->ubState < BOB_STATE_START_DRAWING) {
			// Free slot found - setup explosion
			s_pExplosions[i].uwX = uwX;
			s_pExplosions[i].uwY = uwY;
			s_pExplosions[i].pBob->ubState = BOB_STATE_START_DRAWING;
			s_pExplosions[i].pBob->isDrawn = 0;
			s_pExplosions[i].uwDuration = 0;
			return;
		}
	}
}

void explosionsCreate(void) {
	logBlockBegin("explosionsCreate()");
	s_pBitmap = bitmapCreateFromFile("data/explosion.bm");
	s_pMask = bitmapCreateFromFile("data/explosion_mask.bm");

	for(UWORD i = EXPLOSIONS_MAX; i--;) {
		s_pExplosions[i].pBob = bobCreate(s_pBitmap, s_pMask, 0, EXPLOSION_SIZE, 0);
		s_pExplosions[i].pBob->ubState = BOB_STATE_NODRAW;
		s_pExplosions[i].uwDuration = 0;
	}
	logBlockEnd("explosionsCreate()");
}

void explosionsDestroy(void) {
	logBlockBegin("explosionsDestroy()");
	for(UWORD i = EXPLOSIONS_MAX; i--;)
		bobDestroy(s_pExplosions[i].pBob);

	bitmapDestroy(s_pBitmap);
	bitmapDestroy(s_pMask);
	logBlockEnd("explosionsDestroy()");
}

void explosionsUndraw(tSimpleBufferManager *pBfr) {
	for(UWORD i = EXPLOSIONS_MAX; i--;) {
		++s_pExplosions[i].uwDuration;
		if(!bobUndraw(s_pExplosions[i].pBob, pBfr))
			continue;
		if(s_pExplosions[i].uwDuration >= EXPLOSION_DURATION) {
			s_pExplosions[i].pBob->ubState = BOB_STATE_STOP_DRAWING;
		}
		else {
			bobChangeFrame(
				s_pExplosions[i].pBob,
				s_pExplosions[i].uwDuration/EXPLOSION_FRAME_LENGTH
			);
		}
	}
}

void explosionsDraw(tSimpleBufferManager *pBfr) {
	for(UWORD i = 0; i != EXPLOSIONS_MAX; ++i) {
		bobDraw(
			s_pExplosions[i].pBob, pBfr,
			s_pExplosions[i].uwX, s_pExplosions[i].uwY,
			0, 32
		);
	}
}
