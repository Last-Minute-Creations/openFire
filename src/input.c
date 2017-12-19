#include "input.h"
#include <ace/managers/mouse.h>
#include <ace/managers/joy.h>

/* Globals */

/* Functions */
void inputOpen() {
	mouseCreate(MOUSE_PORT_1);
	joyOpen();
	keyCreate();
}

void inputProcess() {
	ULONG ulWindowSignal;
	ULONG ulSignals;
	struct IntuiMessage *pMsg;

	ulWindowSignal = 1L << g_sWindowManager.pWindow->UserPort->mp_SigBit;
	ulSignals = SetSignal(0L, 0L);

	joyProcess();
	mouseProcess();

	if (!(ulSignals & ulWindowSignal))
		return;
	while (pMsg = (struct IntuiMessage *) GetMsg(g_sWindowManager.pWindow->UserPort)) {
		ReplyMsg((struct Message *) pMsg);
	}
}

void inputClose() {
	mouseDestroy();
	keyDestroy();
	joyClose();
}
