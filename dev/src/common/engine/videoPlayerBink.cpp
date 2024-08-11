/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"
#include "videoPlayerBink.h"

#if defined(USE_BINK_VIDEO)

namespace // Anonymous
{
	const size_t BINK_MEM_ALIGN = 32;

	void* RADLINK SBinkAlloc( U32 bytes )
	{
		void* ptr = RED_MEMORY_ALLOCATE_ALIGNED( MemoryPool_Default, MC_Bink, static_cast< size_t >( bytes ), BINK_MEM_ALIGN );
		ASSERT( ptr );
		ASSERT( ( reinterpret_cast< uintptr_t >( ptr ) & ( BINK_MEM_ALIGN - 1 ) ) == 0 );
#ifndef USE_BINK_MEMORY_FALLBACK
		if ( ! ptr )
		{
			// Tells bink that there's no memory and to not try to bypass the allocator.
			ptr = reinterpret_cast< void* >( -1 );
		}
#endif
		return ptr;
	}

	void RADLINK SBinkFree( void* ptr )
	{
		RED_MEMORY_FREE( MemoryPool_Default, MC_Bink, ptr );
	}
}

CVideoPlayerBink::CVideoPlayerBink()
{
	BinkSetMemory( SBinkAlloc, SBinkFree );

	if ( ! BinkSoundUseXAudio2( nullptr ) )
	{
		VIDEO_ERROR( TXT("Bink can't use XAudio2!") );
	}
}

CVideoPlayerBink::~CVideoPlayerBink()
{
}

SVideoParamsBink::SVideoParamsBink( const SVideoParams& videoParams )
	: SVideoParams( videoParams )
{
}

void CVideoPlayerBink::StartVideo( const SVideoParams& videoParams )
{	
	SVideoParamsBink binkVideoParams( videoParams );

	Base::StartVideo( binkVideoParams );
}

#endif