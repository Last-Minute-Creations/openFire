#include "gamestates/initloading/worker.h"
#include <exec/tasks.h>
#include <clib/dos_protos.h>
#include <clib/alib_protos.h>
#include <ace/config.h>
#include <ace/managers/log.h>

#include "vehicletypes.h"
#include "gamestates/initloading/menu.h"
#include "gamestates/game/projectile.h"
#include "gamestates/game/turret.h"
#include "gamestates/game/gamemath.h"

#define WORKER_REQUEST_WORK 0
#define WORKER_REQUEST_KILL 8
#define WORKER_STACK_SIZE 32000L

typedef struct _tSeg {
	ULONG ulSegSize;   ///< sizeof(struct tCodeHeader)
	ULONG ulNextSeg;   ///< Must be NULL
	UWORD uwJumpInstr; ///< Set to 0x4EF9 (a jump instruction)
	void (*pFn)(void); ///< Pointer to the function
} tSeg;

UWORD s_uwWorkerRequest; ///< Worker reads this and does stuff.
UBYTE g_ubWorkerStep;

/**
 * Allocates & calculates stuff for rest of game.
 */
void workerMain(void) {
	// Vehicle stuff
	logWrite("Working on vehicles...\n");
	vehicleTypesCreate(g_pLoadProgress);

	// Turret stuff
	logWrite("Loading brown turret frames...\n");
	vehicleTypeBobSourceLoad("turret_brown_16", &g_sBrownTurretSource, 0, &g_pLoadProgress[3]);

	// Generate math table
	logWrite("Generating sine table...\n");
	generateSine();
	g_ubWorkerStep = 50;

	logWrite("Working on projectiles...\n");
	projectileListCreate(20);
	g_ubWorkerStep = 100;
}

const tSeg s_sFakeSeg = {sizeof(tSeg), 0, 0x4EF9, workerMain};

UWORD workerCreate(void) {
	// Start worker
	s_uwWorkerRequest = WORKER_REQUEST_WORK;
	g_ubWorkerStep = 0;
	struct MsgPort *pResult;

	pResult = CreateProc(
		"OpenFire init proc", 0,
		MKBADDR(&s_sFakeSeg.ulNextSeg),
		WORKER_STACK_SIZE
	);

	if(!pResult) {
		logWrite("Failed to create worker!\n");
		return 0;
	}
	return 1;
}

void workerDestroy(void) {
	logBlockBegin("workerDestroy()");
	s_uwWorkerRequest = WORKER_REQUEST_KILL;
	logBlockEnd("workerDestroy()");
}

/**
 * Cleans up allocated stuff during loading.
 */
void workerCleanup(void) {
	projectileListDestroy();
	vehicleTypesDestroy();
	bitmapDestroy(g_sBrownTurretSource.pBitmap);
	bitmapMaskDestroy(g_sBrownTurretSource.pMask);
}
