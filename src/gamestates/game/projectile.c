#include "gamestates/game/projectile.h"
#include "gamestates/game/vehicle.h"
#include "gamestates/game/bob.h"
#include "gamestates/game/game.h"
#include "gamestates/game/map.h"
#include "gamestates/game/building.h"
#include "gamestates/game/world.h"

#define PROJECTILE_SPEED      (2.0)
#define PROJECTILE_FRAME_LIFE (((320-32)/4)/PROJECTILE_SPEED)

#define PROJECTILE_CANNON_HEIGHT 8

tProjectile *s_pProjectiles;
UBYTE s_ubProjectileCount;
tBitMap *s_pCannonBitmap;
tBitmapMask *s_pCannonMask;

void projectileListCreate(UBYTE ubProjectileCount) {
	UBYTE i;
	
	logBlockBegin("projectileListCreate(ubProjectileCount: %hhu)", ubProjectileCount);
	
	// Load gfx
	s_pCannonBitmap = bitmapCreateFromFile("data/projectiles/cannon.bm");
	s_pCannonMask = bitmapMaskCreateFromFile("data/projectiles/cannon.msk");
	
	// Create projectiles
	s_ubProjectileCount = ubProjectileCount;
	s_pProjectiles = memAllocFast(ubProjectileCount * sizeof(tProjectile));
	for(i = 0; i != ubProjectileCount; ++i) {
		s_pProjectiles[i].ubType = PROJECTILE_TYPE_OFF;
		s_pProjectiles[i].pBob = bobCreate(
			s_pCannonBitmap, s_pCannonMask,
			PROJECTILE_CANNON_HEIGHT, 0
		);
	}
	
	logBlockEnd("projectileListCreate()");
}

void projectileListDestroy() {
	UBYTE i;
	
	logBlockBegin("projectileListDestroy()");
	
	// Dealloc bobs
	for(i = 0; i != s_ubProjectileCount; ++i) {
		bobDestroy(s_pProjectiles[i].pBob);
	}	
	memFree(s_pProjectiles, s_ubProjectileCount * sizeof(tProjectile));
	
	// Dealloc bob bitmaps
	bitmapDestroy(s_pCannonBitmap);
	bitmapMaskDestroy(s_pCannonMask);

	logBlockEnd("projectileListDestroy()");
}

tProjectile *projectileCreate(tVehicle *pOwner, UBYTE ubType) {
	tProjectile *pProjectile;
	float fSin, fCos;
	UBYTE i, ubAngle;
	
	pProjectile = 0;
	for(i = 0; i != s_ubProjectileCount; ++i) {
		if(s_pProjectiles[i].ubType == PROJECTILE_TYPE_OFF) {
			pProjectile = &s_pProjectiles[i];
			break;
		}
	}
	if(!pProjectile)
		return 0;
	
	pProjectile->pOwner = pOwner;
	pProjectile->ubType = ubType;
	
	// Angle
	if(pOwner->pType == &g_pVehicleTypes[VEHICLE_TYPE_TANK])
		ubAngle = pOwner->ubTurretAngle;
	else
		ubAngle = pOwner->ubBodyAngle;
	fSin = csin(ubAngle);
	fCos = ccos(ubAngle);		
	
	// Movement deltas
	pProjectile->fDx = fCos * PROJECTILE_SPEED;
	pProjectile->fDy = fSin * PROJECTILE_SPEED;

	// Initial projectile position
	pProjectile->fX = pOwner->fX + (VEHICLE_BODY_WIDTH/2) * fCos;
	pProjectile->fY = pOwner->fY + (VEHICLE_BODY_HEIGHT/2) * fSin;
	
	// Frame life
	pProjectile->uwFrameLife = PROJECTILE_FRAME_LIFE;
	
	// Bob
	bobChangeFrame(pProjectile->pBob, angleToFrame(ubAngle));
	return pProjectile;
}

void projectileDestroy(tProjectile *pProjectile) {
	pProjectile->ubType = PROJECTILE_TYPE_OFF;
}

void projectileUndraw(void) {
	tProjectile *pProjectile;
	UBYTE i;
	
	pProjectile = &s_pProjectiles[s_ubProjectileCount-1];
	for(i = s_ubProjectileCount; i--;) {
		if(pProjectile->ubType != PROJECTILE_TYPE_OFF)
			bobUndraw(pProjectile->pBob, g_pWorldMainBfr->pBuffer);
		--pProjectile;
	}
}

void projectileDraw(void) {
	tProjectile *pProjectile;
	UBYTE i;
	
	pProjectile = &s_pProjectiles[0];
	for(i = s_ubProjectileCount; i--;) {
		if(pProjectile->ubType != PROJECTILE_TYPE_OFF)
			bobDraw(
				pProjectile->pBob, g_pWorldMainBfr->pBuffer,
				pProjectile->fX-8, pProjectile->fY-PROJECTILE_CANNON_HEIGHT/2
			);
		++pProjectile;
	}
}

void projectileProcess(void) {
	tProjectile *pProjectile;
	tVehicle *pVehicle;
	UBYTE i;
	UBYTE ubMapX, ubMapY, ubBuildingIdx;
	
	for(i = 0; i != s_ubProjectileCount; ++i) {
		pProjectile = &s_pProjectiles[i];
		if(pProjectile->ubType == PROJECTILE_TYPE_OFF)
			continue;
		
		// Increment position
		pProjectile->fX += pProjectile->fDx;
		pProjectile->fY += pProjectile->fDy;
		
		// Verify projectile lifespan
		if(!pProjectile->uwFrameLife) {
			projectileDestroy(pProjectile);
			continue;
		}
		--pProjectile->uwFrameLife;
		
		// Check collision with vehicles
		/*
		for() { // TODO: Iterate through players
			pVehicle = ; // TODO: Player's active vehicle
			if(
				pProjectile->fX > pVehicle->fX-(VEHICLE_SIZE/2) &&
				pProjectile->fX < pVehicle->fX+(VEHICLE_SIZE/2) &&
				pProjectile->fY > pVehicle->fY-(VEHICLE_SIZE/2) &&
				pProjectile->fY < pVehicle->fY+(VEHICLE_SIZE/2)
			) {
				// TODO: Apply damage to vehicle
				projectileDestroy(pProjectile);
			}
		}
		*/
		
		// Check collistion with buildings
		ubMapX = (UWORD)(pProjectile->fX) >> MAP_TILE_SIZE;
		ubMapY = (UWORD)(pProjectile->fY) >> MAP_TILE_SIZE;
		ubBuildingIdx = g_pMap[ubMapX][ubMapY].ubData;
		if(ubBuildingIdx != BUILDING_IDX_INVALID) {
			if(buildingDamage(ubBuildingIdx, 10) == BUILDING_DESTROYED) {
				g_pMap[ubMapX][ubMapY].ubIdx = MAP_LOGIC_DIRT;
				g_pMap[ubMapX][ubMapY].ubData = 0;
				mapDrawTile(ubMapX, ubMapY, MAP_TILE_DIRT);
			}
			projectileDestroy(pProjectile);
			continue;
		}
	}
}
