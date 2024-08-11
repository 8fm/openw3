/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "collisionCacheDataFormat.h"
#include "collisionCachePreloadedData.h"

CCollisionCachePreloadedData::CCollisionCachePreloadedData()
	: m_dataOffset( 0 )
{
}

CCollisionCachePreloadedData::~CCollisionCachePreloadedData()
{

}

void* CCollisionCachePreloadedData::Initialize( const Uint32 dataOffset, const Uint32 dataSize )
{
	RED_FATAL_ASSERT( m_data.Empty(), "Trying to initialize alaredy initialized data" );
	m_data.Resize( dataSize );
	m_dataOffset = dataOffset;
	return m_data.Data();
}

const void* CCollisionCachePreloadedData::GetPreloadedData( const Uint32 fileOffset, const Uint32 size ) const
{
	// is data in range ?
	if ( (fileOffset >= m_dataOffset) && (fileOffset + size < (m_dataOffset + m_data.Size())) )
	{
		return &m_data[ fileOffset - m_dataOffset ];
	}

	// data not in range
	return nullptr;
}