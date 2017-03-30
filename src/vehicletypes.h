#ifndef GUARD_OF_VEHICLETYPES_H
#define GUARD_OF_VEHICLETYPES_H

#include <ace/config.h>
#include "gamestates/game/bob.h"

#define VEHICLE_TYPE_COUNT 4
#define VEHICLE_TYPE_TANK 0
#define VEHICLE_TYPE_CHOPPER 1
#define VEHICLE_TYPE_ASV 2
#define VEHICLE_TYPE_JEEP 3

#define VEHICLE_BODY_WIDTH 32
#define VEHICLE_BODY_HEIGHT 32
#define VEHICLE_BODY_ANGLE_COUNT 64
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
	tBobSource sMainSource;                               ///< Main bob gfx source.
	tBobSource sAuxSource;                                ///< Tank turret & chopper takeoff gfx source.
	tBCoordYX pCollisionPts[VEHICLE_BODY_ANGLE_COUNT][8]; ///< Collision points
} tVehicleType;

void vehicleTypesCreate(void);

void vehicleTypesDestroy(void);

UWORD vehicleTypeBobSourceLoad(char *szName, tBobSource *pBobSource);

extern tVehicleType g_pVehicleTypes[VEHICLE_TYPE_COUNT];
extern tUwCoordYX g_pTurretCoords[64];
extern float g_pSin[128];

#endif // GUARD_OF_VEHICLETYPES_H
