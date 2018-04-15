#include "vehicletypes.h"
#include <ace/managers/blit.h>
#include <ace/utils/chunky.h>
#include <fixmath/fix16.h>
#include "gamestates/game/bob.h"
#include "gamestates/game/gamemath.h"
#include "gamestates/precalc/precalc.h"
#include "adler32.h"

tVehicleType g_pVehicleTypes[VEHICLE_TYPE_COUNT];

tBitMap *vehicleTypeGenerateRotatedFrames(char *szPath) {
	char szBitmapFileName[100], szChecksumFileName[100];
	tBitMap *pBitmap;

	logBlockBegin("vehicleTypeGenerateRotatedFrames(szPath: %s)", szPath);

	// Calc source frame checksum
	sprintf(szBitmapFileName, "data/%s", szPath);
	ULONG ulAdlerBm = adler32file(szBitmapFileName);

	// Check for precalc
	sprintf(szChecksumFileName, "precalc/%s.adl", szPath);
	sprintf(szBitmapFileName, "precalc/%s", szPath);

	tFile *pBitmapFile = fileOpen(szBitmapFileName, "rb");
	tFile *pChecksumFile = fileOpen(szChecksumFileName, "rb");
	if(pChecksumFile && pBitmapFile) {
		ULONG ulPrevAdlerBm;
		fileRead(pChecksumFile, &ulPrevAdlerBm, sizeof(ULONG));
		fileClose(pChecksumFile);
		fileClose(pBitmapFile);
		// Check if adler is same
		if(ulAdlerBm == ulPrevAdlerBm) {
			logWrite("Loading from cache...\n");
			pBitmap = bitmapCreateFromFile(szBitmapFileName);
			logBlockEnd("vehicleTypeGenerateRotatedFrames()");
			return pBitmap;
		}
		else {
			logWrite("WARN: Adler mismatch for %s!\n", szChecksumFileName);
		}
	}
	else {
		if(pBitmapFile) {
			fileClose(pBitmapFile);
		}
		if(pChecksumFile) {
			fileClose(pChecksumFile);
		}
	}

	// Load first frame to determine sizes
	sprintf(szBitmapFileName, "data/%s", szPath);
	tBitMap *pFirstFrame = bitmapCreateFromFile(szBitmapFileName);
	logWrite("Read first frame\n");
	UWORD uwFrameWidth = bitmapGetByteWidth(pFirstFrame) << 3;

	UBYTE *pChunkySrc = memAllocFast(uwFrameWidth * uwFrameWidth);
	UBYTE *pChunkyRotated = memAllocFast(uwFrameWidth * uwFrameWidth);

	// Create huge-ass bitmap for all frames
	UBYTE ubFlags = 0;
	if(bitmapIsInterleaved(pFirstFrame))
		ubFlags = BMF_INTERLEAVED;
	pBitmap = bitmapCreate(
		uwFrameWidth, uwFrameWidth * VEHICLE_BODY_ANGLE_COUNT,
		pFirstFrame->Depth, ubFlags
	);
	if(!pBitmap) {
		logWrite("ERR: Couldn't allocate bitmap\n");
		goto fail;
	}

	// Copy first frame to main bitmap
	blitCopyAligned(pFirstFrame, 0, 0, pBitmap, 0, 0, uwFrameWidth, uwFrameWidth);
	bitmapDestroy(pFirstFrame);

	// Convert first frame to chunky
	for(UWORD y = 0; y != uwFrameWidth; ++y) {
		// Read bitmap row to chunky
		chunkyFromPlanar16(pBitmap,  0, y, &pChunkySrc[y*uwFrameWidth]);
		if(VEHICLE_BODY_WIDTH > 16)
			chunkyFromPlanar16(pBitmap, 16, y, &pChunkySrc[y*uwFrameWidth+16]);
	}

	for(FUBYTE fubFrame = 1; fubFrame != VEHICLE_BODY_ANGLE_COUNT; ++fubFrame) {
		// Rotate chunky source
		UBYTE ubAngle = ANGLE_360 - (fubFrame<<1);
		chunkyRotate(
			pChunkySrc, pChunkyRotated,	csin(ubAngle), ccos(ubAngle),
			0, uwFrameWidth, uwFrameWidth
		);

		// Convert rotated chunky frame to planar on huge-ass bitmap
		for(UWORD y = 0; y != uwFrameWidth; ++y) {
			for(UWORD x = 0; x != (uwFrameWidth>>4); ++x)
				chunkyToPlanar16(
					&pChunkyRotated[(y*uwFrameWidth) + (x<<4)],
					x<<4, y + uwFrameWidth*fubFrame,
					pBitmap
				);
		}
	}

	memFree(pChunkySrc, uwFrameWidth * uwFrameWidth);
	memFree(pChunkyRotated, uwFrameWidth * uwFrameWidth);
	sprintf(szBitmapFileName, "precalc/%s", szPath);
	bitmapSave(pBitmap, szBitmapFileName);
	pChecksumFile = fileOpen(szChecksumFileName, "wb");
	fileWrite(pChecksumFile, &ulAdlerBm, sizeof(ULONG));
	fileClose(pChecksumFile);
	logBlockEnd("vehicleTypeGenerateRotatedFrames()");
	return pBitmap;
fail:
	if(pBitmap)
		bitmapDestroy(pBitmap);
	memFree(pChunkySrc, uwFrameWidth * uwFrameWidth);
	memFree(pChunkyRotated, uwFrameWidth * uwFrameWidth);
	logBlockEnd("vehicleTypeGenerateRotatedFrames()");
	return 0;
}

