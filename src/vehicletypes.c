#include "vehicletypes.h"
#include <ace/managers/blit.h>
#include <ace/utils/chunky.h>
#include <ace/libfixmath/fix16.h>
#include "gamestates/game/bob.h"
#include "gamestates/game/gamemath.h"
#include "gamestates/precalc/precalc.h"
#include "adler32.h"

tVehicleType g_pVehicleTypes[VEHICLE_TYPE_COUNT];

/**
 *  Loads bob gfx data source from appropriate files & prepares rotated frames.
 *  @param szName     Name of vehicle to be loaded.
 *  @param pBobSource Pointer to bob source to be filled.
 *  @return Non-zero on success, otherwise zero.
 *
 *  @todo Make it accept bitmaps wider than 32px?
 */
UWORD vehicleTypeBobSourceLoad(char *szName, tBobSource *pBobSource, UBYTE isWithMask) {
	UBYTE *pChunkySrc;
	UBYTE *pChunkyRotated;
	char szBitmapFileName[50], szMaskFileName[50], szChecksumFileName[50];
	UBYTE ubFrame;
	UWORD x,y;
	tBitMap *pBitmap;
	tBitmapMask *pMask;
	UWORD uwFrameOffs, uwFrameWidth;

	logBlockBegin(
		"vehicleTypeBobSourceLoad(szName: %s, pBobSource: %p, isWithMask: %hhu)",
		szName, pBobSource, isWithMask
	);

	// Calc source frame checksum
	sprintf(szBitmapFileName, "data/vehicles/%s.bm", szName);
	ULONG ulAdlerBm = adler32file(szBitmapFileName);
	ULONG ulAdlerMask = 0;
	if(isWithMask) {
		sprintf(szMaskFileName, "data/vehicles/%s.bm", szName);
		ulAdlerMask = adler32file(szMaskFileName);
	}

	// Check for precalc
	sprintf(szChecksumFileName, "precalc/%s.adl", szName);
	sprintf(szBitmapFileName, "precalc/%s.bm", szName);
	sprintf(szMaskFileName, "precalc/%s.msk", szName);
	FILE *pBitmapFile = fopen(szBitmapFileName, "rb");
	FILE *pMaskFile = 0;
	if(isWithMask)
		pMaskFile = fopen(szMaskFileName, "rb");
	FILE *pChecksumFile = fopen(szChecksumFileName, "rb");
	if(pChecksumFile && pBitmapFile && (!isWithMask || pMaskFile)) {
		ULONG ulPrevAdlerBm, ulPrevAdlerMask;
		fread(&ulPrevAdlerBm, sizeof(ULONG), 1, pChecksumFile);
		fread(&ulPrevAdlerMask, sizeof(ULONG), 1, pChecksumFile);
		fclose(pChecksumFile);
		fclose(pBitmapFile);
		if(isWithMask)
			fclose(pMaskFile);
		// Check if adler is same
		if(ulAdlerBm == ulPrevAdlerBm && ulAdlerMask == ulPrevAdlerMask) {
			logWrite("Loading from cache...\n");
			pBobSource->pBitmap = bitmapCreateFromFile(szBitmapFileName);
			if(isWithMask) {
				pBobSource->pMask = bitmapMaskCreateFromFile(szMaskFileName);
			}
			else
				pBobSource->pMask = 0;
			logBlockEnd("vehicleTypeBobSourceLoad()");
			return 1;
		}
	}
	else {
		if(pBitmapFile) fclose(pBitmapFile);
		if(pMaskFile) fclose(pMaskFile);
		if(pChecksumFile) fclose(pChecksumFile);
	}

	// Load first frame to determine sizes
	sprintf(szBitmapFileName, "data/vehicles/%s.bm", szName);
	tBitMap *pFirstFrame = bitmapCreateFromFile(szBitmapFileName);
	logWrite("Read first frame\n");
	uwFrameWidth = bitmapGetByteWidth(pFirstFrame)*8;

	pChunkySrc = memAllocFast(uwFrameWidth * uwFrameWidth);
	pChunkyRotated = memAllocFast(uwFrameWidth * uwFrameWidth);

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
	pBobSource->pBitmap = pBitmap;

	if(isWithMask) {
		// Create huge-ass mask
		pMask = bitmapMaskCreate(
			uwFrameWidth, uwFrameWidth * pBitmap->Depth * VEHICLE_BODY_ANGLE_COUNT
		);
		if(!pMask) {
			logWrite("ERR: Couldn't allocate vehicle mask\n");
			goto fail;
		}
		pBobSource->pMask = pMask;

		// Load first frame's mask
		sprintf(szMaskFileName, "data/vehicles/%s.msk", szName);
		pMaskFile = fopen(szMaskFileName, "rb");
		if(!pMaskFile) {
			logWrite("ERR: Couldn't open mask file %s\n", szMaskFileName);
			goto fail;
		}
		fseek(pMaskFile, 2*sizeof(UWORD), SEEK_CUR);
		fread(
			pMask->pData, sizeof(UWORD) * (pBitmap->BytesPerRow >> 1),
			uwFrameWidth, pMaskFile
		);
		fclose(pMaskFile);
		pMaskFile = 0;
		logWrite("Read first frame mask\n");
	}
	else
		pBobSource->pMask = 0;

	// Convert first frame & its mask to bpl+mask chunky
	UWORD uwMaskChunk;
	for(y = 0; y != uwFrameWidth; ++y) {
		// Read bitmap row to chunky
		chunkyFromPlanar16(pBitmap,  0, y, &pChunkySrc[y*uwFrameWidth]);
		if(VEHICLE_BODY_WIDTH > 16)
			chunkyFromPlanar16(pBitmap, 16, y, &pChunkySrc[y*uwFrameWidth+16]);

		if(isWithMask) {
			// Add mask bit to chunky pixels
			// TODO add x-coord loop so that it won't be hardcoded to 16/32px
			uwMaskChunk = pMask->pData[y * (pBitmap->BytesPerRow>>1)];
			for(x = 0; x != 16; ++x) {
				if(uwMaskChunk & (1 << 15))
					pChunkySrc[y*uwFrameWidth + x] |= 1 << pBitmap->Depth;
				uwMaskChunk <<= 1;
			}
			if(uwFrameWidth > 16) {
				uwMaskChunk = pMask->pData[y * (pBitmap->BytesPerRow>>1) + 1];
				for(x = 0; x != 16; ++x) {
					if(uwMaskChunk & (1 << 15))
						pChunkySrc[y*uwFrameWidth + 16 + x] |= 1 << pBitmap->Depth;
					uwMaskChunk <<= 1;
				}
			}
		}
	}

	for(ubFrame = 1; ubFrame != VEHICLE_BODY_ANGLE_COUNT; ++ubFrame) {
		// Rotate chunky source
		fix16_t fAngle = fix16_div(
			-2*ubFrame * fix16_pi,
			fix16_from_int(VEHICLE_BODY_ANGLE_COUNT)
		);
		chunkyRotate(
			pChunkySrc, pChunkyRotated,	fAngle, 0, uwFrameWidth, uwFrameWidth
		);

		// Convert rotated chunky frame to planar on huge-ass bitmap
		for(y = 0; y != uwFrameWidth; ++y) {
			for(x = 0; x != (uwFrameWidth>>4); ++x)
				chunkyToPlanar16(
					&pChunkyRotated[(y*uwFrameWidth) + (x<<4)],
					x<<4, y + uwFrameWidth*ubFrame,
					pBitmap
				);
		}

		if(isWithMask) {
			// Extract mask from rotated chunky & write it to planar mask
			uwMaskChunk = 0;
			uwFrameOffs = ubFrame * uwFrameWidth * (pBitmap->BytesPerRow>>1);
			for(y = 0; y != uwFrameWidth; ++y) {
				for(x = 0; x != uwFrameWidth; ++x) {
					uwMaskChunk <<= 1;
					if(pChunkyRotated[x + y*uwFrameWidth] & (1 << pBitmap->Depth))
						uwMaskChunk = (uwMaskChunk | 1);
					if((x & 15) == 15) {
						if(bitmapIsInterleaved(pBitmap))
							for(UWORD d = 0; d != pBitmap->Depth; ++d)
								pMask->pData[uwFrameOffs + y*(pBitmap->BytesPerRow>>1) + d*(uwFrameWidth>>4) + (x>>4)] = uwMaskChunk;
						else
							pMask->pData[uwFrameOffs + y*(pBitmap->BytesPerRow>>1) + (x>>4)] = uwMaskChunk;
						uwMaskChunk = 0;
					}
				}
			}
		}
	}

	memFree(pChunkySrc, uwFrameWidth * uwFrameWidth);
	memFree(pChunkyRotated, uwFrameWidth * uwFrameWidth);
	sprintf(szBitmapFileName, "precalc/%s.bm", szName);
	bitmapSave(pBobSource->pBitmap, szBitmapFileName);
	if(isWithMask) {
		sprintf(szMaskFileName, "precalc/%s.msk", szName);
		bitmapMaskSave(pBobSource->pMask, szMaskFileName);
	}
	pChecksumFile = fopen(szChecksumFileName, "wb");
	fwrite(&ulAdlerBm, sizeof(ULONG), 1, pChecksumFile);
	fwrite(&ulAdlerMask, sizeof(ULONG), 1, pChecksumFile);
	fclose(pChecksumFile);
	logBlockEnd("vehicleTypeBobSourceLoad()");
	return 1;
fail:
	if(pMaskFile)
		fclose(pMaskFile);
	if(pMask)
		bitmapMaskDestroy(pMask);
	if(pBitmap)
		bitmapDestroy(pBitmap);
	memFree(pChunkySrc, uwFrameWidth * uwFrameWidth);
	memFree(pChunkyRotated, uwFrameWidth * uwFrameWidth);
	logBlockEnd("vehicleTypeBobSourceLoad()");
	return 0;
}

