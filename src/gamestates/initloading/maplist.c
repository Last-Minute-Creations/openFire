#include "gamestates/initloading/maplist.h"
#include <string.h>
#include <dos/dos.h>
#include <clib/dos_protos.h>
#include <ace/types.h>
#include <ace/managers/memory.h>

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

void mapListCreate(void) {
	// Get map count
	s_sMapList.uwMapCount = 0;
	BPTR pLock;
	struct FileInfoBlock sFileBlock;
	pLock = Lock("data/maps", ACCESS_READ);
	LONG lResult;
	lResult = Examine(pLock, &sFileBlock);
	while(lResult != DOSFALSE) {
		++s_sMapList.uwMapCount;
		lResult = ExNext(pLock, &sFileBlock);
	}
	UnLock(pLock);

	// Alloc map list
	s_sMapList.pMaps = memAllocFast(s_sMapList.uwMapCount * sizeof(tMapListEntry));
	pLock = Lock("data/maps", ACCESS_READ);
	UWORD i = 0;
	lResult = Examine(pLock, &sFileBlock);
	while(lResult != DOSFALSE) {
		memcpy(s_sMapList.pMaps[i].szFileName, sFileBlock.fib_FileName, MAPLIST_FILENAME_MAX);
		++s_sMapList.uwMapCount;
		++i;
		lResult = ExNext(pLock, &sFileBlock);
	}
	UnLock(pLock);
}

void mapListDestroy(void) {
	memFree(s_sMapList.pMaps, s_sMapList.uwMapCount * sizeof(tMapListEntry));
}
