/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "grassMask.h"

IMPLEMENT_ENGINE_CLASS( CGenericGrassMask );

CGenericGrassMask::CGenericGrassMask()
	: m_grassMask( NULL )
	, m_maskRes( 0 )
{
}

CGenericGrassMask::~CGenericGrassMask()
{
	if( m_grassMask )
	{
		RED_MEMORY_FREE( MemoryPool_FoliageData, MC_FoliageGrassMask, m_grassMask );
	}
}

void CGenericGrassMask::InitGenericGrassMask( Int32 terrainRes )
{
	Uint32 maskRes = terrainRes/4;
	if ( !IsPow2( maskRes ) )
	{
		const Uint32 maskRoundDownPow2 = RoundDownToPow2( maskRes );
		RED_LOG( RED_LOG_CHANNEL( Foliage ), TXT("Rounding generic grass mask resolution down from %i to %i (power of 2)"), maskRes, maskRoundDownPow2 );
		maskRes = maskRoundDownPow2;
	}

	RED_ASSERT( maskRes > 0 );
	RED_ASSERT( IsPow2( maskRes ) );
	m_maskRes = maskRes;

	const Uint32 bitmapSize = m_maskRes * m_maskRes / 8;

	m_grassMask = static_cast< Uint8* >( RED_MEMORY_ALLOCATE( MemoryPool_FoliageData, MC_FoliageGrassMask, bitmapSize ) );
	Red::System::MemorySet( m_grassMask, 0xFFFFFFFF, bitmapSize );
	RED_LOG( RED_LOG_CHANNEL( Foliage ), TXT("Generic grass mask initiated with %i x %i resolution ( %i bytes )"), m_maskRes, m_maskRes, bitmapSize );
}

void CGenericGrassMask::OnSerialize( IFile& file )
{
	TBaseClass::OnSerialize( file );

	if ( file.IsGarbageCollector() || file.IsMapper() )
	{
		return;
	}

	const Uint32 bitmapSize = m_maskRes * m_maskRes / 8;
	if ( bitmapSize > 0 )
	{
		if ( file.IsReader() )
		{
			m_grassMask = static_cast< Uint8* >( RED_MEMORY_ALLOCATE( MemoryPool_FoliageData, MC_FoliageGrassMask, bitmapSize ) );
		}
		file.Serialize( m_grassMask, bitmapSize );
	}
}