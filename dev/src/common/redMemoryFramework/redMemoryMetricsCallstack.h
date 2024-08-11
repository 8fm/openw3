/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#ifndef _RED_MEMORY_METRICS_CALLSTACK_H
#define _RED_MEMORY_METRICS_CALLSTACK_H

#include "../redSystem/types.h"

namespace Red { namespace MemoryFramework {

//////////////////////////////////////////////////////////////////////
// In order to keep this as fast as possible (at the expense of memory)
// it has a fixed size. Try to keep this < cache-line and aligned to word-size
class MetricsCallstack
{
public:
	MetricsCallstack( );	// Do nothing on default constructor
	MetricsCallstack( Red::System::Int32 stackAddressesToIgnore );
	MetricsCallstack( const MetricsCallstack& other );
	~MetricsCallstack( );
	const MetricsCallstack& operator=( const MetricsCallstack& other );

	typedef Red::System::Uint64 CallStackAddress;					// Platform dependant!

	RED_INLINE Red::System::Int32 GetCallstackDepth();
	RED_INLINE CallStackAddress GetAddress( Red::System::Int32 index );
	RED_INLINE void SetHash( Red::System::Uint64 hash );
	RED_INLINE Red::System::Uint64 GetHash( ) const;
	void GetAsString( Red::System::Int32 index, Red::System::Char* stringBuffer, Red::System::Int32 maxLength );

	static const Red::System::Int32 k_largestStackFrame = 20;		// Store no more than k_largestStackFrame levels of callstack

private:
	RED_INLINE void PushAddress( CallStackAddress address );

	CallStackAddress m_callstack[ k_largestStackFrame ];
	Red::System::Uint64 m_hash;										// Keep a hash of the callstack
	Red::System::Int32 m_callStackDepth;							// How many addresses are in the callstack (max k_largestStackFrame)
};

} }

#include "redMemoryMetricsCallstack.inl"

#endif