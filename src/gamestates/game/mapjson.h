#ifndef GUARD_OF_GAMESTATES_GAME_MAPJSON_H
#define GUARD_OF_GAMESTATES_GAME_MAPJSON_H

#include <ace/types.h>
#define JSMN_STRICT       /* Strict JSON parsing */
// JSMN_PARENT_LINKS breaks things up!
// #define JSMN_PARENT_LINKS /* Speeds things up */
#include "jsmn.h"

typedef struct _tJson {
	char *szData;
	jsmntok_t *pTokens;
	FWORD fwTokenCount;
} tJson;

tJson *mapJsonCreate(char *szFilePath);

void mapJsonDestroy(tJson *pJson);

void mapJsonGetMeta(
	IN tJson *pJson,
	OUT FUBYTE *pWidth,
	OUT FUBYTE *pHeight,
	OUT FUBYTE *pControlPointCount
);

void mapJsonReadTiles(
	IN tJson *pJson,
	OUT FUBYTE *pSpawnCount
);

void mapJsonReadControlPoints(
	IN tJson *pJson
);

#endif // GUARD_OF_GAMESTATES_GAME_MAPJSON_H
