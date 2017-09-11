#ifndef GUARD_OF_GAMESTATES_GAME_EXPLOSIONS_H
#define GUARD_OF_GAMESTATES_GAME_EXPLOSIONS_H

#include <ace/types.h>
#include <ace/utils/bitmap.h>

void explosionsAdd(
	const IN UWORD uwX,
	const IN UWORD uwY
);

void explosionsCreate(void);

void explosionsDestroy(void);

void explosionsDraw(
	IN tBitMap *pBfr
);

void explosionsUndraw(
	IN tBitMap *pBfr
);


void explosionsProcess(void);

#endif // GUARD_OF_GAMESTATES_GAME_EXPLOSIONS_H
