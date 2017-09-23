#include "gamestates/game/game.h"
#include <math.h>
#include <hardware/intbits.h> // INTB_COPER
#include <ace/macros.h>
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
#include "gamestates/game/player.h"
#include "gamestates/game/team.h"
#include "gamestates/game/projectile.h"
#include "gamestates/game/data.h"
#include "gamestates/game/hud.h"
#include "gamestates/game/cursor.h"
#include "gamestates/game/turret.h"
#include "gamestates/game/spawn.h"

// Viewport stuff
tView *g_pWorldView;
tVPort *s_pWorldMainVPort;
tSimpleBufferManager *g_pWorldMainBfr;
tCameraManager *g_pWorldCamera;
tBitMap *s_pTiles;

// Silo highlight
// TODO: struct?
tBob *s_pSiloHighlight;
UBYTE s_ubWasSiloHighlighted;
UBYTE g_ubDoSiloHighlight;
UWORD g_uwSiloHighlightTileY;
UWORD g_uwSiloHighlightTileX;

// Speed logging
#define SPEED_LOG
tAvg *s_pDrawAvgExplosions;
tAvg *s_pDrawAvgProjectiles;
tAvg *s_pDrawAvgVehicles;
tAvg *s_pUndrawAvgExplosions;
tAvg *s_pUndrawAvgProjectiles;
tAvg *s_pUndrawAvgVehicles;

UWORD uwLimboX;
UWORD uwLimboY;

ULONG g_ulGameFrame;

void displayPrepareLimbo(void) {
	cursorSetConstraints(0,0, 320, 255);
	hudChangeState(HUD_STATE_SELECTING);

	g_pLocalPlayer->ubSpawnIdx = spawnGetNearest(0, 0, g_pLocalPlayer->ubTeam);

	uwLimboX = MAX(0, (g_pSpawns[g_pLocalPlayer->ubSpawnIdx].ubTileX << MAP_TILE_SIZE) + MAP_HALF_TILE - (WORLD_VPORT_WIDTH/2));
	uwLimboY = MAX(0, (g_pSpawns[g_pLocalPlayer->ubSpawnIdx].ubTileY << MAP_TILE_SIZE) + MAP_HALF_TILE - (WORLD_VPORT_HEIGHT/2));
}

void displayPrepareDriving(void) {
	cursorSetConstraints(0, 0, 320, 191);
	hudChangeState(HUD_STATE_DRIVING);
}

void worldDraw(void) {
	UBYTE ubPlayer;

	// Silo highlight
	if(g_ubDoSiloHighlight)
		bobDraw(
			s_pSiloHighlight, g_pWorldMainBfr->pBuffer,
			g_uwSiloHighlightTileX << MAP_TILE_SIZE,
			g_uwSiloHighlightTileY << MAP_TILE_SIZE
		);

	// Vehicles
	logAvgBegin(s_pDrawAvgVehicles);
	for(ubPlayer = 0; ubPlayer != g_ubPlayerLimit; ++ubPlayer) {
		tPlayer *pPlayer = &g_pPlayers[ubPlayer];
		if(
			pPlayer->ubState == PLAYER_STATE_SURFACING ||
			pPlayer->ubState == PLAYER_STATE_BUNKERING
		) {
			spawnAnimate(pPlayer->ubSpawnIdx);
		}
		else
			vehicleDraw(&pPlayer->sVehicle);
	}
	logAvgEnd(s_pDrawAvgVehicles);

	logAvgBegin(s_pDrawAvgProjectiles);
	projectileDraw();
	logAvgEnd(s_pDrawAvgProjectiles);

	logAvgBegin(s_pDrawAvgExplosions);
	explosionsDraw(g_pWorldMainBfr->pBuffer);
	logAvgEnd(s_pDrawAvgExplosions);

	turretUpdateSprites();

	s_ubWasSiloHighlighted = g_ubDoSiloHighlight;
}

