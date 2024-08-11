/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

namespace Red
{
	namespace Core
	{
		//////////////////////////////////////////////////////////////////////////
		// Allocate returns a new entry with offsets into the local pool
		RED_INLINE CResourceDatabasePoolEntry CResourceDatabasePool::Allocate( Red::System::Uint32 dataSize, Red::System::MemSize alignment )
		{
			Red::System::MemSize adjustedSize = dataSize + alignment - 1;
			Red::System::MemSize bytesRemaining = m_tail - m_head;
			if( bytesRemaining < adjustedSize )
			{
				ResizeBuffer( m_tail - m_buffer + adjustedSize );
			}

			Red::System::MemUint dataPtr = reinterpret_cast< Red::System::MemUint >( m_head );
			dataPtr = (dataPtr + alignment - 1) & ~(alignment - 1);

			Red::System::MemUint dataOffset = dataPtr - reinterpret_cast< Red::System::MemUint >( m_buffer );
			RED_ASSERT( dataPtr >= reinterpret_cast< Red::System::MemUint >( m_head ), TXT( "Bad alignment!" ) );
			m_head = reinterpret_cast< Red::System::Uint8* >( dataPtr ) + dataSize;
			RED_ASSERT( m_tail >= m_head, TXT( "Buffer overrun in database pool" ) );

			return CResourceDatabasePoolEntry( static_cast< Red::System::Uint32 >( dataOffset ), dataSize );
		}


		//////////////////////////////////////////////////////////////////////////
		// Get a pointer to the data for an entry
		RED_INLINE const void* CResourceDatabasePool::GetEntryData( const CResourceDatabasePoolEntry& theEntry ) const
		{
			if( theEntry.GetSize() > 0 )
			{
				Red::System::MemUint dataAddress = reinterpret_cast< Red::System::MemUint >( m_buffer + theEntry.GetOffset() );
				ASSERT(dataAddress + theEntry.GetSize() <= reinterpret_cast< Red::System::MemUint >( m_tail ), TXT( "Entry data is too big for this pool! Bad entry?" ) );
				return reinterpret_cast< void* >( dataAddress );
			}
			else
			{
				return nullptr;
			}
		}

		RED_INLINE void* CResourceDatabasePool::GetEntryData( const CResourceDatabasePoolEntry& theEntry )
		{
			return const_cast< void* >( static_cast< const CResourceDatabasePool* >( this )->GetEntryData( theEntry ) );
		}
	}
}
