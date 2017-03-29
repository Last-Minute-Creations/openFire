#include "gamestates/initloading/initloading.h"
#include <ace/managers/game.h>
#include <ace/managers/log.h>
#include "gamestates/initloading/worker.h"
#include "gamestates/game/game.h"

#define INITLOADING_STATE_OFF  0
#define INITLOADING_STATE_BUSY 1
#define INITLOADING_STATE_DONE 2
#define INITLOADING_STATE_EXIT 3

void gsInitLoadingCreate(void) {
	logBlockBegin("gsInitLoadingCreate()");

	// Draw some loading screen
	// TODO

	logBlockEnd("gsInitLoadingCreate()");
}

void gsInitLoadingLoop(void) {
	static UBYTE ubState = INITLOADING_STATE_OFF;

	switch(ubState) {
		case INITLOADING_STATE_OFF:
			// Create worker
			workerCreate();
			ubState = INITLOADING_STATE_BUSY;
			break;
		case INITLOADING_STATE_BUSY:
			// Query worker status
			// TODO

			// Update status bar
			// TODO
			if(g_uwWorkerStep >= WORKER_MAX_STEP)
				ubState = INITLOADING_STATE_DONE;
			break;
		case INITLOADING_STATE_DONE:
			// Stop worker
			workerDestroy();
			// Fadeout
			// TODO
			// Move to proper gamestate
			gamePushState(gsGameCreate, gsGameLoop, gsGameDestroy);
			ubState = INITLOADING_STATE_EXIT;
			break;
		case INITLOADING_STATE_EXIT:
		default:
			// Close game
			gameClose();
			break;
	}
}

void gsInitLoadingDestroy(void) {
	// Free all things done by worker
	workerCleanup();
}
