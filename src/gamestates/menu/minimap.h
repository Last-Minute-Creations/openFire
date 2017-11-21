#ifndef GUARD_OF_GAMESTATES_MENU_MINIMAP_H
#define GUARD_OF_GAMESTATES_MENU_MINIMAP_H

#include <ace/types.h>
#include <ace/utils/bitmap.h>
#include "map.h"

void minimapDraw(
	IN tBitMap *pDest,
	IN tMap *pMap
);

#endif // GUARD_OF_GAMESTATES_MENU_MINIMAP_H
