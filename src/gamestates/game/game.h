#ifndef GUARD_OF_GAMESTATES_GAME_GAME_H
#define GUARD_OF_GAMESTATES_GAME_GAME_H

#include <ace/config.h>
#include <ace/managers/viewport/simplebuffer.h>
#include <ace/managers/key.h>
#include "gamestates/game/bob.h"

#define GAME_BPP 5

#define OF_KEY_FORWARD      KEY_W
#define OF_KEY_BACKWARD     KEY_S
#define OF_KEY_LEFT         KEY_A
#define OF_KEY_RIGHT        KEY_D
#define OF_KEY_TURRET_LEFT  KEY_Q
#define OF_KEY_TURRET_RIGHT KEY_E
#define OF_KEY_ACTION1      KEY_F
#define OF_KEY_ACTION2      KEY_R
#define OF_KEY_ACTION3      KEY_V

#define ACTIVESTATE_BUNKER 0
#define ACTIVESTATE_WORLD 1

extern UBYTE g_ubActiveState;

void gsGameCreate(void);
void gsGameLoop(void);
void gsGameDestroy(void);

#endif
