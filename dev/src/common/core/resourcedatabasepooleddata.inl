
#include "resourceid.h"

namespace Red
{
	namespace Core
	{
		//////////////////////////////////////////////////////////////////////////
		template< typename StoredType >
		RED_INLINE CResourceDatabasePoolEntry CPooledResourceData< StoredType >::Add( const AnsiChar* buffer )
		{
			RED_ASSERT( buffer != nullptr, TXT( "Trying to store an invalid string in CPooledResourceData" ) );

			System::Uint32 dataSize = static_cast< System::Uint32 >( ( System::StringLength( buffer ) + 1 ) * sizeof( AnsiChar ) );

			// Allocate memory for the whole string including null terminator
			CResourceDatabasePoolEntry resultEntry = m_dataPool.Allocate( dataSize, sizeof( System::MemSize ) );

			// Also copies null terminator
			System::MemoryCopy( m_dataPool.GetEntryData( resultEntry ), buffer, dataSize );
			return resultEntry;
		}

		//////////////////////////////////////////////////////////////////////////
		template< typename StoredType >
		RED_INLINE CResourceDatabasePoolEntry CPooledResourceData< StoredType >::Add( const StoredType& item )
		{
			CResourceDatabasePoolEntry entry = m_dataPool.Allocate( sizeof( StoredType ) );
			System::MemoryCopy( m_dataPool.GetEntryData( entry ), &item, sizeof( StoredType ) );
			return entry;
		}

		//////////////////////////////////////////////////////////////////////////
		template<>
		RED_INLINE const AnsiChar* CPooledResourceData< AnsiChar >::Get( const CResourceDatabasePoolEntry& entry ) const
		{
			RED_ASSERT( entry.GetSize() != 0, TXT( "Size of AnsiChar entry in CPooledResourceData is invalid." ) );

			return static_cast< const AnsiChar* >( m_dataPool.GetEntryData( entry ) );
		}

		//////////////////////////////////////////////////////////////////////////
		template< typename StoredType >
		RED_INLINE const StoredType* CPooledResourceData< StoredType >::Get( const CResourceDatabasePoolEntry& entry ) const
		{
			if( entry.GetSize() == sizeof( StoredType ) )
			{
				return static_cast< const StoredType* >( m_dataPool.GetEntryData( entry ) );
			}

			RED_ASSERT( entry.GetSize() == 0, TXT( "Size of type in CPooledResourceData is invalid. Expected %u, got %u" ), sizeof( StoredType ), entry.GetSize() );

			return nullptr;
		}

		//////////////////////////////////////////////////////////////////////////
		template< typename StoredType >
		RED_INLINE void CPooledResourceData< StoredType >::Serialise( IFile& theFile )
		{
			m_dataPool.Serialise( theFile );
		}
	} 
}
