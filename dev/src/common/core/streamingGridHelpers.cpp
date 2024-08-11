#include "build.h"
#include "streamingGrid.h"
#include "streamingGridHelpers.h"

//-------------------------

const __m128 CStreamingPositionQuantizer::MinC = _mm_setzero_ps();
const __m128 CStreamingPositionQuantizer::MaxC = _mm_set1_ps( 65535.0f );

CStreamingPositionQuantizer::CStreamingPositionQuantizer( Float worldSize )
{
	const Float ofs = worldSize / 2.0f;
	m_offset = _mm_set_ps( 0.0f, ofs, ofs, ofs );
	m_scale = _mm_set1_ps( 65535.0f / worldSize );
}

//-------------------------

CStreamingEntryMask::CStreamingEntryMask( const Uint32 initialSize )
{
	m_bits = nullptr;
	m_maxIndex = 0;

	Prepare( initialSize );
}

CStreamingEntryMask::~CStreamingEntryMask()
{
	if ( m_bits != nullptr )
	{
		RED_MEMORY_FREE( MemoryPool_Default, MC_EntityManager, m_bits );
		m_bits = nullptr;
	}
}

void CStreamingEntryMask::Prepare( const Uint32 maxIndex )
{
	if ( maxIndex > m_maxIndex )
	{
		const Uint32 prevIndex = m_maxIndex;
		m_maxIndex = maxIndex;

		const Uint32 numWords = (maxIndex + 63) / 64;
		m_bits = (Uint64*) RED_MEMORY_REALLOCATE( MemoryPool_Default, m_bits, MC_EntityManager, numWords * sizeof(Uint64) );

		for ( Uint32 i=prevIndex; i<maxIndex; ++i )
		{
			Clear(i);
		}
	}
}

//-------------------------