static void vehicleTypeFramesCreate(tVehicleType *pType, char *szVehicleName, UBYTE isAux) {
	char szFilePath[100];

	sprintf(szFilePath, "vehicles/%s/main_blue.bm", szVehicleName);
	pType->pMainFrames[TEAM_BLUE] = vehicleTypeGenerateRotatedFrames(szFilePath);

	sprintf(szFilePath, "vehicles/%s/main_red.bm", szVehicleName);
	pType->pMainFrames[TEAM_RED] = vehicleTypeGenerateRotatedFrames(szFilePath);

	sprintf(szFilePath, "vehicles/%s/main_mask.bm", szVehicleName);
	pType->pMainMask = vehicleTypeGenerateRotatedFrames(szFilePath);
	if(isAux) {
		sprintf(szFilePath, "vehicles/%s/aux_blue.bm", szVehicleName);
		pType->pAuxFrames[TEAM_BLUE] = vehicleTypeGenerateRotatedFrames(szFilePath);

		sprintf(szFilePath, "vehicles/%s/aux_red.bm", szVehicleName);
		pType->pAuxFrames[TEAM_RED] = vehicleTypeGenerateRotatedFrames(szFilePath);

		sprintf(szFilePath, "vehicles/%s/aux_mask.bm", szVehicleName);
		pType->pAuxMask = vehicleTypeGenerateRotatedFrames(szFilePath);
	}
	else {
		pType->pAuxFrames[TEAM_BLUE] = 0;
		pType->pAuxFrames[TEAM_RED] = 0;
		pType->pAuxMask = 0;
	}
}

static void vehicleTypeGenerateRotatedCollisions(tCollisionPts *pFrameCollisions) {
	logBlockBegin(
		"vehicleTypeGenerateRotatedCollisions(pFrameCollisions: %p)", pFrameCollisions
	);
	fix16_t fHalf = fix16_one >> 1;
	for(FUBYTE fubFrame = VEHICLE_BODY_ANGLE_COUNT; fubFrame--;) {
		FUBYTE fubAngle = fubFrame << 1;
		tCollisionPts *pNewCollisions = &pFrameCollisions[fubFrame];
		for(FUBYTE fubPoint = 0; fubPoint != 8; ++fubPoint) {
			pNewCollisions->pPts[fubPoint].bX = fix16_to_int(fix16_sub(
				pFrameCollisions[0].pPts[fubPoint].bX * ccos(fubAngle),
				pFrameCollisions[0].pPts[fubPoint].bY * csin(fubAngle)
			) + fHalf);
			pNewCollisions->pPts[fubPoint].bY = fix16_to_int(fix16_add(
				pFrameCollisions[0].pPts[fubPoint].bX * csin(fubAngle),
				pFrameCollisions[0].pPts[fubPoint].bY * ccos(fubAngle)
			) + fHalf);
		}

		pNewCollisions->bLeftmost = MIN(
			pNewCollisions->pPts[0].bX, MIN(
				pNewCollisions->pPts[2].bX, MIN(
					pNewCollisions->pPts[5].bX,
					pNewCollisions->pPts[7].bX))
		);
		pNewCollisions->bRightmost = MAX(
			pNewCollisions->pPts[0].bX, MAX(
				pNewCollisions->pPts[2].bX, MAX(
					pNewCollisions->pPts[5].bX,
					pNewCollisions->pPts[7].bX))
		);
		pNewCollisions->bTopmost = MIN(
			pNewCollisions->pPts[0].bY, MIN(
				pNewCollisions->pPts[2].bY, MIN(
					pNewCollisions->pPts[5].bY,
					pNewCollisions->pPts[7].bY))
		);
		pNewCollisions->bBottommost = MAX(
			pNewCollisions->pPts[0].bY, MAX(
				pNewCollisions->pPts[2].bY, MAX(
					pNewCollisions->pPts[5].bY,
					pNewCollisions->pPts[7].bY))
		);
	}
	logBlockEnd("vehicleTypeGenerateRotatedCollisions()");
}

