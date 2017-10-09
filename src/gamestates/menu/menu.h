#ifndef GUARD_OF_GAMESTATES_MENU_MENU_H
#define GUARD_OF_GAMESTATES_MENU_MENU_H

#include <ace/config.h>

void menuCreate(void);

void menuLoop(void);

void menuDestroy(void);

void menuDrawButton(
	IN UWORD uwX,
	IN UWORD uwY,
	IN UWORD uwWidth,
	IN UWORD uwHeight,
	IN char *szText,
	IN UBYTE isSelected
);

#endif // GUARD_OF_GAMESTATES_MENU_MENU_H
