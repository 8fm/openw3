/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#ifndef _RED_RESOURCE_DB_RECORD_H_
#define _RED_RESOURCE_DB_RECORD_H_

#include "resourcedatabasepoolentry.h"
#include "staticarray.h"

namespace Red
{
	namespace Core
	{
		// This represents a single record in the database. Note that this can represent any object in the graph,
		// not just a resource (CObjects, normal classes, etc)
		class CResourceDatabaseRecord
		{
		public:
			CResourceDatabaseRecord();
			~CResourceDatabaseRecord();

			////////////////////////////////////////////////////////////////////////////
			// Required data
			System::Uint32 GetTypeHash() const;
			void SetTypeHash( System::Uint32 typeHash );
			
			////////////////////////////////////////////////////////////////////////////
			// Optional data - keep this in pooled entries or as hashes. This data can be stripped when required
			enum EEntryType
			{
				ET_Name = 0,	// A name for the resource (mainly used for debugging)
				ET_Path,		// Path to the source file of the resource
				ET_GUID,		// GUID for the resource (mainly used for CNode-derived stuff) 
				ET_Id,			// Resource Id (Hash) for the resource

				ET_Max
			};

			void SetEntry( EEntryType type, const CResourceDatabasePoolEntry& entry );
			const CResourceDatabasePoolEntry& GetEntry( EEntryType type ) const;

		private:
			//////////////////////////////////////////////////////////////////////
			// Required data - this should be set for all records
			System::Uint32 m_resourceTypeHash;						// Hash of the resource type string

			//////////////////////////////////////////////////////////////////////
			// Optional data - this can be stripped when not needed
			TStaticArray< CResourceDatabasePoolEntry, ET_Max > m_entries;
		};
	}
}

#include "resourcedatabaserecord.inl"

#endif
