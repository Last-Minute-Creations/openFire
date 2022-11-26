#include "gamestates/precalc/precalc.h"
#include <ace/managers/log.h>
#include <ace/managers/game.h>
#include <ace/managers/viewport/simplebuffer.h>
#include <ace/utils/font.h>
#include <ace/utils/palette.h>
#include <ace/utils/custom.h>
#include "vehicletypes.h"
#include "open_fire.h"
#include "gamestates/menu/menu.h"
#include "gamestates/game/projectile.h"
#include "gamestates/game/turret.h"
#include "gamestates/game/gamemath.h"

#define PRECALC_BPP 4
// Colors
#define PRECALC_COLOR_TEXT             13
#define PRECALC_COLOR_PROGRESS_OUTLINE 15
#define PRECALC_COLOR_PROGRESS_FILL    8

static tView *s_pView;
static tVPort *s_pVPort;
static tSimpleBufferManager *s_pBuffer;
static tFont *s_pFont;
static tTextBitMap *s_pTextBitMap;
static FUBYTE s_isHdd;
static FUBYTE s_fubProgress;
static tBitMap *s_pLoadingVehicle;

static void precalcCreate(void) {
	logBlockBegin("precalcCreate()");

	s_pView = viewCreate(0,
		TAG_VIEW_GLOBAL_PALETTE, 1,
	TAG_DONE);
	s_pVPort = vPortCreate(0,
		TAG_VPORT_VIEW, s_pView,
		TAG_VPORT_BPP, PRECALC_BPP,
	TAG_DONE);
	s_pBuffer = simpleBufferCreate(0,
		TAG_SIMPLEBUFFER_VPORT, s_pVPort,
		TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_CLEAR | BMF_INTERLEAVED,
	TAG_DONE);

	paletteLoad("data/loading.plt", s_pVPort->pPalette, 1 << PRECALC_BPP);
	bitmapLoadFromFile(s_pBuffer->pBack, "data/menu/logo.bm", 80, 16);
	const char* pVehicleSources[] = {
		"data/loading/tank.bm", "data/loading/jeep.bm", "data/loading/chopper.bm"
	};
	s_pLoadingVehicle = bitmapCreateFromFile(pVehicleSources[getRayPos().ulValue % 3], 0);

	s_isHdd = 1;

	s_pFont = fontCreate("data/silkscreen5.fnt");
	s_pTextBitMap = fontCreateTextBitMap(320, s_pFont->uwHeight);
	fontDrawStr(
		s_pFont, s_pBuffer->pBack, 320/2, 256/4,
		"Precalculating...", PRECALC_COLOR_TEXT, FONT_TOP | FONT_HCENTER, s_pTextBitMap
	);
	if(s_isHdd) {
		fontDrawStr(
			s_pFont, s_pBuffer->pBack, 320/2, 256/4 + 10,
			"This will take a long time only once",
			PRECALC_COLOR_TEXT, FONT_TOP | FONT_HCENTER, s_pTextBitMap
		);
	}
	else {
		fontDrawStr(
			s_pFont, s_pBuffer->pBack, 320/2, 256/4 + 10,
			"For better load times put this game on HDD",
			PRECALC_COLOR_TEXT, FONT_TOP | FONT_HCENTER, s_pTextBitMap
		);
	}

	UWORD uwVehicleWidth = bitmapGetByteWidth(s_pLoadingVehicle) * 8;
	UWORD uwVehicleHeight = s_pLoadingVehicle->Rows/2;
	blitCopy(
		s_pLoadingVehicle, 0, 0,
		s_pBuffer->pBack,
		(s_pBuffer->uBfrBounds.uwX - uwVehicleWidth)/2,
		(s_pBuffer->uBfrBounds.uwY - uwVehicleHeight)/2,
		uwVehicleWidth, uwVehicleHeight, MINTERM_COOKIE
	);

	s_fubProgress = 0;

	viewLoad(s_pView);
	logBlockEnd("precalcCreate()");
}

