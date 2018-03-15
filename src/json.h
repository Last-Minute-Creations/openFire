#ifndef GUARD_OF_JSON_H
#define GUARD_OF_JSON_H

#define JSMN_STRICT       /* Strict JSON parsing */
// JSMN_PARENT_LINKS breaks things up!
// #define JSMN_PARENT_LINKS /* Speeds things up */
#include "jsmn.h"
#include <ace/types.h>

typedef struct _tJson {
	char *szData;
	jsmntok_t *pTokens;
	FWORD fwTokenCount;
} tJson;

tJson *jsonCreate(
	IN const char *szFilePath
);

void jsonDestroy(
	IN tJson *pJson
);

UWORD jsonGetElementInArray(
	IN const tJson *pJson,
	IN UWORD uwParentIdx,
	IN UWORD uwIdx
);

UWORD jsonGetElementInStruct(
	IN const tJson *pJson,
	IN UWORD uwParentIdx,
	IN const char *szElement
);

UWORD jsonGetDom(
	IN const tJson *pJson,
	IN const char *szPattern
);

ULONG jsonTokToUlong(
	IN const tJson *pJson,
	IN UWORD uwTok,
	IN LONG lBase
);

UWORD jsonTokStrCpy(
	IN const tJson *pJson,
	IN UWORD uwTok,
	IN char *pDst,
	IN UWORD uwMaxLength
);

#endif // GUARD_OF_JSON_H
