#include "gamestates/precalc/precalc.h"
#include <ace/config.h>
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
#define PRECALC_COLOR_PROGRESS_OUTLINE 7
#define PRECALC_COLOR_PROGRESS_FILL    3

static tView *s_pView;
static tVPort *s_pVPort;
static tSimpleBufferManager *s_pBuffer;
static tFont *s_pFont;
FUBYTE s_isHdd;
FUBYTE s_fubProgress;

void precalcCreate(void) {
	logBlockBegin("precalcCreate()");

	s_pView = viewCreate(0,
		TAG_VIEW_GLOBAL_CLUT, 1,
		TAG_DONE
	);
	s_pVPort = vPortCreate(0,
		TAG_VPORT_VIEW, s_pView,
		TAG_VPORT_BPP, WINDOW_SCREEN_BPP,
		TAG_DONE
	);
	s_pBuffer = simpleBufferCreate(0,
		TAG_SIMPLEBUFFER_VPORT, s_pVPort,
		TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_CLEAR,
		TAG_DONE
	);

	copBlockDisableSprites(s_pView->pCopList, 0xFF);
	paletteLoad("data/game.plt", s_pVPort->pPalette, 1 << PRECALC_BPP);
	bitmapLoadFromFile(s_pBuffer->pBuffer, "data/menu/logo.bm", 80, 16);

	s_isHdd = 1;

	s_pFont = fontCreate("data/silkscreen5.fnt");
	fontDrawStr(
		s_pBuffer->pBuffer, s_pFont, 320/2, 256/4,
		"Precalcing...", PRECALC_COLOR_TEXT, FONT_TOP | FONT_HCENTER
	);
	if(s_isHdd) {
		fontDrawStr(
			s_pBuffer->pBuffer, s_pFont, 320/2, 256/4 + 10,
			"This will take a long time only once",
			PRECALC_COLOR_TEXT, FONT_TOP | FONT_HCENTER
		);
	}
	else {
		fontDrawStr(
			s_pBuffer->pBuffer, s_pFont, 320/2, 256/4 + 10,
			"For better load times put this game on HDD",
			PRECALC_COLOR_TEXT, FONT_TOP | FONT_HCENTER
		);
	}

	s_fubProgress = 0;

	viewLoad(s_pView);
	logBlockEnd("precalcCreate()");
}

void precalcLoop(void) {
	static FUBYTE fubInited = 0;

	if(fubInited) {
		gameClose();
		return;
	}
	logBlockBegin("precalcLoop()");

	precalcIncreaseProgress(5, "INIT");
	vehicleTypesCreate();

	// Turret stuff
	precalcIncreaseProgress(5, "Generating turret frames");
	vehicleTypeBobSourceLoad("turret", &g_sTurretSource, 0);

	// Generate math table
	precalcIncreaseProgress(5, "Calculating sine table");
	generateSine();

	precalcIncreaseProgress(5, "Working on projectiles");
	projectileListCreate(80);

	// View is no longer needed
	viewLoad(0);
	viewDestroy(s_pView);
	fontDestroy(s_pFont);

	// All done - load menu
	gamePushState(menuCreate, menuLoop, menuDestroy);
	fubInited = 1;

	// If returned - close game
	logBlockEnd("precalcLoop()");
}

void precalcDestroy(void) {
	logBlockBegin("precalcDestroy()");

	projectileListDestroy();
	vehicleTypesDestroy();
	vehicleTypeBobSourceUnload(&g_sTurretSource);

	logBlockEnd("precalcDestroy()");
}

void precalcIncreaseProgress(FUBYTE fubAmountToAdd, char *szText) {
	const UWORD uwProgressX = 60;
	const UWORD uwProgressY = 208;
	const UWORD uwProgressWidth = 200;
	const UWORD uwProgressHeight = 16;

	s_fubProgress = MIN(100, s_fubProgress+fubAmountToAdd);
	logWrite("precalcIncreaseProgress() -> %"PRI_FUBYTE"%% - %s\n", s_fubProgress, szText);

	// BG + outline
	blitRect(
		s_pBuffer->pBuffer,
		uwProgressX - 1,	uwProgressY - 1,
		1+ uwProgressWidth + 1, 1 + uwProgressHeight + 1,
		PRECALC_COLOR_PROGRESS_OUTLINE
	);

	// Progress
	UWORD uwFillWidth = (s_fubProgress*uwProgressWidth)/100;
	blitRect(
		s_pBuffer->pBuffer,
		uwProgressX,	uwProgressY, uwFillWidth,	uwProgressHeight,
		PRECALC_COLOR_PROGRESS_FILL
	);
	blitRect(
		s_pBuffer->pBuffer,
		uwProgressX + uwFillWidth, uwProgressY,
		uwProgressWidth - uwFillWidth, uwProgressHeight, 0
	);

	// Text
	fontDrawStr(
		s_pBuffer->pBuffer, s_pFont,
		uwProgressX + uwProgressWidth/2, uwProgressY + uwProgressHeight/2,
		szText, PRECALC_COLOR_TEXT, FONT_CENTER | FONT_SHADOW | FONT_COOKIE
	);
}
