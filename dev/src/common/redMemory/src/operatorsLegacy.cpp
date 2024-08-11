/**
 * Copyright © 2016 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

//////////////////////////////////////////////////////////////////////////
// Microsoft platforms allow us to define global new / delete in Core.
// PS4 will only let us define them in object files explicitly linked to main
// Therefore, we only define them on MS compilers here
#if 0
#ifdef RED_USE_NEW_MEMORY_SYSTEM
	#include "operatorsLegacy.h"
#endif
#endif
