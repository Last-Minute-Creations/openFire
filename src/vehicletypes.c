#include "vehicletypes.h"
#include <math.h>
#include <ace/utils/chunky.h>
#include <ace/libfixmath/fix16.h>
#include "gamestates/game/bob.h"
#include "gamestates/initloading/worker.h"

float g_pSin[128];
tUwCoordYX g_pTurretCoords[VEHICLE_BODY_ANGLE_COUNT];
tVehicleType g_pVehicleTypes[VEHICLE_TYPE_COUNT];

/**
 *  Loads bob gfx data source from appropriate files & prepares rotated frames.
 *  @param szName     Name of vehicle to be loaded.
 *  @param pBobSource Pointer to bob source to be filled.
 *  @return Non-zero on success, otherwise zero.
 *  
 *  @todo Make it accept bitmaps wider than 32px?
 */
UWORD vehicleTypeBobSourceLoad(char *szName, tBobSource *pBobSource, BYTE *pProgress) {
	UBYTE *pChunkySrc;
	UBYTE *pChunkyRotated;
	char szFullFileName[50];
	UBYTE ubFrame;
	FILE *pMaskFile;
	UWORD x,y;
	tBitMap *pBitmap;
	tBitmapMask *pMask;
	UWORD uwFrameOffs;
	
	if(pProgress)
		*pProgress = -1;

	logBlockBegin(
		"vehicleBobSourceLoad(szName: %s, pBobSource: %p)",
		szName, pBobSource
	);
	
	pChunkySrc = memAllocFast(VEHICLE_BODY_WIDTH * VEHICLE_BODY_HEIGHT);
	pChunkyRotated = memAllocFast(VEHICLE_BODY_WIDTH * VEHICLE_BODY_HEIGHT);
	
	// Create huge-ass bitmap for all frames
	pBitmap = bitmapCreate(
		VEHICLE_BODY_WIDTH, VEHICLE_BODY_HEIGHT * VEHICLE_BODY_ANGLE_COUNT, 5, 0
	);
	if(!pBitmap) {
		logWrite("Couldn't allocate bitmap\n");
		goto fail;
	}
	
	// Create huge-ass mask
	pMask = bitmapMaskCreate(
		VEHICLE_BODY_WIDTH, VEHICLE_BODY_HEIGHT * VEHICLE_BODY_ANGLE_COUNT
	);
	if(!pMask) {
		logWrite("Couldn't allocate vehicle mask\n");
		goto fail;
	}
	pBobSource->pBitmap = pBitmap;
	pBobSource->pMask = pMask;
	
	// Load source bitmap as first frame
	strcpy(szFullFileName, "data/vehicles/");
	strcat(szFullFileName, szName);
	strcat(szFullFileName, ".bm");
	bitmapLoadFromFile(pBitmap, szFullFileName, 0, 0);
	logWrite("Read first frame\n");

	// Load first frame's mask
	strcpy(szFullFileName, "data/vehicles/");
	strcat(szFullFileName, szName);
	strcat(szFullFileName, ".msk");
	pMaskFile = fopen(szFullFileName, "rb");
	if(!pMaskFile) {
		logWrite("Couldn't open mask file %s\n", szFullFileName);
		goto fail;
	}
	fseek(pMaskFile, 2*sizeof(UWORD), SEEK_CUR);
	fread(
		pMask->pData, sizeof(UWORD),
		(VEHICLE_BODY_WIDTH>>4) * (VEHICLE_BODY_HEIGHT),
		pMaskFile
	);
	fclose(pMaskFile);
	pMaskFile = 0;
	logWrite("Read first frame mask\n");
	if(pProgress)
		*pProgress = 0;

	// Convert first frame & its mask to 6bpp chunky
	UWORD uwMaskChunk;
	for(y = 0; y != VEHICLE_BODY_HEIGHT; ++y) {
		// Read bitmap row to chunky
		chunkyFromPlanar16(pBitmap,  0, y, &pChunkySrc[y*VEHICLE_BODY_WIDTH]);
		if(VEHICLE_BODY_WIDTH > 16)
			chunkyFromPlanar16(pBitmap, 16, y, &pChunkySrc[y*VEHICLE_BODY_WIDTH+16]);
		
		// Add mask bit to chunky pixels
		uwMaskChunk = pMask->pData[y*(VEHICLE_BODY_WIDTH>>4)];
		for(x = 0; x != 16; ++x) {
			if(uwMaskChunk & (1 << 15))
				pChunkySrc[y*VEHICLE_BODY_WIDTH + x] |= 1 << 5;
			uwMaskChunk <<= 1;
		}
		if(VEHICLE_BODY_WIDTH > 16) {
			uwMaskChunk = pMask->pData[y*(VEHICLE_BODY_WIDTH>>4) + 1];
			for(x = 0; x != 16; ++x) {
				if(uwMaskChunk & (1 << 15))
					pChunkySrc[y*VEHICLE_BODY_WIDTH + 16 + x] |= 1 << 5;
				uwMaskChunk <<= 1;
			}
		}
	}
	
	for(ubFrame = 1; ubFrame != VEHICLE_BODY_ANGLE_COUNT; ++ubFrame) {
		// Rotate chunky source
		chunkyRotate(
			pChunkySrc, pChunkyRotated,
			//-2*M_PI*ubFrame/VEHICLE_BODY_ANGLE_COUNT,
			fix16_from_float(-2*M_PI*ubFrame/VEHICLE_BODY_ANGLE_COUNT),
			0,
			VEHICLE_BODY_WIDTH, VEHICLE_BODY_HEIGHT
		);
		
		// Convert rotated chunky frame to planar on huge-ass bitmap
		for(y = 0; y != VEHICLE_BODY_HEIGHT; ++y) {
			for(x = 0; x != (VEHICLE_BODY_WIDTH>>4); ++x)
				chunkyToPlanar16(
					&pChunkyRotated[(y*VEHICLE_BODY_WIDTH) + (x<<4)],
					x<<4, y + VEHICLE_BODY_HEIGHT*ubFrame,
					pBitmap
				);
		}

		// Extract mask from rotated chunky & write it to planar mask
		uwMaskChunk = 0;
		uwFrameOffs = ubFrame * VEHICLE_BODY_HEIGHT * (VEHICLE_BODY_WIDTH>>4);
		for(y = 0; y != VEHICLE_BODY_HEIGHT; ++y) {
			for(x = 0; x != VEHICLE_BODY_WIDTH; ++x) {
				uwMaskChunk <<= 1;
				if(pChunkyRotated[x + y*VEHICLE_BODY_WIDTH] & (1 << 5))
					uwMaskChunk = (uwMaskChunk | 1);
				if((x & 15) == 15) {
					pMask->pData[uwFrameOffs + y*(VEHICLE_BODY_WIDTH>>4) + (x>>4)] = uwMaskChunk;
					uwMaskChunk = 0;
				}
			}
		}
		if(pProgress)
			*pProgress = ubFrame;
	}
	
	memFree(pChunkySrc, VEHICLE_BODY_WIDTH * VEHICLE_BODY_HEIGHT);
	memFree(pChunkyRotated, VEHICLE_BODY_WIDTH * VEHICLE_BODY_HEIGHT);
	logBlockEnd("vehicleBobSourceLoad()");
	if(pProgress)
		*pProgress = 0;
	return 1;
fail:
	if(pMaskFile)
		fclose(pMaskFile);
	if(pMask)
		bitmapMaskDestroy(pMask);
	if(pBitmap)
		bitmapDestroy(pBitmap);
	memFree(pChunkySrc, VEHICLE_BODY_WIDTH * VEHICLE_BODY_HEIGHT);
	memFree(pChunkyRotated, VEHICLE_BODY_WIDTH * VEHICLE_BODY_HEIGHT);
	logBlockEnd("vehicleBobSourceLoad()");
	return 0;
}

