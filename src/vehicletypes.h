#ifndef GUARD_OF_VEHICLETYPES_H
#define GUARD_OF_VEHICLETYPES_H

#include <ace/config.h>
#include "gamestates/game/bob.h"
#include "gamestates/game/team.h"

#define VEHICLE_TYPE_COUNT 4
#define VEHICLE_TYPE_TANK 0
#define VEHICLE_TYPE_CHOPPER 1
#define VEHICLE_TYPE_ASV 2
#define VEHICLE_TYPE_JEEP 3

#define VEHICLE_BODY_WIDTH 32
#define VEHICLE_BODY_HEIGHT 32
#define VEHICLE_BODY_ANGLE_COUNT 64
#define VEHICLE_TURRET_ANGLE_COUNT 128
#define VEHICLE_TURRET_WIDTH 32
#define VEHICLE_TURRET_HEIGHT 32

typedef struct {
	UBYTE ubFwdSpeed;                                     ///< Forward movement speed
	UBYTE ubBwSpeed;                                      ///< Backward movement speed
	UBYTE ubRotSpeed;                                     ///< Rotate speed
	UBYTE ubRotSpeedDiv;                                  ///< Rotate speed divider - do rotation every ubRotSpeedDiv frames
	UBYTE ubMaxBaseAmmo;                                  ///< Tank cannon, chopper gun, ASV rockets, jeep 'nades
	UBYTE ubMaxSuperAmmo;                                 ///< Chopper rockets, ASV mines
	UBYTE ubMaxFuel;
	UBYTE ubMaxLife;
	tBobSource sMainSource[TEAM_COUNT];                   ///< Main bob gfx source.
	tBobSource sAuxSource[TEAM_COUNT];                    ///< Tank turret & chopper takeoff gfx source.
	tBCoordYX pCollisionPts[VEHICLE_BODY_ANGLE_COUNT][8]; ///< Collision points
} tVehicleType;

void vehicleTypesCreate(
	OUT BYTE *pProgress
);

void vehicleTypesDestroy(void);

UWORD vehicleTypeBobSourceLoad(
	IN char *szName,
	IN tBobSource *pBobSource,
	IN UBYTE isWithMask
);

void vehicleTypeBobSourceUnload(
	IN tBobSource *pSource
);

extern tVehicleType g_pVehicleTypes[VEHICLE_TYPE_COUNT];

#endif // GUARD_OF_VEHICLETYPES_H
