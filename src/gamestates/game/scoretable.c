#include "gamestates/game/scoretable.h"
#include <ace/managers/viewport/simplebuffer.h>
#include <ace/utils/palette.h>
#include <ace/utils/sprite.h>
#include "gamestates/game/player.h"

#define SCORE_TABLE_BPP 4

static tView *s_pView;
static tVPort *s_pVPort;
static tSimpleBufferManager *s_pBfr;
static tFont *s_pFont;

static const UBYTE s_ubColorBlue = 12;
static const UBYTE s_ubColorRed  = 10;
static const UBYTE s_ubColorHeader = 13;
static const UBYTE s_ubColorBot = 4;
static const UBYTE s_pColX[4] = {32, 96, 160, 224};

tTextBitMap *s_pBotTextBfr;
tTextBitMap *s_pNameTextBfr;

void scoreTableCreate(tVPort *pHudVPort, tFont *pFont) {
	logBlockBegin("scoreTableCreate(pHudVPort: %p, pFont: %p)", pHudVPort, pFont);
	s_pView = viewCreate(0,
		TAG_VIEW_COPLIST_MODE, VIEW_COPLIST_MODE_RAW,
		TAG_VIEW_COPLIST_RAW_COUNT, 8*2 + (6+2*SCORE_TABLE_BPP) + 3,
		TAG_VIEW_GLOBAL_PALETTE, 1,
	TAG_DONE);
	s_pVPort = vPortCreate(0,
		TAG_VPORT_VIEW, s_pView,
		TAG_VPORT_HEIGHT, 192+1,
		TAG_VPORT_BPP, SCORE_TABLE_BPP,
	TAG_DONE);
	s_pBfr = simpleBufferCreate(0,
		TAG_SIMPLEBUFFER_VPORT, s_pVPort,
		TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_CLEAR | BMF_INTERLEAVED,
		TAG_SIMPLEBUFFER_COPLIST_OFFSET, 8*2,
	TAG_DONE);
	paletteLoad("data/game.plt", s_pVPort->pPalette, 16);
	paletteLoad("data/sprites.plt", &s_pVPort->pPalette[16], 16);

	tSimpleBufferManager *pHudBfr = (tSimpleBufferManager*)vPortGetManager(pHudVPort, VPM_SCROLL);

	spriteDisableInCopRawMode(
		s_pView->pCopList,
		SPRITE_0 | SPRITE_1 | SPRITE_2 | SPRITE_3 | SPRITE_4 | SPRITE_5 | SPRITE_6 | SPRITE_7,
		0
	);

	// Jump to HUD - back buffer to back buffer
	ULONG ulHudListAddr = (ULONG)((void*)
		&pHudBfr->sCommon.pVPort->pView->pCopList->pBackBfr->pList[pHudBfr->uwCopperOffset]
	);
	copSetMove(
		&s_pView->pCopList->pBackBfr->pList[8*2 + (6+2*SCORE_TABLE_BPP) + 0].sMove,
		&g_pCopLc[1].uwHi, ulHudListAddr >> 16
	);
	copSetMove(
		&s_pView->pCopList->pBackBfr->pList[8*2 + (6+2*SCORE_TABLE_BPP) + 1].sMove,
		&g_pCopLc[1].uwLo, (UWORD)(ulHudListAddr & 0xFFFFF)
	);
	copSetMove(
		&s_pView->pCopList->pBackBfr->pList[8*2 + (6+2*SCORE_TABLE_BPP) + 2].sMove,
		&g_pCustom->copjmp2, 1
	);

	// Jump to HUD - front buffer to front buffer
	ulHudListAddr = (ULONG)((void*)
		&pHudBfr->sCommon.pVPort->pView->pCopList->pFrontBfr->pList[pHudBfr->uwCopperOffset]
	);
	copSetMove(
		&s_pView->pCopList->pFrontBfr->pList[8*2 + (6+2*SCORE_TABLE_BPP) + 0].sMove,
		&g_pCopLc[1].uwHi, (UWORD)(ulHudListAddr >> 16)
	);
	copSetMove(
		&s_pView->pCopList->pFrontBfr->pList[8*2 + (6+2*SCORE_TABLE_BPP) + 1].sMove,
		&g_pCopLc[1].uwLo, (UWORD)(ulHudListAddr & 0xFFFFF)
	);
	copSetMove(
		&s_pView->pCopList->pFrontBfr->pList[8*2 + (6+2*SCORE_TABLE_BPP) + 2].sMove,
		&g_pCustom->copjmp2, 1
	);

	// Add a border
	// TODO blitLine
	blitRect(s_pBfr->pBack, 0, 0, 1, 192, 13);
	blitRect(s_pBfr->pBack, 0, 0, 320, 1, 13);
	blitRect(s_pBfr->pBack, 1, 1, 1, 191, 9);
	blitRect(s_pBfr->pBack, 1, 1, 319, 1, 9);

	blitRect(s_pBfr->pBack, 0, 190, 320, 1, 13);
	blitRect(s_pBfr->pBack, 318, 0, 1, 191, 13);
	blitRect(s_pBfr->pBack, 0, 191, 320, 1, 9);
	blitRect(s_pBfr->pBack, 319, 0, 1, 192, 9);

	s_pFont = pFont;
	s_pBotTextBfr = fontCreateTextBitMapFromStr(s_pFont, "[BOT]");
	s_pNameTextBfr = fontCreateTextBitMap(128, pFont->uwHeight);
	const UBYTE ubColHeaderOffsY = 4;
	const char *pColHeaders[4] = {"Name", "Deaths", "Kills", "Capture points"};
	for(UBYTE i = 0; i < 4; ++i) {
		fontDrawStr(
			s_pFont, s_pBfr->pBack, s_pColX[i], ubColHeaderOffsY, pColHeaders[i],
			s_ubColorHeader, FONT_TOP | FONT_LEFT | FONT_COOKIE, s_pNameTextBfr
		);
	}
	logBlockEnd("scoreTableCreate()");
}

void scoreTableDestroy(void) {
	fontDestroyTextBitMap(s_pBotTextBfr);
	fontDestroyTextBitMap(s_pNameTextBfr);
	viewDestroy(s_pView);
}

void scoreTableUpdate(void) {
	for(UBYTE i = 0; i < g_ubPlayerCount; ++i) {
		if(g_pPlayers[i].isBot) {
			fontDrawTextBitMap(
				s_pBfr->pBack, s_pBotTextBfr,	6, 16 + 7*i,
				s_ubColorBot, FONT_TOP | FONT_LEFT | FONT_COOKIE
			);
		}
		fontDrawStr(s_pFont, s_pBfr->pBack, 32, 16 + 7*i, g_pPlayers[i].szName,
			(g_pPlayers[i].ubTeam == TEAM_RED ? s_ubColorRed : s_ubColorBlue),
			FONT_TOP | FONT_LEFT | FONT_COOKIE, s_pNameTextBfr
		);
	}
}

void scoreTableShowSummary(void) {
	const char *szMsg = !g_pTeams[TEAM_BLUE].uwTicketsLeft ? "Red Wins!" : "Blue Wins!";
	fontDrawStr(
		s_pFont, s_pBfr->pBack, 160, 160, szMsg,
		s_ubColorRed, FONT_COOKIE | FONT_CENTER, s_pNameTextBfr
	);
	scoreTableShow();
}

void scoreTableShow(void) {
	scoreTableUpdate();
	viewLoad(s_pView);
}

void scoreTableProcessView(void) {
	viewProcessManagers(s_pView);
}
