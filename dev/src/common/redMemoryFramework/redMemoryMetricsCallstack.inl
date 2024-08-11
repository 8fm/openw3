/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "redMemoryAssert.h"
#include "../redSystem/crt.h"
#include "../redSystem/utility.h"

namespace Red { namespace MemoryFramework {

//////////////////////////////////////////////////////////////////////
// Default constructor
//	Does nothing
RED_INLINE MetricsCallstack::MetricsCallstack()
	: m_hash( 0 )
	, m_callStackDepth( 0 )
{
}

//////////////////////////////////////////////////////////////////////
// Copy constructor
//	Return how many addresses are in this stack frame
RED_INLINE MetricsCallstack::MetricsCallstack( const MetricsCallstack& other )
{
	m_callStackDepth = other.m_callStackDepth;
	m_hash = other.m_hash;
	Red::System::MemoryCopy( &m_callstack, &other.m_callstack, sizeof( m_callstack[0] ) * m_callStackDepth );
}

/////////////////////////////////////////////////////////////
// Copy Ctor
RED_INLINE const MetricsCallstack& MetricsCallstack::operator=( const MetricsCallstack& other )
{
	m_callStackDepth = other.m_callStackDepth;
	m_hash = other.m_hash;
	Red::System::MemoryCopy( &m_callstack, &other.m_callstack, sizeof( m_callstack[0] ) * m_callStackDepth );

	return *this;
}

//////////////////////////////////////////////////////////////////////
// SetHash
//	Hashes are stored so we can save memory by reducing duplicates
RED_INLINE void MetricsCallstack::SetHash( Red::System::Uint64 hash )
{
	m_hash = hash;
}

//////////////////////////////////////////////////////////////////////
// GetHash
//	Hashes are stored so we can save memory by reducing duplicates
RED_INLINE Red::System::Uint64 MetricsCallstack::GetHash( ) const
{
	return m_hash;
}


//////////////////////////////////////////////////////////////////////
// GetCallstackDepth
//	Return how many addresses are in this stack frame
RED_INLINE Red::System::Int32 MetricsCallstack::GetCallstackDepth()
{
	return m_callStackDepth;
}

//////////////////////////////////////////////////////////////////////
// GetAddress
//	Get a address from the callstack
RED_INLINE MetricsCallstack::CallStackAddress MetricsCallstack::GetAddress( Red::System::Int32 index )
{
	RED_MEMORY_ASSERT( index < m_callStackDepth, "Trying to index a bad callstack address" );

	CallStackAddress result = 0u;
	if( index < m_callStackDepth )
	{
		result = m_callstack[ index ];
	}

	return result;
}

//////////////////////////////////////////////////////////////////////
// PushAddress
//	Push an address to the stack
RED_INLINE void MetricsCallstack::PushAddress( MetricsCallstack::CallStackAddress address )
{
	if( m_callStackDepth < k_largestStackFrame )
	{
		m_callstack[m_callStackDepth++] = address;
	}
}

} }
