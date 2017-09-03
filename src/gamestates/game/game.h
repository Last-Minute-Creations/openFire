#ifndef GUARD_OF_GAMESTATES_GAME_GAME_H
#define GUARD_OF_GAMESTATES_GAME_GAME_H

#include <ace/config.h>
#include <ace/managers/viewport/simplebuffer.h>
#include <ace/managers/key.h>
#include "gamestates/game/bob.h"


#define OF_KEY_FORWARD      KEY_W
#define OF_KEY_BACKWARD     KEY_S
#define OF_KEY_LEFT         KEY_A
#define OF_KEY_RIGHT        KEY_D
#define OF_KEY_ACTION1      KEY_F
#define OF_KEY_ACTION2      KEY_R
#define OF_KEY_ACTION3      KEY_V

extern UBYTE g_ubActiveState;
extern UWORD g_uwMouseX, g_uwMouseY;

void gsGameCreate(void);
void gsGameLoop(void);
void gsGameDestroy(void);

#endif
