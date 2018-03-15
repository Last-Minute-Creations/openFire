#ifndef GUARD_OF_MAPJSON_H
#define GUARD_OF_MAPJSON_H

#include <ace/types.h>
#include "map.h"
#include "json.h"

UBYTE mapJsonGetMeta(
	IN const tJson *pJson,
	IN tMap *pMap
);

void mapJsonReadTiles(
	IN const tJson *pJson,
	IN tMap *pMap
);

void mapJsonReadControlPoints(
	IN const tJson *pJson
);

#endif // GUARD_OF_MAPJSON_H
