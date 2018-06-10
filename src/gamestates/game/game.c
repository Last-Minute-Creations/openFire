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
#include <ace/managers/system.h>
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

tBitMap *s_pHighlightBitmap, *s_pHighlightMask;
static tBobNew s_sHighlightBob;

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

void gsGameCreate(void) {
	logBlockBegin("gsGameCreate()");
	randInit(2184);

	// Prepare view
	// Must be before mapCreate 'cuz turretListCreate() needs copperlist
	g_pWorldView = viewCreate(0,
		TAG_VIEW_GLOBAL_CLUT, 1,
		TAG_VIEW_COPLIST_MODE, VIEW_COPLIST_MODE_RAW,
		TAG_VIEW_COPLIST_RAW_COUNT, WORLD_COP_SIZE,
	TAG_DONE);

	// Load gfx
	s_pTiles = bitmapCreateFromFile("data/tiles.bm");

	teamsInit();

	worldMapCreate();
	// Create viewports
	s_pWorldMainVPort = vPortCreate(0,
		TAG_VPORT_VIEW, g_pWorldView,
		TAG_VPORT_HEIGHT, WORLD_VPORT_HEIGHT,
		TAG_VPORT_BPP, WORLD_BPP,
	TAG_DONE);
	g_pWorldMainBfr = simpleBufferCreate(0,
		TAG_SIMPLEBUFFER_VPORT, s_pWorldMainVPort,
		TAG_SIMPLEBUFFER_BOUND_WIDTH, g_sMap.fubWidth << MAP_TILE_SIZE,
		TAG_SIMPLEBUFFER_BOUND_HEIGHT, g_sMap.fubHeight << MAP_TILE_SIZE,
		TAG_SIMPLEBUFFER_COPLIST_OFFSET, WORLD_COP_VPMAIN_POS,
		TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_INTERLEAVED,
		TAG_SIMPLEBUFFER_IS_DBLBUF, 1,
	TAG_DONE);
	if(!g_pWorldMainBfr) {
		logWrite("Buffer creation failed");
		gamePopState();
		return;
	}
	g_pWorldCamera = g_pWorldMainBfr->pCameraManager;
	worldMapSetBuffers(s_pTiles, g_pWorldMainBfr->pFront, g_pWorldMainBfr->pBack);
	paletteLoad("data/game.plt", s_pWorldMainVPort->pPalette, 16);
	paletteLoad("data/sprites.plt", &s_pWorldMainVPort->pPalette[16], 16);

	const UBYTE ubProjectilesMax = 16;
	const UBYTE ubPlayersMax = 8;
	bobNewManagerCreate(
		ubPlayersMax*2 + ubProjectilesMax,
		ubPlayersMax*2*(VEHICLE_BODY_WIDTH/16 + 1)*VEHICLE_BODY_HEIGHT +
			ubProjectilesMax*2*2 + EXPLOSIONS_MAX*3*32,
		g_pWorldMainBfr->pFront, g_pWorldMainBfr->pBack
	);

	s_pHighlightBitmap = bitmapCreateFromFile("data/silohighlight.bm");
	s_pHighlightMask = bitmapCreateFromFile("data/silohighlight_mask.bm");
	bobNewInit(
		&s_sHighlightBob, 32, 32, 1,
		s_pHighlightBitmap, s_pHighlightMask, 0, 0
	);

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

	// Initial values
	g_ubDoSiloHighlight = 0;
	g_ulGameFrame = 0;

	// AI
	playerListCreate(ubPlayersMax);
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
	if(g_pWorldMainBfr->pBack != g_pWorldMainBfr->pFront) {
		// Might be too big to do in a single blit
		for(UWORD i = 0; i < g_pWorldMainBfr->pBack->Rows; i+=32) {
			blitCopyAligned(
				g_pWorldMainBfr->pBack, 0, i, g_pWorldMainBfr->pFront, 0, i,
				bitmapGetByteWidth(g_pWorldMainBfr->pBack) * 8, 32
			);
		}
	}

	viewLoad(g_pWorldView);
	logBlockEnd("gsGameCreate()");
	systemUnuse();
}

static void gameSummaryLoop(void) {
	if(keyUse(KEY_ESCAPE) || mouseUse(MOUSE_PORT_1, MOUSE_LMB)) {
		gameChangeState(menuCreate, menuLoop, menuDestroy);
		return;
	}
	scoreTableProcessView();
}

void gameDebugKeys(void) {
#if defined(GAME_DEBUG)
	if(keyUse(KEY_C)) {
		bitmapSaveBmp(
			g_pWorldMainBfr->pFront, s_pWorldMainVPort->pPalette, "debug/bufDump.bmp"
		);
	}
	if(keyUse(KEY_L)) {
		copDumpBfr(g_pWorldView->pCopList->pFrontBfr);
	}
#endif
}

void gsGameLoop(void) {
	++g_ulGameFrame;
	// Quit?
	if(keyUse(KEY_ESCAPE)) {
		gameChangeState(menuCreate, menuLoop, menuDestroy);
		return;
	}
	gameDebugKeys();
	// Steering-irrelevant player input
	if(keyUse(KEY_T)) {
		consoleChatBegin();
	}
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

	dataRecv(); // Receives positions of other players from server

  cursorUpdate();

	playerLocalProcessInput(); // Steer requests, chat, limbo
	botProcess();

	dataSend(); // Send input requests to server

	spawnSim();

	controlSim();

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

	bobNewBegin();
	worldMapUpdateTiles();
	controlRedrawPoints();

	// Silo highlight
	if(g_ubDoSiloHighlight) {
		s_sHighlightBob.sPos.sUwCoord.uwX = g_uwSiloHighlightX - (1<<MAP_TILE_SIZE)/2;
		s_sHighlightBob.sPos.sUwCoord.uwY = g_uwSiloHighlightY - (1<<MAP_TILE_SIZE)/2;
		bobNewPush(&s_sHighlightBob);
	}

	playerSim(); // Players & vehicles states
	projectileDraw();
	explosionsDraw(g_pWorldMainBfr);

	if(!s_isScoreShown) {
		turretUpdateSprites();
	}

	// This should be done on vblank interrupt
	if(!s_isScoreShown) {
		viewProcessManagers(g_pWorldView);
		copProcessBlocks();
	}
	else {
		scoreTableProcessView();
	}

	turretSim(); // Turrets: targeting, rotation & projectile spawn
	projectileSim(); // Projectiles: new positions, damage
	hudUpdate();
}

void gsGameDestroy(void) {
	systemUse();
	logBlockBegin("gsGameDestroy()");

	bobNewManagerDestroy();

	aiManagerDestroy();

	cursorDestroy();
	scoreTableDestroy();
	hudDestroy();
	fontDestroy(s_pSmallFont);
	explosionsDestroy();
	viewDestroy(g_pWorldView);
	bitmapDestroy(s_pHighlightBitmap);
	bitmapDestroy(s_pHighlightMask);
	bitmapDestroy(s_pTiles);

	worldMapDestroy();
	playerListDestroy();

	logBlockEnd("gsGameDestroy()");
}
