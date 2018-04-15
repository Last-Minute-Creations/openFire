#include "adler32.h"
#include <ace/utils/file.h>
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
	tFile *pFile = fileOpen(szPath, "rb");
	if(!pFile) {
		logWrite("ERR: File doesn't exist\n");
		logBlockEnd("adler32File()");
		return 0;
	}
	UBYTE ubBfr;
	ULONG a = 1, b = 0;
	while(!fileIsEof(pFile)) {
		fileRead(pFile, &ubBfr, 1);
		a += ubBfr;
		if(a > ADLER32_MODULO)
			a -= ADLER32_MODULO;
		b += a;
		if(b > ADLER32_MODULO)
			b -= ADLER32_MODULO;
	}
	fileClose(pFile);
	logBlockEnd("adler32File()");
	return b << 16 | a;
}
