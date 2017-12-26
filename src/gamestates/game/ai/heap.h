#ifndef GUARD_OF_GAMESTATES_GAME_AI_HEAP_H
#define GUARD_OF_GAMESTATES_GAME_AI_HEAP_H

#include <ace/types.h>

typedef struct _tHeapEntry {
	UWORD uwPriority;
	void *pData;
} tHeapEntry;

typedef struct _tHeap {
	UWORD uwMaxEntries;
	UWORD uwCount;
	tHeapEntry *pEntries;
} tHeap;

tHeap *heapCreate(
	IN UWORD uwMaxEntries
);

void heapDestroy(
	IN tHeap *pHeap
);

void heapPush(
	IN tHeap *pHeap,
	IN void *pData,
	IN UWORD uwPriority
);

void *heapPop(
	IN tHeap *pHeap
);

static inline void heapClear(tHeap *pHeap) {
	pHeap->uwCount = 0;
}

#endif // GUARD_OF_GAMESTATES_GAME_AI_HEAP_H
