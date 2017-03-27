#include "gamestates/game/vehicle.h"
#include <math.h>
#include <ace/managers/blit.h>
#include <ace/utils/bitmap.h>
#include <ace/utils/bitmapmask.h>
#include <ace/utils/chunky.h>
#include "gamestates/game/game.h"
#include "gamestates/game/team.h"
#include "gamestates/game/map.h"
#include "gamestates/game/projectile.h"

float g_pSin[128];

tUwCoordYX g_pTurretCoords[VEHICLE_BODY_ANGLE_COUNT];

void vehicleGenerateRotatedCollisions(tBCoordYX pCollisions[][8]) {
	logBlockBegin("vehicleGenerateRotatedCollisions(pCollisions: %p)", pCollisions);
	UBYTE p, i;
	float fAng;
	for(i = VEHICLE_BODY_ANGLE_COUNT; i--;) {
		fAng = i*2*PI/VEHICLE_BODY_ANGLE_COUNT;
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
	logBlockEnd("vehicleGenerateRotatedCollisions()");
}

/**
 *  Loads bob gfx data source from appropriate files & prepares rotated frames.
 *  @param szName     Name of vehicle to be loaded.
 *  @param pBobSource Pointer to bob source to be filled.
 *  @return Non-zero on success, otherwise zero.
 *  
 *  @todo Make it accept bitmaps wider than 32px?
 */
UWORD vehicleBobSourceLoad(char *szName, tBobSource *pBobSource) {
	UBYTE *pChunkySrc;
	UBYTE *pChunkyRotated;
	char szFullFileName[50];
	UBYTE ubFrame;
	FILE *pMaskFile;
	UWORD x,y;
	tBitMap *pBitmap;
	tBitmapMask *pMask;
	UWORD uwFrameOffs;
	
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
			-2*M_PI*ubFrame/VEHICLE_BODY_ANGLE_COUNT, 0,
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
	}
	
	memFree(pChunkySrc, VEHICLE_BODY_WIDTH * VEHICLE_BODY_HEIGHT);
	memFree(pChunkyRotated, VEHICLE_BODY_WIDTH * VEHICLE_BODY_HEIGHT);
	pBobSource->pBitmap = pBitmap;
	pBobSource->pMask = pMask;
	logBlockEnd("vehicleBobSourceLoad()");
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
	
	vehicleBobSourceLoad("tank", &pType->sMainSource);
	vehicleBobSourceLoad("tankturret", &pType->sAuxSource);
	
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
	vehicleGenerateRotatedCollisions(pType->pCollisionPts);
	
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
	vehicleBobSourceLoad("jeep", &pType->sMainSource);
	pType->sAuxSource.pBitmap = 0;
	pType->sAuxSource.pMask = 0;
	
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
	vehicleGenerateRotatedCollisions(pType->pCollisionPts);
	
	// Generate math table
	logWrite("Generating sine table...\n");
	for(i = 0; i != 128; ++i)
		g_pSin[i] = sin(i*2*PI/128);
	
	// Tank turret coords
	logWrite("Generating tank turret coords...\n");
	UBYTE ubStartX = 9;
	UBYTE ubStartY = 16;
	for(i = 0; i != 64; ++i) {
		float fAng = i*2*PI/64;
		g_pTurretCoords[i].sUwCoord.uwX = (ubStartX-16)*cos(fAng) - (ubStartY-16)*sin(fAng) + 16;
		g_pTurretCoords[i].sUwCoord.uwY = (ubStartX-16)*sin(fAng) + (ubStartY-16)*cos(fAng) + 16;
	}
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

tVehicle *vehicleCreate(UBYTE ubVehicleType) {
	tVehicle *pVehicle;
	
	logBlockBegin("vehicleCreate(ubVehicleType: %hhu)", ubVehicleType);
	pVehicle = memAllocFast(sizeof(tVehicle));
	if(!pVehicle) {
		logWrite("Can't alloc mem for vehicle!");
		logBlockEnd("vehicleCreate()");
		return 0;
	}
	
	// Fill struct fields
	pVehicle->pType = &g_pVehicleTypes[ubVehicleType];
	pVehicle->fX = (g_pTeams[TEAM_GREEN].ubSpawnX << MAP_TILE_SIZE) + (1 << (MAP_TILE_SIZE-1));
	pVehicle->fY = (g_pTeams[TEAM_GREEN].ubSpawnY << MAP_TILE_SIZE) + (1 << (MAP_TILE_SIZE-1));
	pVehicle->ubBodyAngle = ANGLE_90;
	pVehicle->ubTurretAngle = ANGLE_90;
	pVehicle->ubBaseAmmo = pVehicle->pType->ubMaxBaseAmmo;
	pVehicle->ubSuperAmmo = pVehicle->pType->ubMaxSuperAmmo;
	pVehicle->ubFuel = pVehicle->pType->ubMaxFuel;
	pVehicle->ubLife = pVehicle->pType->ubMaxLife;
	pVehicle->bRotDiv = 0;
	
	// Create bob
	pVehicle->pBob = bobCreate(
		pVehicle->pType->sMainSource.pBitmap, pVehicle->pType->sMainSource.pMask,
		VEHICLE_BODY_HEIGHT, angleToFrame(ANGLE_90)
	);
	
	if(ubVehicleType == VEHICLE_TYPE_TANK) {
		pVehicle->pAuxBob = bobCreate(
			pVehicle->pType->sAuxSource.pBitmap, pVehicle->pType->sAuxSource.pMask,
			VEHICLE_TURRET_HEIGHT, angleToFrame(ANGLE_90)
		);
	}
	else
		pVehicle->pAuxBob = 0;
	
	pVehicle->ubCooldown = 0;
	
	logBlockEnd("vehicleCreate()");
	return pVehicle;
}

void vehicleDestroy(tVehicle *pVehicle) {
	bobDestroy(pVehicle->pBob);
	if(pVehicle->pAuxBob)
		bobDestroy(pVehicle->pAuxBob);
	memFree(pVehicle, sizeof(tVehicle));
}

UBYTE vehicleCollides(float fX, float fY, tBCoordYX *pCollisionPoints) {
	UBYTE p;
	UBYTE ubLogicTile;
	UWORD uwPX, uwPY;
	for(p = 0; p != 9; ++p) {
		uwPX = fX - VEHICLE_BODY_WIDTH/2 + pCollisionPoints[p].bX;
		uwPY = fY - VEHICLE_BODY_HEIGHT/2 + pCollisionPoints[p].bY;
		ubLogicTile = g_pMap[uwPX >> MAP_TILE_SIZE][uwPY >> MAP_TILE_SIZE].ubIdx;
		if(
			ubLogicTile == MAP_LOGIC_WALL    ||
			ubLogicTile == MAP_LOGIC_SENTRY1 ||
			ubLogicTile == MAP_LOGIC_SENTRY2 ||
			ubLogicTile == MAP_LOGIC_FLAG1   ||
			ubLogicTile == MAP_LOGIC_FLAG2   ||
			0
		)
			return 1;
	}
	return 0;
}

void vehicleSteerTank(tVehicle *pVehicle, tSteerRequest *pSteerRequest) {
	UBYTE ubNewAngle;
	UBYTE ubNewTurretAngle;
	float fNewPosX, fNewPosY;
	
	// Move forward/backward
	fNewPosX = pVehicle->fX;
	fNewPosY = pVehicle->fY;
	if(pSteerRequest->ubForward) {
		fNewPosX += ccos(pVehicle->ubBodyAngle) * pVehicle->pType->ubFwdSpeed;
		fNewPosY += csin(pVehicle->ubBodyAngle) * pVehicle->pType->ubFwdSpeed;
	}
	else if(pSteerRequest->ubBackward) {
		fNewPosX -= ccos(pVehicle->ubBodyAngle) * pVehicle->pType->ubBwSpeed;
		fNewPosY -= csin(pVehicle->ubBodyAngle) * pVehicle->pType->ubBwSpeed;
	}
	
	// Body rotation: left/right
	ubNewAngle = pVehicle->ubBodyAngle;
	ubNewTurretAngle = pVehicle->ubTurretAngle;
	if(pSteerRequest->ubLeft) {
		if(pVehicle->bRotDiv > -pVehicle->pType->ubRotSpeedDiv)
			--pVehicle->bRotDiv;
		else {
			pVehicle->bRotDiv = 0;
			if(ubNewAngle < pVehicle->pType->ubRotSpeed)
				ubNewAngle += ANGLE_360 - pVehicle->pType->ubRotSpeed;
			else
				ubNewAngle -= pVehicle->pType->ubRotSpeed;
			if(ubNewTurretAngle < pVehicle->pType->ubRotSpeed)
				ubNewTurretAngle += ANGLE_360 - pVehicle->pType->ubRotSpeed;
			else		
				ubNewTurretAngle -= pVehicle->pType->ubRotSpeed;
			bobChangeFrame(pVehicle->pBob, angleToFrame(ubNewAngle));
			bobChangeFrame(pVehicle->pAuxBob, angleToFrame(ubNewTurretAngle));
		}
	}
	else if(pSteerRequest->ubRight) {
		if(pVehicle->bRotDiv < pVehicle->pType->ubRotSpeedDiv)
			++pVehicle->bRotDiv;
		else {
			pVehicle->bRotDiv = 0;
			ubNewAngle += pVehicle->pType->ubRotSpeed;
			if(ubNewAngle >= ANGLE_360)
				ubNewAngle -= ANGLE_360;
			ubNewTurretAngle += pVehicle->pType->ubRotSpeed;
			if(ubNewTurretAngle >= ANGLE_360)
				ubNewTurretAngle -= ANGLE_360;
			bobChangeFrame(pVehicle->pBob, angleToFrame(ubNewAngle));
			bobChangeFrame(pVehicle->pAuxBob, angleToFrame(ubNewTurretAngle));
		}
	}
	
	// Check collision
	if(!vehicleCollides(fNewPosX, fNewPosY, pVehicle->pType->pCollisionPts[ubNewAngle>>1])) {
		pVehicle->fX = fNewPosX;
		pVehicle->fY = fNewPosY;
		pVehicle->ubBodyAngle = ubNewAngle;
		pVehicle->ubTurretAngle = ubNewTurretAngle;
	}
	
	// Turret rotation: left/right
	if(pSteerRequest->ubTurretLeft) {
		if(pVehicle->ubTurretAngle < pVehicle->pType->ubRotSpeed)
			pVehicle->ubTurretAngle += ANGLE_360 - pVehicle->pType->ubRotSpeed;
		else
			pVehicle->ubTurretAngle -= pVehicle->pType->ubRotSpeed;
		bobChangeFrame(pVehicle->pAuxBob, angleToFrame(pVehicle->ubTurretAngle));
	}
	else if(pSteerRequest->ubTurretRight) {
		pVehicle->ubTurretAngle += pVehicle->pType->ubRotSpeed;
		if(pVehicle->ubTurretAngle >= ANGLE_360)
			pVehicle->ubTurretAngle -= ANGLE_360;
		bobChangeFrame(pVehicle->pAuxBob, angleToFrame(pVehicle->ubTurretAngle));
	}
	
	// Fire straight
	if(pVehicle->ubCooldown) {
		--pVehicle->ubCooldown;
	}
	else if(pSteerRequest->ubAction1) {
		projectileCreate(pVehicle, PROJECTILE_TYPE_CANNON);
		pVehicle->ubCooldown = VEHICLE_TANK_COOLDOWN;
	}
	
}

void vehicleSteerJeep(tVehicle *pVehicle, tSteerRequest *pSteerRequest) {
	UBYTE ubNewAngle;
	float fNewPosX, fNewPosY;
	
	ubNewAngle = pVehicle->ubBodyAngle;
	if(pSteerRequest->ubLeft) {
		if(!pSteerRequest->ubBackward)
			pSteerRequest->ubForward = 1;
		if(pVehicle->bRotDiv > -pVehicle->pType->ubRotSpeedDiv)
			--pVehicle->bRotDiv;
		else {
			pVehicle->bRotDiv = 0;
			if(ubNewAngle < pVehicle->pType->ubRotSpeed)
				ubNewAngle += ANGLE_360 - pVehicle->pType->ubRotSpeed;
			else
				ubNewAngle -= pVehicle->pType->ubRotSpeed;
			bobChangeFrame(pVehicle->pBob, angleToFrame(ubNewAngle));
		}
	}
	else if(pSteerRequest->ubRight) {
		if(!pSteerRequest->ubBackward)
			pSteerRequest->ubForward = 1;
		if(pVehicle->bRotDiv < pVehicle->pType->ubRotSpeedDiv)
			++pVehicle->bRotDiv;
		else {
			pVehicle->bRotDiv = 0;
			ubNewAngle += pVehicle->pType->ubRotSpeed;
			if(ubNewAngle >= ANGLE_360)
				ubNewAngle -= ANGLE_360;
			bobChangeFrame(pVehicle->pBob, angleToFrame(ubNewAngle));
		}
	}
	
	fNewPosX = pVehicle->fX;
	fNewPosY = pVehicle->fY;
	if(pSteerRequest->ubForward) {
		fNewPosX += ccos(ubNewAngle) * pVehicle->pType->ubFwdSpeed;
		fNewPosY += csin(ubNewAngle) * pVehicle->pType->ubFwdSpeed;
	}
	else if(pSteerRequest->ubBackward) {
		fNewPosX -= ccos(ubNewAngle) * pVehicle->pType->ubBwSpeed;
		fNewPosY -= csin(ubNewAngle) * pVehicle->pType->ubBwSpeed;
	}
	
	if(!vehicleCollides(fNewPosX, fNewPosY, pVehicle->pType->pCollisionPts[ubNewAngle>>1])) {
		pVehicle->fX = fNewPosX;
		pVehicle->fY = fNewPosY;
		pVehicle->ubBodyAngle = ubNewAngle;
	}
}

void vehicleDraw(tVehicle *pVehicle) {
	bobDraw(
		pVehicle->pBob,
		g_pGameBfr->pBuffer,
		pVehicle->fX - VEHICLE_BODY_WIDTH/2,
		pVehicle->fY - VEHICLE_BODY_HEIGHT/2
	);
	if(pVehicle->pType == &g_pVehicleTypes[VEHICLE_TYPE_TANK])
		bobDraw(
			pVehicle->pAuxBob,
			g_pGameBfr->pBuffer,
			pVehicle->fX - g_pTurretCoords[angleToFrame(pVehicle->ubTurretAngle)].sUwCoord.uwX,
			pVehicle->fY - g_pTurretCoords[angleToFrame(pVehicle->ubTurretAngle)].sUwCoord.uwY
		);
}

void vehicleUndraw(tVehicle *pVehicle) {
if(pVehicle->pType == &g_pVehicleTypes[VEHICLE_TYPE_TANK])
		bobUndraw(
			pVehicle->pAuxBob,
			g_pGameBfr->pBuffer
		);
	bobUndraw(
		pVehicle->pBob,
		g_pGameBfr->pBuffer
	);
}


tVehicleType g_pVehicleTypes[VEHICLE_TYPE_COUNT];
