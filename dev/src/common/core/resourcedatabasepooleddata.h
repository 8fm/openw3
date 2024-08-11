/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#ifndef _RED_RESOURCE_DATABASE_POOLED_DATA_H_
#define _RED_RESOURCE_DATABASE_POOLED_DATA_H_

#include "resourcedatabasepool.h"

namespace Red
{
	namespace Core
	{
		using System::AnsiChar;

		// This class acts as a wrapper around a data pool, with utility functions to make life a bit easier
		template< typename StoredType >
		class CPooledResourceData
		{
		public:
			CResourceDatabasePoolEntry Add( const AnsiChar* buffer );
			CResourceDatabasePoolEntry Add( const StoredType& item );

			const StoredType* Get( const CResourceDatabasePoolEntry& entry ) const;

			void Serialise( IFile& theFile );

		private:
			CResourceDatabasePool m_dataPool;
		};
	}
}

#include "resourcedatabasepooleddata.inl"

#endif
