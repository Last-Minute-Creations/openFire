#ifndef GUARD_OF_GAMESTATES_GAME_VEHICLE_H
#define GUARD_OF_GAMESTATES_GAME_VEHICLE_H

#include <ace/config.h>
#include <ace/utils/bitmap.h>
#include <ace/utils/bitmapmask.h>
#include "vehicletypes.h"
#include "gamestates/game/map.h"

/// Vehicle-specific constants
#define VEHICLE_TANK_COOLDOWN 25

#define ANGLE_0    0
#define ANGLE_45   16
#define ANGLE_90   32
#define ANGLE_180  64
#define ANGLE_360  128
#define ANGLE_LAST 127
#define csin(x) (g_pSin[x])
#define ccos(x) ((x < 96?csin(ANGLE_90+x):csin(x-3*ANGLE_90)))
#define angleToFrame(angle) (angle>>1)

typedef struct _tSteerRequest {
	UBYTE ubForward;
	UBYTE ubBackward;
	UBYTE ubLeft;
	UBYTE ubRight;
	UBYTE ubTurretLeft;
	UBYTE ubTurretRight;
	UBYTE ubAction1;
	UBYTE ubAction2;
	UBYTE ubAction3;
} tSteerRequest;

typedef struct {
	tVehicleType *pType; ///< Ptr to vehicle type definition
	tBob *pBob;          ///< Main body bob
	tBob *pAuxBob;       ///< Tank - turret, chopper - takeoff anim
	float fX;            ///< Vehicle X-position relative to center of gfx.
	float fY;            ///< Ditto, vehicle Y.
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

tVehicle *vehicleCreate(
	IN UBYTE ubVehicleType
);

void vehicleDestroy(
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

#endif
