#include "gamestates/game/ai/heap.h"
#include <ace/managers/log.h>
#include <ace/managers/memory.h>

tHeap *heapCreate(UWORD uwMaxEntries) {
	tHeap *pHeap = memAllocFast(sizeof(tHeap));
	pHeap->uwMaxEntries = uwMaxEntries;
	pHeap->uwCount = 0;
	pHeap->pEntries = memAllocFastClear(sizeof(tHeapEntry) * uwMaxEntries);
	return pHeap;
}

void heapDestroy(tHeap *pHeap) {
	memFree(pHeap->pEntries, pHeap->uwMaxEntries * sizeof(tHeapEntry));
	memFree(pHeap, sizeof(tHeap));
}

void heapPush(tHeap *pHeap, void *pData, UWORD uwPriority) {
	UWORD uwIdx = pHeap->uwCount++;
	tHeapEntry * const pEntries = pHeap->pEntries;

	if(pHeap->uwCount > pHeap->uwMaxEntries) {
		logWrite(
			"ERR: too much entries: %hu > %hu\n", pHeap->uwCount, pHeap->uwMaxEntries
		);
	}

	// Add the element to the bottom level of the heap.
	pEntries[uwIdx].uwPriority = uwPriority;
	pEntries[uwIdx].pData = pData;

	while(uwIdx) {
		UWORD uwParentIdx = (uwIdx - 1) >> 1;
		// Compare the added element with its parent
		if(pEntries[uwIdx].uwPriority >= pEntries[uwParentIdx].uwPriority) {
			// If they are in the correct order, stop.
			break;
		}
		// If not, swap the element with its parent and return to the previous step.
		tHeapEntry sParentEntry = pEntries[uwParentIdx];
		pEntries[uwParentIdx] = pEntries[uwIdx];
		pEntries[uwIdx] = sParentEntry;
		uwIdx = uwParentIdx;
	}
}

void *heapPop(tHeap *pHeap) {
	tHeapEntry * const pEntries = pHeap->pEntries;

	void *pRet = pEntries[0].pData;
	UWORD uwIdx = --pHeap->uwCount;
	if(!pHeap->uwCount)
		return pRet;


	// Replace the root of the heap with the last element on the last level.
	pEntries[0].pData = pEntries[uwIdx].pData;
	pEntries[0].uwPriority = pEntries[uwIdx].uwPriority;

	uwIdx = 0;
	UWORD uwChildIdx;
	while((uwChildIdx = (uwIdx<<1)+1) < pHeap->uwCount) {
		// Get the smaller child
		if(
			uwChildIdx < pHeap->uwCount-1 &&
			pEntries[uwChildIdx+1].uwPriority < pEntries[uwChildIdx].uwPriority
		) {
			++uwChildIdx;
		}

		// Compare the new root with smaller child
		if(pEntries[uwIdx].uwPriority < pEntries[uwChildIdx].uwPriority) {
			// If they are in the correct order, stop.
			break;
		}
		// If not, swap the element with smaller one of its children and continue.
		tHeapEntry sChildEntry = pEntries[uwChildIdx];
		pEntries[uwChildIdx] = pEntries[uwIdx];
		pEntries[uwIdx] = sChildEntry;
		uwIdx = uwChildIdx;
	}

	return pRet;
}
