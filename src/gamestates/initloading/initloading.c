#include "gamestates/initloading/initloading.h"
#include <clib/dos_protos.h>
#include <ace/managers/game.h>
#include <ace/managers/log.h>
#include <ace/managers/viewport/simplebuffer.h>
#include <ace/utils/font.h>
#include <ace/utils/palette.h>
#include "gamestates/initloading/worker.h"
#include "gamestates/initloading/loadingscreen.h"
#include "gamestates/game/game.h"

#define INITLOADING_STATE_OFF  0
#define INITLOADING_STATE_BUSY 1
#define INITLOADING_STATE_DONE 2
#define INITLOADING_STATE_EXIT 3

void gsInitLoadingCreate(void) {
	logBlockBegin("gsInitLoadingCreate()");

	menuCreate();

	logBlockEnd("gsInitLoadingCreate()");
}

void gsInitLoadingLoop(void) {
	static UBYTE ubState = INITLOADING_STATE_OFF;
	static UBYTE ubPrevState = 0;

	switch(ubState) {
		case INITLOADING_STATE_OFF:
			// Create worker
			workerCreate();
			ubState = INITLOADING_STATE_BUSY;
			break;
		case INITLOADING_STATE_BUSY:
			// Query worker status
			menuLoop();
			if(ubPrevState != g_ubWorkerStep) {
				// Update status bar
				// loadingScreenSetProgress(g_ubWorkerStep);
				ubPrevState = g_ubWorkerStep;
			}
			if(g_ubWorkerStep >= WORKER_MAX_STEP)
				ubState = INITLOADING_STATE_DONE;
			break;
		case INITLOADING_STATE_DONE:
			// Stop worker
			workerDestroy();
			// Fadeout
			// TODO
			menuDestroy();
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
	Delay(2);
}

void gsInitLoadingDestroy(void) {
	// Free all things done by worker
	workerCleanup();
}
