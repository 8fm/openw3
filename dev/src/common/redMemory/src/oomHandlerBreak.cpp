/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "oomHandlerBreak.h"
#include "log.h"
#include "assert.h"
#include "reporter.h"

namespace red
{
namespace memory
{
	OOMHandlerBreak::OOMHandlerBreak()
		: m_reporter( nullptr )
	{}

	OOMHandlerBreak::~OOMHandlerBreak()
	{}

	void OOMHandlerBreak::Initialize( const Reporter * reporter )
	{
		m_reporter = reporter;
	}

	void OOMHandlerBreak::OnHandleAllocateFailure( const char * poolName, u32 size, u32 /*alignment*/ )
	{
		RED_MEMORY_LOG( " ***** OUT OF MEMORY! *****" );
		RED_MEMORY_LOG( "Failed to allocate %d bytes from pool '%s'", size, poolName );
		
		if( m_reporter )
		{
			m_reporter->WriteReportToLog();
		}

		RED_MEMORY_HALT( "Out of Memory! Failed to allocate %d bytes.", size );

#ifdef RED_FINAL_BUILD
		// We force a crash in final builds, just so we can see this is a OOM situation.
		// On shipping, we should most likely disable this, unless we will be getting crash dumps from live machines
		int* forceCrashNow = nullptr;
		*forceCrashNow = 0xF0000000;
#endif
	
		RED_UNUSED( poolName );
		RED_UNUSED( size );
	}

}
}
