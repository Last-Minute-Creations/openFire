#ifndef GUARD_OF_GAMESTATES_GAME_CONSOLE_H
#define GUARD_OF_GAMESTATES_GAME_CONSOLE_H

#include <ace/types.h>

void consoleCreate(void);

void consoleDestroy(void);

void consoleWrite(char *szMsg, UBYTE ubColor);

#endif // GUARD_OF_GAMESTATES_GAME_CONSOLE_H
