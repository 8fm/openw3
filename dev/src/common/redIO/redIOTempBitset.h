/**
 * Copyright © 2009 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "../redSystem/types.h"
#include "../redSystem/crt.h"
#include "../redSystem/bitUtils.h"

namespace Red { namespace IO {

using Red::System::Uint32;
using Red::System::Int32;

template <
	Int32 size
>
class TBitSet
{
	Uint32 m_bits[ (size+31) / 32 + 1 ];

public:
	TBitSet() { ClearAll(); }
	Bool Get( Int32 index ) const	{ return ( m_bits[ index/32 ] &   ( (Uint32)0x80000000 >> (index&31) ) ) ? true : false; }
	void Set( Int32 index )			{		   m_bits[ index/32 ] |=  ( (Uint32)0x80000000 >> (index&31) ); }
	void Clear ( Int32 index )		{		   m_bits[ index/32 ] &= ~( (Uint32)0x80000000 >> (index&31) ); }
	void Toggle( Int32 index )		{		   m_bits[ index/32 ] ^=  ( (Uint32)0x80000000 >> (index&31) ); }
	void SetAll()					{ Red::System::MemorySet( &m_bits, -1, (((size+31) / 32) + 1 )*sizeof(Uint32) ); }
	void ClearAll()				{ Red::System::MemorySet( &m_bits,  0, (((size+31) / 32) + 1 )*sizeof(Uint32) ); }

	Int32 FindNextSet( Int32 startIndex ) const
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
};

} }