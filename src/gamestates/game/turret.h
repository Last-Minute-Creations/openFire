#ifndef GUARD_OF_GAMESTATES_GAME_TURRET_H
#define GUARD_OF_GAMESTATES_GAME_TURRET_H

#include <ace/config.h>
#include <gamestates/game/bob.h>
#include <gamestates/game/projectile.h>

#define TURRET_INVALID      0xFFFF
#define TURRET_SPRITE_SIZE  16
#define TURRET_MIN_DISTANCE (PROJECTILE_RANGE+32)
#define TURRET_COOLDOWN     PROJECTILE_FRAME_LIFE

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

extern tBobSource g_sTurretSource;
extern UWORD g_uwTurretCount;
extern tTurret *g_pTurrets;

void turretListCreate(void);
void turretListDestroy(void);

UWORD turretAdd(
	IN UWORD uwX,
	IN UWORD uwY,
	IN UBYTE ubTeam
);

void turretDestroy(
	IN UWORD uwIdx
);

void turretCapture(
	IN UWORD uwIdx,
	IN FUBYTE fubTeam
);

void turretSim(void);

void turretUpdateSprites(void);

#endif
