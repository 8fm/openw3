/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "nameHash.h"

namespace Red
{

//////////////////////////////////////////////////////////////////////////
// CNameHash
//////////////////////////////////////////////////////////////////////////
CNameHash CNameHash::Hash( const AnsiChar* name )
{
	if ( ! name )
	{
		return CNameHash( INVALID_HASH_VALUE );
	}

	TValue hash = HASH_OFFSET;
	for ( const AnsiChar* ch = name; *ch; ++ch )
	{
		hash ^= *ch;
		hash *= HASH_PRIME;
	}

	hash ^= '\0';
	hash *= HASH_PRIME;

// 	if ( name && *name && hash == INVALID_HASH_VALUE )
// 	{
// 		__debugbreak();
// 	}

	return CNameHash( hash );
}

// Assumes endianness
CNameHash CNameHash::Hash( const UniChar* name )
{
	static_assert( sizeof( UniChar ) == sizeof( Uint16 ), "Create another hash function." );
	static_assert( ((Uint16)L'A' & 0x00FF) == (Uint16)'A' && ((Uint16)L'A' & 0xFF00) == 0, "Bad hi/lo" );

	if ( ! name )
	{
		return CNameHash( INVALID_HASH_VALUE );
	}

	TValue hash = HASH_OFFSET;
	for ( const UniChar* ch = name; *ch; ++ch )
	{
		const TValue hashHi = ( TValue(*ch) & 0xFF00 ) >> 8;
		const TValue hashLo = ( TValue(*ch) & 0x00FF );

		// Create the same hash as the AnsiChar* if UniChar* is really just ASCII text
		if ( hashHi )
		{
			hash ^= hashHi;
			hash *= HASH_PRIME;
		}

		hash ^= hashLo;
		hash *= HASH_PRIME;
	}

	// Only checking one zero, but no compile time hash to UniChar*, so doesn't matter
	hash ^= '\0';
	hash *= HASH_PRIME;

	// 	if ( name && *name && hash == INVALID_HASH_VALUE )
	// 	{
	// 		__debugbreak();
	// 	}

	return CNameHash( hash );
}

} // namespace Red