#include "gamestates/game/game.h"
#include <math.h>
#include <hardware/intbits.h> // INTB_COPER
#include <ace/managers/copper.h>
#include <ace/managers/blit.h>
#include <ace/managers/viewport/simplebuffer.h>
#include <ace/managers/key.h>
#include <ace/managers/game.h>
#include <ace/managers/rand.h>
#include "gamestates/game/map.h"
#include "gamestates/game/vehicle.h"
#include "gamestates/game/world.h"
#include "gamestates/game/bunker.h"
#include "gamestates/game/player.h"
#include "gamestates/game/team.h"
#include "gamestates/game/projectile.h"
#include "gamestates/game/data.h"
#include "gamestates/game/sim.h"

UBYTE g_ubActiveState;

void gsGameCreate(void) {
	UBYTE i;
	
	logBlockBegin("gsGameCreate()");
	randInit(2184);

	// Setup teams
	teamsInit();

	// Load map
	mapCreate("data/maps/test.txt");

	// Add players
	playerListCreate(1);
	g_pLocalPlayer = playerAdd("player", TEAM_GREEN);
	
	// Create everything needed to display world view
	worldCreate();
	
	// Prepare bunker gfx
	bunkerCreate();
	
	// Start game in bunker
	bunkerShow();
	logBlockEnd("gsGameCreate()");
}

void gsGameLoop(void) {
	dataRecv();

	simVehicles();
	simProjectiles();
	simTurrets();

	dataSend();

	if(g_ubActiveState == ACTIVESTATE_BUNKER)
		bunkerProcess();
	else
		worldProcess();
}

void gsGameDestroy(void) {
	bunkerDestroy();
	worldDestroy();
	mapDestroy();
	
	playerListDestroy();
}
