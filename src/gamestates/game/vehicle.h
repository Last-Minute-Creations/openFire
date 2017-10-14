#ifndef GUARD_OF_GAMESTATES_GAME_VEHICLE_H
#define GUARD_OF_GAMESTATES_GAME_VEHICLE_H

#include <ace/config.h>
#include <ace/utils/bitmap.h>
#include <ace/utils/bitmapmask.h>
#include "vehicletypes.h"
#include "gamestates/game/map.h"
#include "gamestates/game/gamemath.h"
#include "gamestates/game/projectile.h"

/// Vehicle-specific constants
#define VEHICLE_TANK_COOLDOWN PROJECTILE_FRAME_LIFE

typedef struct _tSteerRequest {
	UBYTE ubForward;
	UBYTE ubBackward;
	UBYTE ubLeft;
	UBYTE ubRight;
	UBYTE ubAction1;
	UBYTE ubAction2;
	UBYTE ubAction3;
	UBYTE ubDestAngle;
} tSteerRequest;

typedef struct _tVehicle {
	tVehicleType *pType; ///< Ptr to vehicle type definition
	tBob *pBob;          ///< Main body bob
	tBob *pAuxBob;       ///< Tank - turret, chopper - takeoff anim
	fix16_t fX;          ///< Vehicle X-position relative to center of gfx.
	fix16_t fY;          ///< Ditto, vehicle Y.
	UWORD uwX;           ///< Same as fX, but converted to UWORD. Read-only.
	UWORD uwY;           ///< Ditto.
	UBYTE ubBodyAngle;   ///< Measured clockwise, +90deg is to bottom.
	UBYTE ubTurretAngle; ///< NOT relative to body angle, measured as above.
	UBYTE ubBaseAmmo;
	UBYTE ubSuperAmmo;
	BYTE  bRotDiv;
	UBYTE ubFuel;
	UBYTE ubLife;
	UBYTE ubCooldown; ///< Cooldown timer after fire
	UBYTE ubIsOnSilo;
} tVehicle;

void vehicleInit(
	IN tVehicle *pVehicle,
	IN UBYTE ubVehicleType,
	IN UBYTE ubSpawnIdx
);

void vehicleUnset(
	tVehicle *pVehicle
);

void vehicleDrawFrame(
	IN UWORD uwX,
	IN UWORD uwY,
	IN UBYTE ubDAngle
);

void vehicleSteerTank(
	IN tVehicle *pVehicle,
	IN tSteerRequest *pSteerRequest
);
void vehicleSteerJeep(
	IN tVehicle *pVehicle,
	IN tSteerRequest *pSteerRequest
);

void vehicleDraw(
	IN tVehicle *pVehicle
);

void vehicleUndraw(
	IN tVehicle *pVehicle
);

void vehicleSetupBob(
	IN tVehicle *pVehicle
);

#endif
