#include "gamestates/game/projectile.h"
#include "gamestates/game/vehicle.h"
#include "gamestates/game/bob.h"
#include "gamestates/game/game.h"
#include "gamestates/game/map.h"
#include "gamestates/game/building.h"
#include "gamestates/game/player.h"
#include "gamestates/game/explosions.h"
#include "gamestates/game/console.h"

#define PROJECTILE_BULLET_HEIGHT 2
#define PROJECTILE_DAMAGE 10

tProjectile *s_pProjectiles;
FUBYTE s_fubProjectileMaxCount;
FUBYTE s_fubPrevProjectileAdded;
tBitMap *s_pBulletBitmap;
tBitMap *s_pBulletMask;

fix16_t s_pProjectileDx[VEHICLE_TURRET_ANGLE_COUNT];
fix16_t s_pProjectileDy[VEHICLE_TURRET_ANGLE_COUNT];

void projectileListCreate(FUBYTE fubProjectileMaxCount) {
	logBlockBegin(
		"projectileListCreate(ubProjectileMaxCount: %hhu)", fubProjectileMaxCount
	);

	// Load gfx
	s_pBulletBitmap = bitmapCreateFromFile("data/projectiles/bullet.bm");
	s_pBulletMask = bitmapCreateFromFile("data/projectiles/bullet_mask.bm");

	// Create projectiles
	s_fubProjectileMaxCount = fubProjectileMaxCount;
	s_pProjectiles = memAllocFastClear(fubProjectileMaxCount * sizeof(tProjectile));
	s_fubPrevProjectileAdded = fubProjectileMaxCount - 1;
	for(FUBYTE i = 0; i != fubProjectileMaxCount; ++i) {
		s_pProjectiles[i].ubType = PROJECTILE_TYPE_OFF;
		s_pProjectiles[i].pBob = bobCreate(
			s_pBulletBitmap, s_pBulletMask, 0,
			PROJECTILE_BULLET_HEIGHT, 0
		);
		s_pProjectiles[i].pBob->ubFlags = BOB_FLAG_NODRAW;
	}

	logBlockEnd("projectileListCreate()");

	// Prepare projectile speeds
	for(FUBYTE i = 0; i != VEHICLE_TURRET_ANGLE_COUNT; ++i) {
		s_pProjectileDx[i] = fix16_mul(ccos(i), PROJECTILE_SPEED);
		s_pProjectileDy[i] = fix16_mul(csin(i), PROJECTILE_SPEED);
	}
}

void projectileListDestroy(void) {
	UBYTE i;

	logBlockBegin("projectileListDestroy()");

	// Dealloc bobs
	for(i = s_fubProjectileMaxCount; i--;)
		bobDestroy(s_pProjectiles[i].pBob);
	memFree(s_pProjectiles, s_fubProjectileMaxCount * sizeof(tProjectile));

	// Dealloc bob bitmaps
	bitmapDestroy(s_pBulletBitmap);
	bitmapDestroy(s_pBulletMask);

	logBlockEnd("projectileListDestroy()");
}

