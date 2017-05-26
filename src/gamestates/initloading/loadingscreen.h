#ifndef GUARD_OF_GAMESTATES_INITLOADING_LOADINGSCREEN_H
#define GUARD_OF_GAMESTATES_INITLOADING_LOADINGSCREEN_H

#include <ace/config.h>

/**
 * Progress values:
 * - 0: tank base
 * - 1: tank turret
 * - 2: jeep base
 * - 3: brown turret
 */
#define LOADINGSCREEN_BOBSOURCE_COUNT 4

void loadingScreenCreate(void);

void loadingScreenDestroy(void);

void loadingScreenSetProgress(
	IN UBYTE ubProgress
);

void loadingScreenUpdate(void);

extern BYTE g_pLoadProgress[LOADINGSCREEN_BOBSOURCE_COUNT];

#endif // GUARD_OF_GAMESTATES_INITLOADING_LOADINGSCREEN_H
