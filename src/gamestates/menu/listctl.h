#ifndef OF_GAMESTATES_MENU_LISTCTL_H
#define OF_GAMESTATES_MENU_LISTCTL_H

#include <ace/types.h>
#include <ace/utils/font.h>

#define LISTCTL_ENTRY_INVALID 0xFFFF
#define LISTCTL_DRAWSTATE_OK 0
#define LISTCTL_DRAWSTATE_NEEDS_REDRAW 1

typedef struct _tListCtl {
	tUwRect sRect;
	UBYTE ubDrawState;
	UBYTE ubEntryHeight;
	UWORD uwEntryCnt;
	UWORD uwEntryMaxCnt;
	UWORD uwEntrySel;
	char **pEntries;
	tFont *pFont;
	tBitMap *pBfr;
	void (*onChange)(void);
} tListCtl;

tListCtl *listCtlCreate(
	IN tBitMap *pBfr,
	IN UWORD uwX,
	IN UWORD uwY,
	IN UWORD uwWidth,
	IN UWORD uwHeight,
	IN tFont *pFont,
	IN UWORD uwEntryMaxCnt,
	IN void (*onChange)(void)
);

void listCtlDestroy(
	IN tListCtl *pCtl
);

UWORD listCtlAddEntry(
	IN tListCtl *pCtl,
	IN char *szTxt
);

void listCtlRemoveEntry(
	IN tListCtl *pCtl,
	IN UWORD uwIdx
);

void listCtlDraw(
	IN tListCtl *pCtl
);

FUBYTE listCtlProcessClick(
	IN tListCtl *pCtl,
	IN UWORD uwMouseX,
	IN UWORD uwMouseY
);

#endif // OF_GAMESTATES_MENU_LISTCTL_H
