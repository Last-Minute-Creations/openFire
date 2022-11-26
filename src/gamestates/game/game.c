#include "gamestates/game/game.h"
#include <ace/macros.h>
#include <ace/managers/copper.h>
#include <ace/managers/blit.h>
#include <ace/managers/viewport/simplebuffer.h>
#include <ace/managers/key.h>
#include <ace/managers/mouse.h>
#include <ace/managers/game.h>
#include <ace/managers/system.h>
#include <ace/utils/sprite.h>
#include <ace/utils/extview.h>
#include <ace/utils/palette.h>
#include "../../open_fire.h"
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
static UBYTE s_isScoreShown;
static UBYTE s_isSummary;
static tFont *s_pSmallFont;

ULONG g_ulGameFrame;
UBYTE g_isLocalBot;
UBYTE g_ubFps;
UBYTE s_ubFpsCounter;
UBYTE s_ubFpsCalcTimeout;
tRandManager g_sRandManager;

void displayPrepareLimbo(void) {
	mouseSetBounds(MOUSE_PORT_1, 0,0, 320, 255);
	hudChangeState(HUD_STATE_SELECTING);
}

void displayPrepareDriving(void) {
	mouseSetBounds(MOUSE_PORT_1, 0, 0, 320, 191);
	hudChangeState(HUD_STATE_DRIVING);
}

FN_HOTSPOT
static void INTERRUPT gameVblankServer(
  UNUSED_ARG REGARG(volatile tCustom *pCustom, "a0"),
  UNUSED_ARG REGARG(volatile void *pData, "a1")
) {
	++s_ubFpsCalcTimeout;
	if(s_ubFpsCalcTimeout == 50) {
		s_ubFpsCalcTimeout = 0;
		g_ubFps = s_ubFpsCounter;
		s_ubFpsCounter = 0;
	}
	if(!g_isChatting) {
		steerRequestCapture();
	}
}

void gsGameCreate(void) {
	logBlockBegin("gsGameCreate()");
	randInit(&g_sRandManager, 2184, 1911);

	// Prepare view
	g_pWorldView = viewCreate(0,
		TAG_VIEW_GLOBAL_PALETTE, 1,
		TAG_VIEW_COPLIST_MODE, VIEW_COPLIST_MODE_RAW,
		TAG_VIEW_COPLIST_RAW_COUNT, WORLD_COP_SIZE,
	TAG_DONE);

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
		TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_INTERLEAVED | BMF_CLEAR,
		TAG_SIMPLEBUFFER_IS_DBLBUF, 1,
	TAG_DONE);
	if(!g_pWorldMainBfr) {
		logWrite("Buffer creation failed");
		statePop(g_pStateManager);
		return;
	}
	g_pWorldCamera = g_pWorldMainBfr->pCamera;

	const UBYTE ubProjectilesMax = 16;
	const UBYTE ubPlayersMax = 8;
	bobNewManagerCreate(
		ubPlayersMax*2 + EXPLOSIONS_MAX + ubProjectilesMax,
		ubPlayersMax*2*(VEHICLE_BODY_WIDTH/16 + 1)*VEHICLE_BODY_HEIGHT +
			ubProjectilesMax*2*(1+1)*2 + EXPLOSIONS_MAX*2*(2+1)*32,
		g_pWorldMainBfr->pFront, g_pWorldMainBfr->pBack
	);

	worldMapCreate(g_pWorldMainBfr->pFront, g_pWorldMainBfr->pBack);

	teamsInit();
	paletteLoad("data/game.plt", s_pWorldMainVPort->pPalette, 16);
	paletteLoad("data/sprites.plt", &s_pWorldMainVPort->pPalette[16], 16);

	projectileListCreate(ubProjectilesMax);

	s_pSmallFont = fontCreate("data/silkscreen5.fnt");
	hudCreate(s_pSmallFont);
	scoreTableCreate(g_pHudBfr->sCommon.pVPort, s_pSmallFont);
	s_isScoreShown = 0;
	s_isSummary = 0;

	// Enabling sprite DMA
	tCopCmd *pSpriteEnList = &g_pWorldView->pCopList->pBackBfr->pList[WORLD_COP_SPRITEEN_POS];
	copSetMove(&pSpriteEnList[0].sMove, &g_pCustom->dmacon, BITSET | DMAF_SPRITE);
	g_pWorldView->pCopList->pFrontBfr->pList[WORLD_COP_SPRITEEN_POS].ulCode = g_pWorldView->pCopList->pBackBfr->pList[WORLD_COP_SPRITEEN_POS].ulCode;
	spriteDisableInCopRawMode(
		g_pWorldView->pCopList,
		SPRITE_0 | SPRITE_1 | SPRITE_3 | SPRITE_4 | SPRITE_5 | SPRITE_6 | SPRITE_7,
		WORLD_COP_SPRITEEN_POS + 1
	);

	// Crosshair stuff
	cursorCreate(g_pWorldView, 2, "data/crosshair.bm", WORLD_COP_CROSS_POS);

	// Explosions
	explosionsCreate();

	// Initial values
	g_ulGameFrame = 0;
	s_ubFpsCounter = 0;
	g_ubFps = 0;
	s_ubFpsCalcTimeout = 0;
	steerRequestInit();

	// AI
	playerListInit(ubPlayersMax);
	aiManagerCreate();

	// Add players
	if(g_isLocalBot) {
		botAdd("player", TEAM_BLUE);
		g_pLocalPlayer = &g_pPlayers[0];
	}
	else {
		g_pLocalPlayer = playerAdd("player", TEAM_BLUE);
	}
	// botAdd("enemy", TEAM_RED);
	displayPrepareLimbo();

	blitWait();
	systemSetInt(INTB_VERTB, gameVblankServer, 0);

	viewLoad(g_pWorldView);
	logBlockEnd("gsGameCreate()");
	systemUnuse();
}

