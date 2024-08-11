/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#ifndef _RED_SYSTEM_CRASH_REPORTER_DURANGO_H_
#define _RED_SYSTEM_CRASH_REPORTER_DURANGO_H_
#pragma once

namespace Red 
{
	namespace System
	{
		namespace Error
		{
			// Stubbed for now, implement me later!
			class CrashReportDataBuffer : public NonCopyable
			{
			public:
				CrashReportDataBuffer( const Char* identifier, MemSize maximumSize )
				{
					RED_UNUSED( identifier );
					RED_UNUSED( maximumSize );
				}

				~CrashReportDataBuffer()
				{
				}

				void Write( const void* data, MemSize size )
				{
					RED_UNUSED( data );
					RED_UNUSED( size );
				}

				void* ReadBufferContents( MemSize& bufferSize )
				{
					RED_UNUSED( bufferSize );
					return nullptr;
				}
			};
		}
	}
}

#endif
