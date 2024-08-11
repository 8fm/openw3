/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "object.h"
#include "objectRootSet.h"

//////////////////////////////////////////////////////////////////////////

CObjectsRootSet* GObjectsRootSet = NULL;

//////////////////////////////////////////////////////////////////////////

CObjectsRootSet::CObjectsRootSet()
{
}

CObjectsRootSet::~CObjectsRootSet()
{
}

const Uint32 CObjectsRootSet::GetCount( CObject* object ) const
{
	Red::Threads::CScopedLock< Mutex > lock( m_lock );

	Uint32 rootRefCount = 0;
	m_roots.Find( object, rootRefCount );
	return rootRefCount;
}

void CObjectsRootSet::Add( CObject* object )
{
	Red::Threads::CScopedLock< Mutex > lock( m_lock );

	RED_FATAL_ASSERT( object, "Trying to add NULL object to the root set" );
	RED_FATAL_ASSERT( !object->HasFlag( OF_Discarded ), "Trying to add discarded object to the root set" );
	RED_FATAL_ASSERT( !object->HasFlag( OF_Finalized ), "Trying to add finalized object to the root set" );

	// Insert to root set or update root set reference count
	Uint32 rootRefCount = 0;
	if ( m_roots.Find( object, rootRefCount ) )
	{
		RED_FATAL_ASSERT( object->HasFlag( OF_Root ), "Root set map corruption" );

		m_roots.Set( object, rootRefCount + 1 );
	}
	else
	{
		RED_FATAL_ASSERT( !object->HasFlag( OF_Root ), "Root set map corruption" );
		object->SetFlag( OF_Root );

		m_roots.Insert( object, 1 );
	}
}

void CObjectsRootSet::Remove( CObject* object )
{
	Red::Threads::CScopedLock< Mutex > lock( m_lock );

	// Insert to root set or update root set reference count
	Uint32 rootRefCount = 0;
	if ( m_roots.Find( object, rootRefCount ) )
	{
		RED_FATAL_ASSERT( object->HasFlag( OF_Root ), "Root set map corruption" );

		if ( rootRefCount > 1 )
		{
			m_roots.Set( object, rootRefCount - 1 );
		}
		else
		{
			RED_FATAL_ASSERT( rootRefCount == 1, "Root set map corruption" );
			object->ClearFlag( OF_Root );
			m_roots.Erase( object );
		}
	}
	else
	{
		RED_HALT(  "Object not in root set" );
	}
}

//////////////////////////////////////////////////////////////////////////
