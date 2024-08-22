/**
* Copyright � 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#if defined(_MSC_VER) && (_MSC_VER >= 1900)
FILE _iob[] = { *stdin, *stdout, *stderr };

extern "C" FILE* __cdecl __iob_func(void)
{
	return _iob;
}
#endif
