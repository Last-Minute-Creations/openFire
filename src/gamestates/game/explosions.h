#ifndef GUARD_OF_GAMESTATES_GAME_EXPLOSIONS_H
#define GUARD_OF_GAMESTATES_GAME_EXPLOSIONS_H

#include <ace/types.h>
#include <ace/utils/bitmap.h>
#include <ace/managers/viewport/simplebuffer.h>

#define EXPLOSIONS_MAX 8

void explosionsAdd(
	const IN UWORD uwX,
	const IN UWORD uwY
);

void explosionsCreate(void);

void explosionsDestroy(void);

void explosionsDraw(
	IN tSimpleBufferManager *pBfr
);

void explosionsUndraw(
	IN tSimpleBufferManager *pBfr
);


void explosionsProcess(void);

#endif // GUARD_OF_GAMESTATES_GAME_EXPLOSIONS_H
