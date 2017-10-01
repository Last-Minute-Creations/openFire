#include "gamestates/game/gamemath.h"

UBYTE getAngleBetweenPoints(
	UWORD uwSrcX, UWORD uwSrcY, UWORD uwDstX, UWORD uwDstY
) {
	UWORD uwDx = uwDstX - uwSrcX;
	UWORD uwDy = uwDstY - uwSrcY;
	// calc: ubAngle = ((pi + atan2(uwDx, uwDy)) * 64)/(2*pi) * 2
	UBYTE ubAngle = ANGLE_90 + 2 * fix16_to_int(
		fix16_div(
			fix16_mul(
				fix16_add(fix16_pi, fix16_atan2(fix16_from_int(uwDx), fix16_from_int(-uwDy))),
				fix16_from_int(64)
			),
			fix16_pi*2
		)
	);
	if(ubAngle >= ANGLE_360)
		ubAngle -= ANGLE_360;
	return ubAngle;
}

WORD getDeltaAngleDirection(UBYTE ubPrevAngle, UBYTE ubNewAngle, WORD wUnit) {
	WORD wDelta = ubNewAngle - ubPrevAngle;
	if(!wDelta)
		return 0;
	if((wDelta > 0 && wDelta < ANGLE_180) || wDelta + ANGLE_360 < ANGLE_180)
		return wUnit;
	return -wUnit;
}

void generateSine(void) {
	UBYTE i;
	for(i = 0; i != 128; ++i)
		g_pSin[i] = fix16_sin(fix16_div(i*2*fix16_pi, fix16_from_int(128)));
}

fix16_t g_pSin[128];
