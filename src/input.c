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
	joyProcess();
	mouseProcess();
	keyProcess();
}

void inputClose() {
	mouseDestroy();
	keyDestroy();
	joyClose();
}
