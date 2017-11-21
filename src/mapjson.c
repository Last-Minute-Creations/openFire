#include "mapjson.h"
#include <ace/macros.h>
#include <ace/managers/log.h>
#include "map.h"
#include "gamestates/game/control.h"
#include "gamestates/game/building.h"

tJson *mapJsonCreate(char *szFilePath) {
	logBlockBegin("mapJsonCreate(szFilePath: %s)", szFilePath);
	tJson *pJson = memAllocFast(sizeof(tJson));

	// Read whole file to string
	FILE *pMapFile = fopen(szFilePath, "rb");
	if(!pMapFile)
		logWrite("ERR: File doesn't exist: '%s'\n", szFilePath);
	fseek(pMapFile, 0, SEEK_END);
	ULONG ulFileSize = ftell(pMapFile);
	fseek(pMapFile, 0, SEEK_SET);

	pJson->szData = memAllocFast(ulFileSize+1);
	fread(pJson->szData, 1, ulFileSize, pMapFile);
	pJson->szData[ulFileSize] = '\0';
	fclose(pMapFile);

	jsmn_parser sJsonParser;
	jsmn_init(&sJsonParser);

	// Count tokens & alloc
	pJson->fwTokenCount = jsmn_parse(&sJsonParser, pJson->szData, ulFileSize+1, 0, 0);
	if(pJson->fwTokenCount < 0) {
		logWrite(
			"ERR: JSON during token counting: %"PRI_FWORD"\n", pJson->fwTokenCount
		);
		logBlockEnd("mapJsonCreate()");
		return 0;
	}
	pJson->pTokens = memAllocFast(pJson->fwTokenCount * sizeof(jsmntok_t));

	// Read tokens
	jsmn_init(&sJsonParser);
	FWORD fwResult = jsmn_parse(
		&sJsonParser, pJson->szData, ulFileSize+1, pJson->pTokens, pJson->fwTokenCount
	);
	if(fwResult < 0) {
		logWrite("ERR: JSON during tokenize: %"PRI_FWORD"\n", fwResult);
		logBlockEnd("mapJsonCreate()");
		return 0;
	}

	logBlockEnd("mapJsonCreate()");
	return pJson;
}

void mapJsonDestroy(tJson *pJson) {
	memFree(pJson->pTokens, sizeof(jsmntok_t) * pJson->fwTokenCount);
	memFree(pJson->szData, strlen(pJson->szData) + 1);
	memFree(pJson, sizeof(tJson));
}

void mapJsonGetMeta(tJson *pJson, tMap *pMap) {
	logBlockBegin(
		"mapJsonGetMeta(pJson: %p, pMap: %p",
		pJson, pMap
	);
	for(FUWORD i = 1; i < pJson->fwTokenCount; ++i) { // Skip first gigantic obj
		FUWORD fuwWidth = pJson->pTokens[i].end - pJson->pTokens[i].start;
		if(pJson->pTokens[i].type != JSMN_STRING)
			continue;
		// Read property name
		if(!memcmp(pJson->szData + pJson->pTokens[i].start, "width", fuwWidth))
			pMap->fubWidth = strtoul(pJson->szData + pJson->pTokens[++i].start, 0, 10);
		else if(!memcmp(pJson->szData + pJson->pTokens[i].start, "height", fuwWidth))
			pMap->fubHeight = strtoul(pJson->szData + pJson->pTokens[++i].start, 0, 10);
		else if(!memcmp(pJson->szData + pJson->pTokens[i].start, "controlPoints", fuwWidth))
			pMap->fubControlPointCount = pJson->pTokens[++i].size;
	}
	logBlockEnd("mapJsonGetMeta()");
}

void mapJsonReadTiles(tJson *pJson,	tMap *pMap) {
	// Find 'tiles' in JSON
	FUBYTE fubFound = 0;
	FUWORD i;
	for(i = 1; i < pJson->fwTokenCount && !fubFound; ++i) { // Skip first obj
		FUWORD fuwWidth = pJson->pTokens[i].end - pJson->pTokens[i].start;
		if(pJson->pTokens[i].type != JSMN_STRING)
			continue;
		if(memcmp(pJson->szData + pJson->pTokens[i].start, "tiles", fuwWidth))
			continue;
		fubFound = 1;
	}
	if(!fubFound) {
		logWrite("ERR: JSON 'tiles' array not found!\n");
	}

	// Tiles found - check row count
	if(pJson->pTokens[i].size != pMap->fubHeight) {
		logWrite(
			"ERR: Only %d rows provided, %"PRI_FUBYTE"expected\n",
			pJson->pTokens[i].size, pMap->fubHeight
		);
		return;
	}

	// Do some reading
	pMap->fubSpawnCount = 0;
	for(FUBYTE y = 0; y < pMap->fubHeight; ++y) {
		++i;
		FUWORD fuwWidth = pJson->pTokens[i].end - pJson->pTokens[i].start;

		// Sanity check
		if(pJson->pTokens[i].type != JSMN_STRING) {
			logWrite("ERR: Unexpected row type @y %"PRI_FUBYTE": %d\n", y, pJson->pTokens[i].type);
			return;
		}
		if(fuwWidth != pMap->fubWidth) {
			logWrite(
				"ERR: Row @y %"PRI_FUBYTE" is too short: %"PRI_FUWORD", expected %"PRI_FUBYTE"\n",
				y, fuwWidth, pMap->fubWidth
			);
			return;
		}

		// Read row
		for(FUBYTE x = 0; x < fuwWidth; ++x) {
			pMap->pData[x][y].ubIdx = (UBYTE)pJson->szData[pJson->pTokens[i].start + x];
			pMap->pData[x][y].ubData = BUILDING_IDX_INVALID;
			if(
				pMap->pData[x][y].ubIdx == MAP_LOGIC_SPAWN0 ||
				pMap->pData[x][y].ubIdx == MAP_LOGIC_SPAWN1 ||
				pMap->pData[x][y].ubIdx == MAP_LOGIC_SPAWN2
			)
				++pMap->fubSpawnCount;
			else if(pMap->pData[x][y].ubIdx == MAP_LOGIC_WALL_VERTICAL)
				pMap->pData[x][y].ubIdx = MAP_LOGIC_WALL;
		}
	}
}

