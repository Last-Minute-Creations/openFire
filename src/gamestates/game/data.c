#include "gamestates/game/data.h"
#include <ace/managers/mouse.h>
#include "gamestates/game/game.h"
#include "gamestates/game/vehicle.h"
#include "gamestates/game/player.h"
#include "gamestates/game/gamemath.h"

static UBYTE s_pDataBfr[DATA_MAX_PACKET_SIZE];
static UBYTE s_isPacketRead;

void dataTryReadPacket(void) {
	if(0) {
		// TODO packet successfully received
		s_isPacketRead = 1;
	}
}

void dataSend(void) {
	// TODO: dataSend: ???
}

void dataRecv(void) {
	// Packet spoofing
	tDataFrame *pFrame = (tDataFrame*)&s_pDataBfr;
	pFrame->sHeader.uwServerTime = 0;
	pFrame->sHeader.uwSize = 0;
	pFrame->sHeader.uwType = DATA_PACKET_TYPE_SRV_STATE;
	for(UWORD i = 0; i != 8; ++i) {
		// Fill players
		pFrame->pPlayerStates[i].fDx = 0;
		pFrame->pPlayerStates[i].fDy = 0;
		pFrame->pPlayerStates[i].fX = 4 << MAP_TILE_SIZE;
		pFrame->pPlayerStates[i].fY = (7+2*i) << MAP_TILE_SIZE;
		pFrame->pPlayerStates[i].ubBodyAngle = 0;
		pFrame->pPlayerStates[i].ubDestAngle = 0;
		pFrame->pPlayerStates[i].ubTurretAngle = 0;
		pFrame->pPlayerStates[i].ubVehicleState = PLAYER_STATE_DRIVING;
		pFrame->pPlayerStates[i].ubVehicleType = VEHICLE_TYPE_TANK;
	}
	s_isPacketRead = 1;


	if(s_isPacketRead) {
		// TODO: process
		for(UBYTE i = 0; i != 8; ++i) {
			if(&g_pPlayers[i] != g_pLocalPlayer) {
				tPlayer *pPlayer = &g_pPlayers[i];
				pPlayer->sVehicle.fX = pFrame->pPlayerStates[i].fX;
				pPlayer->sVehicle.fY = pFrame->pPlayerStates[i].fY;
				pPlayer->sVehicle.ubBodyAngle = pFrame->pPlayerStates[i].ubBodyAngle;
				pPlayer->sVehicle.ubTurretAngle = pFrame->pPlayerStates[i].ubTurretAngle;
				pPlayer->ubState = pFrame->pPlayerStates[i].ubVehicleState;
			}
		}
		s_isPacketRead = 0;
	}
	else {
		// Prediction
	}

	// Receive player's steer request
	tSteerRequest *pReq = &g_pLocalPlayer->sSteerRequest;
	pReq->ubForward     = keyCheck(OF_KEY_FORWARD);
	pReq->ubBackward    = keyCheck(OF_KEY_BACKWARD);
	pReq->ubLeft        = keyCheck(OF_KEY_LEFT);
	pReq->ubRight       = keyCheck(OF_KEY_RIGHT);
	pReq->ubAction1     = mouseCheck(MOUSE_LMB);
	pReq->ubAction2     = mouseCheck(MOUSE_RMB);
	pReq->ubAction3     = keyCheck(OF_KEY_ACTION3);

	pReq->ubDestAngle = getAngleBetweenPoints(
		g_pLocalPlayer->sVehicle.fX, g_pLocalPlayer->sVehicle.fY,
		g_pWorldCamera->uPos.sUwCoord.uwX + g_uwMouseX,
		g_pWorldCamera->uPos.sUwCoord.uwY + g_uwMouseY
	);
}
