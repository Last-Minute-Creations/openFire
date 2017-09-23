#ifndef GUARD_OF_GAMESTATES_GAME_CURSOR_H
#define GUARD_OF_GAMESTATES_GAME_CURSOR_H

#include <ace/types.h>

void cursorCreate(void);

void cursorDestroy(void);

void cursorSetConstraints(
	IN UWORD uwXLo,
	IN UWORD uwYLo,
	IN UWORD uwXHi,
	IN UWORD uwYHi
);

void cursorUpdate(void);

extern UWORD g_uwMouseX, g_uwMouseY; ///< Cursor position, read only

#endif // GUARD_OF_GAMESTATES_GAME_CURSOR_H
