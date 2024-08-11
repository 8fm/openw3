#include "build.h"
#include "bitset.h"

BitSet64Dynamic::BitSet64Dynamic()
{
	m_bits = nullptr;
	m_size = 0;
	m_numWords = 0;
}

BitSet64Dynamic::~BitSet64Dynamic()
{
	if ( m_bits != nullptr )
	{
		RED_MEMORY_FREE( MemoryPool_Default, MC_Default, m_bits );
		m_bits = nullptr;
	}
}

void BitSet64Dynamic::Resize( const Uint32 numEntries )
{
	if ( m_size != numEntries )
	{
		const Uint32 numWords = (numEntries + 63) / 64;
		if ( numWords != m_numWords )
		{
			m_bits = (Uint64*) RED_MEMORY_REALLOCATE( MemoryPool_Default, m_bits, MC_Default, numWords * sizeof(Uint64) );
			m_numWords = numWords;
		}

		m_size = numEntries;
	}
}

void BitSet64Dynamic::SetAll()
{
	Red::System::MemorySet( m_bits, -1, sizeof(Uint64) * m_numWords );
}

void BitSet64Dynamic::ClearAll()
{
	Red::System::MemorySet( m_bits, 0, sizeof(Uint64) * m_numWords );
}
