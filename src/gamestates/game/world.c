#include "gamestates/game/world.h"
#include <ace/managers/key.h>
#include <ace/managers/game.h>
#include <ace/managers/blit.h>
#include <ace/managers/viewport/camera.h>
#include <ace/managers/copper.h>
#include <ace/utils/bitmap.h>
#include <ace/utils/palette.h>
#include "gamestates/game/game.h"
#include "gamestates/game/bunker.h"
#include "gamestates/game/vehicle.h"
#include "gamestates/game/player.h"
#include "gamestates/game/map.h"
#include "gamestates/game/projectile.h"
#include "gamestates/game/hud.h"

tView *g_pWorldView;
tVPort *s_pWorldMainVPort;
tSimpleBufferManager *g_pWorldMainBfr;
tCameraManager *s_pWorldCamera;
tBitMap *s_pTiles;
tBob *s_pSiloHighlight;
UBYTE s_ubIsSiloHighlighted;

UBYTE worldCreate(void) {
	// Prepare view & viewport
	g_pWorldView = viewCreate(V_GLOBAL_CLUT);
	s_pWorldMainVPort = vPortCreate(g_pWorldView, WINDOW_SCREEN_WIDTH, WINDOW_SCREEN_HEIGHT-64-1, GAME_BPP, 0);
	g_pWorldMainBfr = simpleBufferCreate(s_pWorldMainVPort, 20<<MAP_TILE_SIZE, 20<<MAP_TILE_SIZE, 0);
	if(!g_pWorldMainBfr) {
		logWrite("Buffer creation failed");
		gamePopState();
		return 0;
	}
	paletteLoad("data/amidb32.plt", s_pWorldMainVPort->pPalette, 32);
	s_pWorldCamera = g_pWorldMainBfr->pCameraManager;

	hudCreate();

	// Load gfx
	s_pTiles = bitmapCreateFromFile("data/tiles.bm");
	s_pSiloHighlight = bobUniqueCreate("data/silohighlight.bm", "data/silohighlight.msk", 0, 0);

	// Load map
	mapCreate(s_pWorldMainVPort, s_pTiles, "data/maps/test.txt");

	return 1;
}

void worldDestroy(void) {
	viewDestroy(g_pWorldView);
	bobUniqueDestroy(s_pSiloHighlight);
	bitmapDestroy(s_pTiles);
}

void worldShow(void) {
	viewLoad(g_pWorldView);
	g_ubActiveState = ACTIVESTATE_WORLD;
}

void worldHide(void) {
	bunkerShow();
}

void worldProcess(void) {
	UWORD uwLocalX, uwLocalY, uwLocalTileX, uwLocalTileY;
	
	// Player input
	if(!worldProcessInput())
		return;
	
	uwLocalX = g_pLocalPlayer->pCurrentVehicle->fX;
	uwLocalY = g_pLocalPlayer->pCurrentVehicle->fY;
	uwLocalTileX = uwLocalX >> MAP_TILE_SIZE;
	uwLocalTileY = uwLocalY >> MAP_TILE_SIZE;
	
	// Update camera
	cameraCenterAt(s_pWorldCamera, uwLocalX, uwLocalY);
	
	// Draw objects on map
	if(g_pMap[uwLocalTileX][uwLocalTileY].ubIdx == MAP_LOGIC_SPAWN1) {
		UWORD uwSiloDiffX, uwSiloDiffY;
		
		uwSiloDiffX = uwLocalX - (uwLocalTileX << MAP_TILE_SIZE);
		uwSiloDiffY = uwLocalY - (uwLocalTileY << MAP_TILE_SIZE);
		if(uwSiloDiffX > 12 && uwSiloDiffX < 18 && uwSiloDiffY > 12 && uwSiloDiffY < 18)
			s_ubIsSiloHighlighted = 1;
		else
			s_ubIsSiloHighlighted = 0;
	}
	else
		s_ubIsSiloHighlighted = 0;
	if(s_ubIsSiloHighlighted)
		bobDraw(
			s_pSiloHighlight, g_pWorldMainBfr->pBuffer,
			uwLocalTileX << MAP_TILE_SIZE, uwLocalTileY << MAP_TILE_SIZE
		);
	else if(g_pMap[uwLocalTileX][uwLocalTileY].ubIdx == MAP_LOGIC_WATER) {
		playerLoseVehicle(g_pLocalPlayer);
		bunkerShow();
		return;
	}
	projectileProcess();

	if(g_pLocalPlayer->pCurrentVehicle)
		vehicleDraw(g_pLocalPlayer->pCurrentVehicle);
	projectileDraw();
	
	viewProcessManagers(g_pWorldView);
	copProcessBlocks();
	vPortWaitForEnd(s_pWorldMainVPort);
		
	// Undraw objects
	projectileUndraw();
	if(g_pLocalPlayer->pCurrentVehicle) {
		vehicleUndraw(g_pLocalPlayer->pCurrentVehicle);
		if(s_ubIsSiloHighlighted)
			bobUndraw(s_pSiloHighlight, g_pWorldMainBfr->pBuffer);
	}
}

UBYTE worldProcessInput(void) {
	tSteerRequest sReq;
	
	if (keyUse(KEY_ESCAPE)) {
		gamePopState();
		return 0;
	}

	if(keyUse(KEY_C))
		bitmapSaveBmp(g_pWorldMainBfr->pBuffer, s_pWorldMainVPort->pPalette, "bufDump.bmp");
	
	sReq.ubForward     = keyCheck(OF_KEY_FORWARD);
	sReq.ubBackward    = keyCheck(OF_KEY_BACKWARD);
	sReq.ubLeft        = keyCheck(OF_KEY_LEFT);
	sReq.ubRight       = keyCheck(OF_KEY_RIGHT);
	sReq.ubTurretLeft  = keyCheck(OF_KEY_TURRET_LEFT);
	sReq.ubTurretRight = keyCheck(OF_KEY_TURRET_RIGHT);
	sReq.ubAction1     = keyCheck(OF_KEY_ACTION1);
	sReq.ubAction2     = keyCheck(OF_KEY_ACTION2);
	sReq.ubAction3     = keyCheck(OF_KEY_ACTION3);
	
	if(sReq.ubAction1 && s_ubIsSiloHighlighted) {
		playerHideInBunker(g_pLocalPlayer);
		bunkerShow();
		return 0;
	}

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
}
