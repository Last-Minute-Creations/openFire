#include "gamestates/menu/maplist.h"
#include <string.h>
#include <dos/dos.h>
#include <clib/dos_protos.h>
#include <ace/types.h>
#include <ace/managers/memory.h>
#include <ace/managers/key.h>
#include <ace/managers/game.h>
#include <ace/managers/mouse.h>
#include "cursor.h"
#include "gamestates/menu/menu.h"
#include "gamestates/menu/button.h"

#define MAPLIST_FILENAME_MAX 108
#define MAP_NAME_MAX 30

typedef struct _tMapListEntry {
	char szFileName[MAPLIST_FILENAME_MAX];
	char szMapName[MAP_NAME_MAX];
} tMapListEntry;

typedef struct _tMapList {
	UWORD uwMapCount;
	tMapListEntry *pMaps;
} tMapList;

tMapList s_sMapList;

void mapListPrepareListCtrl(void) {
	const UWORD uwCtrlX = 10;
	const UWORD uwCtrlY = 10;
	const UWORD uwCtrlWidth = 100;
	const UWORD uwCtrlHeight = 200;
	const UBYTE ubEntryHeight = 7;

	blitRect(g_pMenuBuffer->pBuffer, uwCtrlX, uwCtrlY, uwCtrlWidth, 1, 9);
	blitRect(g_pMenuBuffer->pBuffer, uwCtrlX, uwCtrlY + uwCtrlHeight-1, uwCtrlWidth, 1, 4);
	blitRect(g_pMenuBuffer->pBuffer, uwCtrlX, uwCtrlY, 1, uwCtrlHeight, 9);
	blitRect(g_pMenuBuffer->pBuffer, uwCtrlX + uwCtrlWidth-1, uwCtrlY, 1, uwCtrlHeight, 4);
	blitRect(g_pMenuBuffer->pBuffer, uwCtrlX + uwCtrlWidth-10-2, uwCtrlY+10+3, 10, uwCtrlHeight-20-6, 1);

	UWORD uwFirstVisible = 0;
	UWORD uwLastVisible = MIN(
		s_sMapList.uwMapCount,
		uwFirstVisible + (uwCtrlHeight - 4) / ubEntryHeight
	);
	for(UWORD i = uwFirstVisible; i != uwLastVisible; ++i) {
		s_sMapList.pMaps[i].szFileName[strlen(s_sMapList.pMaps[i].szFileName)-5] = 0;
		fontDrawStr(
			g_pMenuBuffer->pBuffer, g_pMenuFont,
			uwCtrlX+2, uwCtrlY+2 + i*ubEntryHeight, s_sMapList.pMaps[i].szFileName,
			13, FONT_LEFT| FONT_TOP | FONT_COOKIE
		);
		s_sMapList.pMaps[i].szFileName[strlen(s_sMapList.pMaps[i].szFileName)-5] = '.';
	}
	buttonAdd(uwCtrlX + uwCtrlWidth - 10-2, uwCtrlY+2, 10, 10, "U", 0);
	buttonAdd(uwCtrlX + uwCtrlWidth - 10-2, uwCtrlY + uwCtrlHeight - 10 -2, 10, 10, "D", 0);
}

void mapListPrepareList(void) {
	// Get map count
	s_sMapList.uwMapCount = 0;
	BPTR pLock;
	struct FileInfoBlock sFileBlock;
	pLock = Lock("data/maps", ACCESS_READ);
	LONG lResult;
	lResult = Examine(pLock, &sFileBlock);
	if(lResult != DOSFALSE) {
		lResult = ExNext(pLock, &sFileBlock); // Skip dir name
		while(lResult != DOSFALSE) {
			if(!memcmp(
				&sFileBlock.fib_FileName[strlen(sFileBlock.fib_FileName)-strlen(".json")],
				".json", strlen(".json")
			)) {
				++s_sMapList.uwMapCount;
				lResult = ExNext(pLock, &sFileBlock);
			}
		}
	}
	UnLock(pLock);

	// Alloc map list
	if(!s_sMapList.uwMapCount)
		return;
	s_sMapList.pMaps = memAllocFast(s_sMapList.uwMapCount * sizeof(tMapListEntry));
	pLock = Lock("data/maps", ACCESS_READ);
	UWORD i = 0;
	lResult = Examine(pLock, &sFileBlock);
	if(lResult != DOSFALSE) {
		lResult = ExNext(pLock, &sFileBlock); // Skip dir name
		while(lResult != DOSFALSE) {
			if(!memcmp(
				&sFileBlock.fib_FileName[strlen(sFileBlock.fib_FileName)-strlen(".json")],
				".json", strlen(".json")
			)) {
				memcpy(
					s_sMapList.pMaps[i].szFileName, sFileBlock.fib_FileName,
					MAPLIST_FILENAME_MAX
				);
				++i;
				lResult = ExNext(pLock, &sFileBlock);
			}
		}
	}
	UnLock(pLock);
}

void mapListCreate(void) {
	// Clear bg
	blitRect(
		g_pMenuBuffer->pBuffer, 0, 0,
		bitmapGetByteWidth(g_pMenuBuffer->pBuffer) << 3, g_pMenuBuffer->pBuffer->Rows,
		0
	);
	WaitBlit();

	buttonListCreate(10, g_pMenuBuffer->pBuffer, g_pMenuFont);

	mapListPrepareList();

	mapListPrepareListCtrl();

	buttonDrawAll();
}

void mapListLoop(void) {
	if(keyUse(KEY_ESCAPE)) {
		gameChangeState(menuMainCreate, menuLoop, menuMainDestroy);
		return;
	}

	if(mouseUse(MOUSE_LMB))
		buttonProcessClick(g_uwMouseX, g_uwMouseY);

	menuProcess();
}

void mapListDestroy(void) {
	memFree(s_sMapList.pMaps, s_sMapList.uwMapCount * sizeof(tMapListEntry));
}
