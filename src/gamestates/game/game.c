#include "gamestates/game/game.h"
#include <math.h>
#include <hardware/intbits.h> // INTB_COPER
#include <ace/managers/copper.h>
#include <ace/managers/blit.h>
#include <ace/managers/viewport/simplebuffer.h>
#include <ace/managers/key.h>
#include <ace/managers/game.h>
#include <ace/managers/rand.h>
#include <ace/utils/extview.h>
#include <ace/utils/palette.h>
#include "gamestates/game/map.h"
#include "gamestates/game/vehicle.h"
#include "gamestates/game/world.h"
#include "gamestates/game/bunker.h"
#include "gamestates/game/player.h"
#include "gamestates/game/team.h"
#include "gamestates/game/projectile.h"
#include "gamestates/game/hud.h"

tView *g_pGameView;
tVPort *g_pGameMainVPort;
tSimpleBufferManager *g_pGameBfr;
tCameraManager *g_pCamera;
tBitMap *s_pTiles;
tBob *g_pSiloHighlight;

void gsGameCreate(void) {
	UBYTE i;
	
	logBlockBegin("gsGameCreate()");
	randInit(2184);
	teamsInit();
	
	// Prepare view & viewport
	g_pGameView = viewCreate(V_GLOBAL_CLUT);
	g_pGameMainVPort = vPortCreate(g_pGameView, WINDOW_SCREEN_WIDTH, WINDOW_SCREEN_HEIGHT-64-1, GAME_BPP, 0);
	g_pGameBfr = simpleBufferCreate(g_pGameMainVPort, 20<<MAP_TILE_SIZE, 20<<MAP_TILE_SIZE, 0);
	hudCreate();
	if(!g_pGameBfr) {
		logWrite("Buffer creation failed");
		gameClose();
		return;
	}
	g_pCamera = g_pGameBfr->pCameraManager;
	logWrite("Allocated buffer\n");
	bitmapDump(g_pGameBfr->pBuffer);
	paletteLoad("data/amidb32.plt", g_pGameMainVPort->pPalette, 32);
	
	// Load gfx
	s_pTiles = bitmapCreateFromFile("data/tiles.bm");
	g_pSiloHighlight = bobUniqueCreate("data/silohighlight.bm", "data/silohighlight.msk", 0, 0);
	
	// Load player
	playerListCreate(1);
	g_pLocalPlayer = playerAdd("player", TEAM_GREEN);
	
	// Load map
	mapCreate(g_pGameMainVPort, s_pTiles, "data/maps/test.txt");
	
	// Prepare bunker gfx
	bunkerCreate();
	
	// Load objs
	vehicleTypesCreate();
	projectileListCreate(20);

	// Display view with its viewports
	viewLoad(g_pGameView);
	logBlockEnd("gsGameCreate()");
}

void gsGameLoop(void) {
	bunkerShow();
}

void gsGameDestroy(void) {
	bunkerDestroy();
	projectileListDestroy();
	vehicleTypesDestroy();
	viewDestroy(g_pGameView);
	bobUniqueDestroy(g_pSiloHighlight);
	bitmapDestroy(s_pTiles);
	mapDestroy();
	
	playerListDestroy();
}