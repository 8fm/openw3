/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#include "guid.h"
#include "os.h"
#include "../redThreads/redThreadsAtomic.h"

#if ( defined( RED_PLATFORM_WIN32 ) || defined( RED_PLATFORM_WIN64 ) )
#	include <objbase.h>
#endif

// CRuntimeGUIDGenerator

Red::Threads::CAtomic< Red::System::Uint64 > g_runtimeGUIDCounter( 0 );

Red::System::Uint64 Red::System::CRuntimeGUIDGenerator::GetRuntimeCounter()
{
	return g_runtimeGUIDCounter.GetValue();
}

void Red::System::CRuntimeGUIDGenerator::SetRuntimeCounter( Uint64 counter )
{
	g_runtimeGUIDCounter.SetValue( counter );
}

Red::System::Bool Red::System::CRuntimeGUIDGenerator::CanCollideWithRuntimeGUID( const Red::System::GUID& guid )
{
	return guid.parts.A == 0xFFFFFFFF && guid.parts.B == 0xFFFFFFFF;
}

Red::System::GUID Red::System::CRuntimeGUIDGenerator::CreateGUID()
{
	// Note:
	// It seems CoCreateGuid() generates GUIDs that have 0 set on the following 4 bits: 60,61,63,70
	// So, to avoid collision with these we're setting bits 0..63 to 1 (ignore the 70th bit for convenience reasons)
	// Also, to assure GUID uniqueness, we put the next value of persistent (stored in game savegame) 64-bit counter into remaining 64 bits

	GUID guid;
	guid.parts.A = 0xFFFFFFFF;
	guid.parts.B = 0xFFFFFFFF;
	*( Uint64* ) &guid.parts.C = g_runtimeGUIDCounter.Increment();

	return guid;
}

// GUID

const Red::System::GUID Red::System::GUID::ZERO;

Red::System::GUID::GUID( const Char* str )
{
	MemoryZero( this, sizeof( GUID ) );
	FromString( str );
}

Red::System::Bool Red::System::GUID::FromString( const Char* str )
{
	const Char* lastPtr = str + StringLength( str );
	Char* endPtr = nullptr;
	for( Uint32 i = 0; i < RED_GUID_NUM_PARTS; ++i )
	{
		RED_VERIFY( StringToInt( guid[ i ], str, &endPtr, BaseSixteen ), TXT( "Error converting string to guid: %" ) MACRO_TXT( RED_PRIs ), str );

		RED_ASSERT( str != endPtr, TXT( "Error converting string to guid: %" ) MACRO_TXT( RED_PRIs ), str );

		// Quit if conversion failed
		if ( str == endPtr )
		{
			return false;
		}

		// Quit if we get to the end of the string
		if ( endPtr >= lastPtr )
		{
		    return ( i == RED_GUID_NUM_PARTS - 1 );
		}

		// Make sure we skip over the "-" char
		str = endPtr + 1;
	}

	return true;
}

const Red::System::AnsiChar* GCurrentCookedResourcePath = nullptr;
Red::System::Uint32 GCurrentCookedResourceGUIDIndex = 0;
Red::System::Bool GDeterministicGUIDGeneration = false;

Red::System::GUID Red::System::GUID::Create()
{
	GUID newGuid;

	if( GDeterministicGUIDGeneration )
	{
		if( GCurrentCookedResourcePath == nullptr )
			return Red::System::GUID::ZERO;

		Uint64 resourceHash = Red::CalculateHash64( GCurrentCookedResourcePath );
		newGuid.guid[ 0 ] = resourceHash >> 32;
		newGuid.guid[ 1 ] = resourceHash & 0xffffffff;
		newGuid.guid[ 2 ] = 0xBAADF00D;
		newGuid.guid[ 3 ] = GCurrentCookedResourceGUIDIndex++;
		return newGuid;
	}

#if ( defined( RED_PLATFORM_WIN32 ) || defined( RED_PLATFORM_WIN64 ) ) && !defined( RED_FINAL_BUILD )

	Bool canCollideWithRuntimeGUID;
	do 
	{
		::GUID guid;
		RED_VERIFY( CoCreateGuid( &guid ) == S_OK, TXT( "Failed to create GUID" ) );
		static_assert( sizeof( ::GUID ) == sizeof( GUID ), "GUID size is no equal to Windows GUID" );
		MemoryCopy( &newGuid, &guid, sizeof( GUID ) );

		canCollideWithRuntimeGUID = CRuntimeGUIDGenerator::CanCollideWithRuntimeGUID( newGuid );
		RED_ASSERT( !canCollideWithRuntimeGUID );

	} while ( canCollideWithRuntimeGUID );

#else

	newGuid = CRuntimeGUIDGenerator::CreateGUID();

#endif

	return newGuid;
}

Red::System::GUID Red::System::GUID::Create( const Char* str )
{
	GUID newGuid;
	newGuid.FromString( str );
	return newGuid;
}

Red::System::GUID Red::System::GUID::CreateFromUint32( Uint32 value )
{
	GUID newGuid;
	newGuid.parts.A = 0;
	newGuid.parts.B = 0;
	newGuid.parts.C = 0;
	newGuid.parts.D = value;
	return newGuid;
}

Red::System::GUID Red::System::GUID::CreateFromUint64( Uint64 value )
{
	GUID newGuid;
	newGuid.parts.A = 0xFFFFFFFE; // Set last bit to 0 to avoid collision with both runtime and static GUIDs
	newGuid.parts.B = 0xFFFFFFFF;
	*( Uint64* ) &newGuid.parts.C = value;
	return newGuid;
}
