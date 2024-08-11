/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#ifndef _RED_RESOURCE_DB_POOL_H_
#define _RED_RESOURCE_DB_POOL_H_

#include "resourcedatabasepoolentry.h"

namespace Red
{
	namespace Core
	{
		// This handles the allocation of database record entry memory
		class CResourceDatabasePool
		{
		public:
			CResourceDatabasePool();
			~CResourceDatabasePool();

			// Allocate returns a new entry with offsets into the local pool
			CResourceDatabasePoolEntry Allocate( Red::System::Uint32 dataSize, Red::System::MemSize alignment = 16 );

			// Get a pointer to the data for an entry
			const void* GetEntryData( const CResourceDatabasePoolEntry& theEntry ) const;
			void* GetEntryData( const CResourceDatabasePoolEntry& theEntry );

			// Serialisation
			void Serialise( IFile& theFile );

		private:

			// Reallocate the internal buffer
			void ResizeBuffer( Red::System::MemSize size );

			Red::System::Uint8* m_buffer;		// The entire buffer
			Red::System::Uint8* m_head;			// The write-head
			Red::System::Uint8* m_tail;			// The end of the buffer
		};
	}
}

#include "resourcedatabasepool.inl"

#endif