void vehicleTypeGenerateRotatedCollisions(tBCoordYX pCollisions[][8]) {
	logBlockBegin(
		"vehicleTypeGenerateRotatedCollisions(pCollisions: %p)", pCollisions
	);
	UBYTE p, i;
	float fAng;
	for(i = VEHICLE_BODY_ANGLE_COUNT; i--;) {
		fAng = i*2*M_PI/VEHICLE_BODY_ANGLE_COUNT;
		for(p = 0; p != 8; ++p) {
			pCollisions[i][p].bX =
				+ (pCollisions[0][p].bX-VEHICLE_BODY_WIDTH/2)*cos(fAng)
				- (pCollisions[0][p].bY-VEHICLE_BODY_HEIGHT/2)*sin(fAng)
				+ VEHICLE_BODY_WIDTH/2;
			pCollisions[i][p].bY =
				+ (pCollisions[0][p].bX-VEHICLE_BODY_WIDTH/2)*sin(fAng)
				+ (pCollisions[0][p].bY-VEHICLE_BODY_HEIGHT/2)*cos(fAng)
				+ VEHICLE_BODY_HEIGHT/2;
		}
	}
	logBlockEnd("vehicleTypeGenerateRotatedCollisions()");
}

/**
 *  Generates vehicle type defs.
 *  This fn fills g_pVehicleTypes array
 *  @todo Chopper
 *  @todo ASV
 */
