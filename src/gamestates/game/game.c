#include "gamestates/game/game.h"
#include <hardware/intbits.h> // INTB_COPER
#include <ace/macros.h>
#include <ace/managers/copper.h>
#include <ace/managers/blit.h>
#include <ace/managers/viewport/simplebuffer.h>
#include <ace/managers/key.h>
#include <ace/managers/mouse.h>
#include <ace/managers/game.h>
#include <ace/managers/rand.h>
#include <ace/utils/extview.h>
#include <ace/utils/palette.h>
#include "cursor.h"
#include "gamestates/game/worldmap.h"
#include "gamestates/game/vehicle.h"
#include "gamestates/game/player.h"
#include "gamestates/game/team.h"
#include "gamestates/game/projectile.h"
#include "gamestates/game/data.h"
#include "gamestates/game/hud.h"
#include "gamestates/game/turret.h"
#include "gamestates/game/spawn.h"
#include "gamestates/game/control.h"
#include "gamestates/game/console.h"
#include "gamestates/game/ai/ai.h"
#include "gamestates/game/ai/bot.h"
#include "gamestates/game/scoretable.h"
#include "gamestates/menu/menu.h"

// Viewport stuff
tView *g_pWorldView;
tSimpleBufferManager *g_pWorldMainBfr;
tCameraManager *g_pWorldCamera;
static tVPort *s_pWorldMainVPort;
static tBitMap *s_pTiles;
static UBYTE s_isScoreShown;

// Silo highlight
// TODO: struct?
UBYTE g_ubDoSiloHighlight;
UWORD g_uwSiloHighlightY;
UWORD g_uwSiloHighlightX;
static tBob *s_pSiloHighlight;
static UBYTE s_ubWasSiloHighlighted;

// Speed logging
// #ifdef GAME_DEBUG
#define SPEED_LOG /* Log speed in main loop */
// #endif

#ifdef SPEED_LOG
static tAvg *s_pDrawAvgExplosions;
static tAvg *s_pDrawAvgProjectiles;
static tAvg *s_pUndrawAvgExplosions;
static tAvg *s_pUndrawAvgProjectiles;
static tAvg *s_pUndrawAvgVehicles;
static tAvg *s_pProcessAvgCursor;
static tAvg *s_pProcessAvgDataRecv;
static tAvg *s_pProcessAvgInput;
static tAvg *s_pProcessAvgSpawn;
static tAvg *s_pProcessAvgPlayer;
static tAvg *s_pProcessAvgControl;
static tAvg *s_pProcessAvgTurret;
static tAvg *s_pProcessAvgProjectile;
static tAvg *s_pProcessAvgDataSend;
static tAvg *s_pAvgRedrawControl;
static tAvg *s_pAvgUpdateSprites;
static tAvg *s_pProcessAvgHud;
#endif

ULONG g_ulGameFrame;
static tFont *s_pSmallFont;

UBYTE g_isLocalBot;

void displayPrepareLimbo(void) {
	mouseSetBounds(MOUSE_PORT_1, 0,0, 320, 255);
	hudChangeState(HUD_STATE_SELECTING);
}

void displayPrepareDriving(void) {
	mouseSetBounds(MOUSE_PORT_1, 0, 0, 320, 191);
	hudChangeState(HUD_STATE_DRIVING);
}

static void worldUndraw(void) {
	UBYTE ubPlayer;

	logAvgBegin(s_pUndrawAvgExplosions);
	explosionsUndraw(g_pWorldMainBfr);
	logAvgEnd(s_pUndrawAvgExplosions);

	logAvgBegin(s_pUndrawAvgProjectiles);
	projectileUndraw();
	logAvgEnd(s_pUndrawAvgProjectiles);

	// Vehicles
	logAvgBegin(s_pUndrawAvgVehicles);
	for(ubPlayer = g_ubPlayerLimit; ubPlayer--;)
		vehicleUndraw(&g_pPlayers[ubPlayer].sVehicle);
	logAvgEnd(s_pUndrawAvgVehicles);

	// Silo highlight
	if(s_ubWasSiloHighlighted)
		bobUndraw(s_pSiloHighlight, g_pWorldMainBfr);
}

