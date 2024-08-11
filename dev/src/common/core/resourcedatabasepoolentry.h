/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#ifndef _RED_RESOURCE_DB_POOL_ENTRY_H_
#define _RED_RESOURCE_DB_POOL_ENTRY_H_

#include "file.h"

namespace Red
{
	namespace Core
	{
		// This represents a single entry into a resource pool
		class CResourceDatabasePoolEntry
		{
		public:
			CResourceDatabasePoolEntry();
			CResourceDatabasePoolEntry(Red::System::Uint32 offset, Red::System::Uint32 size);

			Red::System::Uint32 GetOffset() const;
			Red::System::Uint32 GetSize() const;

			friend IFile &operator<<( IFile &file, CResourceDatabasePoolEntry &header )
			{
				file.Serialize( &header, sizeof( header ) );
				return file;
			}

		private:
			Red::System::Uint32 m_dataOffset;	// Offset into the pool memory
			Red::System::Uint32 m_dataSize;		// Data size
		};
	}
}


#include "resourcedatabasepoolentry.inl"

#endif