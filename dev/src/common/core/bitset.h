/**
 * Copyright © 2009 CD Projekt Red. All Rights Reserved.
 */

#pragma once

template <
	Int32 size
>
class TBitSet
{
	Uint32 m_bits[ (size+31) / 32 + 1 ];

public:
	RED_INLINE TBitSet() { ClearAll(); }
	RED_INLINE Bool Get( Int32 index ) const	{ return ( m_bits[ index/32 ] &   ( (Uint32)0x80000000 >> (index&31) ) ) ? true : false; }
	RED_INLINE void Set( Int32 index )			{		   m_bits[ index/32 ] |=  ( (Uint32)0x80000000 >> (index&31) ); }
	RED_INLINE void Clear ( Int32 index )		{		   m_bits[ index/32 ] &= ~( (Uint32)0x80000000 >> (index&31) ); }
	RED_INLINE void Toggle( Int32 index )		{		   m_bits[ index/32 ] ^=  ( (Uint32)0x80000000 >> (index&31) ); }
	RED_INLINE void SetAll()					{ Red::System::MemorySet( &m_bits, -1, (((size+31) / 32) + 1 )*sizeof(Uint32) ); }
	RED_INLINE void ClearAll()				{ Red::System::MemorySet( &m_bits,  0, (((size+31) / 32) + 1 )*sizeof(Uint32) ); }

	RED_INLINE Int32 FindNextSet( Int32 startIndex ) const
	{
		for( Uint32 index = startIndex; index < size; index+=32 )
		{
			Uint32 mask = 0xFFFFFFFF >> (index&31);
			Uint32 maskA = ( m_bits[ index/32 + 0 ] &  mask ) << (index&31);
			Uint32 maskB = ( m_bits[ index/32 + 1 ] & ~mask ) >> (32 - index&31);
			Uint32 maskAB  = maskA | maskB;

			if ( maskAB != 0 )
			{
				Uint32 leadingZeros = Red::System::BitUtils::CountLeadingZeros( maskAB );
				return index + leadingZeros;
			}
		}
		return size;
	}

	RED_INLINE void operator^=( const TBitSet& rhs )
	{
		for ( Uint32 i = 0; i < ARRAY_COUNT_U32(m_bits); ++i )
		{
			m_bits[ i ] ^= rhs.m_bits[ i ];
		}
	}

	RED_INLINE void operator&=( const TBitSet& rhs )
	{
		for ( Uint32 i = 0; i < ARRAY_COUNT_U32(m_bits); ++i )
		{
			m_bits[ i ] &= rhs.m_bits[ i ];
		}
	}

	RED_INLINE void operator|=( const TBitSet& rhs )
	{
		for ( Uint32 i = 0; i < ARRAY_COUNT_U32(m_bits); ++i )
		{
			m_bits[ i ] |= rhs.m_bits[ i ];
		}
	}

	RED_INLINE Bool IsAnySet() const
	{
		for ( Int32 i=0; i<size; ++i )
		{
			if ( Get(i) )
				return true;
		}

		return false;
	}

	RED_INLINE Bool IsNoneSet() const
	{
		for ( Int32 i=0; i<size; ++i )
		{
			if ( Get(i) )
				return false;
		}

		return true;
	}

	RED_INLINE Bool IsAllSet() const
	{
		for ( Int32 i=0; i<size; ++i )
		{
			if ( !Get(i) )
				return false;
		}

		return true;
	}
};

template < Uint32 size >
class TBitSet64
{
	Uint64 m_bits[ (size+63) / 64 ];

	static RED_INLINE Uint64 Mask( const Uint32 index ) { return (Uint64)0x8000000000000000 >> (index & 63 );	}
	static RED_INLINE Uint32 Word( const Uint32 index ) { return index / 64; }

public:	
	RED_INLINE TBitSet64()
	{
		ClearAll();
	}

	RED_INLINE Bool Get( const Uint32 index ) const
	{
		return ( m_bits[ Word(index) ] & Mask(index) ) != 0;
	}