void worldUndraw(void) {
	UBYTE ubPlayer;

	explosionsUndraw(g_pWorldMainBfr->pBuffer);
	projectileUndraw();

	// Vehicles
	for(ubPlayer = g_ubPlayerLimit; ubPlayer--;)
		vehicleUndraw(&g_pPlayers[ubPlayer].sVehicle);

	// Silo highlight
	if(s_ubWasSiloHighlighted)
		bobUndraw(
			s_pSiloHighlight, g_pWorldMainBfr->pBuffer
		);
}

// One 32x32@5bpp tile takes 640 bytes.
// 20x20 map (400 tiles) takes 256000 bytes - 250 KB
// One 32x32@4bpp tile takes 512 bytes
// so 250KB consists of 500 tiles - 20x25
void gsGameCreate(void) {
	UBYTE i;

	logBlockBegin("gsGameCreate()");
	randInit(2184);
	teamsInit();
	mapCreate("data/maps/snafu.txt");

	// Add players
	playerListCreate(8);
	g_pLocalPlayer = playerAdd("player", TEAM_GREEN);
	for(UBYTE i = 0; i != 7; ++i) {
		char szName[10];
		sprintf(szName, "player%hhu", i);
		playerAdd(szName, TEAM_GREEN);
	}

	// Create everything needed to display world view
	// Prepare view & viewport
	g_pWorldView = viewCreate(0,
		TAG_VIEW_GLOBAL_CLUT, 1,
		TAG_VIEW_COPLIST_MODE, VIEW_COPLIST_MODE_RAW,
		TAG_VIEW_COPLIST_RAW_COUNT, WORLD_COP_SIZE,
		TAG_DONE
	);
	s_pWorldMainVPort = vPortCreate(0,
		TAG_VPORT_VIEW, g_pWorldView,
		TAG_VPORT_HEIGHT, WORLD_VPORT_HEIGHT,
		TAG_VPORT_BPP, WORLD_BPP,
		TAG_DONE
	);
	g_pWorldMainBfr = simpleBufferCreate(0,
		TAG_SIMPLEBUFFER_VPORT, s_pWorldMainVPort,
		TAG_SIMPLEBUFFER_BOUND_WIDTH, g_fubMapTileWidth << MAP_TILE_SIZE,
		TAG_SIMPLEBUFFER_BOUND_HEIGHT, g_fubMapTileHeight << MAP_TILE_SIZE,
		TAG_SIMPLEBUFFER_COPLIST_OFFSET, WORLD_COP_VPMAIN_POS,
		TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_INTERLEAVED,
		TAG_DONE
	);
	if(!g_pWorldMainBfr) {
		logWrite("Buffer creation failed");
		gamePopState();
		return;
	}
	paletteLoad("data/amidb16.plt", s_pWorldMainVPort->pPalette, 16);
	paletteLoad("data/amidb16.plt", &s_pWorldMainVPort->pPalette[16], 16);
	g_pWorldCamera = g_pWorldMainBfr->pCameraManager;

	hudCreate();

	// Load gfx
	s_pTiles = bitmapCreateFromFile("data/tiles.bm");
	s_pSiloHighlight = bobUniqueCreate("data/silohighlight.bm", "data/silohighlight.msk", 0, 0);

	// Associate map with world's buffer & tileset
	mapSetSrcDst(s_pTiles, g_pWorldMainBfr->pBuffer);

	// Enabling sprite DMA
	tCopCmd *pSpriteEnList = &g_pWorldView->pCopList->pBackBfr->pList[WORLD_COP_SPRITEEN_POS];
	copSetMove(&pSpriteEnList[0].sMove, &custom.dmacon, BITSET | DMAF_SPRITE);
	CopyMemQuick(
		&g_pWorldView->pCopList->pBackBfr->pList[WORLD_COP_SPRITEEN_POS],
		&g_pWorldView->pCopList->pFrontBfr->pList[WORLD_COP_SPRITEEN_POS],
		sizeof(tCopCmd)
	);

	// Crosshair stuff
	cursorCreate();
	cursorSetConstraints(0, 0, 320, 192);

	// Explosions
	explosionsCreate();

	#ifdef SPEED_LOG
	s_pDrawAvgExplosions = logAvgCreate("draw explosions", 50);
	s_pDrawAvgProjectiles = logAvgCreate("draw projectiles", 50);
	s_pDrawAvgVehicles = logAvgCreate("draw vehicles", 50);

	s_pUndrawAvgExplosions = logAvgCreate("undraw explosions", 50);
	s_pUndrawAvgProjectiles = logAvgCreate("undraw projectiles", 50);
	s_pUndrawAvgVehicles = logAvgCreate("undraw vehicles", 50);
	#endif

	// Initial values
	s_ubWasSiloHighlighted = 0;
	g_ubDoSiloHighlight = 0;
	g_ulGameFrame = 0;

	// Now that world copperlist is created, prepare map logic & do the first draw
	mapGenerateLogic();
	mapRedraw();
	displayPrepareLimbo();

	// Get some speed out of unnecessary DMA
	custom.dmacon = BITCLR | DMAF_DISK;

	viewLoad(g_pWorldView);
	logBlockEnd("gsGameCreate()");
}

