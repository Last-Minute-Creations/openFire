#include "open_fire.h"
#include <ace/generic/main.h>
#include <ace/managers/game.h>

#include "input.h"
#include "gamestates/precalc/precalc.h"

tStateManager *g_pStateManager;

void genericCreate(void) {
	inputOpen();
	g_pStateManager = stateManagerCreate();
	statePush(g_pStateManager, &g_sStatePrecalc);
}

void genericProcess(void) {
	inputProcess();
	stateProcess(g_pStateManager);
}

void genericDestroy(void) {
	inputClose();
	stateManagerDestroy(g_pStateManager);
}