void gsGameCreate(void) {
	logBlockBegin("gsGameCreate()");
	randInit(2184);

	// Prepare view
	// Must be before mapCreate 'cuz turretListCreate() needs copperlist
	g_pWorldView = viewCreate(0,
		TAG_VIEW_GLOBAL_CLUT, 1,
		TAG_VIEW_COPLIST_MODE, VIEW_COPLIST_MODE_RAW,
		TAG_VIEW_COPLIST_RAW_COUNT, WORLD_COP_SIZE,
		TAG_DONE
	);

	// Load gfx
	s_pTiles = bitmapCreateFromFile("data/tiles.bm");
	s_pSiloHighlight = bobUniqueCreate(
		"data/silohighlight.bm", "data/silohighlight_mask.bm", 0, 0, 0
	);

	teamsInit();

	worldMapCreate();
	// Create viewports
	s_pWorldMainVPort = vPortCreate(0,
		TAG_VPORT_VIEW, g_pWorldView,
		TAG_VPORT_HEIGHT, WORLD_VPORT_HEIGHT,
		TAG_VPORT_BPP, WORLD_BPP,
		TAG_DONE
	);
	g_pWorldMainBfr = simpleBufferCreate(0,
		TAG_SIMPLEBUFFER_VPORT, s_pWorldMainVPort,
		TAG_SIMPLEBUFFER_BOUND_WIDTH, g_sMap.fubWidth << MAP_TILE_SIZE,
		TAG_SIMPLEBUFFER_BOUND_HEIGHT, g_sMap.fubHeight << MAP_TILE_SIZE,
		TAG_SIMPLEBUFFER_COPLIST_OFFSET, WORLD_COP_VPMAIN_POS,
		TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_INTERLEAVED,
		TAG_DONE
	);
	if(!g_pWorldMainBfr) {
		logWrite("Buffer creation failed");
		gamePopState();
		return;
	}
	g_pWorldCamera = g_pWorldMainBfr->pCameraManager;
	worldMapSetSrcDst(s_pTiles, g_pWorldMainBfr->pBuffer);
	paletteLoad("data/game.plt", s_pWorldMainVPort->pPalette, 16);
	paletteLoad("data/sprites.plt", &s_pWorldMainVPort->pPalette[16], 16);

	s_pSmallFont = fontCreate("data/silkscreen5.fnt");
	hudCreate(s_pSmallFont);
	scoreTableCreate(g_pHudBfr->sCommon.pVPort, s_pSmallFont);
	s_isScoreShown = 0;

	// Enabling sprite DMA
	tCopCmd *pSpriteEnList = &g_pWorldView->pCopList->pBackBfr->pList[WORLD_COP_SPRITEEN_POS];
	copSetMove(&pSpriteEnList[0].sMove, &g_pCustom->dmacon, BITSET | DMAF_SPRITE);
	CopyMemQuick(
		&g_pWorldView->pCopList->pBackBfr->pList[WORLD_COP_SPRITEEN_POS],
		&g_pWorldView->pCopList->pFrontBfr->pList[WORLD_COP_SPRITEEN_POS],
		sizeof(tCopCmd)
	);
	copRawDisableSprites(g_pWorldView->pCopList, 251, WORLD_COP_SPRITEEN_POS+1);

	// Crosshair stuff
	cursorCreate(g_pWorldView, 2, "data/crosshair.bm", WORLD_COP_CROSS_POS);

	// Explosions
	explosionsCreate();

	#ifdef SPEED_LOG
	s_pDrawAvgExplosions = logAvgCreate("draw explosions", 50);
	s_pDrawAvgProjectiles = logAvgCreate("draw projectiles", 50);

	s_pUndrawAvgExplosions = logAvgCreate("undraw explosions", 50);
	s_pUndrawAvgProjectiles = logAvgCreate("undraw projectiles", 50);
	s_pUndrawAvgVehicles = logAvgCreate("undraw vehicles", 50);

	s_pProcessAvgCursor = logAvgCreate("cursor", 50);
	s_pProcessAvgDataRecv = logAvgCreate("data recv", 50);
	s_pProcessAvgInput = logAvgCreate("input", 50);
	s_pProcessAvgSpawn = logAvgCreate("spawn", 50);
	s_pProcessAvgPlayer = logAvgCreate("player", 50);
	s_pProcessAvgControl = logAvgCreate("control", 50);
	s_pProcessAvgTurret = logAvgCreate("turret", 50);
	s_pProcessAvgProjectile = logAvgCreate("projectile", 50);
	s_pProcessAvgDataSend = logAvgCreate("data send", 50);
	s_pAvgRedrawControl = logAvgCreate("redraw control points", 50);
	s_pAvgUpdateSprites = logAvgCreate("update sprites", 50);
	s_pProcessAvgHud = logAvgCreate("hud update", 50);
	#endif

	// Initial values
	s_ubWasSiloHighlighted = 0;
	g_ubDoSiloHighlight = 0;
	g_ulGameFrame = 0;

	// AI
	playerListCreate(8);
	aiManagerCreate();

	// Add players
	if(g_isLocalBot) {
		botAdd("player", TEAM_BLUE);
		g_pLocalPlayer = &g_pPlayers[0];
	}
	else {
		g_pLocalPlayer = playerAdd("player", TEAM_BLUE);
	}
	botAdd("enemy", TEAM_RED);
	displayPrepareLimbo();

	for(FUBYTE i = 0; i != 7; ++i) {
		char szName[10];
		sprintf(szName, "player %hhu", i);
		playerAdd(szName, TEAM_BLUE);
	}

	// Now that world buffer is created, do the first draw
	worldMapRedraw();

	// Get some speed out of unnecessary DMA
	g_pCustom->dmacon = BITCLR | DMAF_DISK;

	viewLoad(g_pWorldView);
	logBlockEnd("gsGameCreate()");
}

