#include "gamestates/game/vehicle.h"
#include <ace/macros.h>
#include <ace/managers/blit.h>
#include <ace/utils/bitmap.h>
#include "gamestates/game/game.h"
#include "gamestates/game/team.h"
#include "gamestates/game/worldmap.h"
#include "gamestates/game/explosions.h"
#include "gamestates/game/spawn.h"
#include "gamestates/game/player.h"
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
	pVehicle->uwX = fix16_to_int(pVehicle->fX);
	pVehicle->uwY = fix16_to_int(pVehicle->fY);
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
	bobSetData(
		pVehicle->pBob,	pVehicle->pType->pMainFrames[TEAM_BLUE],
		pVehicle->pType->pMainMask, pVehicle->pType->pMainFrameOffsets
	);
	bobChangeFrame(pVehicle->pBob, angleToFrame(pVehicle->ubBodyAngle));
	pVehicle->pBob->isDrawn = 0;

	// Set aux bob frames
	if(pVehicle->pType == &g_pVehicleTypes[VEHICLE_TYPE_TANK]) {
		bobSetData(
			pVehicle->pAuxBob, pVehicle->pType->pAuxFrames[TEAM_BLUE],
			pVehicle->pType->pAuxMask, pVehicle->pType->pAuxFrameOffsets
		);
		bobChangeFrame(pVehicle->pAuxBob, angleToFrame(pVehicle->ubTurretAngle));
	}
	else
		pVehicle->pAuxBob->ubState = BOB_STATE_NODRAW;
}

void vehicleUnset(tVehicle *pVehicle) {
	pVehicle->pBob->ubState = BOB_STATE_STOP_DRAWING;
	pVehicle->pAuxBob->ubState = BOB_STATE_STOP_DRAWING;
}

UBYTE vehicleCollidesWithOtherVehicle(tVehicle *pVehicle, UWORD uwX, UWORD uwY, UBYTE ubAngle) {
	// Transform player's vehicle position so that it's plain rectangle
	UWORD uwRotdX = fix16_to_int(fix16_sub(
		pVehicle->uwX * ccos(ubAngle),
		pVehicle->uwY * csin(ubAngle)
	));
	UWORD uwRotdY = fix16_to_int(fix16_add(
		fix16_mul(pVehicle->fX, csin(ubAngle)),
		fix16_mul(pVehicle->fY, ccos(ubAngle))
	));
	tUwAbsRect sRect;
	sRect.uwX1 = uwRotdX + pVehicle->pType->pCollisionPts[0].pPts[0].bX-1;
	sRect.uwY1 = uwRotdY + pVehicle->pType->pCollisionPts[0].pPts[0].bY-1;
	sRect.uwX2 = uwRotdX + pVehicle->pType->pCollisionPts[0].pPts[7].bX+1;
	sRect.uwY2 = uwRotdY + pVehicle->pType->pCollisionPts[0].pPts[7].bY+1;
	tPlayer *pChkPlayer;
	FUBYTE i;
	for(i = 0, pChkPlayer = &g_pPlayers[0]; i != g_ubPlayerCount; ++i, ++pChkPlayer) {
		if(pChkPlayer->ubState != PLAYER_STATE_DRIVING || &pChkPlayer->sVehicle == pVehicle)
			continue;

		// Check if player is nearby
		if(
			ABS(uwX - pChkPlayer->sVehicle.uwX) > VEHICLE_BODY_WIDTH ||
			ABS(uwY - pChkPlayer->sVehicle.uwY) > VEHICLE_BODY_WIDTH
		) {
			continue;
		}

		// Transform other player's vehicle pos to same axes
		UWORD uwChkRotdX = fix16_to_int(fix16_sub(
			pChkPlayer->sVehicle.uwX * ccos(ubAngle),
			pChkPlayer->sVehicle.uwY * csin(ubAngle)
		));
		UWORD uwChkRotdY = fix16_to_int(fix16_add(
			pChkPlayer->sVehicle.uwX * csin(ubAngle),
			pChkPlayer->sVehicle.uwY * ccos(ubAngle)
		));

		UBYTE ubChkAngle = ANGLE_360 + pChkPlayer->sVehicle.ubBodyAngle - ubAngle;
		if(ubChkAngle >= ANGLE_360)
			ubChkAngle -= ANGLE_360;
		tCollisionPts *pChkPoints = &pChkPlayer->sVehicle.pType->pCollisionPts[ubChkAngle >> 1];


		UWORD uwEdgeL, uwEdgeR, uwEdgeT, uwEdgeB;
		uwEdgeL = uwChkRotdX + pChkPoints->bLeftmost;
		uwEdgeR = uwChkRotdX + pChkPoints->bRightmost;
		uwEdgeT = uwChkRotdY + pChkPoints->bTopmost;
		uwEdgeB = uwChkRotdY + pChkPoints->bBottommost;
		if(
			(
				(uwEdgeT <= sRect.uwY1 && uwEdgeB >= sRect.uwY1) ||
				(uwEdgeT <= sRect.uwY2 && uwEdgeB >= sRect.uwY2)
			) && (
				(uwEdgeL <= sRect.uwX1 && uwEdgeR >= sRect.uwX1) ||
				(uwEdgeL <= sRect.uwX2 && uwEdgeR >= sRect.uwX2)
			)
		) {
			return 1;
		}
	}

	return 0;
}

