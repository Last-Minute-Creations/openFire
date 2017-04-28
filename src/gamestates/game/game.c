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
	
	// Select first view
	if(g_pLocalPlayer->ubState == PLAYER_STATE_BUNKERED)
		bunkerShow();
	else
		worldShow();
	logBlockEnd("gsGameCreate()");
}

void gsGameLoop(void) {
	if(keyCheck(KEY_ESCAPE)) {
		gamePopState(); // Pop to threaded loader so it can free stuff
		return;
	}

	custom.color[0] = 0x0080;
	dataRecv();
	custom.color[0] = 0x300;
	simPlayers();
	custom.color[0] = 0x600;
	simProjectiles();
	custom.color[0] = 0x900;
	simTurrets();
	custom.color[0] = 0x0008;
	dataSend();
	custom.color[0] = 0x0000;

	if(
		g_pLocalPlayer->ubState == PLAYER_STATE_BUNKERED ||
		g_pLocalPlayer->ubState == PLAYER_STATE_SURFACING
	) {
		custom.color[0] = 0x0880;
		bunkerProcess();
	}
	else {
		custom.color[0] = 0x0088;
		worldProcess();
	}
	custom.color[0] = 0x0000;
}

void gsGameDestroy(void) {
	logBlockBegin("gsGameDestroy()");

	bunkerDestroy();
	worldDestroy();
	mapDestroy();
	
	playerListDestroy();

	logBlockEnd("gsGameDestroy()");
}
