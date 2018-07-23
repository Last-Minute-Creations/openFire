#include "gamestates/game/steer_request.h"
#include <ace/managers/key.h>
#include <ace/managers/mouse.h>

// Steer requests
#define OF_KEY_FORWARD      KEY_W
#define OF_KEY_BACKWARD     KEY_S
#define OF_KEY_LEFT         KEY_A
#define OF_KEY_RIGHT        KEY_D
#define OF_KEY_ACTION1      KEY_F
#define OF_KEY_ACTION2      KEY_R
#define OF_KEY_ACTION3      KEY_V

#define STEER_REQUESTS_MAX 50

UBYTE s_ubReadBfr;
volatile tSteerRequest s_pRequests[2][STEER_REQUESTS_MAX];
volatile UBYTE s_pRequestCounts[2];
UBYTE s_ubProcessedCount;

void steerRequestInit(void) {
	s_ubReadBfr = 0;
	s_ubProcessedCount = 0;
	s_pRequestCounts[0] = 0;
	s_pRequestCounts[1] = 0;
}

void steerRequestSwap(void) {
	s_pRequestCounts[s_ubReadBfr] = 0;
	s_ubReadBfr = !s_ubReadBfr;
	s_ubProcessedCount = 0;
}

tSteerRequest *steerRequestGetCurr(void) {
	return &s_pRequests[s_ubReadBfr][s_ubProcessedCount];
}

UBYTE steerRequestIsLast(void) {
	return s_ubProcessedCount >= s_pRequestCounts[s_ubReadBfr]-1;
}

UBYTE steerRequestIsFirst(void) {
	return !s_ubProcessedCount;
}

void steerRequestCapture(void) {
	tSteerRequest * const pReq = &s_pRequests[!s_ubReadBfr][s_pRequestCounts[!s_ubReadBfr]];
	pReq->ubForward  = keyCheck(OF_KEY_FORWARD);
	pReq->ubBackward = keyCheck(OF_KEY_BACKWARD);
	pReq->ubLeft     = keyCheck(OF_KEY_LEFT);
	pReq->ubRight    = keyCheck(OF_KEY_RIGHT);
	pReq->ubAction1  = mouseCheck(MOUSE_PORT_1, MOUSE_LMB);
	pReq->ubAction2  = mouseCheck(MOUSE_PORT_2, MOUSE_RMB);
	pReq->ubAction3  = keyCheck(OF_KEY_ACTION3);
	++s_pRequestCounts[!s_ubReadBfr];
}

UBYTE steerRequestReadCount(void) {
	return s_pRequestCounts[s_ubReadBfr];
}

UBYTE steerRequestProcessedCount(void) {
	return s_ubProcessedCount;
}

void steerRequestMoveToNext(void) {
	++s_ubProcessedCount;
}
