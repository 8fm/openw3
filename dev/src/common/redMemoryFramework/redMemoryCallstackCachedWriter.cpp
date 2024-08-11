/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "redMemoryCallstackCachedWriter.h"
#include "redMemoryMetricsCallstack.h"

namespace Red { namespace MemoryFramework {

/////////////////////////////////////////////////////////////////
// CTor
//
CallstackCachedWriter::CallstackCachedWriter( OSAPI::FileWriter& myFileWriter )
	: m_writeHead( m_hashes )
	, m_count( 0 )
	, m_fileWriter( myFileWriter )
{
}

/////////////////////////////////////////////////////////////////
// DTor
//
CallstackCachedWriter::~CallstackCachedWriter()
{
	m_count = 0;
}

/////////////////////////////////////////////////////////////////
// CallstackCached
//	Returns true if the callstack has already been written
// If this is too slow, search forwards instead
bool CallstackCachedWriter::CallstackCached( Red::System::Uint64 csHash )
{
	// We search backwards from the write pointer as repeating callstacks will most likely happen close together
	Red::System::Uint32 numberSearched = 0;
	Red::System::Uint64* searchPtr = m_writeHead;
	Red::System::Uint32 cacheSize = c_CacheSize;
	while( searchPtr-- > m_hashes )
	{
		if( *searchPtr == csHash )
		{
			return true;
		}
		numberSearched++;
	}

	// If numSearched < count, we need to now search backwards from the end of the buffer
	if( numberSearched < m_count )
	{
		searchPtr = m_hashes + cacheSize;
		while( searchPtr-- > m_writeHead )
		{
			if( *searchPtr == csHash )
			{
				return true;
			}
		}
	}

	return false;
}

/////////////////////////////////////////////////////////////////
// WriteCallstack
//	If a callstack has been written already, just output its hash,
//	otherwise, write the stack trace addresses
void CallstackCachedWriter::WriteCallstack( MetricsCallstack& callstack )
{
	Red::System::Uint8 csDepth = static_cast< Red::System::Uint8 >( callstack.GetCallstackDepth() );
	Red::System::Uint64 csHash = callstack.GetHash();
	Red::System::Uint32 cacheSize = c_CacheSize;
	if( !CallstackCached( csHash ) )
	{
		m_fileWriter.Write( &csDepth, sizeof( csDepth ) );
		m_fileWriter.Write( &csHash, sizeof( csHash ) );
		for( Red::System::Uint8 csIndex = 0; csIndex < csDepth; ++csIndex )
		{
			MetricsCallstack::CallStackAddress csAddress = callstack.GetAddress( csIndex );
#ifdef RED_ARCH_X86
			Red::System::Uint32 callstackAddress32 = static_cast< Red::System::Uint32 >( csAddress );
			m_fileWriter.Write( &callstackAddress32, sizeof( callstackAddress32 ) );
#else
			m_fileWriter.Write( &csAddress, sizeof( csAddress ) );
#endif
		}

		// Add the callstack 
		*m_writeHead++ = csHash;
		if( m_writeHead >= m_hashes + cacheSize )
		{
			m_writeHead = m_hashes;
		}
		m_count = Red::Math::NumericalUtils::Min( m_count + 1, cacheSize );
	}
	else
	{
		csDepth |= 128u;		// Top bit set = cached stack trace
		m_fileWriter.Write( &csDepth, sizeof( csDepth ) );
		m_fileWriter.Write( &csHash, sizeof( csHash ) );
	}
}

} }