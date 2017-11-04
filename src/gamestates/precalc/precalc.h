#ifndef GUARD_OF_PRECALC_PRECALC_H
#define GUARD_OF_PRECALC_PRECALC_H

#include "vehicletypes.h"

void precalcCreate(void);

void precalcLoop(void);

void precalcDestroy(void);

void precalcIncreaseProgress(
	IN FUBYTE fubAmountToAdd,
	IN char *szText
);

#endif // GUARD_OF_PRECALC_PRECALC_H
