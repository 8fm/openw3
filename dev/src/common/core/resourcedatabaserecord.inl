/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

namespace Red
{
	namespace Core
	{
		//////////////////////////////////////////////////////////////////////////
		RED_INLINE CResourceDatabaseRecord::CResourceDatabaseRecord()
			: m_resourceTypeHash(0)
		{
			m_entries.Resize( ET_Max );
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE CResourceDatabaseRecord::~CResourceDatabaseRecord()
		{
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void CResourceDatabaseRecord::SetTypeHash( System::Uint32 typeHash )
		{
			m_resourceTypeHash = typeHash;
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE System::Uint32 CResourceDatabaseRecord::GetTypeHash() const
		{
			return m_resourceTypeHash;
		}

		RED_INLINE void CResourceDatabaseRecord::SetEntry( EEntryType type, const CResourceDatabasePoolEntry& entry )
		{
			RED_ASSERT( type < ET_Max, TXT( "Invalid resource database pool entry type specified: %i" ), type );

			RED_ASSERT( m_entries[ type ].GetSize() == 0, TXT( "You *REALLY* should not overwrite pooled data in a record!" ) );
			RED_ASSERT( entry.GetSize() > 0, TXT( "Invalid Parameter: Missing entry data" ) );

			m_entries[ type ] = entry;
		}

		RED_INLINE const CResourceDatabasePoolEntry& CResourceDatabaseRecord::GetEntry( EEntryType type ) const
		{
			RED_ASSERT( type < ET_Max, TXT( "Invalid resource database pool entry type specified: %i" ), type );

			return m_entries[ type ];
		}
	} 
}
