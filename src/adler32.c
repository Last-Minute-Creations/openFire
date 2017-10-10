#include "adler32.h"
#include <stdio.h>

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
	FILE *pFile = fopen(szPath, "rb");
	if(!pFile)
		return 0;
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
	return b << 16 | a;
}
