#include "gamestates/precalc/precalc.h"
#include <ace/managers/log.h>
#include <ace/managers/game.h>
#include <ace/utils/font.h>
#include <ace/utils/palette.h>
#include "vehicletypes.h"
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
static FUBYTE s_isHdd;
static FUBYTE s_fubProgress;
static tBitMap *s_pLoadingVehicle;

void precalcCreate(void) {
	logBlockBegin("precalcCreate()");

	s_pView = viewCreate(0,
		TAG_VIEW_GLOBAL_CLUT, 1,
		TAG_DONE
	);
	s_pVPort = vPortCreate(0,
		TAG_VPORT_VIEW, s_pView,
		TAG_VPORT_BPP, PRECALC_BPP,
		TAG_DONE
	);
	s_pBuffer = simpleBufferCreate(0,
		TAG_SIMPLEBUFFER_VPORT, s_pVPort,
		TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_CLEAR | BMF_INTERLEAVED,
		TAG_DONE
	);

	copBlockDisableSprites(s_pView->pCopList, 0xFF);
	paletteLoad("data/loading.plt", s_pVPort->pPalette, 1 << PRECALC_BPP);
	bitmapLoadFromFile(s_pBuffer->pBack, "data/menu/logo.bm", 80, 16);
	s_pLoadingVehicle = bitmapCreateFromFile("data/loading/tank.bm");

	s_isHdd = 1;

	s_pFont = fontCreate("data/silkscreen5.fnt");
	fontDrawStr(
		s_pBuffer->pBack, s_pFont, 320/2, 256/4,
		"Precalculating...", PRECALC_COLOR_TEXT, FONT_TOP | FONT_HCENTER
	);
	if(s_isHdd) {
		fontDrawStr(
			s_pBuffer->pBack, s_pFont, 320/2, 256/4 + 10,
			"This will take a long time only once",
			PRECALC_COLOR_TEXT, FONT_TOP | FONT_HCENTER
		);
	}
	else {
		fontDrawStr(
			s_pBuffer->pBack, s_pFont, 320/2, 256/4 + 10,
			"For better load times put this game on HDD",
			PRECALC_COLOR_TEXT, FONT_TOP | FONT_HCENTER
		);
	}

	s_fubProgress = 0;

	viewLoad(s_pView);
	logBlockEnd("precalcCreate()");
}

void precalcLoop(void) {
	static FUBYTE isInit = 0;

	if(isInit) {
		gameClose();
		return;
	}
	logBlockBegin("precalcLoop()");

	precalcIncreaseProgress(10, "Initializing vehicle types");
	vehicleTypesCreate();

	// Turret stuff
	precalcIncreaseProgress(20, "Generating turret frames");
	g_pTurretFrames = vehicleTypeGenerateRotatedFrames("vehicles/turret.bm");

	precalcIncreaseProgress(10, "Working on projectiles");
	projectileListCreate(5);

	// View is no longer needed
	viewLoad(0);
	viewDestroy(s_pView);
	fontDestroy(s_pFont);

	bitmapDestroy(s_pLoadingVehicle);

	// All done - load menu
	gamePushState(menuCreate, menuLoop, menuDestroy);
	isInit = 1;

	// If returned - close game
	logBlockEnd("precalcLoop()");
}

void precalcDestroy(void) {
	logBlockBegin("precalcDestroy()");

	projectileListDestroy();
	vehicleTypesDestroy();
	bitmapDestroy(g_pTurretFrames);

	logBlockEnd("precalcDestroy()");
}

void precalcIncreaseProgress(FUBYTE fubAmountToAdd, char *szText) {
	const UWORD uwProgressX = 60;
	const UWORD uwProgressY = 208;
	const UWORD uwProgressWidth = 200;
	const UWORD uwProgressHeight = 10;

	s_fubProgress = MIN(100, s_fubProgress+fubAmountToAdd);
	logWrite("precalcIncreaseProgress() -> %"PRI_FUBYTE"%% - %s\n", s_fubProgress, szText);

	UWORD uwVehicleWidth = s_pLoadingVehicle->BytesPerRow<<3;
	UWORD uwVehicleHeight = s_pLoadingVehicle->Rows/6;
	blitCopy(
		s_pLoadingVehicle, 0, ((s_fubProgress*6)/100) * s_pLoadingVehicle->Rows/6,
		s_pBuffer->pBack,
		(s_pBuffer->uBfrBounds.sUwCoord.uwX - uwVehicleWidth)/2,
		(s_pBuffer->uBfrBounds.sUwCoord.uwY - uwVehicleHeight)/2,
		uwVehicleWidth, uwVehicleHeight, MINTERM_COOKIE, 0xFF
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
		s_pBuffer->pBack, s_pFont,
		uwProgressX + uwProgressWidth/2, uwProgressY + uwProgressHeight/2,
		szText, PRECALC_COLOR_TEXT, FONT_CENTER | FONT_SHADOW | FONT_COOKIE
	);
}
