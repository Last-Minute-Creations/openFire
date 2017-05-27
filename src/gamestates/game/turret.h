#ifndef GUARD_OF_GAMESTATES_GAME_TURRET_H
#define GUARD_OF_GAMESTATES_GAME_TURRET_H

#include <ace/config.h>
#include <gamestates/game/bob.h>
#include <gamestates/game/projectile.h>

#define TURRET_INVALID 0xFFFF
#define TURRET_MAX_HP 100
#define TURRET_BOB_SIZE 16
#define TURRET_MIN_DISTANCE 96

typedef struct _tTurret {
	UBYTE ubHp;
	UBYTE ubTeam;
	UWORD uwY;
	UWORD uwX;
	UBYTE ubAngle;
	tBob *pBob;
} tTurret;

extern tBobSource g_sBrownTurretSource;

void turretListCreate(
	IN UBYTE ubMaxTurrets
);

void turretListDestroy(void);

UWORD turretCreate(
	IN UWORD uwX,
	IN UWORD uwY,
	IN UBYTE ubTeam
);

void turretCheckHit(
	IN tTurret *pTurret,
	IN tProjectile *pProjectile
);

void turretDestroy(
	IN UWORD uwIdx
);

void turretProcess(void);

void turretDrawAll(void);

void turretUndrawAll(void);

#endif
