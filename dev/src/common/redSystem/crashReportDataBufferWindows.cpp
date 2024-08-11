/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#include "crt.h"
#include "crashReportDataBufferWindows.h"
#include "error.h"

#if defined( RED_PLATFORM_WIN32 ) || defined( RED_PLATFORM_WIN64 )

namespace Red 
{
	namespace System
	{
		namespace Error
		{
			//////////////////////////////////////////////////////////////////////
			// CTor
			//	Immediately maps memory for writing
			CrashReportDataBuffer::CrashReportDataBuffer( const Char* identifier, MemSize maximumSize )
				: m_mappedFileHandle( 0 )
				, m_fileBuffer( nullptr )
				, m_writeHead( nullptr )
				, m_bufferSize( 0 )
			{
				RED_ASSERT( identifier != NULL, TXT( "CrashReportDataBuffer cannot be created with no identifier string" ) );
				RED_ASSERT( maximumSize > 0, TXT( "CrashReportDataBuffer cannot be created with no maximum size" ) );

				OpenMemoryMappedFile( identifier, maximumSize );
			}

			//////////////////////////////////////////////////////////////////////
			// DTor
			//  Closes the memory-mapped file
			CrashReportDataBuffer::~CrashReportDataBuffer()
			{
				CloseMemoryMappedFile();
			}

			//////////////////////////////////////////////////////////////////////
			// Write
			//	Copy the passed data and increment the write head
			void CrashReportDataBuffer::Write( const void* data, MemSize size )
			{
				RED_ASSERT( data, TXT( "Trying to pass null to CrashReportDataBuffer::Write" ) );
				if( data != NULL && size > 0 && m_bufferSize > 0 )
				{
					MemUint bufferRemaining = reinterpret_cast< MemUint >( m_fileBuffer ) + m_bufferSize - reinterpret_cast< MemUint >( m_writeHead );
					if( size <= bufferRemaining )
					{
						MemoryCopy( m_writeHead, data, size );
						FlushViewOfFile( m_writeHead, size );	// Flush immediately (avoids caching)
						m_writeHead = reinterpret_cast< LPVOID >( reinterpret_cast< MemUint >( m_writeHead ) + size );
					}
				}
			}

			//////////////////////////////////////////////////////////////////////
			// OpenMemoryMappedFile
			//	Map some memory from the system page file for read/write
			//	Used to share data between processes
			void CrashReportDataBuffer::OpenMemoryMappedFile( const Char* identifier, MemSize size )
			{
				DWORD sizeHigh = 0;
				DWORD sizeLow = 0;

#ifdef RED_ARCH_X86
				// MemSize is 32 bit, we don't need to set high dword
				sizeLow = size;
#else
				// DWORDs are 32 bit, therefore we split the size
				sizeHigh = static_cast< DWORD >( ( size >> 32 ) & (MemSize)0x00000000ffffffffu );
				sizeLow =  static_cast< DWORD >( size & (MemSize)0x00000000ffffffffu );
#endif

				m_mappedFileHandle = CreateFileMapping( INVALID_HANDLE_VALUE,	// Map the page file rather than a specific file
														NULL,					// Default security attribs
														PAGE_READWRITE,			// We only need to write, but the crash reporter needs to read
														sizeHigh,				// High word of size
														sizeLow,				// Low word of size
														identifier );			// Global identifier used to open the mapping elsewhere
				
				if( m_mappedFileHandle == INVALID_HANDLE_VALUE )
				{
					RED_HALT( "Failed to create memory mapped file : %d", GetLastError() );
					return ;
				}

				m_fileBuffer = MapViewOfFile( m_mappedFileHandle,				// Use the previous mapped file
											  FILE_MAP_WRITE,					// We only want to write
											  0,								// File offset (high)
											  0,								// File offset (low)
											  size );							// The size of the buffer to map for writing
				RED_ASSERT( m_fileBuffer, TXT( "Failed to map view of file : %d"), GetLastError() );

				if( m_fileBuffer != nullptr )
				{
					m_writeHead = m_fileBuffer;
					m_bufferSize = size;
				}
			}

			//////////////////////////////////////////////////////////////////////
			// CloseMemoryMappedFile
			//	...
			void CrashReportDataBuffer::CloseMemoryMappedFile()
			{
				if( m_fileBuffer != NULL )
				{
					UnmapViewOfFile( m_fileBuffer );
					m_fileBuffer = nullptr;
				}

				if( m_mappedFileHandle != INVALID_HANDLE_VALUE )
				{
					CloseHandle( m_mappedFileHandle );
					m_mappedFileHandle = INVALID_HANDLE_VALUE;
				}
			}

			//////////////////////////////////////////////////////////////////////
			// ReadBufferContents
			//	...
			void* CrashReportDataBuffer::ReadBufferContents( MemSize& bufferSize )
			{
				MemSize contentsWritten = reinterpret_cast< MemUint >( m_writeHead ) - reinterpret_cast< MemUint >( m_fileBuffer );
				bufferSize = contentsWritten;

				if( contentsWritten > 0 )
				{
					return m_fileBuffer;
				}

				return nullptr;
			}
		}
	}
}

#endif // RED_PLATFORM_WIN32 || RED_PLATFORM_WIN64