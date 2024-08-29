/**
* Copyright (c) 2013 CD Projekt Red. All Rights Reserved.
*/
#include "redMemoryMetricsCallstack.h"
#include "redMemoryAssert.h"
#include "../redSystem/hash.h"

namespace Red { namespace MemoryFramework {

/////////////////////////////////////////////////////////////
// Linux Callstack helper
namespace LinuxAPI {

struct StackFrame
{
	uintptr_t nextFrame;
	uintptr_t functionReturnAddress;
	uintptr_t unknown;
};

Red::System::Uint32 CaptureStackBackTrace( Red::System::Uint32 framesToSkip, Red::System::Uint32 maxDepth, uintptr_t* frames, Red::System::Uint64& hash )
{
	StackFrame* stackFrame = reinterpret_cast< StackFrame* >( __builtin_frame_address( 0 ) );

	while ( stackFrame && framesToSkip )
	{
		stackFrame = reinterpret_cast< StackFrame* >( stackFrame->nextFrame );
		--framesToSkip;
	}

	Red::System::Uint32 depth = 0;
	while ( stackFrame && depth < maxDepth )
	{
		frames[depth] = stackFrame->functionReturnAddress;

		const Red::System::Uint64 address = frames[depth];
		if ( hash == 0 )
		{
			hash = Red::System::CalculateHash64( &address, sizeof( address ) );
		}
		else
		{
			hash = Red::System::CalculateHash64( &address, sizeof( address ), hash );
		}

		++depth;
		stackFrame = reinterpret_cast< StackFrame* >( stackFrame->nextFrame );
	}

	return depth > 0 ? depth - 1 : 0;
}

} // namespace LinuxAPI {

/////////////////////////////////////////////////////////////
// CTor
//	This should grab the callstack addresses on construction
MetricsCallstack::MetricsCallstack( Red::System::Int32 stackAddressesToIgnore )
	: m_callStackDepth(0)
{
	// Temporary list of stack frame pointers used for capture
	uintptr_t callStackFramesTemp[k_largestStackFrame] = { 0 };
	m_callStackDepth = LinuxAPI::CaptureStackBackTrace( stackAddressesToIgnore, k_largestStackFrame, callStackFramesTemp, m_hash );

	for ( Red::System::Uint32 frame = 1; frame < m_callStackDepth; ++frame )
	{
		m_callstack[ frame ] = reinterpret_cast< MetricsCallstack::CallStackAddress >( callStackFramesTemp[ frame ] );
	}
}

/////////////////////////////////////////////////////////////
// GetAsString
//	Resolve a stack frame pointer to a string
void MetricsCallstack::GetAsString( Red::System::Int32 index, Red::System::Char* stringBuffer, Red::System::Int32 maxLength )
{
	RED_MEMORY_ASSERT( index < m_callStackDepth,  "No data at this stack-frame depth" );
	if( index < m_callStackDepth )
	{
		Red::System::SNPrintF( stringBuffer, maxLength, TXT( "%p" ), m_callstack[ index ] );
	}
	else
	{
		Red::System::SNPrintF( stringBuffer, maxLength, TXT( "< Unknown >" ) );
	}
}

} }
