#ifndef GUARD_OF_GAMESTATES_INITLOADING_MENU_H
#define GUARD_OF_GAMESTATES_INITLOADING_MENU_H

#include <ace/config.h>

/**
 * Progress values:
 * - 0: tank base
 * - 1: tank turret
 * - 2: jeep base
 * - 3: brown turret
 */
#define LOADINGSCREEN_BOBSOURCE_COUNT 4

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

void menuDrawProgress(
	IN UWORD uwProgress
);

extern BYTE g_pLoadProgress[LOADINGSCREEN_BOBSOURCE_COUNT];

#endif // GUARD_OF_GAMESTATES_INITLOADING_MENU_H
