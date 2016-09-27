#ifndef GUARD_OF_GAMESTATES_GAME_BUNKER_H
#define GUARD_OF_GAMESTATES_GAME_BUNKER_H

#include <ace/config.h>
#include "gamestates/game/bob.h"

typedef struct {
	tBob *pBob;
	UWORD uwX;
	UWORD uwY;
} tBunkerVehicle;

void bunkerCreate(void);

void bunkerProcess(void);

void bunkerShow(void);

void bunkerHide(void);

void bunkerDestroy(void);

#endif