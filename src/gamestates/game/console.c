#include "gamestates/game/console.h"
#include <ace/utils/font.h>
#include "gamestates/game/game.h"
#include "gamestates/game/hud.h"
#include "gamestates/game/player.h"

// 210x59
#define CONSOLE_MAX_ENTRIES 8
#define CHAT_MAX (192/6)

static tFont *s_pConsoleFont;
static char s_pChatBfr[CHAT_MAX] = "say: ";
FUBYTE g_isChatting;
static FUBYTE s_fubChatLineLength;

static tTextBitMap *s_pChatLineBfr;


void consoleCreate(tFont *pFont) {
	s_pConsoleFont = pFont;
	g_isChatting = 0;
	s_pChatLineBfr = fontCreateTextBitMap(192, pFont->uwHeight);
}

void consoleDestroy(void) {
	fontDestroyTextBitMap(s_pChatLineBfr);
}

void consoleWrite(char *szMsg, UBYTE ubColor) {
	// Move remaining messages up
	blitCopyAligned(
		g_pHudBfr->pBack, 112, 9,
		g_pHudBfr->pBack, 112, 3,
		192, 41
	);

	// Clear last line
	blitRect(g_pHudBfr->pBack, 112,45, 192, 5, 0);

	// Draw new message
	fontFillTextBitMap(s_pConsoleFont, s_pChatLineBfr, szMsg);
	fontDrawTextBitMap(
		g_pHudBfr->pBack, s_pChatLineBfr, 112, 45,
		ubColor, FONT_TOP | FONT_LEFT | FONT_LAZY
	);
}

void consoleChatBegin(void) {
	s_fubChatLineLength = 5;
	s_pChatBfr[s_fubChatLineLength] = 0;
	g_isChatting = 1;
	fontFillTextBitMap(s_pConsoleFont, s_pChatLineBfr, s_pChatBfr);
	fontDrawTextBitMap(
		g_pHudBfr->pBack, s_pChatLineBfr, 112, 51,
		CONSOLE_COLOR_GENERAL, FONT_TOP | FONT_LEFT | FONT_LAZY
	);
}

void consoleChatEnd(void) {
	// Erase chat line
	blitRect(g_pHudBfr->pBack, 112,51, 192, 5, 0);
	g_isChatting = 0;
}

FUBYTE consoleChatProcessChar(char c) {
	// Try control chars
	if(c == KEY_RETURN || c == KEY_NUMENTER) {
		if(s_fubChatLineLength == 5)
			return 0;
		s_pChatBfr[s_fubChatLineLength] = 0;
		playerSay(g_pLocalPlayer, &s_pChatBfr[5], 0);
		consoleChatEnd();
		return 0;
	}
	else {
		// Printable chars
		c= g_pToAscii[(UBYTE)c];
		if(
			(c >= 'A' && c <= 'Z') ||
			(c >= 'a' && c <= 'z') ||
			(c >= '0' && c <= '9') ||
			c == ' '
		) {
			s_pChatBfr[s_fubChatLineLength++] = c;
			s_pChatBfr[s_fubChatLineLength] = 0; // for printing
			fontFillTextBitMap(s_pConsoleFont, s_pChatLineBfr, s_pChatBfr);
			fontDrawTextBitMap(
				g_pHudBfr->pBack, s_pChatLineBfr, 112, 51,
				CONSOLE_COLOR_GENERAL, FONT_TOP | FONT_LEFT | FONT_LAZY
			);
		}
	}
	return 1;
}
