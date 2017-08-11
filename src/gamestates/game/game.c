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

#include "gamestates/game/turret.h"

void gsGameCreate(void) {
	UBYTE i;
	
	logBlockBegin("gsGameCreate()");
	randInit(2184);

	// Setup teams
	teamsInit();

	// Load map
	mapCreate("data/maps/turretsFull.txt");

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

	dataRecv();       // Receives steer requests & positions of other players
	simPlayers();     // Simulates players: vehicle positions, death states, etc.
	simTurrets();     // Simulates turrets: targeting, rotation & projectile spawn
	simProjectiles(); // Simulates projectiles: new positions, damage
	dataSend();       // Sends data to other players

	if(
		g_pLocalPlayer->ubState == PLAYER_STATE_BUNKERED ||
		g_pLocalPlayer->ubState == PLAYER_STATE_SURFACING
	) {
		bunkerProcess(); // Process bunker view: decisions on steer request, 
	}
	else {
		// Undraw projectiles
		// Undraw vehicles
		// Undraw silo
		// Update camera
		// Draw silo
		// Draw vehicles
		// Draw projectiles
		worldProcess();
	}
}

void gsGameDestroy(void) {
	logBlockBegin("gsGameDestroy()");

	bunkerDestroy();
	worldDestroy();
	mapDestroy();
	
	playerListDestroy();

	logBlockEnd("gsGameDestroy()");
}