void vehicleTypeGenerateRotatedCollisions(tBCoordYX pCollisions[][8]) {
	logBlockBegin(
		"vehicleTypeGenerateRotatedCollisions(pCollisions: %p)", pCollisions
	);
	for(FUBYTE fubFrame = VEHICLE_BODY_ANGLE_COUNT; fubFrame--;) {
		FUBYTE fubAngle = fubFrame << 1;
		for(FUBYTE fubPoint = 0; fubPoint != 8; ++fubPoint) {
			pCollisions[fubFrame][fubPoint].bX = fix16_to_int(fix16_sub(
				pCollisions[0][fubPoint].bX * ccos(fubAngle),
				pCollisions[0][fubPoint].bY * csin(fubAngle)
			));
			pCollisions[fubFrame][fubPoint].bY = fix16_to_int(fix16_add(
				pCollisions[0][fubPoint].bX * csin(fubAngle),
				pCollisions[0][fubPoint].bY * ccos(fubAngle)
			));
		}
	}
	logBlockEnd("vehicleTypeGenerateRotatedCollisions()");
}

void vehicleTypeSetBlankBobSource(tBobSource *pSource) {
	pSource->pBitmap = 0;
	pSource->pMask = 0;
}

/**
 *  Generates vehicle type defs.
 *  This fn fills g_pVehicleTypes array
 *  @todo Chopper
 *  @todo ASV
 */