	RED_INLINE void Set( const Uint32 index )
	{
		m_bits[ Word(index) ] |= Mask(index);
	}

	RED_INLINE void Clear( const Uint32 index )
	{
		m_bits[ Word(index) ] &= ~Mask(index);
	}
	
	RED_INLINE void Toggle( Int32 index )
	{		   
		m_bits[ Word(index) ] ^= Mask(index);
	}

	void SetAll()
	{
		Red::System::MemorySet( &m_bits, -1, sizeof(m_bits) );
	}

	void ClearAll()
	{
		Red::System::MemorySet( &m_bits, 0, sizeof(m_bits) );
	}

	RED_INLINE const Uint32 FindNextSet( Uint32 startIndex ) const
	{
		// start with misaligned data first
		Uint32 wordStart = Word( startIndex );
		if ( startIndex & 63 )
		{
			const Uint64 startMask = (Uint64)0xFFFFFFFFFFFFFFFF >> (startIndex & 63); // 000011111111 - mask out bits not yet visited
			const Uint64 testMask = ( m_bits[ wordStart ] & startMask ) << (startIndex & 63); // xxxxxxxxx000 - move the bits to test to the front

			// if there's something get it
			if ( testMask )
			{
				Uint32 leadingZeros = (Uint32) Red::System::BitUtils::CountLeadingZeros< Uint64 >( testMask );
				return startIndex + leadingZeros;
			}
			
			// skip this word
			++wordStart;
		}

		// process the rest in a nice loop
		while ( wordStart < ARRAY_COUNT_U32( m_bits ) )
		{
			const Uint64 mask = m_bits[ wordStart ];
			if ( mask != 0 )
			{
				const Uint32 leadingZeros = (Uint32) Red::System::BitUtils::CountLeadingZeros< Uint64 >( mask );
				return leadingZeros + (wordStart * 64);
			}

			// next word
			++wordStart;
		}

		// nothing found
		return size;
	}

	RED_INLINE void operator^=( const TBitSet64& rhs )
	{
		for ( Uint32 i = 0; i < ARRAY_COUNT_U32(m_bits); ++i )
		{
			m_bits[ i ] ^= rhs.m_bits[ i ];
		}
	}

	RED_INLINE void operator&=( const TBitSet64& rhs )
	{
		for ( Uint32 i = 0; i < ARRAY_COUNT_U32(m_bits); ++i )
		{
			m_bits[ i ] &= rhs.m_bits[ i ];
		}
	}

	RED_INLINE void operator|=( const TBitSet64& rhs )
	{
		for ( Uint32 i = 0; i < ARRAY_COUNT_U32(m_bits); ++i )
		{
			m_bits[ i ] |= rhs.m_bits[ i ];
		}
	}

	template< typename F >
	RED_FORCE_INLINE Bool VisitSetBitsEarlyExit( F visitor ) const
	{
		// call the visit function for all of the items that have the bit set
		Uint32 bitBase = 0;
		for ( Uint32 i=0; i<ARRAY_COUNT_U32(m_bits); ++i, bitBase += 64 )
		{
			Uint64 bit = bitBase + 63;
			Uint64 word = m_bits[i];
			while ( word != 0 )
			{
				if ( word & 1 )
				{
					if ( visitor( (Uint32) bit) )
						return true;
				}

				word >>= 1;
				bit -= 1;
			}
		}

		// visitor function was not trigger
		return false;
	}

	template< typename F >
	RED_FORCE_INLINE void VisitSetBits( F visitor ) const
	{
		// call the visit function for all of the items that have the bit set
		Uint32 bitBase = 0;
		for ( Uint32 i=0; i<ARRAY_COUNT_U32(m_bits); ++i, bitBase += 64 )
		{
			Uint64 bit = bitBase + 63;
			Uint64 word = m_bits[i];
			while ( word != 0 )
			{
				if ( word & 1 )
				{
					visitor( (Uint32)bit );
				}

				word >>= 1;
				bit -= 1;
			}
		}
	}

};

class BitSet64Dynamic
{
public:	
	BitSet64Dynamic();
	~BitSet64Dynamic();

