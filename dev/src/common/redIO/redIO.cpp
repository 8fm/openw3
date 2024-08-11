/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "redIO.h"

#include "redIOPlatform.h"

#ifdef RED_PLATFORM_ORBIS
# include <fios2.h>
#endif

REDIO_NAMESPACE_BEGIN

#ifdef RED_PLATFORM_ORBIS
///////////////////////////////////////////////////////////////////////////
// TEMP BUGFIX: Because needed even if not using async Fios2 I/O (yet)
//////////////////////////////////////////////////////////////////////////
static Uint64 g_OpStorage[SCE_FIOS_OP_STORAGE_SIZE(512,256)/sizeof(Uint64)];
static Uint64 g_ChunkStorage[8*1024]; /* 64KiB */
static Uint64 g_FHStorage[SCE_FIOS_FH_STORAGE_SIZE(255,256)/sizeof(Uint64)];
static Uint64 g_DHStorage[512];       /* 4KiB */
#endif // RED_PLATFORM_ORBIS

Bool Initialize()
{
	///////////////////////////////////////////////////////////////////////////
	// TEMP BUGFIX: Because needed even if not using async Fios2 I/O (yet)
	//////////////////////////////////////////////////////////////////////////
#ifdef RED_PLATFORM_ORBIS
	SceFiosParams params = SCE_FIOS_PARAMS_INITIALIZER;
	params.opStorage.pPtr = g_OpStorage;
	params.opStorage.length = sizeof(g_OpStorage);
	params.chunkStorage.pPtr = g_ChunkStorage;
	params.chunkStorage.length = sizeof(g_ChunkStorage);
	params.fhStorage.pPtr = g_FHStorage;
	params.fhStorage.length = sizeof(g_FHStorage);
	params.dhStorage.pPtr = g_DHStorage;
	params.dhStorage.length = sizeof(g_DHStorage);
	params.pathMax = 256;
	params.threadAffinity[SCE_FIOS_IO_THREAD] = SCE_FIOS_IO_THREAD_DEFAULT_PRIORITY;
	params.threadAffinity[SCE_FIOS_IO_THREAD] = (1<<4)|(1<<5);
	//params.pVprintf = vprintf;
	//params.profiling = SCE_FIOS_PROFILE_ALL;
	// 	params.pMemcpy = memcpy;

	const Int32 fiosErr = sceFiosInitialize(&params);
	if ( fiosErr != SCE_OK )
	{
		// For now, just warn
		REDIO_WARN(TXT("Failed to initialize FIOS2 with error 0x%08X"), fiosErr );
	}
#endif // RED_PLATFORM_ORBIS

	if ( ! GAsyncIO.Init() )
	{
		REDIO_ERR( TXT("Failed to intialize asyncIO") );
		return false;
	}

	return true;
}

void Shutdown()
{
	GAsyncIO.Shutdown();
}

REDIO_NAMESPACE_END