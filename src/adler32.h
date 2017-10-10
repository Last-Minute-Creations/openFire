#ifndef GUARD_OF_ADLER32
#define GUARD_OF_ADLER32

#include <ace/types.h>

ULONG adler32array(
	IN UBYTE *pData,
	IN ULONG ulDataSize
);

ULONG adler32file(
	IN char *szPath
);

#endif // GUARD_OF_ADLER32
