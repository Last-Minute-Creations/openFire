#include "gamestates/game/data.h"
#include <ace/managers/mouse.h>
#include "gamestates/game/game.h"
#include "gamestates/game/vehicle.h"
#include "gamestates/game/player.h"
#include "gamestates/game/world.h"
#include "gamestates/game/gamemath.h"


void dataSend(void) {
	// TODO: dataSend: ???
}

void dataRecv(void) {
	// TODO: dataRecv: ???

	// Receive player's steer request
	tSteerRequest *pReq = &g_pLocalPlayer->sSteerRequest;
	pReq->ubForward     = keyCheck(OF_KEY_FORWARD);
	pReq->ubBackward    = keyCheck(OF_KEY_BACKWARD);
	pReq->ubLeft        = keyCheck(OF_KEY_LEFT);
	pReq->ubRight       = keyCheck(OF_KEY_RIGHT);
	pReq->ubAction1     = mouseCheck(MOUSE_LMB);
	pReq->ubAction2     = mouseCheck(MOUSE_RMB);
	pReq->ubAction3     = keyCheck(OF_KEY_ACTION3);

	// Destination angle from mouse
	g_uwMouseX = mouseGetX();
	g_uwMouseY = mouseGetY();
	if(g_uwMouseY >= WORLD_VPORT_HEIGHT) {
		mouseMove(0, WORLD_VPORT_HEIGHT - g_uwMouseY - 1);
		g_uwMouseY = WORLD_VPORT_HEIGHT-1;
	}

	pReq->ubDestAngle = getAngleBetweenPoints(
		g_pLocalPlayer->sVehicle.fX, g_pLocalPlayer->sVehicle.fY,
		g_pWorldCamera->uPos.sUwCoord.uwX + g_uwMouseX,
		g_pWorldCamera->uPos.sUwCoord.uwY + g_uwMouseY
	);

	if(1) {
		// Assume that other vehicles have same steer requests as before
		// TODO: better prediction?
	}
}
