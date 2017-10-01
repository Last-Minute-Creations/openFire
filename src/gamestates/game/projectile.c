#include "gamestates/game/projectile.h"
#include "gamestates/game/vehicle.h"
#include "gamestates/game/bob.h"
#include "gamestates/game/game.h"
#include "gamestates/game/map.h"
#include "gamestates/game/building.h"
#include "gamestates/game/player.h"
#include "gamestates/game/explosions.h"

#define PROJECTILE_SPEED      (fix16_one*2)
#define PROJECTILE_FRAME_LIFE fix16_to_int(fix16_div(fix16_from_int((320-32)/4), PROJECTILE_SPEED))

#define PROJECTILE_BULLET_HEIGHT 2

tProjectile *s_pProjectiles;
UBYTE s_ubProjectileCount;
tBitMap *s_pBulletBitmap;
tBitmapMask *s_pBulletMask;

void projectileListCreate(UBYTE ubProjectileCount) {
	UBYTE i;

	logBlockBegin("projectileListCreate(ubProjectileCount: %hhu)", ubProjectileCount);

	// Load gfx
	s_pBulletBitmap = bitmapCreateFromFile("data/projectiles/bullet.bm");
	s_pBulletMask = bitmapMaskCreateFromFile("data/projectiles/bullet.msk");

	// Create projectiles
	s_ubProjectileCount = ubProjectileCount;
	s_pProjectiles = memAllocFast(ubProjectileCount * sizeof(tProjectile));
	for(i = 0; i != ubProjectileCount; ++i) {
		s_pProjectiles[i].ubType = PROJECTILE_TYPE_OFF;
		s_pProjectiles[i].pBob = bobCreate(
			s_pBulletBitmap, s_pBulletMask,
			PROJECTILE_BULLET_HEIGHT, 0
		);
		s_pProjectiles[i].pBob->ubFlags = BOB_FLAG_NODRAW;
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
	bitmapDestroy(s_pBulletBitmap);
	bitmapMaskDestroy(s_pBulletMask);

	logBlockEnd("projectileListDestroy()");
}

tProjectile *projectileCreate(
	UBYTE ubOwnerType, tProjectileOwner uOwner, UBYTE ubType
) {
	tProjectile *pProjectile;
	fix16_t fSin, fCos;
	UBYTE i, ubAngle;

	// Find free projectile
	pProjectile = 0;
	for(i = 0; i != s_ubProjectileCount; ++i) {
		if(s_pProjectiles[i].ubType == PROJECTILE_TYPE_OFF) {
			pProjectile = &s_pProjectiles[i];
			break;
		}
	}
	if(!pProjectile)
		return 0;

	pProjectile->uOwner = uOwner;
	pProjectile->ubType = ubType;
	pProjectile->ubOwnerType = ubOwnerType;

	// Initial projectile position & angle
	if(ubOwnerType == PROJECTILE_OWNER_TYPE_VEHICLE) {
		if(uOwner.pVehicle->pType == &g_pVehicleTypes[VEHICLE_TYPE_TANK])
			ubAngle = uOwner.pVehicle->ubTurretAngle;
		else
			ubAngle = uOwner.pVehicle->ubBodyAngle;
		fSin = csin(ubAngle);
		fCos = ccos(ubAngle);
		pProjectile->fX = fix16_add(uOwner.pVehicle->fX, (VEHICLE_BODY_WIDTH/2) * fCos);
		pProjectile->fY = fix16_add(uOwner.pVehicle->fY, (VEHICLE_BODY_HEIGHT/2) * fSin);
	}
	else {
		pProjectile->fX = fix16_from_int(uOwner.pTurret->uwX);
		pProjectile->fY = fix16_from_int(uOwner.pTurret->uwY);
	}

	// Movement deltas
	pProjectile->fDx = fix16_mul(fCos, PROJECTILE_SPEED);
	pProjectile->fDy = fix16_mul(fSin, PROJECTILE_SPEED);

	// Frame life
	pProjectile->uwFrameLife = PROJECTILE_FRAME_LIFE;

	// Bob
	pProjectile->pBob->ubFlags = BOB_FLAG_START_DRAWING;
	pProjectile->pBob->isDrawn = 0;
	return pProjectile;
}

void projectileDestroy(tProjectile *pProjectile) {
	pProjectile->ubType = PROJECTILE_TYPE_OFF;
	pProjectile->pBob->ubFlags = BOB_FLAG_STOP_DRAWING;
}

void projectileUndraw(void) {
	tProjectile *pProjectile;
	UBYTE i;

	pProjectile = &s_pProjectiles[s_ubProjectileCount-1];
	for(i = s_ubProjectileCount; i--;) {
		bobUndraw(pProjectile->pBob, g_pWorldMainBfr);
		--pProjectile;
	}
}

void projectileDraw(void) {
	tProjectile *pProjectile;
	UBYTE i;

	pProjectile = &s_pProjectiles[0];
	for(i = s_ubProjectileCount; i--;) {
		const fix16_t fHalf = fix16_div(fix16_one, fix16_from_int(2));
		WORD wProjectileX, wProjectileY;
		wProjectileX = fix16_to_int(fix16_add(pProjectile->fX, fHalf))-PROJECTILE_BULLET_HEIGHT/2;
		wProjectileY = fix16_to_int(fix16_add(pProjectile->fY, fHalf))-PROJECTILE_BULLET_HEIGHT/2;
		bobDraw(
			pProjectile->pBob, g_pWorldMainBfr,
			wProjectileX, wProjectileY
		);
		++pProjectile;
	}
}

void projectileSim(void) {
	tProjectile *pProjectile;
	tVehicle *pVehicle;
	UBYTE i;
	UBYTE ubMapX, ubMapY, ubBuildingIdx;

	const UBYTE ubDamage = 10;

	for(i = 0; i != s_ubProjectileCount; ++i) {
		pProjectile = &s_pProjectiles[i];
		if(pProjectile->ubType == PROJECTILE_TYPE_OFF)
			continue;

		// Verify projectile lifespan
		if(!pProjectile->uwFrameLife) {
			projectileDestroy(pProjectile);
			continue;
		}
		--pProjectile->uwFrameLife;

		// Increment position
		pProjectile->fX = fix16_add(pProjectile->fX, pProjectile->fDx);
		pProjectile->fY = fix16_add(pProjectile->fY, pProjectile->fDy);

		// Check collision with vehicles
		if(projectileHasCollidedWithAnyPlayer(pProjectile)) {
			projectileDestroy(pProjectile);
			continue;
		}

		// Check collistion with buildings
		ubMapX = fix16_to_int(pProjectile->fX) >> MAP_TILE_SIZE;
		ubMapY = fix16_to_int(pProjectile->fY) >> MAP_TILE_SIZE;
		ubBuildingIdx = g_pMap[ubMapX][ubMapY].ubData;
		if(ubBuildingIdx != BUILDING_IDX_INVALID) {
			if(pProjectile->ubOwnerType == PROJECTILE_OWNER_TYPE_TURRET
				&& g_pMap[ubMapX][ubMapY].ubIdx == MAP_LOGIC_WALL
			)
				continue;
			if(buildingDamage(ubBuildingIdx, ubDamage) == BUILDING_DESTROYED) {
				g_pMap[ubMapX][ubMapY].ubIdx = MAP_LOGIC_DIRT;
				g_pMap[ubMapX][ubMapY].ubData = 0;
				mapRequestUpdateTile(ubMapX, ubMapY);
				explosionsAdd(ubMapX << MAP_TILE_SIZE, ubMapY << MAP_TILE_SIZE);
			}
			projectileDestroy(pProjectile);
			continue;
		}
	}
}

UBYTE projectileHasCollidedWithAnyPlayer(tProjectile *pProjectile) {
	const UBYTE ubDamage = 10;

	for(UBYTE i = 0; i != g_ubPlayerLimit; ++i) {
		if(!g_pPlayers[i].szName[0])
			continue;
		tVehicle *pVehicle = &g_pPlayers[i].sVehicle;
		const fix16_t fQuarterWidth = fix16_from_int(VEHICLE_BODY_WIDTH/4);
		if(
			pProjectile->fX > fix16_sub(pVehicle->fX, fQuarterWidth) &&
			pProjectile->fX < fix16_add(pVehicle->fX, fQuarterWidth) &&
			pProjectile->fY > fix16_sub(pVehicle->fY, fQuarterWidth) &&
			pProjectile->fY < fix16_add(pVehicle->fY, fQuarterWidth)
		) {
			playerDamageVehicle(&g_pPlayers[i], ubDamage);
			projectileDestroy(pProjectile);
			return 1;
		}
	}
	return 0;
}