static void gameSummaryLoop(void) {
	if(keyUse(KEY_ESCAPE) || mouseUse(MOUSE_PORT_1, MOUSE_LMB)) {
		gameChangeState(menuCreate, menuLoop, menuDestroy);
		return;
	}
	scoreTableProcessView();
}

void gsGameLoop(void) {
	++g_ulGameFrame;
	// Quit?
	if(keyUse(KEY_ESCAPE)) {
		gameChangeState(menuCreate, menuLoop, menuDestroy);
		return;
	}
	// Steering-irrelevant player input
	if(keyUse(KEY_C))
		bitmapSaveBmp(g_pWorldMainBfr->pBuffer, s_pWorldMainVPort->pPalette, "debug/bufDump.bmp");
	if(keyUse(KEY_L))
		copDumpBfr(g_pWorldView->pCopList->pBackBfr);
	if(keyUse(KEY_T))
		consoleChatBegin();
	if(keyUse(KEY_TAB)) {
		s_isScoreShown = 1;
		scoreTableShow();
	}
	else if(s_isScoreShown) {
		if(keyCheck(KEY_TAB) == KEY_NACTIVE) {
			s_isScoreShown = 0;
			viewLoad(g_pWorldView);
		}
	}

	logAvgBegin(s_pProcessAvgDataRecv);
	dataRecv(); // Receives positions of other players from server
	logAvgEnd(s_pProcessAvgDataRecv);

  logAvgBegin(s_pProcessAvgCursor);
  cursorUpdate();
  logAvgEnd(s_pProcessAvgCursor);

	logAvgBegin(s_pProcessAvgInput);
	playerLocalProcessInput(); // Steer requests, chat, limbo
	logAvgEnd(s_pProcessAvgInput);
	botProcess();

	logAvgBegin(s_pProcessAvgDataSend);
	dataSend(); // Send input requests to server
	logAvgEnd(s_pProcessAvgDataSend);

	logAvgBegin(s_pProcessAvgSpawn);
	spawnSim();
	logAvgEnd(s_pProcessAvgSpawn);

	logAvgBegin(s_pProcessAvgControl);
	controlSim();
	logAvgEnd(s_pProcessAvgControl);

	if(!g_pTeams[TEAM_RED].uwTicketsLeft || !g_pTeams[TEAM_BLUE].uwTicketsLeft) {
		scoreTableShowSummary();
		gameChangeLoop(gameSummaryLoop);
	}

	if(g_pLocalPlayer->ubState != PLAYER_STATE_LIMBO) {
		cameraCenterAt(
			g_pWorldCamera,
			g_pLocalPlayer->sVehicle.uwX & 0xFFFE, g_pLocalPlayer->sVehicle.uwY
		);
	}
	else {
		UWORD uwSpawnX = g_pLocalPlayer->sVehicle.uwX;
		UWORD uwSpawnY = g_pLocalPlayer->sVehicle.uwY;
		UWORD uwLimboX = MAX(0, uwSpawnX - WORLD_VPORT_WIDTH/2);
		UWORD uwLimboY = MAX(0, uwSpawnY- WORLD_VPORT_HEIGHT/2);
		WORD wDx = (WORD)CLAMP(uwLimboX - g_pWorldCamera->uPos.sUwCoord.uwX, -2, 2);
		WORD wDy = (WORD)CLAMP(uwLimboY - g_pWorldCamera->uPos.sUwCoord.uwY, -2, 2);
		cameraMoveBy(g_pWorldCamera, wDx, wDy);
	}

	// Start refreshing gfx at hud
	vPortWaitForEnd(s_pWorldMainVPort);
	worldUndraw(); // This will take almost whole HUD time

	worldMapUpdateTiles();

	logAvgBegin(s_pAvgRedrawControl);
	controlRedrawPoints();
	logAvgEnd(s_pAvgRedrawControl);

	// Silo highlight
	if(g_ubDoSiloHighlight) {
		if(bobDraw(
			s_pSiloHighlight, g_pWorldMainBfr,
			g_uwSiloHighlightX, g_uwSiloHighlightY, 0, 32
		)) {
			s_ubWasSiloHighlighted = 1;
		}
		else
			s_ubWasSiloHighlighted = 0;
	}

	logAvgBegin(s_pProcessAvgPlayer);
	playerSim(); // Players & vehicles states
	logAvgEnd(s_pProcessAvgPlayer);

	logAvgBegin(s_pDrawAvgProjectiles);
	projectileDraw();
	logAvgEnd(s_pDrawAvgProjectiles);

	logAvgBegin(s_pDrawAvgExplosions);
	explosionsDraw(g_pWorldMainBfr);
	logAvgEnd(s_pDrawAvgExplosions);

	if(!s_isScoreShown) {
		logAvgBegin(s_pAvgUpdateSprites);
		turretUpdateSprites();
		logAvgEnd(s_pAvgUpdateSprites);
	}

	// This should be done on vblank interrupt
	if(!s_isScoreShown) {
		viewProcessManagers(g_pWorldView);
		copProcessBlocks();
	}
	else
		scoreTableProcessView();

	logAvgBegin(s_pProcessAvgTurret);
	turretSim();               // Turrets: targeting, rotation & projectile spawn
	logAvgEnd(s_pProcessAvgTurret);

	logAvgBegin(s_pProcessAvgProjectile);
	projectileSim();           // Projectiles: new positions, damage
	logAvgEnd(s_pProcessAvgProjectile);

	logAvgBegin(s_pProcessAvgHud);
	hudUpdate();
	logAvgEnd(s_pProcessAvgHud);
}

