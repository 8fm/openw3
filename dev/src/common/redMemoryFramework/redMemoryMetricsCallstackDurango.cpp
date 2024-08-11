/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "redMemoryMetricsCallstack.h"

namespace Red { namespace MemoryFramework {

/////////////////////////////////////////////////////////////
// CTor
//	This should grab the callstack addresses on construction
MetricsCallstack::MetricsCallstack( Red::System::Int32 stackAddressesToIgnore )
	: m_callStackDepth(0)
{
	void* virtualAddresses[ k_largestStackFrame ] = { nullptr };
	DWORD dwHash = 0;
	Red::System::Uint32 callstackCaptured = RtlCaptureStackBackTrace( stackAddressesToIgnore, k_largestStackFrame, virtualAddresses, &dwHash );

	// Virtual addresses need to be translated back to module offsets.
	for( Red::System::Uint32 i = 0; i < callstackCaptured; ++i )
	{
		m_callstack[ i ] = reinterpret_cast<CallStackAddress>( virtualAddresses[ i ]);
	}	

	m_hash = dwHash;
	m_callStackDepth = callstackCaptured;
}

/////////////////////////////////////////////////////////////
// GetAsString
//	Resolve a stack frame pointer to a string
void MetricsCallstack::GetAsString( Red::System::Int32 index, Red::System::Char* stringBuffer, Red::System::Int32 maxLength )
{
	RED_UNUSED( index );
	RED_UNUSED( stringBuffer );
	RED_UNUSED( maxLength );
}

} }