UBYTE vehicleCollidesWithWall(UWORD uwX, fix16_t uwY, tBCoordYX *pCollisionPoints) {
	UBYTE p;
	UBYTE ubLogicTile;
	UWORD uwPX, uwPY;
	for(p = 0; p != 9; ++p) {
		uwPX = uwX + pCollisionPoints[p].bX;
		uwPY = uwY + pCollisionPoints[p].bY;
		ubLogicTile = g_sMap.pData[uwPX >> MAP_TILE_SIZE][uwPY >> MAP_TILE_SIZE].ubIdx;
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
	UWORD uwNewPosX = fix16_to_int(fNewPosX);
	UWORD uwNewPosY = fix16_to_int(fNewPosY);

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
	if(
		!vehicleCollidesWithWall(uwNewPosX, uwNewPosY, pVehicle->pType->pCollisionPts[ubNewAngle>>1].pPts) &&
		(
			pVehicle == &g_pLocalPlayer->sVehicle &&
			!vehicleCollidesWithOtherVehicle(pVehicle, uwNewPosX, uwNewPosY, ubNewAngle)
		)
	) {
		pVehicle->fX = fNewPosX;
		pVehicle->fY = fNewPosY;
		pVehicle->uwX = fix16_to_int(fNewPosX);
		pVehicle->uwY = fix16_to_int(fNewPosY);
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
		fNewPosX = fix16_sub(fNewPosX, ccos(ubNewAngle) * pVehicle->pType->ubBwSpeed);
		fNewPosY = fix16_sub(fNewPosY, csin(ubNewAngle) * pVehicle->pType->ubBwSpeed);
	}
	UWORD uwNewPosX = fix16_to_int(fNewPosX);
	UWORD uwNewPosY = fix16_to_int(fNewPosY);

	if(
		!vehicleCollidesWithWall(uwNewPosX, uwNewPosY, pVehicle->pType->pCollisionPts[ubNewAngle>>1].pPts) &&
		(
			pVehicle == &g_pLocalPlayer->sVehicle &&
			!vehicleCollidesWithOtherVehicle(pVehicle, uwNewPosX, uwNewPosY, ubNewAngle)
		)
	) {
		pVehicle->fX = fNewPosX;
		pVehicle->fY = fNewPosY;
		pVehicle->uwX = fix16_to_int(fNewPosX);
		pVehicle->uwY = fix16_to_int(fNewPosY);
		pVehicle->ubBodyAngle = ubNewAngle;
	}
}

void vehicleDraw(tVehicle *pVehicle) {
	UWORD uwX = pVehicle->uwX - VEHICLE_BODY_WIDTH/2;
	UWORD uwY = pVehicle->uwY - VEHICLE_BODY_HEIGHT/2;
	tBob *pMainBob = pVehicle->pBob;
	if(pVehicle->pType == &g_pVehicleTypes[VEHICLE_TYPE_TANK]) {
		tBob *pAuxBob = pVehicle->pAuxBob;
		UBYTE ubMainY1 = pMainBob->sData.pFrameOffsets[pMainBob->fubCurrFrame].uwDy;
		UBYTE ubMainY2 = ubMainY1 + pMainBob->sData.pFrameOffsets[pMainBob->fubCurrFrame].uwHeight - 1;
		UBYTE ubAuxY1 = pAuxBob->sData.pFrameOffsets[pAuxBob->fubCurrFrame].uwDy;
		UBYTE ubAuxY2 = ubAuxY1 + pAuxBob->sData.pFrameOffsets[pAuxBob->fubCurrFrame].uwHeight - 1;
		UBYTE ubBgDy = MIN(ubMainY1, ubAuxY1);
		UBYTE ubBgHeight = MAX(ubMainY2, ubAuxY2) - ubBgDy + 1;
		if(bobDraw(pMainBob, g_pWorldMainBfr, uwX, uwY, ubBgDy, ubBgHeight)) {
			blitCopyMask(
				pAuxBob->sData.pBitmap, 0, pAuxBob->uwOffsY,
				g_pWorldMainBfr->pBuffer, uwX, uwY,
				VEHICLE_BODY_WIDTH, VEHICLE_BODY_HEIGHT,
				(UWORD*)pAuxBob->sData.pMask->Planes[0]
			);
		}
	}
	else {
		bobDraw(
			pMainBob, g_pWorldMainBfr, uwX, uwY,
			pMainBob->sData.pFrameOffsets[pMainBob->fubCurrFrame].uwDy,
			pMainBob->sData.pFrameOffsets[pMainBob->fubCurrFrame].uwHeight
		);
	}
}

void vehicleUndraw(tVehicle *pVehicle) {
	bobUndraw(pVehicle->pBob,	g_pWorldMainBfr);
}
