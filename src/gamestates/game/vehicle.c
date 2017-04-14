#include "gamestates/game/vehicle.h"
#include <math.h>
#include <ace/managers/blit.h>
#include <ace/utils/bitmap.h>
#include <ace/utils/bitmapmask.h>
#include "gamestates/game/game.h"
#include "gamestates/game/team.h"
#include "gamestates/game/map.h"
#include "gamestates/game/projectile.h"
#include "gamestates/game/world.h"
#include "vehicletypes.h"

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
		g_pWorldMainBfr->pBuffer,
		pVehicle->fX - VEHICLE_BODY_WIDTH/2,
		pVehicle->fY - VEHICLE_BODY_HEIGHT/2
	);
	if(pVehicle->pType == &g_pVehicleTypes[VEHICLE_TYPE_TANK])
		bobDraw(
			pVehicle->pAuxBob,
			g_pWorldMainBfr->pBuffer,
			pVehicle->fX - g_pTurretCoords[angleToFrame(pVehicle->ubTurretAngle)].sUwCoord.uwX,
			pVehicle->fY - g_pTurretCoords[angleToFrame(pVehicle->ubTurretAngle)].sUwCoord.uwY
		);
}

void vehicleUndraw(tVehicle *pVehicle) {
if(pVehicle->pType == &g_pVehicleTypes[VEHICLE_TYPE_TANK])
		bobUndraw(
			pVehicle->pAuxBob,
			g_pWorldMainBfr->pBuffer
		);
	bobUndraw(
		pVehicle->pBob,
		g_pWorldMainBfr->pBuffer
	);
}
