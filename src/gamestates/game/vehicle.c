#include "gamestates/game/vehicle.h"
#include <ace/managers/blit.h>
#include <ace/utils/bitmap.h>
#include <ace/utils/bitmapmask.h>
#include "gamestates/game/game.h"
#include "gamestates/game/team.h"
#include "gamestates/game/map.h"
#include "gamestates/game/projectile.h"
#include "gamestates/game/explosions.h"
#include "gamestates/game/spawn.h"
#include "vehicletypes.h"

void vehicleInit(tVehicle *pVehicle, UBYTE ubVehicleType, UBYTE ubSpawnIdx) {

	logBlockBegin(
		"vehicleInit(pVehicle: %p, ubVehicleType: %hhu)",
		pVehicle, ubVehicleType
	);

	// Fill struct fields
	pVehicle->pType = &g_pVehicleTypes[ubVehicleType];
	pVehicle->fX = fix16_from_int((g_pSpawns[ubSpawnIdx].ubTileX << MAP_TILE_SIZE) + MAP_HALF_TILE);
	pVehicle->fY = fix16_from_int((g_pSpawns[ubSpawnIdx].ubTileY << MAP_TILE_SIZE) + MAP_HALF_TILE);
	pVehicle->ubBodyAngle = ANGLE_90;
	pVehicle->ubTurretAngle = ANGLE_90;
	pVehicle->ubBaseAmmo = pVehicle->pType->ubMaxBaseAmmo;
	pVehicle->ubSuperAmmo = pVehicle->pType->ubMaxSuperAmmo;
	pVehicle->ubFuel = pVehicle->pType->ubMaxFuel;
	pVehicle->ubLife = pVehicle->pType->ubMaxLife;
	pVehicle->bRotDiv = 0;
	pVehicle->ubCooldown = 0;

	vehicleSetupBob(pVehicle);
	spawnSetBusy(ubSpawnIdx, SPAWN_BUSY_SURFACING, VEHICLE_TYPE_TANK);

	logBlockEnd("vehicleInit()");
}

void vehicleSetupBob(tVehicle *pVehicle) {
	// Set main bob frames
	bobSetSource(pVehicle->pBob, &pVehicle->pType->sMainSource[TEAM_GREEN]);
	bobChangeFrame(pVehicle->pBob, angleToFrame(pVehicle->ubBodyAngle));
	pVehicle->pBob->isDrawn = 0;

	// Set aux bob frames
	if(pVehicle->pType == &g_pVehicleTypes[VEHICLE_TYPE_TANK]) {
		bobSetSource(pVehicle->pAuxBob, &pVehicle->pType->sAuxSource[TEAM_GREEN]);
		bobChangeFrame(pVehicle->pAuxBob, angleToFrame(pVehicle->ubTurretAngle));
	}
	else
		pVehicle->pAuxBob->ubFlags = BOB_FLAG_NODRAW;
}

void vehicleUnset(tVehicle *pVehicle) {
	pVehicle->pBob->ubFlags = BOB_FLAG_STOP_DRAWING;
	pVehicle->pAuxBob->ubFlags = BOB_FLAG_STOP_DRAWING;
}

// TODO accept UWORD instead of fix16_t? Conversion may then be counted only once.
UBYTE vehicleCollides(fix16_t fX, fix16_t fY, tBCoordYX *pCollisionPoints) {
	UBYTE p;
	UBYTE ubLogicTile;
	UWORD uwPX, uwPY;
	UWORD uwX = fix16_to_int(fX), uwY = fix16_to_int(fY);
	for(p = 0; p != 9; ++p) {
		uwPX = uwX - VEHICLE_BODY_WIDTH/2 + pCollisionPoints[p].bX;
		uwPY = uwY - VEHICLE_BODY_HEIGHT/2 + pCollisionPoints[p].bY;
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
	fix16_t fNewPosX, fNewPosY;

	// Move forward/backward
	fNewPosX = pVehicle->fX;
	fNewPosY = pVehicle->fY;
	if(pSteerRequest->ubForward) {
		fNewPosX = fix16_add(fNewPosX, ccos(pVehicle->ubBodyAngle) * pVehicle->pType->ubFwdSpeed);
		fNewPosY = fix16_add(fNewPosY, csin(pVehicle->ubBodyAngle) * pVehicle->pType->ubFwdSpeed);
	}
	else if(pSteerRequest->ubBackward) {
		fNewPosX = fix16_sub(fNewPosX, ccos(pVehicle->ubBodyAngle) * pVehicle->pType->ubBwSpeed);
		fNewPosY = fix16_sub(fNewPosY, csin(pVehicle->ubBodyAngle) * pVehicle->pType->ubBwSpeed);
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
	if(pVehicle->ubCooldown)
		--pVehicle->ubCooldown;
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
	fix16_t fNewPosX, fNewPosY;

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
		fNewPosX = fix16_add(fNewPosX, ccos(ubNewAngle) * pVehicle->pType->ubFwdSpeed);
		fNewPosY = fix16_add(fNewPosY, csin(ubNewAngle) * pVehicle->pType->ubFwdSpeed);
	}
	else if(pSteerRequest->ubBackward) {
		fNewPosX = fix16_add(fNewPosX, ccos(ubNewAngle) * pVehicle->pType->ubBwSpeed);
		fNewPosY = fix16_add(fNewPosY, csin(ubNewAngle) * pVehicle->pType->ubBwSpeed);
	}

	if(!vehicleCollides(fNewPosX, fNewPosY, pVehicle->pType->pCollisionPts[ubNewAngle>>1])) {
		pVehicle->fX = fNewPosX;
		pVehicle->fY = fNewPosY;
		pVehicle->ubBodyAngle = ubNewAngle;
	}
}

void vehicleDraw(tVehicle *pVehicle) {
	UWORD uwX = fix16_to_int(pVehicle->fX) - VEHICLE_BODY_WIDTH/2;
	UWORD uwY = fix16_to_int(pVehicle->fY) - VEHICLE_BODY_HEIGHT/2;
	if(
		bobDraw(pVehicle->pBob, g_pWorldMainBfr, uwX, uwY)
		&& pVehicle->pType == &g_pVehicleTypes[VEHICLE_TYPE_TANK]
	) {
		blitCopyMask(
			pVehicle->pAuxBob->sSource.pBitmap, 0, pVehicle->pAuxBob->uwOffsY,
			g_pWorldMainBfr->pBuffer, uwX, uwY,
			VEHICLE_BODY_WIDTH, VEHICLE_BODY_HEIGHT,
			pVehicle->pAuxBob->sSource.pMask->pData
		);
	}
}

void vehicleUndraw(tVehicle *pVehicle) {
	bobUndraw(pVehicle->pBob,	g_pWorldMainBfr);
}