static tBobFrameOffset *vehicleTypeFramesGenerateOffsets(tBitMap *pMask) {
	tBobFrameOffset *pOffsets = memAllocFast(
		sizeof(tBobFrameOffset) * VEHICLE_BODY_ANGLE_COUNT
	);

	const UWORD uwWordsPerRow = pMask->BytesPerRow >> 1;

	for(uint8_t i = 0; i != VEHICLE_BODY_ANGLE_COUNT; ++i) {
		UBYTE ubFirst = 0xFF;
		UBYTE ubLast = 0;
		for(uint8_t y = 0; y != VEHICLE_BODY_HEIGHT; ++y) {
			for(uint8_t x = 0; x <= VEHICLE_BODY_WIDTH; x += 16) {
				UWORD uwWordOffs = (i*VEHICLE_BODY_HEIGHT + y)*uwWordsPerRow + (x>>4);
				if(((UWORD*)pMask->Planes[0])[uwWordOffs]) {
					if(y < ubFirst)
						ubFirst = y;
					if(y > ubLast)
						ubLast = y;
				}
			}
		}
		pOffsets[i].uwDy = ubFirst;
		pOffsets[i].uwHeight = ubLast - ubFirst+1;
	}
	return pOffsets;
}

/**
 *  Generates vehicle type defs.
 *  This fn fills g_pVehicleTypes array
 *  @todo Chopper
 *  @todo ASV
 */
