#include "gamestates/game/turret.h"

#include <ace/managers/memory.h>
#include <ace/managers/key.h>
#include <ace/libfixmath/fix16.h>
#include "gamestates/game/world.h"
#include "gamestates/game/vehicle.h"
#include "gamestates/game/bob.h"
#include "gamestates/game/player.h"

static UBYTE s_ubMaxTurrets;
static tTurret *s_pTurretList;
tBobSource g_sBrownTurretSource;

void turretListCreate(UBYTE ubMaxTurrets) {
	int i;

	s_ubMaxTurrets = ubMaxTurrets;
	s_pTurretList = memAllocFast(ubMaxTurrets * sizeof(tTurret));
	for(i = 0; i != s_ubMaxTurrets; ++i) {
		s_pTurretList[i].pBob = bobCreate(g_sBrownTurretSource.pBitmap, g_sBrownTurretSource.pMask, TURRET_BOB_SIZE, 0);
		s_pTurretList[i].pBob->ubFlags = BOB_FLAG_NODRAW;
	}
}

void turretListDestroy(void) {
	int i;
	for(i = 0; i != s_ubMaxTurrets; ++i) {
		bobDestroy(s_pTurretList[i].pBob);
	}
	memFree(s_pTurretList, s_ubMaxTurrets * sizeof(tTurret));
}

UWORD turretCreate(UWORD uwTileX, UWORD uwTileY, UBYTE ubTeam) {
	tTurret *pTurret;
	UBYTE i;

	// Find next free turret
	for(i = 0; i != s_ubMaxTurrets; ++i)
		if(!s_pTurretList[i].ubHp)
			break;
	if(i == s_ubMaxTurrets)
		return TURRET_INVALID;
	pTurret = &s_pTurretList[i];

	// Initial values
	pTurret->uwX = (uwTileX << MAP_TILE_SIZE) + (1 << (MAP_TILE_SIZE-1));
	pTurret->uwY = (uwTileY << MAP_TILE_SIZE) + (1 << (MAP_TILE_SIZE-1));
	pTurret->ubHp = TURRET_MAX_HP;
	pTurret->ubTeam = ubTeam;
	pTurret->ubAngle = ANGLE_90;
	bobChangeFrame(pTurret->pBob, angleToFrame(pTurret->ubAngle));
	pTurret->pBob->ubFlags = BOB_FLAG_START_DRAWING;
	return i;
}

void turretDestroy(UWORD uwIdx) {
	if(uwIdx >= s_ubMaxTurrets) {
		logWrite("ERR: turretDestroy() - Index out of range %u\n", uwIdx);
		return;
	}
	s_pTurretList[uwIdx].pBob->ubFlags = BOB_FLAG_STOP_DRAWING;
}

void turretProcess(void) {
	UBYTE ubPlayerIdx, ubTurretIdx;
	tPlayer *pPlayer, *pClosestPlayer;
	UWORD uwClosestDist, uwDist, uwDx, uwDy;
	UBYTE ubDestAngle;
	tTurret *pTurret;

	for(ubTurretIdx = 0; ubTurretIdx != s_ubMaxTurrets; ++ubTurretIdx) {
		pTurret = &s_pTurretList[ubTurretIdx];

		if(!pTurret->ubHp)
			continue;

		// Scan nearby enemies
		uwClosestDist = TURRET_MIN_DISTANCE;
		pClosestPlayer = 0;
		for(ubPlayerIdx = 0; ubPlayerIdx != g_ubPlayerCount; ++ubPlayerIdx) {
			pPlayer = &g_pPlayers[ubPlayerIdx];

			// Same team or not on map?
			if(/*pPlayer->ubTeam == pTurret->ubTeam ||*/ pPlayer->ubState != PLAYER_STATE_DRIVING)
				continue;

			// Calculate distance between turret & player
			uwDx = pPlayer->sVehicle.fX - pTurret->uwX;
			uwDy = pPlayer->sVehicle.fY - pTurret->uwY;
			uwDist = fix16_to_int(fix16_sqrt(fix16_from_int(uwDx*uwDx + uwDy*uwDy)));
			if(uwDist < TURRET_MIN_DISTANCE && uwDist <= uwClosestDist) {
				pClosestPlayer = pPlayer;
				uwClosestDist = uwDist;
			}
		}

		// Anything in range?
		if(!pClosestPlayer)
			continue;
		uwDx = pClosestPlayer->sVehicle.fX - pTurret->uwX;
		uwDy = pClosestPlayer->sVehicle.fY - pTurret->uwY;

		// Determine destination angle
		// ubDestAngle = ((pi + atan2(uwDx, uwDy)) * 64)/(2*pi) * 2
		ubDestAngle = ANGLE_90 + 2 * fix16_to_int(
			fix16_div(
				fix16_mul(
					fix16_add(fix16_pi, fix16_atan2(fix16_from_int(uwDx), fix16_from_int(-uwDy))),
					fix16_from_int(64)
				),
				fix16_pi*2
			)
		);
		if(ubDestAngle >= ANGLE_360)
			ubDestAngle -= ANGLE_360;

		if(pTurret->ubAngle != ubDestAngle) {
			// TODO: Rotate turret into enemy position
			WORD wDelta;
			wDelta = ubDestAngle - pTurret->ubAngle;
			if((wDelta > 0 && wDelta < ANGLE_180) || wDelta + ANGLE_360 < ANGLE_180) {
				// Rotate clockwise
				pTurret->ubAngle += 2;
			}
			else {
				// Rotate anti-clockwise
				pTurret->ubAngle += ANGLE_360 - 2;
			}
			while(pTurret->ubAngle >= ANGLE_360)
				pTurret->ubAngle -= ANGLE_360;
			bobChangeFrame(pTurret->pBob, angleToFrame(pTurret->ubAngle));
		}
		else {
			// TODO: Fire
			// projectileCreate();
		}
	}
}

void turretDrawAll(void) {
	WORD uwTurret;
	tTurret *pTurret;
	for(uwTurret = 0; uwTurret != s_ubMaxTurrets; ++uwTurret) {
		pTurret = &s_pTurretList[uwTurret];
		bobDraw(
			pTurret->pBob,
			g_pWorldMainBfr->pBuffer,
			pTurret->uwX - VEHICLE_BODY_WIDTH/2,
			pTurret->uwY - VEHICLE_BODY_HEIGHT/2
		);
	}
}

void turretUndrawAll(void) {
	UWORD uwTurret;
	for(uwTurret = 0; uwTurret != s_ubMaxTurrets; ++uwTurret) {
		bobUndraw(
			s_pTurretList[uwTurret].pBob,
			g_pWorldMainBfr->pBuffer
		);
	}
}