void gsGameDestroy(void) {
	// Return DMA to correct state
	logBlockBegin("gsGameDestroy()");
	g_pCustom->dmacon = BITSET | DMAF_DISK;

	aiManagerDestroy();

	cursorDestroy();
	scoreTableDestroy();
	hudDestroy();
	fontDestroy(s_pSmallFont);
	explosionsDestroy();
	viewDestroy(g_pWorldView);
	bobUniqueDestroy(s_pSiloHighlight);
	bitmapDestroy(s_pTiles);

#ifdef SPEED_LOG
	logAvgDestroy(s_pUndrawAvgExplosions);
	logAvgDestroy(s_pUndrawAvgProjectiles);
	logAvgDestroy(s_pUndrawAvgVehicles);
	logAvgDestroy(s_pDrawAvgProjectiles);
	logAvgDestroy(s_pDrawAvgExplosions);
	logAvgDestroy(s_pProcessAvgCursor);
	logAvgDestroy(s_pProcessAvgDataRecv);
	logAvgDestroy(s_pProcessAvgInput);
	logAvgDestroy(s_pProcessAvgSpawn);
	logAvgDestroy(s_pProcessAvgPlayer);
	logAvgDestroy(s_pProcessAvgControl);
	logAvgDestroy(s_pProcessAvgTurret);
	logAvgDestroy(s_pProcessAvgProjectile);
	logAvgDestroy(s_pProcessAvgDataSend);
	logAvgDestroy(s_pAvgRedrawControl);
	logAvgDestroy(s_pAvgUpdateSprites);
	logAvgDestroy(s_pProcessAvgHud);
#endif

	worldMapDestroy();
	playerListDestroy();

	logBlockEnd("gsGameDestroy()");
}