tProjectile *projectileCreate(
	UBYTE ubOwnerType, tProjectileOwner uOwner, UBYTE ubType
) {
	// Find free projectile
	tProjectile *pProjectile = 0;
	for(FUBYTE i = s_fubPrevProjectileAdded+1; i != s_fubPrevProjectileAdded; ++i) {
		if(i == s_fubProjectileMaxCount)
			i = 0;
		if(s_pProjectiles[i].ubType == PROJECTILE_TYPE_OFF) {
			pProjectile = &s_pProjectiles[i];
			s_fubPrevProjectileAdded = i;
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
			pProjectile->ubAngle = uOwner.pVehicle->ubTurretAngle;
		else
			pProjectile->ubAngle = uOwner.pVehicle->ubBodyAngle;
		fix16_t fSin = csin(pProjectile->ubAngle);
		fix16_t fCos = ccos(pProjectile->ubAngle);
		pProjectile->fX = fix16_add(uOwner.pVehicle->fX, (VEHICLE_BODY_WIDTH/2) * fCos);
		pProjectile->fY = fix16_add(uOwner.pVehicle->fY, (VEHICLE_BODY_HEIGHT/2) * fSin);
	}
	else {
		pProjectile->fX = fix16_from_int(uOwner.pTurret->uwX);
		pProjectile->fY = fix16_from_int(uOwner.pTurret->uwY);
		pProjectile->ubAngle = uOwner.pTurret->ubAngle;
	}

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

	for(i = s_fubProjectileMaxCount, pProjectile = &s_pProjectiles[0]; i--; ++pProjectile)
		bobUndraw(pProjectile->pBob, g_pWorldMainBfr);
}

void projectileDraw(void) {
	tProjectile *pProjectile;
	FUBYTE i;

	for(i = s_fubProjectileMaxCount, pProjectile = &s_pProjectiles[0]; i--; ++pProjectile) {
		if(!pProjectile->uwFrameLife)
			continue;
		WORD wProjectileX, wProjectileY;
		wProjectileX = fix16_to_int(pProjectile->fX)-PROJECTILE_BULLET_HEIGHT/2;
		wProjectileY = fix16_to_int(pProjectile->fY)-PROJECTILE_BULLET_HEIGHT/2;
		bobDraw(
			pProjectile->pBob, g_pWorldMainBfr,
			wProjectileX, wProjectileY
		);
	}
}

void projectileSim(void) {
	tVehicle *pVehicle;

	tProjectile *pProjectile;
	FUBYTE i;
	for(i = s_fubProjectileMaxCount, pProjectile = &s_pProjectiles[0]; i--; ++pProjectile) {
		if(pProjectile->ubType == PROJECTILE_TYPE_OFF)
			continue;

		// Verify projectile lifespan
		if(!pProjectile->uwFrameLife) {
			projectileDestroy(pProjectile);
			continue;
		}
		--pProjectile->uwFrameLife;

		// Increment position
		pProjectile->fX = fix16_add(pProjectile->fX, s_pProjectileDx[pProjectile->ubAngle]);
		pProjectile->fY = fix16_add(pProjectile->fY, s_pProjectileDy[pProjectile->ubAngle]);

		// Check collision with vehicles
		if(projectileHasCollidedWithAnyPlayer(pProjectile)) {
			projectileDestroy(pProjectile);
			continue;
		}

		// Check collistion with buildings
		UBYTE ubMapX = fix16_to_int(pProjectile->fX) >> MAP_TILE_SIZE;
		UBYTE ubMapY = fix16_to_int(pProjectile->fY) >> MAP_TILE_SIZE;
		UBYTE ubBuildingIdx = g_pMap[ubMapX][ubMapY].ubData;
		if(ubBuildingIdx != BUILDING_IDX_INVALID) {
			if(
				pProjectile->ubOwnerType == PROJECTILE_OWNER_TYPE_TURRET
				&& g_pMap[ubMapX][ubMapY].ubIdx == MAP_LOGIC_WALL
			) {
				continue;
			}
			if(buildingDamage(ubBuildingIdx, PROJECTILE_DAMAGE) == BUILDING_DESTROYED) {
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
	for(FUBYTE i = g_ubPlayerLimit; i--;) {
		if(!g_pPlayers[i].szName[0] || g_pPlayers[i].ubState != PLAYER_STATE_DRIVING)
			continue;
		tVehicle *pVehicle = &g_pPlayers[i].sVehicle;
		const fix16_t fQuarterWidth = fix16_from_int(VEHICLE_BODY_WIDTH/4);
		if(
			pProjectile->fX > fix16_sub(pVehicle->fX, fQuarterWidth) &&
			pProjectile->fX < fix16_add(pVehicle->fX, fQuarterWidth) &&
			pProjectile->fY > fix16_sub(pVehicle->fY, fQuarterWidth) &&
			pProjectile->fY < fix16_add(pVehicle->fY, fQuarterWidth)
		) {
			if(playerDamageVehicle(&g_pPlayers[i], PROJECTILE_DAMAGE)) {
				char szBfr[CONSOLE_MESSAGE_MAX];
				if(pProjectile->ubOwnerType == PROJECTILE_OWNER_TYPE_TURRET)
					sprintf(szBfr, "%s was killed by turret", g_pPlayers[i].szName);
				else
					sprintf(
						szBfr, "%s was killed by %s",g_pPlayers[i].szName,
						playerGetByVehicle(pProjectile->uOwner.pVehicle)->szName
					);
				consoleWrite(szBfr, CONSOLE_COLOR_GENERAL);
			}
			projectileDestroy(pProjectile);
			return 1;
		}
	}
	return 0;
}
