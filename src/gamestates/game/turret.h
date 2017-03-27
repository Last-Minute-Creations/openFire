#ifndef GUARD_OF_GAMESTATES_GAME_TURRET_H
#define GUARD_OF_GAMESTATES_GAME_TURRET_H

#include <ace/config.h>


void turretManagerCreate(void);

void turretManagerDestroy(void);

UWORD turretCreate(UWORD uwX, UWORD uwX, UBYTE ubTeam);

void turretProcess(UWORD uwIdx);

#endif