void gsGameLoop(void) {
	if(keyCheck(KEY_ESCAPE)) {
		gamePopState(); // Pop to threaded loader so it can free stuff
		return;
	}

	cursorUpdate();
	dataRecv();                // Receives positions of other players from server
	playerLocalProcessInput(); // Steer requests & limbo
	spawnSim();
	playerSim();               // Players: vehicle positions, death states, etc.
	turretSim();               // Turrets: targeting, rotation & projectile spawn
	projectileSim();           // Projectiles: new positions, damage
	dataSend();                // Sends data to server

	// Steering-irrelevant player input
	if(keyUse(KEY_C))
		bitmapSaveBmp(g_pWorldMainBfr->pBuffer, s_pWorldMainVPort->pPalette, "debug/bufDump.bmp");
	if(keyUse(KEY_L))
		copDumpBfr(g_pWorldView->pCopList->pBackBfr);

	hudUpdate();

	// Update main vport
	vPortWaitForEnd(s_pWorldMainVPort);
	worldUndraw();
	if(g_pLocalPlayer->sVehicle.ubLife) {
		UWORD uwLocalX, uwLocalY;
		uwLocalX = g_pLocalPlayer->sVehicle.fX;
		uwLocalY = g_pLocalPlayer->sVehicle.fY;
		cameraCenterAt(g_pWorldCamera, uwLocalX & 0xFFFE, uwLocalY);
	}
	else {
		cameraMoveBy(
			g_pWorldCamera,
			2 * SGN(uwLimboX - g_pWorldCamera->uPos.sUwCoord.uwX),
			2 * SGN(uwLimboY - g_pWorldCamera->uPos.sUwCoord.uwY)
		);
	}
	mapUpdateTiles();
	worldDraw();

	viewProcessManagers(g_pWorldView);
	copProcessBlocks();
	++g_ulGameFrame;
}

void gsGameDestroy(void) {
	// Return DMA to correct state
	custom.dmacon = BITSET | DMAF_DISK;
	logBlockBegin("gsGameDestroy()");

	cursorDestroy();
	hudDestroy();
	explosionsDestroy();
	viewDestroy(g_pWorldView);
	bobUniqueDestroy(s_pSiloHighlight);
	bitmapDestroy(s_pTiles);

	#ifdef SPEED_LOG
	logAvgDestroy(s_pUndrawAvgExplosions);
	logAvgDestroy(s_pUndrawAvgProjectiles);
	logAvgDestroy(s_pUndrawAvgVehicles);
	logAvgDestroy(s_pDrawAvgVehicles);
	logAvgDestroy(s_pDrawAvgProjectiles);
	logAvgDestroy(s_pDrawAvgExplosions);
	#endif

	mapDestroy();
	playerListDestroy();

	logBlockEnd("gsGameDestroy()");
}