void vehicleTypesCreate(BYTE *pProgress) {
	tVehicleType *pType;
	UBYTE i;
	
	logBlockBegin("vehicleTypesCreate");

	// Tank turret coords
	logWrite("Generating tank turret coords...\n");
	UBYTE ubStartX = 9;
	UBYTE ubStartY = 16;
	for(i = 0; i != 64; ++i) {
		float fAng, fCos, fSin;
		fAng = i*2*M_PI/64;
		fCos = cos(fAng);
		fSin = sin(fAng);
		g_pTurretCoords[i].sUwCoord.uwX = (ubStartX-16)*fCos - (ubStartY-16)*fSin + 16;
		g_pTurretCoords[i].sUwCoord.uwY = (ubStartX-16)*fSin + (ubStartY-16)*fCos + 16;
	}
	g_ubWorkerStep += 5;

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
	vehicleTypeBobSourceLoad("tank", &pType->sMainSource, &pProgress[0]);
	g_ubWorkerStep += 10;
	vehicleTypeBobSourceLoad("tankturret", &pType->sAuxSource, &pProgress[1]);
	g_ubWorkerStep += 10;
	
	// Tank collision coords
	logWrite("Generating tank coords...\n");
	pType->pCollisionPts[0][0].bX = 2;  pType->pCollisionPts[0][0].bY = 6;
	pType->pCollisionPts[0][1].bX = 15; pType->pCollisionPts[0][1].bY = 6;
	pType->pCollisionPts[0][2].bX = 29; pType->pCollisionPts[0][2].bY = 6;
	pType->pCollisionPts[0][3].bX = 2;  pType->pCollisionPts[0][3].bY = 15;
	pType->pCollisionPts[0][4].bX = 29; pType->pCollisionPts[0][4].bY = 15;
	pType->pCollisionPts[0][5].bX = 2;  pType->pCollisionPts[0][5].bY = 25;
	pType->pCollisionPts[0][6].bX = 15; pType->pCollisionPts[0][6].bY = 25;
	pType->pCollisionPts[0][7].bX = 29; pType->pCollisionPts[0][7].bY = 25;
	vehicleTypeGenerateRotatedCollisions(pType->pCollisionPts);
	g_ubWorkerStep += 5;
	
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
	vehicleTypeBobSourceLoad("jeep", &pType->sMainSource, &pProgress[2]);
	pType->sAuxSource.pBitmap = 0;
	pType->sAuxSource.pMask = 0;
	g_ubWorkerStep += 10;
	
	// Jeep collision coords
	logWrite("Generating jeep coords...\n");
	pType->pCollisionPts[0][0].bX = 8;  pType->pCollisionPts[0][0].bY = 11;
	pType->pCollisionPts[0][1].bX = 16; pType->pCollisionPts[0][1].bY = 11;
	pType->pCollisionPts[0][2].bX = 25; pType->pCollisionPts[0][2].bY = 11;
	pType->pCollisionPts[0][3].bX = 8;  pType->pCollisionPts[0][3].bY = 16;
	pType->pCollisionPts[0][4].bX = 25; pType->pCollisionPts[0][4].bY = 16;
	pType->pCollisionPts[0][5].bX = 8;  pType->pCollisionPts[0][5].bY = 20;
	pType->pCollisionPts[0][6].bX = 16; pType->pCollisionPts[0][6].bY = 20;
	pType->pCollisionPts[0][7].bX = 25; pType->pCollisionPts[0][7].bY = 20;
	vehicleTypeGenerateRotatedCollisions(pType->pCollisionPts);
	g_ubWorkerStep += 5;
		
	logBlockEnd("vehicleTypesCreate");
}

/**
 *  @todo ASV
 *  @todo Chopper
 *  @todo loop
 */
void vehicleTypesDestroy(void) {
	tVehicleType *pType;
	
	logBlockBegin("vehicleTypesDestroy()");
	
	pType = &g_pVehicleTypes[VEHICLE_TYPE_TANK];
	bitmapDestroy(pType->sMainSource.pBitmap);
	bitmapMaskDestroy(pType->sMainSource.pMask);
	if(pType->sAuxSource.pBitmap) {
		bitmapDestroy(pType->sAuxSource.pBitmap);
		bitmapMaskDestroy(pType->sAuxSource.pMask);
	}
	
	pType = &g_pVehicleTypes[VEHICLE_TYPE_JEEP];
	bitmapDestroy(pType->sMainSource.pBitmap);
	bitmapMaskDestroy(pType->sMainSource.pMask);
	if(pType->sAuxSource.pBitmap) {
		bitmapDestroy(pType->sAuxSource.pBitmap);
		bitmapMaskDestroy(pType->sAuxSource.pMask);
	}
	
	logBlockEnd("vehicleTypesDestroy()");
}
