/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#ifndef _RED_SYSTEM_CRASH_REPORTER_WINDOWS_H_
#define _RED_SYSTEM_CRASH_REPORTER_WINDOWS_H_
#pragma once

#include "utility.h"
#include "os.h"

#if defined( RED_PLATFORM_WIN32 ) || defined( RED_PLATFORM_WIN64 )

namespace Red 
{
	namespace System
	{
		namespace Error
		{
			// This class will open a memory-mapped file for the duration of its lifetime
			// Note that writes are NOT thread-safe. Provide your own synchronisation if you need any!
			class CrashReportDataBuffer : public NonCopyable
			{
			public:
				CrashReportDataBuffer( const Char* identifier, MemSize maximumSize );
				~CrashReportDataBuffer();

				void Write( const void* data, MemSize size );
				void* ReadBufferContents( MemSize& bufferSize );

			private:
				CrashReportDataBuffer();

				void OpenMemoryMappedFile( const Char* identifier, MemSize size );
				void CloseMemoryMappedFile();

				HANDLE m_mappedFileHandle;		// Handle to a memory mapped file opened on construct
				LPVOID m_fileBuffer;			// Buffer view of the mapped file
				LPVOID m_writeHead;				// Head ptr for writing
				MemSize m_bufferSize;			// Total size of the buffer
			};
		}
	}
}

#endif // RED_PLATFORM_WIN32 || RED_PLATFORM_WIN64

#endif // _RED_SYSTEM_CRASH_REPORTER_WINDOWS_H_