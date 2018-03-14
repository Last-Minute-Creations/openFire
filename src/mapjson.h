#ifndef GUARD_OF_MAPJSON_H
#define GUARD_OF_MAPJSON_H

#include <ace/types.h>
#include "map.h"
#define JSMN_STRICT       /* Strict JSON parsing */
// JSMN_PARENT_LINKS breaks things up!
// #define JSMN_PARENT_LINKS /* Speeds things up */
#include "jsmn.h"

typedef struct _tJson {
	char *szData;
	jsmntok_t *pTokens;
	FWORD fwTokenCount;
} tJson;

tJson *mapJsonCreate(const char *szFilePath);

void mapJsonDestroy(tJson *pJson);

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