static void gameSummaryLoop(void) {
	if(keyUse(KEY_ESCAPE) || mouseUse(MOUSE_PORT_1, MOUSE_LMB)) {
		stateChange(g_pStateManager, &g_sStateMenu);
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
	if(s_isSummary) {
		gameSummaryLoop();
		return;
	}
	++g_ulGameFrame;
	// Quit?
	if(keyUse(KEY_ESCAPE)) {
		stateChange(g_pStateManager, &g_sStateMenu);
		return;
	}
	if(!g_isChatting) {
		gameDebugKeys();
		// Steering-irrelevant player input
		if(keyUse(KEY_T)) {
			consoleChatBegin();
		}
		else if(keyUse(KEY_F1)) {
			hudToggleFps();
		}
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

	// Refresh HUD before it gets displayed - still single buffered
	hudUpdate();

	// Undraw bobs so that something goes on during data recv
	dataRecv(); // Receives positions of other players from server
	dataSend(); // Send input requests to server
	while(steerRequestProcessedCount() != steerRequestReadCount()) {
		spawnSim();
		controlSim();

		playerLocalProcessInput(); // Steer requests, chat, limbo
		botProcess();

		// Undraw bobs, draw pending tiles
		bobNewBegin();
		controlRedrawPoints();
		worldMapUpdateTiles();

		// sim & draw
		playerSim(); // Players & vehicles states
		turretSim(); // Turrets: targeting, rotation & projectile spawn
		projectileSim(); // Projectiles: new positions, damage -> explosions
		explosionsProcess();
		steerRequestMoveToNext();
	}
	if(!steerRequestReadCount()) {
		bobNewBegin();
	}
	bobNewPushingDone();

	if(!g_pTeams[TEAM_RED].uwTicketsLeft || !g_pTeams[TEAM_BLUE].uwTicketsLeft) {
		scoreTableShowSummary();
		s_isSummary = 1;
	}

	cursorUpdate();
	if(g_pLocalPlayer->ubState != PLAYER_STATE_LIMBO) {
		cameraCenterAt(
			g_pWorldCamera,
			g_pLocalPlayer->sVehicle.uwX, g_pLocalPlayer->sVehicle.uwY
		);
	}
	else {
		UWORD uwSpawnX = g_pLocalPlayer->sVehicle.uwX;
		UWORD uwSpawnY = g_pLocalPlayer->sVehicle.uwY;
		UWORD uwLimboX = MAX(0, uwSpawnX - WORLD_VPORT_WIDTH/2);
		UWORD uwLimboY = MAX(0, uwSpawnY- WORLD_VPORT_HEIGHT/2);
		WORD wDx = (WORD)CLAMP(uwLimboX - g_pWorldCamera->uPos.uwX, -2, 2);
		WORD wDy = (WORD)CLAMP(uwLimboY - g_pWorldCamera->uPos.uwY, -2, 2);
		cameraMoveBy(g_pWorldCamera, wDx, wDy);
	}

	steerRequestSwap();
	bobNewEnd(); // SHOULD BE SOMEWHERE HERE
	worldMapSwapBuffers();

	// Start refreshing gfx at hud
	++s_ubFpsCounter;
	vPortWaitForEnd(s_pWorldMainVPort);
	if(!s_isScoreShown) {
		viewProcessManagers(g_pWorldView);
		copProcessBlocks();
	}
	else {
		scoreTableProcessView();
	}
}

void gsGameDestroy(void) {
	systemUse();
	logBlockBegin("gsGameDestroy()");

	systemSetInt(INTB_VERTB, 0, 0);

	projectileListDestroy();

	bobNewManagerDestroy();

	aiManagerDestroy();

	cursorDestroy();
	scoreTableDestroy();
	hudDestroy();
	fontDestroy(s_pSmallFont);
	explosionsDestroy();
	viewDestroy(g_pWorldView);

	worldMapDestroy();

	logBlockEnd("gsGameDestroy()");
}

tState g_sStateGame = {
	.cbCreate = gsGameCreate, .cbLoop = gsGameLoop, .cbDestroy = gsGameDestroy
};
