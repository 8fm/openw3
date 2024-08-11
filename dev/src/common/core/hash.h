/**
 * Copyright © 2008 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "../redSystem/hash.h"
#include "types.h"
#include "string.h"

// Default implementation
template< typename K > RED_FORCE_INLINE Uint32 GetHash( const K& key ) { return key.CalcHash(); }

// Specializations of GetHash

RED_FORCE_INLINE Uint32 GetHash( const Bool key ) { return key ? 1 : 0; }

RED_FORCE_INLINE Uint32 GetHash( const Int8 key ) { return key; }
RED_FORCE_INLINE Uint32 GetHash( const Uint8 key ) { return key; }

RED_FORCE_INLINE Uint32 GetHash( const Char key ) { return key; }
RED_FORCE_INLINE Uint32 GetHash( const Int16 key ) { return key; }
RED_FORCE_INLINE Uint32 GetHash( const Uint16 key ) { return key; }

RED_FORCE_INLINE Uint32 GetHash( const Int32 key ) { return key; }
RED_FORCE_INLINE Uint32 GetHash( const Uint32 key ) { return key; }

RED_FORCE_INLINE Uint32 GetHash( const Int64 key ) { return (Uint32)(key) ^ (Uint32)(key>>32) ; }
RED_FORCE_INLINE Uint32 GetHash( const Uint64 key ) { return (Uint32)(key) ^ (Uint32)(key>>32) ; }

RED_FORCE_INLINE Uint32 GetHash( const Float key ) { return *((Uint32*)&key); }
RED_FORCE_INLINE Uint32 GetHash( const Double key ) { return *((Uint32*)&key); }

// Expects array
template< typename T >
Uint32 GetArrayHash( const T* key, Uint32 length )
{
	// Set up the internal state
	Uint32 offset = 0;
	Uint32 a = 0x9e3779b9;					// the golden ratio; an arbitrary value
	Uint32 b = 0x9e3779b9;					// the golden ratio; an arbitrary value
	Uint32 c = 0x0;							// variable initialization of internal state
	const Uint32 originalLength = length;

#define HashMix(a,b,c)			 \
{								 \
	a=a-b;  a=a-c;  a=a^(c>>13); \
	b=b-c;  b=b-a;  b=b^(a<<8);  \
	c=c-a;  c=c-b;  c=c^(b>>13); \
	a=a-b;  a=a-c;  a=a^(c>>12); \
	b=b-c;  b=b-a;  b=b^(a<<16); \
	c=c-a;  c=c-b;  c=c^(b>>5);  \
	a=a-b;  a=a-c;  a=a^(c>>3);  \
	b=b-c;  b=b-a;  b=b^(a<<10); \
	c=c-a;  c=c-b;  c=c^(b>>15); \
}

	// Handle case when key length is >= 12 chars
	while ( length >= 12 )
	{
		a = a + (key[offset+0] + ((Uint32)key[offset+1]<<8) + ((Uint32)key[offset+2]<<16)  + ((Uint32)key[offset+3]<<24));
		b = b + (key[offset+4] + ((Uint32)key[offset+5]<<8) + ((Uint32)key[offset+6]<<16)  + ((Uint32)key[offset+7]<<24));
		c = c + (key[offset+8] + ((Uint32)key[offset+9]<<8) + ((Uint32)key[offset+10]<<16) + ((Uint32)key[offset+11]<<24));
		HashMix(a,b,c);
		offset += 12;
		length -= 12;
	}

	// Handle the last 11 chars. All the case statements fall through 
	c = c + originalLength;
	switch( length )              
	{
		case 11: c = c + ((Uint32)key[offset+10]<<24);
		case 10: c = c + ((Uint32)key[offset+9]<<16);
		case 9 : c = c + ((Uint32)key[offset+8]<<8);
		case 8 : b = b + ((Uint32)key[offset+7]<<24);
		case 7 : b = b + ((Uint32)key[offset+6]<<16);
		case 6 : b = b + ((Uint32)key[offset+5]<<8);
		case 5 : b = b + ((Uint32)key[offset+4]);
		case 4 : a = a + ((Uint32)key[offset+3]<<24);
		case 3 : a = a + ((Uint32)key[offset+2]<<16);
		case 2 : a = a + ((Uint32)key[offset+1]<<8);
		case 1 : a = a + ((Uint32)key[offset+0]);
	}
	HashMix(a,b,c);

#undef HashMix

	return c;
}

template< typename T >
RED_FORCE_INLINE Uint32 GetPtrHash( const T* ptr )
{
	Uint32 hash = ( Uint32& ) ptr;
	hash = (hash ^ 61) ^ (hash >> 16);
	hash = hash + (hash << 3);
	hash = hash ^ (hash >> 4);
	hash = hash * 0x27d4eb2d;
	hash = hash ^ (hash >> 15);
	return hash;
}

template< typename K, Bool passByValue = ( sizeof( K ) <= sizeof( int ) ) >
class DefaultHashFunc
{
};

// Default 'default hash function'
template< typename K >
class DefaultHashFunc< K, false >
{
public:
	static RED_FORCE_INLINE Uint32 GetHash( const K& key ) { return ::GetHash( key ); }
};

// Specialization of the default hash function for types smaller than native word size (e.g. 8 bytes on 64 bits)
template< typename K >
class DefaultHashFunc< K, true >
{
public:
	static RED_FORCE_INLINE Uint32 GetHash( const K key ) { return ::GetHash( key ); }
};

// Specialization of the default hash function for pointers (had to specialize for both passValue being true and false; is there a way to avoid this?)

template< typename K >
class DefaultHashFunc< K*, false >
{
public:
	static RED_FORCE_INLINE Uint32 GetHash( const K* key ) { return GetPtrHash( key ); }
};

template< typename K >
class DefaultHashFunc< K*, true >
{
public:
	static RED_FORCE_INLINE Uint32 GetHash( const K* key ) { return GetPtrHash( key ); }
};

// Slower (and supposedly better) implementation of String hashing
class StringQHashFunc
{
public:
	static RED_FORCE_INLINE Uint32 GetHash( const String& key )
	{
		return key.CalcQHash();
	}
};

// Hash container statistics (available via THashSet::GetStats() and THashMap::GetStats())
struct THashContainerStats
{
	Uint32 m_size;					// Number of elements stored in hash container
	Uint32 m_numBuckets;			// Number of buckets with the same hash modulo bucket count
	Uint32 m_numUsedBuckets;		// Number of non-empty buckets (the higher the better)
	Uint32 m_largestBucketSize;		// Number of elements in largest bucket (the lower the better)
	Float m_averageBucketSize;		// Average number of elements in bucket (the lower the better)
};