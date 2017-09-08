#ifndef GUARD_OF_GAMESTATES_GAME_PROJECTILE_H
#define GUARD_OF_GAMESTATES_GAME_PROJECTILE_H

#include <ace/config.h>
#include "gamestates/game/vehicle.h"
#include "gamestates/game/turret.h"
#include "gamestates/game/bob.h"

// Projectile types.
#define PROJECTILE_TYPE_OFF    0
#define PROJECTILE_TYPE_CANNON 1
#define PROJECTILE_TYPE_NADE   2
#define PROJECTILE_TYPE_ROCKET 3

// Projectile owner types.
#define PROJECTILE_OWNER_TYPE_VEHICLE 0
#define PROJECTILE_OWNER_TYPE_TURRET  1

struct _tVehicle;
struct _tTurret;

typedef union _tProjectileOwner {
	struct _tVehicle *pVehicle;
	struct _tTurret *pTurret;
} tProjectileOwner;

typedef struct _tProjectile {
	tProjectileOwner uOwner;  ///< Owner for scoring kills
	tBob *pBob;                ///< Bob for projectile display
	float fX;                  ///< X-coord of current position.
	float fY;                  ///< Ditto, Y-coord.
	float fDx;                 ///< Delta X per frame.
	float fDy;                 ///< Ditto, delta Y.
	UWORD uwFrameLife;         ///< Projectile life, in remaining frame count.
	UBYTE ubType;              ///< See PROJECTILE_TYPE_* defines.
	UBYTE ubOwnerType;         ///< See PROJECTILE_OWNER_TYPE_* defines.
} tProjectile;

void projectileListCreate(
	IN UBYTE ubProjectileCount
);
void projectileListDestroy(void);

tProjectile *projectileCreate(
	IN UBYTE ubOwnerType,
	IN tProjectileOwner uOwner,
	IN UBYTE ubType
);

void projectileDestroy(
	IN tProjectile *pProjectile
);

void projectileUndraw(void);
void projectileDraw(void);
void projectileProcess(void);

UBYTE projectileHasCollidedWithAnyPlayer(
	IN tProjectile *pProjectile
);


#endif // GUARD_OF_GAMESTATES_GAME_PROJECTILE_H
