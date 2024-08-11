/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "resourcedatabasepool.h"

namespace Red
{
	namespace Core
	{
		//////////////////////////////////////////////////////////////////////////
		CResourceDatabasePool::CResourceDatabasePool()
			: m_buffer( nullptr )
			, m_head( nullptr )
			, m_tail( nullptr )
		{

		}

		//////////////////////////////////////////////////////////////////////////
		CResourceDatabasePool::~CResourceDatabasePool()
		{
			if( m_buffer != nullptr )
			{
				RED_MEMORY_FREE( MemoryPool_Default, MC_ResourceDatabase, m_buffer );
			}

			m_buffer = nullptr;
			m_head = nullptr;
			m_tail = nullptr;
		}

		//////////////////////////////////////////////////////////////////////////
		// Reallocate the internal buffer
		void CResourceDatabasePool::ResizeBuffer( Red::System::MemSize size )
		{
			// We attempt to grow by 1.5 times each time, unless the size is much bigger
			Red::System::MemSize currentSizeBytes = m_tail - m_buffer;
			Red::System::MemSize currentDataSizeBytes = m_head - m_buffer;
			Red::System::MemSize growSize = currentSizeBytes + ( currentSizeBytes / 2 );
			Red::System::MemSize newSize = ( growSize > size ) ? growSize : size;

			m_buffer = (Red::System::Uint8*)RED_MEMORY_REALLOCATE_ALIGNED( MemoryPool_Default, m_buffer, MC_ResourceDatabase, newSize, 16 );
			RED_ASSERT( m_buffer, TXT( "Failed to allocate a database pool. Expect crashes" ) );

			m_head = m_buffer + currentDataSizeBytes;
			m_tail = m_buffer + newSize;
		}

		//////////////////////////////////////////////////////////////////////////
		void CResourceDatabasePool::Serialise( IFile& theFile )
		{
			Red::System::Uint32 sizeInBytes = 0;
			if( theFile.IsReader() )
			{
				theFile << sizeInBytes;
				if( sizeInBytes > 0 )
				{
					ResizeBuffer( sizeInBytes );
				}
			}
			else if( theFile.IsWriter() )
			{
				sizeInBytes = static_cast< Red::System::Uint32 >( m_tail - m_buffer );
				theFile << sizeInBytes;
			}

			theFile.Serialize( m_buffer, sizeInBytes );
		}
	}
}