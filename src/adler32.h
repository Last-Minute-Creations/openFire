#ifndef GUARD_OF_ADLER32
#define GUARD_OF_ADLER32

#include <ace/types.h>

ULONG adler32array(
	UBYTE *pData,
	ULONG ulDataSize
);

ULONG adler32file(
	char *szPath
);

#endif // GUARD_OF_ADLER32
