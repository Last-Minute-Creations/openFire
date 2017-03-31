#ifndef GUARD_OF_GAMESTATES_INITLOADING_LOADINGSCREEN_H
#define GUARD_OF_GAMESTATES_INITLOADING_LOADINGSCREEN_H

#include <ace/config.h>

void loadingScreenCreate(void);

void loadingScreenDestroy(void);

void loadingScreenSetProgress(
	IN UBYTE ubProgress
);

void loadingScreenUpdate(void);

#endif // GUARD_OF_GAMESTATES_INITLOADING_LOADINGSCREEN_H
