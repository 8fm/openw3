/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

namespace Red
{
	namespace Core
	{
		//////////////////////////////////////////////////////////////////////////
		template< typename StoredType, typename IdentifierType >
		RED_INLINE TSharedResourceData< StoredType, IdentifierType >::TSharedResourceData()
		{
		}

		//////////////////////////////////////////////////////////////////////////
		template< typename StoredType, typename IdentifierType >
		RED_INLINE TSharedResourceData< StoredType, IdentifierType >::~TSharedResourceData()
		{
		}

		template< typename StoredType, typename IdentifierType >
		template< typename RawStoredType >
		RED_INLINE CResourceDatabasePoolEntry TSharedResourceData< StoredType, IdentifierType >::AddInternal( IdentifierType identifier, RawStoredType item )
		{
			// See if this entry already exists
			CResourceDatabasePoolEntry returnValue = FindEntry( identifier );

			if( returnValue.GetSize() == 0 )
			{
				// Item doesn't exist in the database, add a new entry
				returnValue = m_dataPool.Add( item );
				m_dataLookup.Insert( identifier, returnValue );
			}

			return returnValue;
		}

		//////////////////////////////////////////////////////////////////////////
		template< typename StoredType, typename IdentifierType >
		RED_INLINE CResourceDatabasePoolEntry TSharedResourceData< StoredType, IdentifierType >::Add( IdentifierType identifier, const StoredType& item )
		{
			return AddInternal( identifier, item );
		}

		//////////////////////////////////////////////////////////////////////////
		template< typename StoredType, typename IdentifierType >
		RED_INLINE CResourceDatabasePoolEntry TSharedResourceData< StoredType, IdentifierType >::Add( IdentifierType identifier, const StoredType* item )
		{
			return AddInternal( identifier, item );
		}

		//////////////////////////////////////////////////////////////////////////
		template< typename StoredType, typename IdentifierType >
		RED_INLINE const StoredType* TSharedResourceData< StoredType, IdentifierType >::Get( IdentifierType identifier ) const
		{
			CResourceDatabasePoolEntry entry = FindEntry( identifier );

			return Get( entry ); 
		}

		//////////////////////////////////////////////////////////////////////////
		template< typename StoredType, typename IdentifierType >
		RED_INLINE const StoredType* TSharedResourceData< StoredType, IdentifierType >::Get( CResourceDatabasePoolEntry entry ) const
		{
			return m_dataPool.Get( entry ); 
		}

		//////////////////////////////////////////////////////////////////////////
		// Find a raw entry by identifier
		template< typename StoredType, typename IdentifierType >
		RED_INLINE CResourceDatabasePoolEntry TSharedResourceData< StoredType, IdentifierType >::FindEntry( IdentifierType identifier ) const
		{
			CResourceDatabasePoolEntry result;
			auto it = m_dataLookup.Find( identifier );
			if( it != m_dataLookup.End() )
			{
				result = it.Value();
			}
			return result;
		}

		//////////////////////////////////////////////////////////////////////////
		template< typename StoredType, typename IdentifierType >
		RED_INLINE Red::System::Bool TSharedResourceData< StoredType, IdentifierType >::DataExists( IdentifierType identifier ) const
		{
			return FindEntry( identifier ).GetSize() != 0;
		}

		//////////////////////////////////////////////////////////////////////////
		template< typename StoredType, typename IdentifierType >
		void TSharedResourceData< StoredType, IdentifierType >::Serialise( IFile& theFile )
		{
			m_dataPool.Serialise( theFile );
			theFile << m_dataLookup;
		}
	}
}
