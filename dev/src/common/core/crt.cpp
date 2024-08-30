/**
* Copyright � 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "crt.h"
#include "string.h"

//-------------------------------------------------------------------------------------------------
// FNV1a no-table, fast, low collision hash
//-------------------------------------------------------------------------------------------------

Uint32 ACalcBufferHash32Merge( const void* buffer, size_t sizeInBytes, Uint32 previousHash )
{
	const Uint8* data = (const Uint8*) buffer;
	Uint32 hash = previousHash;
	while( sizeInBytes-- )
	{
		hash ^= *data++;
		hash *= 0x01000193;
	}
	return hash;
}

//-------------------------------------------------------------------------------------------------

Uint64 ACalcBufferHash64Merge( const void* buffer, size_t sizeInBytes, Uint64 previousHash )
{
	const Uint8* data = (const Uint8*) buffer;
	Uint64 hash = previousHash;
	while( sizeInBytes-- )
	{
		hash ^= *data++;
		hash *= 0x100000001B3;
	}
	return hash;
}

//-------------------------------------------------------------------------------------------------

Uint64 ACalcBufferHash64Merge( const String& string, Uint64 previousHash )
{
	const Uint8* data = (const Uint8*) string.AsChar();
	Uint32 sizeInChars = static_cast< Uint32 >(string.GetLength());
	Uint64 hash = previousHash;
	while( sizeInChars-- )
	{
		hash ^= data[1];
		hash *= 0x100000001B3;
		hash ^= data[0];
		hash *= 0x100000001B3;
		data += 2;
	}
	return hash;
}

//-------------------------------------------------------------------------------------------------

Uint64 ACalcBufferHash64Merge( const StringAnsi& string, Uint64 previousHash )
{
	return ACalcBufferHash64Merge( string.AsChar(), string.GetLength(), previousHash );
}

//-------------------------------------------------------------------------------------------------

Uint64 ACalcBufferHash64MergeWithRespectToEndians( const Char* string, size_t sizeInChars, Uint64 previousHash )
{
	const Uint8* data = (const Uint8*) string;
	Uint64 hash = previousHash;
	while( sizeInChars-- )
	{
		hash ^= data[1];
		hash *= 0x100000001B3;
		hash ^= data[0];
		hash *= 0x100000001B3;
		data += 2;
	}
	return hash;
}

//-------------------------------------------------------------------------------------------------

Uint32 ACalcBufferHash32MergeWithRespectToEndians( const Char* buffer, size_t sizeInChars, Uint32 previousHash )
{
	const Uint8* data = (const Uint8*) buffer;
	Uint32 hash = previousHash;
	while( sizeInChars-- )
	{
		hash ^= data[1];
		hash *= 0x01000193;
		hash ^= data[0];
		hash *= 0x01000193;
		data += 2;
	}
	return hash;
}

//-------------------------------------------------------------------------------------------------

void ReplaceUnicodeCharsWithAscii( Char buffer[], size_t length )
{
	RED_UNUSED( buffer );
	RED_UNUSED( length );
//RED_MESSAGE("FIXME>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>")
#if !(defined( RED_PLATFORM_ORBIS ) || defined( RED_PLATFORM_LINUX ))
	for( size_t i = 0; i < length; ++i )
	{
		switch( buffer[ i ] )
		{
		// Polish
		case L'�':
			buffer[ i ] = L'A';
			break;

		case L'�':
			buffer[ i ] = L'a';
			break;

		case L'�':
			buffer[ i ] = L'C';
			break;

		case L'�':
			buffer[ i ] = L'c';
			break;

		case L'�':
			buffer[ i ] = L'E';
			break;

		case L'�':
			buffer[ i ] = L'e';
			break;

		case L'�':
			buffer[ i ] = L'L';
			break;

		case L'�':
			buffer[ i ] = L'l';
			break;

		case L'�':
			buffer[ i ] = L'N';
			break;

		case L'�':
			buffer[ i ] = L'n';
			break;

		case L'�':
			buffer[ i ] = L'O';
			break;

		case L'�':
			buffer[ i ] = L'o';
			break;

		case L'�':
			buffer[ i ] = L'S';
			break;

		case L'�':
			buffer[ i ] = L's';
			break;

		case L'�':
			buffer[ i ] = L'Z';
			break;

		case L'�':
			buffer[ i ] = L'z';
			break;

		case L'�':
			buffer[ i ] = L'Z';
			break;

		case L'�':
			buffer[ i ] = L'z';
			break;
		}
	}
#endif
}
