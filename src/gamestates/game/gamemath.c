#include "gamestates/game/gamemath.h"
#include <ace/generic/screen.h>

#define ATAN2_SCALE 8

static WORD s_pAtan2[SCREEN_PAL_HEIGHT / ATAN2_SCALE][SCREEN_PAL_WIDTH / ATAN2_SCALE];

UBYTE getAngleBetweenPoints(
	UWORD uwSrcX, UWORD uwSrcY, UWORD uwDstX, UWORD uwDstY
) {
	WORD wDx = uwDstX - uwSrcX;
	WORD wDy = uwDstY - uwSrcY;
	UBYTE ubAngle = catan2(wDy, wDx);
	if(ubAngle >= ANGLE_360) {
		ubAngle -= ANGLE_360;
	}
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

WORD catan2(WORD wDy, WORD wDx) {
	return
		wDx >= 0 && wDy >= 0 ? s_pAtan2[wDy / ATAN2_SCALE][wDx / ATAN2_SCALE] :
		wDx >= 0 && wDy < 0 ? (ANGLE_360 - s_pAtan2[((UWORD)-wDy) / ATAN2_SCALE][wDx / ATAN2_SCALE]) :
		wDx < 0 && wDy >= 0 ? (ANGLE_180 - s_pAtan2[wDy / ATAN2_SCALE][((UWORD)-wDx) / ATAN2_SCALE]) :
		/* wDx < 0 && wDy < 0 ? */ (ANGLE_180 + s_pAtan2[((UWORD)-wDy) / ATAN2_SCALE][((UWORD)-wDx) / ATAN2_SCALE]);
}

UWORD fastMagnitude(UWORD uwDx, UWORD uwDy) {
	// Get approximate distance |(xo,yo)| using aMin+bMax with a=15/16 and b=15/32
	// Average error is around 1.8%
	// |(dx,dy)| ~= (P+0.5Q) - ((P+0.5Q) >> 4)
	// P = max(|dx|,|dy|)
	// Q = min(|dx|,|dy|)
	UWORD uwP, uwQ;
	if(uwDx > uwDy) {
		uwP = uwDx;
		uwQ = uwDy;
	}
	else {
		uwP = uwDy;
		uwQ = uwDx;
	}
	UWORD uwHalfQ = uwQ / 2;
	UWORD uwPPlusHalfQ = uwP + uwHalfQ;
	return uwPPlusHalfQ - (uwPPlusHalfQ >> 4);
}

void gameMathInit(void) {
	for(UWORD uwY = 0; uwY < SCREEN_PAL_HEIGHT / ATAN2_SCALE; ++uwY) {
		for(UWORD uwX = 0; uwX < SCREEN_PAL_WIDTH / ATAN2_SCALE; ++uwX) {
			s_pAtan2[uwY][uwX] = fix16_to_int(fix16_one/2 + fix16_div(ANGLE_180 * fix16_atan2(fix16_from_int(uwY), fix16_from_int(uwX)), fix16_pi));
			if(s_pAtan2[uwY][uwX] >= ANGLE_360) {
				s_pAtan2[uwY][uwX] -= ANGLE_360;
			}
		}
	}
	s_pAtan2[0][0] = ANGLE_0;
}

fix16_t g_pSin[128] = {
	0, 3215, 6423, 9616, 12785, 15923, 19024, 22078, 25079,
	28020, 30893, 33692, 36409, 39039, 41575, 44011, 46340,
	48558, 50660, 52639, 54491, 56212, 57797, 59243, 60547,
	61705, 62714, 63571, 64276, 64826, 65220, 65457, 65536,
	65457, 65220, 64826, 64276, 63571, 62714, 61705, 60547,
	59243, 57797, 56212, 54491, 52639, 50660, 48558, 46340,
	44011, 41575, 39039, 36409, 33692, 30893, 28020, 25079,
	22078, 19024, 15923, 12785, 9616, 6423, 3215, 0,
	-3215, -6423, -9616, -12785, -15923, -19024, -22078, -25079,
	-28020, -30893, -33692, -36409, -39039, -41575, -44011, -46340,
	-48558, -50660, -52639, -54491, -56212, -57797, -59243, -60547,
	-61705, -62714, -63571, -64276, -64826, -65220, -65457, -65536,
	-65457, -65220, -64826, -64276, -63571, -62714, -61705, -60547,
	-59243, -57797, -56212, -54491, -52639, -50660, -48558, -46340,
	-44011, -41575, -39039, -36409, -33692, -30893, -28020, -25079,
	-22078, -19024, -15923, -12785, -9616, -6423, -3215
};
