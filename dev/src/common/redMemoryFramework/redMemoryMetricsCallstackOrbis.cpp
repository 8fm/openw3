/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "redMemoryMetricsCallstack.h"
#include "redMemoryAssert.h"

namespace Red { namespace MemoryFramework {

const Red::System::Int32 k_OrbisSkipDepth = 1;

/////////////////////////////////////////////////////////////
// CTor
//	This should grab the callstack addresses on construction
MetricsCallstack::MetricsCallstack( Red::System::Int32 stackAddressesToIgnore )
	: m_callStackDepth(0)
{
	RED_MEMORY_ASSERT( stackAddressesToIgnore >= k_OrbisSkipDepth,  "Cannot skip less than %d level. Worst case will result in deadly crash", k_OrbisSkipDepth );

	// _Unwind_Backtrace is currently deadly slow and can't be use real time. __builtin_return_address is currently the only alternative.
	// Take not: __builtin_return_address can only be use with constant ...
	// Since thread can do alloc already on 3rd level function, I currently can't go deeper than 3 level.
	m_callstack[ 0 ] = (CallStackAddress)__builtin_return_address( k_OrbisSkipDepth );
	m_callstack[ 1 ] = (CallStackAddress)__builtin_return_address( k_OrbisSkipDepth + 1 );
	m_callstack[ 2 ] = (CallStackAddress)__builtin_return_address( k_OrbisSkipDepth + 2 );
	m_callStackDepth = 3;
}

/////////////////////////////////////////////////////////////
// GetAsString
//	Resolve a stack frame pointer to a string
void MetricsCallstack::GetAsString( Red::System::Int32 index, Red::System::Char* stringBuffer, Red::System::Int32 maxLength )
{
	if( index < 3 )
	{
		Red::System::SNPrintF( stringBuffer, maxLength, TXT( "%p" ), m_callstack[ index ] );
	}
}

} }