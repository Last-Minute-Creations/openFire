#include "gamestates/game/console.h"
#include <ace/utils/font.h>
#include "gamestates/game/game.h"
#include "gamestates/game/hud.h"

// 210x59
#define CONSOLE_MAX_ENTRIES 8
#define CONSOLE_MESSAGE_MAX 35
#define CHAT_MAX (192/6)

#define CONSOLE_COLOR_GENERAL 1
#define CONSOLE_COLOR_BLUE    2
#define CONSOLE_COLOR_RED     3

tFont *s_pConsoleFont;
char s_pChatBfr[CHAT_MAX] = "say: ";
FUBYTE g_isChatting;
FUBYTE s_fubChatLineLength;


void consoleCreate(void) {
	s_pConsoleFont = fontCreate("data/silkscreen5.fnt");
	g_isChatting = 0;
}

void consoleDestroy(void) {
	fontDestroy(s_pConsoleFont);
}

void consoleWrite(char *szMsg, UBYTE ubColor) {
	// Move remaining messages up
	blitCopyAligned(
		g_pHudBfr->pBuffer, 112, 9,
		g_pHudBfr->pBuffer, 112, 3,
		192, 41
	);

	// Clear last line
	blitRect(g_pHudBfr->pBuffer, 112,45, 192, 5, 0);

	// Draw new message
	fontDrawStr(
		g_pHudBfr->pBuffer, s_pConsoleFont, 112, 45,
		szMsg, ubColor, FONT_TOP | FONT_LEFT | FONT_LAZY
	);
}

void consoleChatBegin(void) {
	s_fubChatLineLength = 5;
	s_pChatBfr[s_fubChatLineLength] = 0;
	g_isChatting = 1;
	fontDrawStr(
		g_pHudBfr->pBuffer, s_pConsoleFont, 112, 51, s_pChatBfr,
		CONSOLE_COLOR_GENERAL, FONT_TOP | FONT_LEFT | FONT_LAZY
	);
}

void consoleChatEnd(void) {
	// Erase chat line
	blitRect(g_pHudBfr->pBuffer, 112,51, 192, 5, 0);
	g_isChatting = 0;
}

FUBYTE consoleChatProcessChar(char c) {
	// Try control chars
	if(c == KEY_RETURN || c == KEY_NUMENTER) {
		if(s_fubChatLineLength == 5)
			return 0;
		s_pChatBfr[s_fubChatLineLength] = 0;
		consoleWrite(&s_pChatBfr[5], CONSOLE_COLOR_GENERAL);
		consoleChatEnd();
		return 0;
	}
	else {
		// Printable chars
		c= g_pToAscii[c];
		if(
			(c >= 'A' && c <= 'Z') ||
			(c >= 'a' && c <= 'z') ||
			(c >= '0' && c <= '9') ||
			c == ' '
		) {
			s_pChatBfr[s_fubChatLineLength++] = c;
			s_pChatBfr[s_fubChatLineLength] = 0; // for printing
			fontDrawStr(
				g_pHudBfr->pBuffer, s_pConsoleFont, 112, 51, s_pChatBfr,
				CONSOLE_COLOR_GENERAL, FONT_TOP | FONT_LEFT | FONT_LAZY
			);
		}
	}
	return 1;
}