void vehicleTypesCreate(void) {
	tVehicleType *pType;
	UBYTE i;

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

	precalcIncreaseProgress(5, "Generating blue tank frames");
	vehicleTypeBobSourceLoad("blue/tank", &pType->sMainSource[TEAM_BLUE], 1);

	precalcIncreaseProgress(5, "Generating blue tank turret frames");
	vehicleTypeBobSourceLoad("blue/tank_turret", &pType->sAuxSource[TEAM_BLUE], 1);

	precalcIncreaseProgress(5, "Generating red tank turret frames");
	vehicleTypeBobSourceLoad("red/tank", &pType->sMainSource[TEAM_RED], 1);

	precalcIncreaseProgress(5, "Generating red tank turret frames");
	vehicleTypeBobSourceLoad("red/tank_turret", &pType->sAuxSource[TEAM_RED], 1);

	// Tank collision coords
	precalcIncreaseProgress(5, "Calculating tank collision coords");
	pType->pCollisionPts[0][0].bX = 6;  pType->pCollisionPts[0][0].bY = 8;
	pType->pCollisionPts[0][1].bX = 17; pType->pCollisionPts[0][1].bY = 8;
	pType->pCollisionPts[0][2].bX = 29; pType->pCollisionPts[0][2].bY = 8;
	pType->pCollisionPts[0][3].bX = 6;  pType->pCollisionPts[0][3].bY = 16;
	pType->pCollisionPts[0][4].bX = 29; pType->pCollisionPts[0][4].bY = 16;
	pType->pCollisionPts[0][5].bX = 6;  pType->pCollisionPts[0][5].bY = 24;
	pType->pCollisionPts[0][6].bX = 17; pType->pCollisionPts[0][6].bY = 24;
	pType->pCollisionPts[0][7].bX = 29; pType->pCollisionPts[0][7].bY = 24;
	for(FUBYTE i = 0; i != 8; ++i) {
		pType->pCollisionPts[0][i].bX -= VEHICLE_BODY_WIDTH/2;
		pType->pCollisionPts[0][i].bY -= VEHICLE_BODY_HEIGHT/2;
	}
	vehicleTypeGenerateRotatedCollisions(pType->pCollisionPts);

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

	precalcIncreaseProgress(5, "Generating blue jeep frames");
	vehicleTypeBobSourceLoad("blue/jeep", &pType->sMainSource[TEAM_BLUE], 1);
	vehicleTypeSetBlankBobSource(&pType->sAuxSource[TEAM_BLUE]);

	precalcIncreaseProgress(5, "Generating red jeep frames");
	vehicleTypeBobSourceLoad("red/jeep", &pType->sMainSource[TEAM_RED], 1);
	vehicleTypeSetBlankBobSource(&pType->sAuxSource[TEAM_RED]);

	// Jeep collision coords
	precalcIncreaseProgress(5, "Calculating jeep collision coords");
	pType->pCollisionPts[0][0].bX = 8;  pType->pCollisionPts[0][0].bY = 11;
	pType->pCollisionPts[0][1].bX = 16; pType->pCollisionPts[0][1].bY = 11;
	pType->pCollisionPts[0][2].bX = 25; pType->pCollisionPts[0][2].bY = 11;
	pType->pCollisionPts[0][3].bX = 8;  pType->pCollisionPts[0][3].bY = 16;
	pType->pCollisionPts[0][4].bX = 25; pType->pCollisionPts[0][4].bY = 16;
	pType->pCollisionPts[0][5].bX = 8;  pType->pCollisionPts[0][5].bY = 20;
	pType->pCollisionPts[0][6].bX = 16; pType->pCollisionPts[0][6].bY = 20;
	pType->pCollisionPts[0][7].bX = 25; pType->pCollisionPts[0][7].bY = 20;
	for(FUBYTE i = 0; i != 8; ++i) {
		pType->pCollisionPts[0][i].bX -= VEHICLE_BODY_WIDTH/2;
		pType->pCollisionPts[0][i].bY -= VEHICLE_BODY_HEIGHT/2;
	}
	vehicleTypeGenerateRotatedCollisions(pType->pCollisionPts);

	logBlockEnd("vehicleTypesCreate");
}

void vehicleTypeBobSourceUnload(tBobSource *pSource) {
	if(pSource->pBitmap)
		bitmapDestroy(pSource->pBitmap);
	if(pSource->pMask)
		bitmapMaskDestroy(pSource->pMask);
}

void vehicleTypeUnloadAllBobSources(tVehicleType *pType) {
	vehicleTypeBobSourceUnload(&pType->sMainSource[TEAM_BLUE]);
	vehicleTypeBobSourceUnload(&pType->sAuxSource[TEAM_BLUE]);
	vehicleTypeBobSourceUnload(&pType->sMainSource[TEAM_RED]);
	vehicleTypeBobSourceUnload(&pType->sAuxSource[TEAM_RED]);
}

void vehicleTypesDestroy(void) {
	tVehicleType *pType;

	logBlockBegin("vehicleTypesDestroy()");

	// Free bob sources
	vehicleTypeUnloadAllBobSources(&g_pVehicleTypes[VEHICLE_TYPE_TANK]);
	vehicleTypeUnloadAllBobSources(&g_pVehicleTypes[VEHICLE_TYPE_JEEP]);
	// TODO ASV
	// TODO Chopper

	logBlockEnd("vehicleTypesDestroy()");
}
