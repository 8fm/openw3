/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#ifndef _RED_RESOURCE_DATABASE_SHARED_DATA_H_
#define _RED_RESOURCE_DATABASE_SHARED_DATA_H_

#include "resourcedatabasepooleddata.h"
#include "hashmap.h"

namespace Red
{
	namespace Core
	{
		// This class is used as a helper to facilitate tracking shared data between records in a single pool
		// Use this when to ensure we don't store duplicated data for records
		
		// Stored type is what this pool contains
		// IdentifierType is used to lookup into the internal hash table
		template< typename StoredType, typename IdentifierType >
		class TSharedResourceData
		{
		public:
			TSharedResourceData();
			~TSharedResourceData();

			// Add Generic data method
			CResourceDatabasePoolEntry Add( IdentifierType identifier, const StoredType& item );
			CResourceDatabasePoolEntry Add( IdentifierType identifier, const StoredType* item );

			// Generic get data method
			const StoredType* Get( IdentifierType identifier ) const;
			const StoredType* Get( CResourceDatabasePoolEntry ) const;

			// Find a raw entry by identifier
			CResourceDatabasePoolEntry FindEntry( IdentifierType identifier ) const;

			// Returns true if a entry exists for the identifier
			Red::System::Bool DataExists( IdentifierType identifier ) const;

			// Serialisation
			void Serialise( IFile& theFile );

		private:
			template< typename RawStoredType >
			CResourceDatabasePoolEntry AddInternal( IdentifierType identifier, RawStoredType item );

		private:
			// Data storage
			CPooledResourceData< StoredType > m_dataPool;

			// Lookup table for this pool
			THashMap< IdentifierType, CResourceDatabasePoolEntry, DefaultHashFunc< IdentifierType >, DefaultEqualFunc< IdentifierType >, MC_ResourceDatabase > m_dataLookup;
		};
	}
}

#include "resourcedatabaseshareddata.inl"

#endif
