#ifndef GUARD_OF_GAMESTATES_GAME_CONSOLE_H
#define GUARD_OF_GAMESTATES_GAME_CONSOLE_H

#include <ace/types.h>

#define CONSOLE_COLOR_GENERAL 4
#define CONSOLE_COLOR_SAY     13
#define CONSOLE_COLOR_BLUE    12
#define CONSOLE_COLOR_RED     10

void consoleCreate(void);

void consoleDestroy(void);

void consoleWrite(char *szMsg, UBYTE ubColor);

void consoleChatBegin(void);

void consoleChatEnd(void);

/**
 * Processes given character or keypress in chat entry.
 * @param c Character to be processed.
 * @return 0 if keypress resulted in end of chat edit, otherwise 1.
 */
FUBYTE consoleChatProcessChar(char c);

extern FUBYTE g_isChatting;

#endif // GUARD_OF_GAMESTATES_GAME_CONSOLE_H
