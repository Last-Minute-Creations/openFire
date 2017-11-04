#include "adler32.h"
#include <stdio.h>
#include <ace/managers/log.h>

#define ADLER32_MODULO 65521

ULONG adler32array(UBYTE *pData, ULONG ulDataSize) {
	ULONG a = 1, b = 0;
	for(ULONG i = 0; i != ulDataSize; ++i) {
		a += pData[i];
		if(a > ADLER32_MODULO)
			a -= ADLER32_MODULO;
		b += a;
		if(b > ADLER32_MODULO)
			b -= ADLER32_MODULO;
	}
	return b << 16 | a;
}

ULONG adler32file(char *szPath) {
	logBlockBegin("adler32File(szPath: %s)", szPath);
	FILE *pFile = fopen(szPath, "rb");
	if(!pFile) {
		logWrite("ERR: File doesn't exist\n");
		logBlockEnd("adler32File()");
		return 0;
	}
	UBYTE ubBfr;
	ULONG a = 1, b = 0;
	while(!feof(pFile)) {
		fread(&ubBfr, 1, 1, pFile);
		a += ubBfr;
		if(a > ADLER32_MODULO)
			a -= ADLER32_MODULO;
		b += a;
		if(b > ADLER32_MODULO)
			b -= ADLER32_MODULO;
	}
	fclose(pFile);
	logBlockEnd("adler32File()");
	return b << 16 | a;
}
