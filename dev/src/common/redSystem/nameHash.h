/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "types.h"
#include "crt.h"

namespace Red
{

using namespace System;

//////////////////////////////////////////////////////////////////////////
// CNameHash
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// !!! NOTE !!! NOTE !!! NOTE !!! NOTE !!! NOTE !!! NOTE !!! NOTE !!!
// Since we're using a string literal in a template argument and recursively going backwards
// it includes the null terminator in the compile time hash calculation. Therefore the non-compile time hash
// calculation uses strlen + 1 to be consistent, assuming all strings are null terminated.
//////////////////////////////////////////////////////////////////////////

// Primarily a wrapper around the hash value to help prevent overload ambiguities ( e.g., when implicitly converting to CName from RED_NAME(x) )
// Don't add any conversion operators (e.g., even operator Bool() ) or else you'll hit those amiguities.
class CNameHash
{
public:
	static const Uint32 HASH_OFFSET = 2166136261u;
	static const Uint32 HASH_PRIME = 16777619u;

public:
	typedef Uint32 TValue;

public:
	static const TValue INVALID_HASH_VALUE = 0;

public:
	static CNameHash Hash( const AnsiChar* name );
	static CNameHash Hash( const UniChar* name );

public:
	struct SConstCharWrapper
	{
		const AnsiChar* m_name;

		RED_FORCE_INLINE SConstCharWrapper( const AnsiChar* name )
			: m_name( name )
		{}
	};

private:
	template< size_t N, size_t I >
	struct SHashGenerator
	{
		RED_FORCE_INLINE static TValue Hash( const AnsiChar (&name)[N] )
		{
			return ( SHashGenerator<N, I-1>::Hash(name) ^ name[I-1] ) * HASH_PRIME;
		}
	};

private:
	TValue m_value;

public:
	RED_FORCE_INLINE CNameHash()
		: m_value( 0 )
	{
	}

	explicit RED_FORCE_INLINE CNameHash( TValue value )
		: m_value( value )
	{
	}

	template< size_t N >
	RED_FORCE_INLINE CNameHash( const AnsiChar (&name)[N] )
		: m_value( SHashGenerator< N, N >::Hash( name ) )
	{
	}

	RED_FORCE_INLINE CNameHash( SConstCharWrapper wrapper )
		: m_value( Hash( wrapper.m_name ).GetValue() )
	{
	}

	RED_FORCE_INLINE CNameHash( AnsiChar* name )
		: m_value( Hash( name ).GetValue() )
	{
	}

public:
	Bool operator==( CNameHash rhs ) const { return m_value == rhs.m_value; }
	Bool operator!=( CNameHash rhs ) const { return m_value != rhs.m_value; }
	Bool operator<( CNameHash rhs ) const { return m_value < rhs.m_value; }

public:
	TValue GetValue() const { return m_value; }
};

template< size_t N >
struct CNameHash::SHashGenerator< N, 1 >
{
	RED_FORCE_INLINE static TValue Hash( const AnsiChar (&name)[N] )
	{
		return ( HASH_OFFSET ^ name[0] ) * HASH_PRIME;
	}
};

static_assert( sizeof(CNameHash) == sizeof(CNameHash::TValue), "CNameHash is supposed to be a lightweight wrapper!" );

} // namespace Red
