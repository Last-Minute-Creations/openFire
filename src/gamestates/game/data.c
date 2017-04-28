#include "gamestates/game/data.h"
#include "gamestates/game/game.h"
#include "gamestates/game/vehicle.h"
#include "gamestates/game/player.h"

void dataSend(void) {
	// TODO: dataSend: ???
}

void dataRecv(void) {
	// TODO: dataRecv: ???

	
	if(g_pLocalPlayer->ubState == PLAYER_STATE_DRIVING) {
		// Receive player's steer request
		tSteerRequest *pReq = &g_pLocalPlayer->sSteerRequest;
		pReq->ubForward     = keyCheck(OF_KEY_FORWARD);
		pReq->ubBackward    = keyCheck(OF_KEY_BACKWARD);
		pReq->ubLeft        = keyCheck(OF_KEY_LEFT);
		pReq->ubRight       = keyCheck(OF_KEY_RIGHT);
		pReq->ubTurretLeft  = keyCheck(OF_KEY_TURRET_LEFT);
		pReq->ubTurretRight = keyCheck(OF_KEY_TURRET_RIGHT);
		pReq->ubAction1     = keyCheck(OF_KEY_ACTION1);
		pReq->ubAction2     = keyCheck(OF_KEY_ACTION2);
		pReq->ubAction3     = keyCheck(OF_KEY_ACTION3);
	}
	if(1) {
		// Assume that other vehicles have same steer requests as before
		// TODO: something better
	}
}
