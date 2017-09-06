#ifndef GUARD_OF_GAMESTATES_GAME_TURRET_H
#define GUARD_OF_GAMESTATES_GAME_TURRET_H

#include <ace/config.h>
#include <gamestates/game/bob.h>
#include <gamestates/game/projectile.h>

#define TURRET_INVALID 0xFFFF
#define TURRET_SPRITE_SIZE 16
#define TURRET_MIN_DISTANCE 96
#define TURRET_COOLDOWN 25

/**
 *  Turret struct.
 *  Turrets can't have X = 0, because that's the way for checking
 *  if they are valid.
 */
typedef struct _tTurret {
	UWORD uwY;
	UWORD uwX;
	UBYTE ubTeam;
	UBYTE ubAngle;
	UBYTE ubCooldown;
} tTurret;

extern tBobSource g_sBrownTurretSource;

void turretListCreate(void);
void turretListDestroy(void);

UWORD turretCreate(
	IN UWORD uwX,
	IN UWORD uwY,
	IN UBYTE ubTeam
);

void turretDestroy(
	IN UWORD uwIdx
);

void turretProcess(void);

void turretUpdateSprites(void);

#endif
