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
#include "gamestates/game/explosions.h"
#include "vehicletypes.h"

void vehicleInit(tVehicle *pVehicle, UBYTE ubVehicleType) {

	logBlockBegin(
		"vehicleInit(pVehicle: %p, ubVehicleType: %hhu)",
		pVehicle, ubVehicleType
	);

	// Fill struct fields
	pVehicle->pType = &g_pVehicleTypes[ubVehicleType];
	pVehicle->fX = (g_pTeams[TEAM_GREEN].pSilos[0].ubTileX << MAP_TILE_SIZE) + (1 << (MAP_TILE_SIZE-1));
	pVehicle->fY = (g_pTeams[TEAM_GREEN].pSilos[0].ubTileY << MAP_TILE_SIZE) + (1 << (MAP_TILE_SIZE-1));
	pVehicle->ubBodyAngle = ANGLE_90;
	pVehicle->ubTurretAngle = ANGLE_90;
	pVehicle->ubBaseAmmo = pVehicle->pType->ubMaxBaseAmmo;
	pVehicle->ubSuperAmmo = pVehicle->pType->ubMaxSuperAmmo;
	pVehicle->ubFuel = pVehicle->pType->ubMaxFuel;
	pVehicle->ubLife = pVehicle->pType->ubMaxLife;
	pVehicle->bRotDiv = 0;
	logWrite("Created vehicle %hu @%f,%f\n", ubVehicleType, pVehicle->fX, pVehicle->fY);
	logWrite("Spawn is at tile %hu, %hu\n", g_pTeams[TEAM_GREEN].pSilos[0].ubTileX, g_pTeams[TEAM_GREEN].pSilos[0].ubTileY);

	// Set main bob frames
	bobSetSource(pVehicle->pBob, &pVehicle->pType->sMainSource);
	bobChangeFrame(pVehicle->pBob, angleToFrame(ANGLE_90));
	pVehicle->pBob->ubFlags = BOB_FLAG_START_DRAWING;

	// Set aux bob frames
	if(ubVehicleType == VEHICLE_TYPE_TANK) {
		bobSetSource(pVehicle->pAuxBob, &pVehicle->pType->sAuxSource);
		pVehicle->pAuxBob->uwHeight = VEHICLE_TURRET_HEIGHT;
		bobChangeFrame(pVehicle->pAuxBob, angleToFrame(ANGLE_90));
		pVehicle->pAuxBob->ubFlags = BOB_FLAG_START_DRAWING;
	}
	else
		pVehicle->pAuxBob->ubFlags = BOB_FLAG_NODRAW;

	pVehicle->ubCooldown = 0;

	logBlockEnd("vehicleInit()");
}

void vehicleUnset(tVehicle *pVehicle) {
	pVehicle->pBob->ubFlags = BOB_FLAG_STOP_DRAWING;
	pVehicle->pAuxBob->ubFlags = BOB_FLAG_STOP_DRAWING;
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

	pVehicle->ubTurretAngle += ANGLE_360 + getDeltaAngleDirection(
		pVehicle->ubTurretAngle, pSteerRequest->ubDestAngle, 1
	);
if(pVehicle->ubTurretAngle >= ANGLE_360)
	pVehicle->ubTurretAngle -= ANGLE_360;
	bobChangeFrame(pVehicle->pAuxBob, angleToFrame(pVehicle->ubTurretAngle));

	// Fire straight
	if(pVehicle->ubCooldown) {
		--pVehicle->ubCooldown;
	}
	else if(pSteerRequest->ubAction1 && pVehicle->ubBaseAmmo) {
		tProjectileOwner uOwner;
		uOwner.pVehicle = pVehicle;
		projectileCreate(PROJECTILE_OWNER_TYPE_VEHICLE, uOwner, PROJECTILE_TYPE_BULLET);
		pVehicle->ubCooldown = VEHICLE_TANK_COOLDOWN;
		--pVehicle->ubBaseAmmo;
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
	UWORD uwX = pVehicle->fX - VEHICLE_BODY_WIDTH/2;
	UWORD uwY = pVehicle->fY - VEHICLE_BODY_HEIGHT/2;
	bobDraw(pVehicle->pBob, g_pWorldMainBfr->pBuffer, uwX, uwY);
	if(pVehicle->pType == &g_pVehicleTypes[VEHICLE_TYPE_TANK])
		blitCopyMask(
			pVehicle->pAuxBob->sSource.pBitmap, 0, pVehicle->pAuxBob->uwOffsY,
			g_pWorldMainBfr->pBuffer, uwX, uwY,
			VEHICLE_BODY_WIDTH, VEHICLE_BODY_HEIGHT,
			pVehicle->pAuxBob->sSource.pMask->pData
		);
}

void vehicleUndraw(tVehicle *pVehicle) {
	bobUndraw(pVehicle->pBob,	g_pWorldMainBfr->pBuffer);
}
