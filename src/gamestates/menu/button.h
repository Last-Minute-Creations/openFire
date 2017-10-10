#ifndef GUARD_OF_GAMESTATES_MENU_BUTTON_H
#define GUARD_OF_GAMESTATES_MENU_BUTTON_H

#include <ace/types.h>
#include <ace/utils/bitmap.h>
#include <ace/utils/font.h>

#define BUTTON_MAX_TEXT 20

typedef struct _tButton {
	tUwRect sRect;
	char szText[BUTTON_MAX_TEXT];
	void (*onClick)(void);
} tButton;

void buttonListCreate(
	IN FUBYTE fubButtonCount,
	IN tBitMap *pBfr,
	IN tFont *pFont
);

void buttonListDestroy(void);

void buttonAdd(
	IN UWORD uwX,
	IN UWORD uwY,
	IN UWORD uwWidth,
	IN UWORD uwHeight,
	IN char *szText,
	IN void (*onClick)(void)
);

void buttonDrawAll(void);

void buttonProcessClick(
	IN UWORD uwX,
	IN UWORD uwY
);

#endif // GUARD_OF_GAMESTATES_MENU_BUTTON_H
