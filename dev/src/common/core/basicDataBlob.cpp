/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "basicDataBlob.h"

CDataBlob::CDataBlob()
	: m_data( nullptr )
	, m_dataSize( 0 )
{
}

CDataBlob::CDataBlob( const Uint32 dataSize )
	: m_data( nullptr )
	, m_dataSize( 0 )
{
	if ( dataSize )
	{
		m_dataSize = dataSize;
		m_data = RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_DataBlob, dataSize );
	}
}

CDataBlob::CDataBlob( const void* data, const Uint32 dataSize )
	: m_data( nullptr )
	, m_dataSize( 0 )
{
	if ( data && dataSize )
	{
		m_dataSize = dataSize;
		m_data = RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_DataBlob, dataSize );
		Red::MemoryCopy( m_data, data, dataSize );
	}
}

CDataBlob::CDataBlob( void* data, const Uint32 dataSize, DeallocateFunction dealloc )
	: m_data( data )
	, m_dataSize( dataSize )
	, m_dealloc( dealloc )
{
}

CDataBlob::~CDataBlob()
{
	if ( m_data )
	{
		if ( m_dealloc )
		{
			m_dealloc( m_data );
		}
		else
		{
			RED_MEMORY_FREE( MemoryPool_Default, MC_DataBlob, m_data );
		}

		m_data = nullptr;
		m_dataSize = 0;
	}
}