void mapJsonReadControlPoints(tJson *pJson) {
	logBlockBegin("mapJsonReadControlPoints(pJson: %p)", pJson);
	// Find 'controlPoints' in JSON
	FUBYTE fubFound = 0;
	FUWORD i;
	for(i = 1; i < pJson->fwTokenCount && !fubFound; ++i) { // Skip first obj
		if(pJson->pTokens[i].type != JSMN_STRING)
			continue;
		FUWORD fuwWidth = pJson->pTokens[i].end - pJson->pTokens[i].start;
		if(memcmp(pJson->szData + pJson->pTokens[i].start, "controlPoints", fuwWidth))
			continue;
		fubFound = 1;
	}
	if(!fubFound) {
		logWrite("ERR: JSON 'controlPoints' property not found!\n");
		goto end;
	}

	// Sanity check
	if(pJson->pTokens[i].type != JSMN_ARRAY) {
		logWrite("ERR: Value of 'controlPoints' is not an array (%d)!\n", pJson->pTokens[i].type);
		goto end;
	}
	FUBYTE fubControlPointCount = pJson->pTokens[i].size;
	for(FUBYTE pt = 0; pt != fubControlPointCount; ++pt) {
		++i;
		if(pJson->pTokens[i].type != JSMN_OBJECT) {
			logWrite("ERR: Expected object\n");
			goto end;
		}
		if(pJson->pTokens[i].size != 3) {
			logWrite("ERR: Child count: %d, expected %d\n", pJson->pTokens[i].size, 3);
			goto end;
		}
		char szControlName[CONTROL_NAME_MAX] = "";
		FUBYTE fubCaptureX = 0, fubCaptureY = 0;
		FUBYTE fubPolyPointCnt = 0;
		tUbCoordYX pPolyPoints[10];
		for(FUBYTE ch = 0; ch != 3; ++ch) {
			++i;
			FUWORD fuwWidth = pJson->pTokens[i].end - pJson->pTokens[i].start;
			if(!memcmp(pJson->szData + pJson->pTokens[i].start, "name", fuwWidth)) {
				++i;
				FUBYTE fubNameLength = MIN(pJson->pTokens[i].end - pJson->pTokens[i].start, CONTROL_NAME_MAX-1);
				memcpy(
					szControlName, pJson->szData + pJson->pTokens[i].start,
					fubNameLength
				);
				szControlName[fubNameLength] = '\0';
			}
			else if(!memcmp(pJson->szData + pJson->pTokens[i].start, "capture", fuwWidth)) {
				// Capture point
				++i;
				if(pJson->pTokens[i].type != JSMN_ARRAY || pJson->pTokens[i].size != 2) {
					logWrite(
						"ERR: polygon point not a point: %.*s",
						pJson->pTokens[i].end - pJson->pTokens[i].start, pJson->szData + pJson->pTokens[i].start
					);
					goto end;
				}
				fubCaptureX = strtoul(pJson->szData + pJson->pTokens[++i].start, 0, 10);
				fubCaptureY = strtoul(pJson->szData + pJson->pTokens[++i].start, 0, 10);
			}
			else if(!memcmp(pJson->szData + pJson->pTokens[i].start, "polygon", fuwWidth)) {
				// Polygon points
				fubPolyPointCnt = pJson->pTokens[++i].size;
				if(fubPolyPointCnt >= 10-1) {
					logWrite(
						"ERR: too much poly points: %"PRI_FUBYTE"\n", fubPolyPointCnt
					);
					goto end;
				}
				for(FUBYTE pp = 0; pp != fubPolyPointCnt; ++pp) {
					++i;
					if(pJson->pTokens[i].type != JSMN_ARRAY || pJson->pTokens[i].size != 2) {
						logWrite(
							"ERR: polygon point not a point: %.*s",
							pJson->pTokens[i].end - pJson->pTokens[i].start, pJson->szData + pJson->pTokens[i].start
						);
						goto end;
					}
					pPolyPoints[pp].sUbCoord.ubX = strtoul(pJson->szData + pJson->pTokens[++i].start, 0, 10);
					pPolyPoints[pp].sUbCoord.ubY = strtoul(pJson->szData + pJson->pTokens[++i].start, 0, 10);
				}
			}
			else {
				logWrite("ERR: Unknown attribute: %.*s\n", pJson->szData + pJson->pTokens[i].start);
				goto end;
			}
		}
		if(!fubPolyPointCnt) {
			logWrite("ERR: No polygon points supplied @point %"PRI_FUBYTE"!\n", pt);
			goto end;
		}
		if(!fubCaptureX && !fubCaptureY) {
			logWrite("ERR: No capture point supplied @point %"PRI_FUBYTE"!\n", pt);
			goto end;
		}
		if(!strlen(szControlName)) {
			logWrite("ERR: No control point name! @point %"PRI_FUBYTE"\n", pt);
			goto end;
		}
		// Close polygon
		++fubPolyPointCnt;
		pPolyPoints[fubPolyPointCnt-1].uwYX = pPolyPoints[0].uwYX;
		controlAddPoint(
			szControlName, fubCaptureX, fubCaptureY, fubPolyPointCnt, pPolyPoints
		);
	}

end:
	logBlockEnd("mapJsonReadControlPoints()");
}