void vehicleTypesCreate(void) {
	tVehicleType *pType;

	logBlockBegin("vehicleTypesCreate");

	// Tank
	pType = &g_pVehicleTypes[VEHICLE_TYPE_TANK];
	pType->ubFwdSpeed = 1;
	pType->ubBwSpeed = 1;
	pType->ubRotSpeed = 2;
	pType->ubRotSpeedDiv = 4;
	pType->ubMaxBaseAmmo = 100;
	pType->ubMaxSuperAmmo = 0;
	pType->ubMaxFuel = 100;
	pType->ubMaxLife = 100;

	precalcIncreaseProgress(15, "Generating tank frames");
	vehicleTypeFramesCreate(pType, "tank", 1);

	// Tank collision coords
	precalcIncreaseProgress(15, "Calculating tank collision coords");
	pType->pCollisionPts[0].pPts[0].bX = 6;  pType->pCollisionPts[0].pPts[0].bY = 8;
	pType->pCollisionPts[0].pPts[1].bX = 17; pType->pCollisionPts[0].pPts[1].bY = 8;
	pType->pCollisionPts[0].pPts[2].bX = 29; pType->pCollisionPts[0].pPts[2].bY = 8;
	pType->pCollisionPts[0].pPts[3].bX = 6;  pType->pCollisionPts[0].pPts[3].bY = 16;
	pType->pCollisionPts[0].pPts[4].bX = 29; pType->pCollisionPts[0].pPts[4].bY = 16;
	pType->pCollisionPts[0].pPts[5].bX = 6;  pType->pCollisionPts[0].pPts[5].bY = 24;
	pType->pCollisionPts[0].pPts[6].bX = 17; pType->pCollisionPts[0].pPts[6].bY = 24;
	pType->pCollisionPts[0].pPts[7].bX = 29; pType->pCollisionPts[0].pPts[7].bY = 24;
	for(FUBYTE i = 0; i != 8; ++i) {
		pType->pCollisionPts[0].pPts[i].bX -= VEHICLE_BODY_WIDTH/2;
		pType->pCollisionPts[0].pPts[i].bY -= VEHICLE_BODY_HEIGHT/2;
	}

	vehicleTypeGenerateRotatedCollisions(pType->pCollisionPts);
	pType->pMainFrameOffsets = vehicleTypeFramesGenerateOffsets(pType->pMainMask);
	if(pType->pAuxMask)
		pType->pAuxFrameOffsets = vehicleTypeFramesGenerateOffsets(pType->pAuxMask);
	else
		pType->pAuxFrameOffsets = 0;

	// Jeep
	pType = &g_pVehicleTypes[VEHICLE_TYPE_JEEP];
	pType->ubFwdSpeed = 2;
	pType->ubBwSpeed = 1;
	pType->ubRotSpeed = 2;
	pType->ubRotSpeedDiv = 1;
	pType->ubMaxBaseAmmo = 20;
	pType->ubMaxSuperAmmo = 0;
	pType->ubMaxFuel = 100;
	pType->ubMaxLife = 1;

	precalcIncreaseProgress(15, "Generating jeep frames");
	vehicleTypeFramesCreate(pType, "jeep", 0);

	// Jeep collision coords
	precalcIncreaseProgress(15, "Calculating jeep collision coords");
	pType->pCollisionPts[0].pPts[0].bX = 8;  pType->pCollisionPts[0].pPts[0].bY = 11;
	pType->pCollisionPts[0].pPts[1].bX = 16; pType->pCollisionPts[0].pPts[1].bY = 11;
	pType->pCollisionPts[0].pPts[2].bX = 25; pType->pCollisionPts[0].pPts[2].bY = 11;
	pType->pCollisionPts[0].pPts[3].bX = 8;  pType->pCollisionPts[0].pPts[3].bY = 16;
	pType->pCollisionPts[0].pPts[4].bX = 25; pType->pCollisionPts[0].pPts[4].bY = 16;
	pType->pCollisionPts[0].pPts[5].bX = 8;  pType->pCollisionPts[0].pPts[5].bY = 20;
	pType->pCollisionPts[0].pPts[6].bX = 16; pType->pCollisionPts[0].pPts[6].bY = 20;
	pType->pCollisionPts[0].pPts[7].bX = 25; pType->pCollisionPts[0].pPts[7].bY = 20;
	for(FUBYTE i = 0; i != 8; ++i) {
		pType->pCollisionPts[0].pPts[i].bX -= VEHICLE_BODY_WIDTH/2;
		pType->pCollisionPts[0].pPts[i].bY -= VEHICLE_BODY_HEIGHT/2;
	}
	vehicleTypeGenerateRotatedCollisions(pType->pCollisionPts);
	pType->pMainFrameOffsets = vehicleTypeFramesGenerateOffsets(pType->pMainMask);
	if(pType->pAuxMask)
		pType->pAuxFrameOffsets = vehicleTypeFramesGenerateOffsets(pType->pAuxMask);
	else
		pType->pAuxFrameOffsets = 0;

	logBlockEnd("vehicleTypesCreate");
}

static void vehicleTypeUnloadFrameData(tVehicleType *pType) {
	memFree(
		pType->pMainFrameOffsets,
		VEHICLE_BODY_ANGLE_COUNT * sizeof(tBobFrameOffset)
	);
	if(pType->pAuxFrameOffsets) {
		memFree(
			pType->pAuxFrameOffsets,
			VEHICLE_BODY_ANGLE_COUNT * sizeof(tBobFrameOffset)
		);
	}
	bitmapDestroy(pType->pMainFrames[TEAM_BLUE]);
	bitmapDestroy(pType->pMainFrames[TEAM_RED]);
	bitmapDestroy(pType->pMainMask);
	if(pType->pAuxFrames[TEAM_BLUE])
		bitmapDestroy(pType->pAuxFrames[TEAM_BLUE]);
	if(pType->pAuxFrames[TEAM_RED])
		bitmapDestroy(pType->pAuxFrames[TEAM_RED]);
	if(pType->pAuxMask)
		bitmapDestroy(pType->pAuxMask);
}

void vehicleTypesDestroy(void) {
	logBlockBegin("vehicleTypesDestroy()");

	// Free bob sources
	vehicleTypeUnloadFrameData(&g_pVehicleTypes[VEHICLE_TYPE_TANK]);

	vehicleTypeUnloadFrameData(&g_pVehicleTypes[VEHICLE_TYPE_JEEP]);
	// TODO ASV
	// TODO Chopper

	logBlockEnd("vehicleTypesDestroy()");
}
