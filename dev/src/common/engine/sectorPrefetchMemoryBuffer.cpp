/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "sectorPrefetchMemoryBuffer.h"
#include "../core/configVar.h"

namespace Config
{
	TConfigVar< Int32 >		cvPrefetchMemoryBuffer( "Streaming", "SectorPrefetchMemoryBufferSizeMB", 81 );
}

CSectorPrefetchMemoryBuffer::CSectorPrefetchMemoryBuffer()
	: m_size( Config::cvPrefetchMemoryBuffer.Get() * 1024 * 1024 )
{
	m_memory = RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_IOBurstReadBuffer, m_size );
}

CSectorPrefetchMemoryBuffer::~CSectorPrefetchMemoryBuffer()
{
	if ( m_memory )
	{
		RED_MEMORY_FREE( MemoryPool_Default, MC_IOBurstReadBuffer, m_memory );
		m_memory = nullptr;
	}
}

//---------------------------------------------------------------------------------

/// W3 code hacks
CSectorPrefetchMemoryBuffer* GSectorPrefetchMemoryBuffer = nullptr;

CSectorPrefetchMemoryBuffer* GetGlobalPrefetchBuffer()
{
	if ( !GSectorPrefetchMemoryBuffer )
		GSectorPrefetchMemoryBuffer = new CSectorPrefetchMemoryBuffer();
	return GSectorPrefetchMemoryBuffer;
}

//---------------------------------------------------------------------------------