static void precalcLoop(void) {
	static FUBYTE isInit = 0;

	if(isInit) {
		gameExit();
		return;
	}
	logBlockBegin("precalcLoop()");

	precalcIncreaseProgress(10, "Initializing vehicle types");
	vehicleTypesCreate();

	// TODO load tileset for turret use
	g_pMapTileset = bitmapCreateFromFile("data/tiles.bm", 0);

	// Turret stuff
	precalcIncreaseProgress(20, "Generating turret frames");
	g_pTurretFrames[TEAM_RED] = turretGenerateFrames("vehicles/turret/turret_red.bm");
	g_pTurretFrames[TEAM_BLUE] = turretGenerateFrames("vehicles/turret/turret_blue.bm");
	g_pTurretFrames[TEAM_NONE] = turretGenerateFrames("vehicles/turret/turret_gray.bm");

	precalcIncreaseProgress(10, "Working on projectiles");

	// View is no longer needed
	viewLoad(0);
	viewDestroy(s_pView);
	fontDestroy(s_pFont);
	fontDestroyTextBitMap(s_pTextBitMap);

	bitmapDestroy(s_pLoadingVehicle);

	// All done - load menu
	statePush(g_pStateManager, &g_sStateMenu);
	isInit = 1;

	// If returned - close game
	logBlockEnd("precalcLoop()");
}

static void precalcDestroy(void) {
	logBlockBegin("precalcDestroy()");

	vehicleTypesDestroy();
	for(UBYTE i = 0; i < 3; ++i) {
		bitmapDestroy(g_pTurretFrames[i]);
	}
	bitmapDestroy(g_pMapTileset);

	logBlockEnd("precalcDestroy()");
}

void precalcIncreaseProgress(FUBYTE fubAmountToAdd, char *szText) {
	const UWORD uwProgressX = 60;
	const UWORD uwProgressY = 208;
	const UWORD uwProgressWidth = 200;
	const UWORD uwProgressHeight = 10;

	s_fubProgress = MIN(99, s_fubProgress+fubAmountToAdd);
	logWrite("precalcIncreaseProgress() -> %"PRI_FUBYTE"%% - %s\n", s_fubProgress, szText);

	UWORD uwVehicleWidth = bitmapGetByteWidth(s_pLoadingVehicle)*8;
	UWORD uwVehicleHeight = s_pLoadingVehicle->Rows/2;
	blitCopy(
		s_pLoadingVehicle, 0, uwVehicleHeight,
		s_pBuffer->pBack,
		(s_pBuffer->uBfrBounds.uwX - uwVehicleWidth)/2,
		(s_pBuffer->uBfrBounds.uwY - uwVehicleHeight)/2,
		(s_fubProgress*uwVehicleWidth)/100, uwVehicleHeight, MINTERM_COOKIE
	);

	// BG + outline
	blitRect(
		s_pBuffer->pBack,
		uwProgressX - 1,	uwProgressY - 1,
		1+ uwProgressWidth + 1, 1 + uwProgressHeight + 1,
		PRECALC_COLOR_PROGRESS_OUTLINE
	);

	// Progress
	UWORD uwFillWidth = (s_fubProgress*uwProgressWidth)/100;
	blitRect(
		s_pBuffer->pBack,
		(WORD)uwProgressX,	(WORD)uwProgressY,
		(WORD)uwFillWidth,	(WORD)uwProgressHeight,
		PRECALC_COLOR_PROGRESS_FILL
	);
	blitRect(
		s_pBuffer->pBack,
		(WORD)(uwProgressX + uwFillWidth), (WORD)uwProgressY,
		(WORD)(uwProgressWidth - uwFillWidth), (WORD)uwProgressHeight, 0
	);

	// Text
	fontDrawStr(
		s_pFont, s_pBuffer->pBack,
		uwProgressX + uwProgressWidth/2, uwProgressY + uwProgressHeight/2,
		szText, PRECALC_COLOR_TEXT, FONT_CENTER | FONT_SHADOW | FONT_COOKIE, s_pTextBitMap
	);
}

tState g_sStatePrecalc = {
	.cbCreate = precalcCreate, .cbLoop = precalcLoop, .cbDestroy = precalcDestroy
};
