#include "gamestates/game/world.h"
#include <ace/managers/key.h>
#include <ace/managers/game.h>
#include <ace/managers/blit.h>
#include <ace/managers/viewport/camera.h>
#include <ace/managers/copper.h>
#include <ace/utils/bitmap.h>
#include "gamestates/game/game.h"
#include "gamestates/game/bunker.h"
#include "gamestates/game/vehicle.h"
#include "gamestates/game/player.h"
#include "gamestates/game/map.h"
#include "gamestates/game/projectile.h"

UBYTE g_ubSiloHighlighted;

UBYTE worldProcessInput(void) {
	tSteerRequest sReq;
	
	logWrite("processInput esc\n");
	if (keyUse(KEY_ESCAPE)) {
		gameClose();
		return 0;
	}
	logWrite("processInput c/z\n");
	if(keyUse(KEY_C))
		bitmapSaveBMP(g_pGameBfr->pBuffer, g_pGameMainVPort->pPalette, "bufDump.bmp");
	if(keyUse(KEY_Z))
		copDumpBlocks();
	
	logWrite("keyChecks\n");
	sReq.ubForward     = keyCheck(OF_KEY_FORWARD);
	sReq.ubBackward    = keyCheck(OF_KEY_BACKWARD);
	sReq.ubLeft        = keyCheck(OF_KEY_LEFT);
	sReq.ubRight       = keyCheck(OF_KEY_RIGHT);
	sReq.ubTurretLeft  = keyCheck(OF_KEY_TURRET_LEFT);
	sReq.ubTurretRight = keyCheck(OF_KEY_TURRET_RIGHT);
	sReq.ubAction1     = keyUse(OF_KEY_ACTION1);
	sReq.ubAction2     = keyCheck(OF_KEY_ACTION2);
	sReq.ubAction3     = keyCheck(OF_KEY_ACTION3);
	
	logWrite("actions\n");
	if(sReq.ubAction1 && g_ubSiloHighlighted) {
		playerHideInBunker(g_pLocalPlayer);
		bunkerShow();
		return 0;
	}
	logWrite("processInput steers\n");
	switch(g_pLocalPlayer->ubCurrentVehicleType) {
		case VEHICLE_TYPE_TANK:
			vehicleSteerTank(g_pLocalPlayer->pCurrentVehicle, &sReq);
			break;
		case VEHICLE_TYPE_JEEP:
			vehicleSteerJeep(g_pLocalPlayer->pCurrentVehicle, &sReq);
			break;
	}
	return 1;
}

/**
 *  @todo Optimization by non-masked silo highlight (alternative bunker tile)?
 */
void worldGameLoop(void) {
	UWORD uwLocalX, uwLocalY, uwLocalTileX, uwLocalTileY;
	
	// Player input
	if(!worldProcessInput())
		return;
	
	uwLocalX = g_pLocalPlayer->pCurrentVehicle->fX;
	uwLocalY = g_pLocalPlayer->pCurrentVehicle->fY;
	uwLocalTileX = uwLocalX >> MAP_TILE_SIZE;
	uwLocalTileY = uwLocalY >> MAP_TILE_SIZE;
	
	// Update camera
	cameraCenterAt(g_pCamera, uwLocalX, uwLocalY);
	
	// Draw objects on map
	if(g_pMap[uwLocalTileX][uwLocalTileY] == MAP_LOGIC_SPAWN1) {
		UWORD uwSiloDiffX, uwSiloDiffY;
		
		uwSiloDiffX = uwLocalX - (uwLocalTileX << MAP_TILE_SIZE);
		uwSiloDiffY = uwLocalY - (uwLocalTileY << MAP_TILE_SIZE);
		if(uwSiloDiffX > 12 && uwSiloDiffX < 18 && uwSiloDiffY > 12 && uwSiloDiffY < 18)
			g_ubSiloHighlighted = 1;
		else
			g_ubSiloHighlighted = 0;
	}
	else
		g_ubSiloHighlighted = 0;
	if(g_ubSiloHighlighted)
		bobDraw(
			g_pSiloHighlight, g_pGameBfr->pBuffer,
			uwLocalTileX << MAP_TILE_SIZE, uwLocalTileY << MAP_TILE_SIZE
		);
	else if(g_pMap[uwLocalTileX][uwLocalTileY] == MAP_LOGIC_WATER) {
		playerLoseVehicle(g_pLocalPlayer);
		bunkerShow();
		return;
	}
	projectileProcess();

	if(g_pLocalPlayer->pCurrentVehicle)
		vehicleDraw(g_pLocalPlayer->pCurrentVehicle);
	projectileDraw();
	
	viewProcessManagers(g_pGameView);
	copProcessBlocks();
	vPortWaitForEnd(g_pGameMainVPort);
		
	// Undraw objects
	projectileUndraw();
	if(g_pLocalPlayer->pCurrentVehicle)
		vehicleUndraw(g_pLocalPlayer->pCurrentVehicle);
		if(g_ubSiloHighlighted)
			bobUndraw(g_pSiloHighlight, g_pGameBfr->pBuffer);
}