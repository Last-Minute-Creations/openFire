#ifndef GUARD_OF_GAMESTATES_GAME_PROJECTILE_H
#define GUARD_OF_GAMESTATES_GAME_PROJECTILE_H

#include "gamestates/game/vehicle.h"
#include <ace/config.h>
#include "gamestates/game/bob.h"

#define PROJECTILE_TYPE_OFF    0
#define PROJECTILE_TYPE_CANNON 1
#define PROJECTILE_TYPE_NADE   2
#define PROJECTILE_TYPE_ROCKET 3

typedef struct _tProjectile {
	tVehicle *pOwner;  ///< Owner for scoring kills
	tBob *pBob;        ///< Bob for projectile display
	float fX;          ///< X-coord of current position.
	float fY;          ///< Ditto, Y-coord.
	float fDx;         ///< Delta X per frame.
	float fDy;         ///< Ditto, delta Y.
	UWORD uwFrameLife; ///< Projectile life, in remaining frame count.
	UBYTE ubType;      ///< See PRJECTILE_TYPE_* defines.
} tProjectile;

void projectileListCreate(
	IN UBYTE ubProjectileCount
);
void projectileListDestroy(void);

tProjectile *projectileCreate(
	IN tVehicle *pOwner,
	IN UBYTE ubType
);

void projectileDestroy(
	IN tProjectile *pProjectile
);

void projectileUndraw(void);
void projectileDraw(void);
void projectileProcess(void);


#endif // GUARD_OF_GAMESTATES_GAME_PROJECTILE_H