	RED_FORCE_INLINE Bool Get( const Uint32 index ) const
	{
		RED_FATAL_ASSERT( Word(index) < m_numWords, "Out of Bound access is illegal." );
		return ( m_bits[ Word(index) ] & Mask(index) ) != 0;
	}

	RED_FORCE_INLINE void Set( const Uint32 index )
	{
		RED_FATAL_ASSERT( Word(index) < m_numWords, "Out of Bound access is illegal." );
		m_bits[ Word(index) ] |= Mask(index);
	}

	RED_FORCE_INLINE void Clear( const Uint32 index )
	{
		RED_FATAL_ASSERT( Word(index) < m_numWords, "Out of Bound access is illegal." );
		m_bits[ Word(index) ] &= ~Mask(index);
	}

	RED_FORCE_INLINE void Toggle( Int32 index )
	{		   
		RED_FATAL_ASSERT( Word(index) < m_numWords, "Out of Bound access is illegal." );
		m_bits[ Word(index) ] ^= Mask(index);
	}

	RED_FORCE_INLINE const Uint32 GetNumEntries() const
	{
		return m_size;
	}

	RED_FORCE_INLINE const Uint32 GetNumWords() const
	{
		return m_numWords;
	}

	void Resize( const Uint32 numEntries );

	void SetAll();

	void ClearAll();

	RED_FORCE_INLINE const Uint32 FindNextSet( Uint32 startIndex ) const
	{
		// start with misaligned data first
		Uint32 wordStart = Word( startIndex );
		if ( startIndex & 63 )
		{
			const Uint64 startMask = (Uint64)0xFFFFFFFFFFFFFFFF >> (startIndex & 63); // 000011111111 - mask out bits not yet visited
			const Uint64 testMask = ( m_bits[ wordStart ] & startMask ) << (startIndex & 63); // xxxxxxxxx000 - move the bits to test to the front

			// if there's something get it
			if ( testMask )
			{
				Uint32 leadingZeros = (Uint32) Red::System::BitUtils::CountLeadingZeros< Uint64 >( testMask );
				return startIndex + leadingZeros;
			}

			// skip this word
			++wordStart;
		}

		// process the rest in a nice loop
		while ( wordStart < m_numWords )
		{
			const Uint64 mask = m_bits[ wordStart ];
			if ( mask != 0 )
			{
				const Uint32 leadingZeros = (Uint32) Red::System::BitUtils::CountLeadingZeros< Uint64 >( mask );
				return leadingZeros + (wordStart * 64);
			}

			// next word
			++wordStart;
		}

		// nothing found
		return m_size;
	}

	RED_FORCE_INLINE void operator^=( const BitSet64Dynamic& rhs )
	{
		const Uint32 numWords = Min< Uint32 >( m_numWords, rhs.m_numWords );
		for ( Uint32 i = 0; i < numWords; ++i )
		{
			m_bits[ i ] ^= rhs.m_bits[ i ];
		}
	}

	RED_FORCE_INLINE void operator&=( const BitSet64Dynamic& rhs )
	{
		const Uint32 numWords = Min< Uint32 >( m_numWords, rhs.m_numWords );
		for ( Uint32 i = 0; i < numWords; ++i )
		{
			m_bits[ i ] &= rhs.m_bits[ i ];
		}
	}

	RED_FORCE_INLINE void operator|=( const BitSet64Dynamic& rhs )
	{
		const Uint32 numWords = Min< Uint32 >( m_numWords, rhs.m_numWords );
		for ( Uint32 i = 0; i < numWords; ++i )
		{
			m_bits[ i ] |= rhs.m_bits[ i ];
		}
	}

private:
	static RED_FORCE_INLINE Uint64 Mask( const Uint32 index ) { return (Uint64)0x8000000000000000 >> (index & 63 );	}
	static RED_FORCE_INLINE Uint32 Word( const Uint32 index ) { return index / 64; }

	Uint64*		m_bits;
	Uint32		m_size;
	Uint32		m_numWords;
};