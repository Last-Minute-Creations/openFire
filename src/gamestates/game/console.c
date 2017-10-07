#include "gamestates/game/console.h"
#include <ace/utils/font.h>
#include "gamestates/game/game.h"
#include "gamestates/game/hud.h"

// 210x59
#define CONSOLE_MAX_ENTRIES 8
#define CONSOLE_MESSAGE_MAX 35

tFont *s_pConsoleFont;

void consoleCreate(void) {
	s_pConsoleFont = fontCreate("data/silkscreen5.fnt");
}

void consoleDestroy(void) {
	fontDestroy(s_pConsoleFont);
}

void consoleWrite(char *szMsg, UBYTE ubColor) {
	// Move remaining messages up
	blitCopyAligned(
		g_pHudBfr->pBuffer, 112, 9,
		g_pHudBfr->pBuffer, 112, 3,
		208, 41
	);

	// Clear last line
	blitRect(g_pHudBfr->pBuffer, 112,45, 112, 5, 0);

	// Draw new message
	fontDrawStr(
		g_pHudBfr->pBuffer, s_pConsoleFont, 112, 45,
		szMsg, ubColor, FONT_TOP | FONT_LEFT | FONT_LAZY
	);
}
