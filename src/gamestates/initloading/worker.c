#include "gamestates/initloading/worker.h"
#include <exec/tasks.h>
#include <clib/dos_protos.h>
#include <clib/alib_protos.h>
#include <ace/config.h>
#include <ace/managers/log.h>

#include <gamestates/game/vehicle.h>
#include <gamestates/game/projectile.h>

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
UWORD g_uwWorkerStep;

inline void workerDoStuff(void (*fn)(void)) {
	if(fn)
		fn();
	if(s_uwWorkerRequest == WORKER_REQUEST_KILL) {
		// TODO
	}
}

void workerMain(void) {

	// Do stuff, occasionally checking if it should kill itself
	logWrite("Working on vehicles...\n");
	vehicleTypesCreate();
	logWrite("Working on projectiles...\n");
	projectileListCreate(20);
	++g_uwWorkerStep;
}

const tSeg s_sFakeSeg = {sizeof(tSeg), 0, 0x4EF9, workerMain};

UWORD workerCreate(void) {
	s_uwWorkerRequest = WORKER_REQUEST_WORK;
	g_uwWorkerStep = 0;
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

void workerCleanup(void) {
	projectileListDestroy();
	vehicleTypesDestroy();
}
