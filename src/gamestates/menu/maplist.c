#include "gamestates/menu/maplist.h"
#include <string.h>
#include <dos/dos.h>
#include <clib/dos_protos.h>
#include <ace/types.h>
#include <ace/managers/memory.h>
#include <ace/managers/key.h>
#include <ace/managers/game.h>
#include <ace/managers/mouse.h>
#include <ace/managers/system.h>
#include "cursor.h"
#include "map.h"
#include "gamestates/menu/menu.h"
#include "gamestates/menu/button.h"
#include "gamestates/menu/listctl.h"
#include "gamestates/menu/minimap.h"
#include "gamestates/game/game.h"

#define MAPLIST_FILENAME_MAX 108
#define MAP_NAME_MAX 30

#define MAPLIST_MINIMAP_BORDER 1

typedef struct _tMapListEntry {
	char szFileName[MAPLIST_FILENAME_MAX];
	char szMapName[MAP_NAME_MAX];
} tMapListEntry;

typedef struct _tMapList {
	UWORD uwMapCount;
	tMapListEntry *pMaps;
} tMapList;

static tMapList s_sMapList;
static tListCtl *s_pListCtl;

static void mapListPrepareList(void) {
	systemUse();
	// Get map count
	s_sMapList.uwMapCount = 0;
	BPTR pLock;
	struct FileInfoBlock sFileBlock;
	pLock = Lock((unsigned char*)"data/maps", ACCESS_READ);
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
	if(!s_sMapList.uwMapCount) {
		return;
	}
	s_sMapList.pMaps = memAllocFast(s_sMapList.uwMapCount * sizeof(tMapListEntry));
	pLock = Lock((unsigned char *)"data/maps", ACCESS_READ);
	lResult = Examine(pLock, &sFileBlock);
	if(lResult != DOSFALSE) {
		lResult = ExNext(pLock, &sFileBlock); // Skip dir name
		UWORD i = 0;
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
	systemUnuse();
}

static void mapListSelect(UWORD uwIdx) {
	mapInit(s_sMapList.pMaps[uwIdx].szFileName);
	systemUse();
	minimapDraw(g_pMenuBuffer->pBuffer, &g_sMap);
	systemUnuse();
}

static void mapListOnBtnStart(void) {
	g_isLocalBot = 0;
	gamePopState(); // From menu substate
	gameChangeState(gsGameCreate, gsGameLoop, gsGameDestroy);
}

static void mapListOnBtnBack(void) {
	gameChangeState(menuMainCreate, menuLoop, menuMainDestroy);
}

static void mapListOnMapChange(void) {
	mapListSelect(s_pListCtl->uwEntrySel);
}

void mapListCreate(void) {
	systemUse();
	logBlockBegin("mapListCreate()");
	// Clear bg
	blitRect(
		g_pMenuBuffer->pBuffer, 0, 0,
		(WORD)(bitmapGetByteWidth(g_pMenuBuffer->pBuffer) << 3),
		(WORD)(g_pMenuBuffer->pBuffer->Rows),
		0
	);
	blitWait();

	buttonListCreate(10, g_pMenuBuffer->pBuffer, g_pMenuFont);

	mapListPrepareList();

	s_pListCtl = listCtlCreate(
		g_pMenuBuffer->pBuffer, 10, 10, 100, 200,
		g_pMenuFont, s_sMapList.uwMapCount,
		mapListOnMapChange
	);
	for(UWORD i = 0; i != s_sMapList.uwMapCount; ++i) {
		s_sMapList.pMaps[i].szFileName[strlen(s_sMapList.pMaps[i].szFileName)-5] = 0;
		listCtlAddEntry(s_pListCtl, s_sMapList.pMaps[i].szFileName);
		s_sMapList.pMaps[i].szFileName[strlen(s_sMapList.pMaps[i].szFileName)] = '.';
	}
	listCtlDraw(s_pListCtl);

	buttonAdd(220, 200, 80, 16, "Play", mapListOnBtnStart);
	buttonAdd(220, 220, 80, 16, "Back", mapListOnBtnBack);
	buttonDrawAll();

	blitRect(
		g_pMenuBuffer->pBuffer,
		MAPLIST_MINIMAP_X - 1, MAPLIST_MINIMAP_Y - 1,
		MAPLIST_MINIMAP_WIDTH + 2, MAPLIST_MINIMAP_WIDTH + 2,
		MAPLIST_MINIMAP_BORDER
	);
	mapListSelect(s_pListCtl->uwEntrySel);
	logBlockEnd("mapListCreate()");
	systemUnuse();
}

void mapListLoop(void) {
	if(keyUse(KEY_ESCAPE)) {
		gameChangeState(menuMainCreate, menuLoop, menuMainDestroy);
		return;
	}

	if(mouseUse(MOUSE_PORT_1, MOUSE_LMB))
		if(!buttonProcessClick(mouseGetX(MOUSE_PORT_1), mouseGetY(MOUSE_PORT_1)))
			listCtlProcessClick(s_pListCtl, mouseGetX(MOUSE_PORT_1), mouseGetY(MOUSE_PORT_1));

	menuProcess();
}

void mapListDestroy(void) {
	systemUse();
	logBlockBegin("mapListDestroy()");
	memFree(s_sMapList.pMaps, s_sMapList.uwMapCount * sizeof(tMapListEntry));
	listCtlDestroy(s_pListCtl);
	buttonListDestroy();
	logBlockEnd("mapListDestroy()");
	systemUnuse();
}
