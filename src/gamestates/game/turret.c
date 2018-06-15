#include "gamestates/game/turret.h"
#include <ace/managers/memory.h>
#include <ace/managers/key.h>
#include <ace/managers/rand.h>
#include <ace/managers/system.h>
#include <ace/utils/custom.h>
#include "gamestates/game/vehicle.h"
#include "gamestates/game/player.h"
#include "gamestates/game/explosions.h"
#include "gamestates/game/team.h"

#define TURRET_BOB_SIZE  16

UWORD g_uwTurretCount;
tTurret *g_pTurrets; // 20x25: 1100*7 ~ 8KiB
tBitMap *g_pTurretFrames[TEAM_COUNT+1];
tBitMap *g_pTurretMasks;

static UWORD s_uwMaxTurrets;
UWORD g_pTurretTiles[MAP_MAX_SIZE][MAP_MAX_SIZE]; // 32k FAST

static FUBYTE s_fubMapWidth, s_fubMapHeight;

void turretListCreate(FUBYTE fubMapWidth, FUBYTE fubMapHeight) {
	logBlockBegin("turretListCreate()");
	s_fubMapWidth = fubMapWidth;
	s_fubMapHeight = fubMapHeight;

	g_uwTurretCount = 0;
	s_uwMaxTurrets = (fubMapWidth/2 + 1) * fubMapHeight;
	g_pTurrets = memAllocFastClear(s_uwMaxTurrets * sizeof(tTurret));

	// TODO: could be only number of turrets per frame + prev for undraw (or not)
	for(UWORD i = 0; i < s_uwMaxTurrets; ++i) {
		bobNewInit(
			&g_pTurrets[i].sBob, TURRET_BOB_SIZE, TURRET_BOB_SIZE, 1,
			g_pTurretFrames[TEAM_NONE], g_pTurretMasks, 0, 0
		);
	}

	logBlockEnd("turretListCreate()");
}

void turretListDestroy(void) {
	logBlockBegin("turretListDestroy()");

	memFree(g_pTurrets, s_uwMaxTurrets * sizeof(tTurret));

	logBlockEnd("turretListDestroy()");
}

UWORD turretAdd(UWORD uwTileX, UWORD uwTileY, UBYTE ubTeam) {
	logBlockBegin(
		"turretAdd(uwTileX: %hu, uwTileY: %hu, ubTeam: %hhu)",
		uwTileX, uwTileY, ubTeam
	);

	// Initial values
	tTurret *pTurret = &g_pTurrets[g_uwTurretCount];
	pTurret->sBob.sPos.sUwCoord.uwX = uwTileX << MAP_TILE_SIZE;
	pTurret->sBob.sPos.sUwCoord.uwY = uwTileY << MAP_TILE_SIZE;
	pTurret->uwCenterX = (uwTileX << MAP_TILE_SIZE) + MAP_HALF_TILE;
	pTurret->uwCenterY = (uwTileY << MAP_TILE_SIZE) + MAP_HALF_TILE;
	pTurret->ubTeam = ubTeam;
	pTurret->ubAngle = ANGLE_90;
	pTurret->ubDestAngle = ANGLE_90;
	pTurret->isTargeting = 0;
	pTurret->ubCooldown = 0;
	pTurret->fubSeq = (uwTileX & 3) |	((uwTileY & 3) << 2);
	pTurret->sBob.pBitmap = g_pTurretFrames[ubTeam];

	// Add to tile-based list
	g_pTurretTiles[uwTileX][uwTileY] = g_uwTurretCount;

	logBlockEnd("turretAdd()");
	return g_uwTurretCount++;
}

void turretDestroy(UWORD uwIdx) {
	// Find turret
	if(uwIdx >= s_uwMaxTurrets) {
		logWrite("ERR: turretDestroy() - Index out of range %u\n", uwIdx);
		return;
	}
	tTurret *pTurret = &g_pTurrets[uwIdx];

	// Already destroyed?
	if(!pTurret->uwCenterX)
		return;

	// Remove from tile-based list
	UWORD uwTileX = pTurret->uwCenterX >> MAP_TILE_SIZE;
	UWORD uwTileY = pTurret->uwCenterY >> MAP_TILE_SIZE;
	g_pTurretTiles[uwTileX][uwTileY] = TURRET_INVALID;

	// Add explosion
	explosionsAdd(
		uwTileX << MAP_TILE_SIZE, uwTileY << MAP_TILE_SIZE
	);

	// Mark turret as destroyed
	pTurret->uwCenterX = 0;
}

static void turretUpdateTarget(tTurret *pTurret) {
	pTurret->isTargeting = 0;
	// Scan nearby enemies
	UBYTE ubEnemyTeam = pTurret->ubTeam == TEAM_BLUE ? TEAM_RED : TEAM_BLUE;
	tPlayer *pClosestPlayer = playerGetClosestInRange(
		pTurret->uwCenterX, pTurret->uwCenterY, TURRET_MIN_DISTANCE, ubEnemyTeam
	);

	// Anything in range?
	if(pClosestPlayer) {
		pTurret->isTargeting = 1;
		// Determine destination angle
		pTurret->ubDestAngle = getAngleBetweenPoints(
			pTurret->uwCenterX, pTurret->uwCenterY,
			pClosestPlayer->sVehicle.uwX,
			pClosestPlayer->sVehicle.uwY
		);
	}
}

void turretSim(void) {
	FUBYTE fubSeq = g_ulGameFrame & 15;

	for(UWORD uwTurretIdx = 0; uwTurretIdx != s_uwMaxTurrets; ++uwTurretIdx) {
		tTurret *pTurret = &g_pTurrets[uwTurretIdx];
		if(!pTurret->uwCenterX || pTurret->ubTeam == TEAM_NONE) {
			continue;
		}

		if(pTurret->fubSeq == fubSeq) {
			turretUpdateTarget(pTurret);
		}

		// Process cooldown
		if(pTurret->ubCooldown) {
			--pTurret->ubCooldown;
		}

		if(pTurret->ubAngle != pTurret->ubDestAngle) {
			pTurret->ubAngle += ANGLE_360 + getDeltaAngleDirection(
				pTurret->ubAngle, pTurret->ubDestAngle, 2
			);
			if(pTurret->ubAngle >= ANGLE_360) {
				pTurret->ubAngle -= ANGLE_360;
			}
			bobNewSetBitMapOffset(
				&pTurret->sBob, angleToFrame(pTurret->ubAngle) * TURRET_BOB_SIZE
			);
		}
		else if(pTurret->isTargeting && !pTurret->ubCooldown) {
			tProjectileOwner uOwner;
			uOwner.pTurret = pTurret;
			projectileCreate(PROJECTILE_OWNER_TYPE_TURRET, uOwner, PROJECTILE_TYPE_BULLET);
			pTurret->ubCooldown = TURRET_COOLDOWN;
		}

		bobNewPush(&pTurret->sBob);
	}
}

void turretCapture(UWORD uwIdx, FUBYTE fubTeam) {
	tTurret *pTurret = &g_pTurrets[uwIdx];
	pTurret->ubTeam = fubTeam;
	pTurret->isTargeting = 0;
	pTurret->sBob.pBitmap = g_pTurretFrames[fubTeam];
}
