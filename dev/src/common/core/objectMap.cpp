/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "object.h"
#include "objectMap.h"

//----

CObjectsMap* GObjectsMap = NULL;

//----

CObjectsMap::ObjectVisitor::ObjectVisitor( CObjectsMap* objectMap )
	: m_objectMap( objectMap )
{
	m_objectMap->LockForIteration();
}

CObjectsMap::ObjectVisitor::~ObjectVisitor()
{
	m_objectMap->UnlockForIteration();
}

//----

CObjectsMap::CObjectsMap()
	: m_numLiveObjects( 0 )
	, m_objectLockCount( 0 )
{
	// Reserve some memory
	m_allObjects.Reserve( 1000 * 1000 );
	m_freeIndices.Reserve( 500 * 1024 );

	// Object 0 is always NULL
	m_allObjects.PushBack( NULL );
}

CObjectsMap::~CObjectsMap()
{
	if( m_numLiveObjects > 0 )
	{
		RED_LOG_ERROR( ObjectsMap, TXT( "Not all CObjects have been free'd! There are still %u live objects currently in the system" ), m_numLiveObjects );
	}
}

void CObjectsMap::LockForIteration()
{
	m_objectListMutex.Acquire();
}

void CObjectsMap::UnlockForIteration()
{
	m_objectListMutex.Release();
}

Bool CObjectsMap::Iterate( Int32& currentIndex, CObject*& currentObject, const CClass* classFilter, const Uint32 includedFlags, const Uint32 excludedFlags ) const
{
	const Int32 numObjects = m_allObjects.Size();
	while ( ++currentIndex < numObjects )
	{
		CObject* object = m_allObjects[ currentIndex ];
		if ( object && ( !classFilter || object->IsA( classFilter ) ) )
		{
			const Uint32 objectFlags = object->GetFlags();
			if ( (!objectFlags || (objectFlags & includedFlags)) && !(objectFlags & excludedFlags) )
			{
				currentObject = object;
				return true;
			}
		}
	}

	// end of list
	return false;
}

void CObjectsMap::GetAllObjects( CClass* objectClass, TDynArray< CObject* >& allObjects, const Uint32 includedFlags /*= DEFAULT_INCLUSION_FLAGS*/, const Uint32 excludedFlags /*= DEFAULT_EXCLUSION_FLAGS*/ ) const
{
	Red::Threads::CScopedLock< Mutex > lock( m_objectListMutex );

	const Uint32 topObjectIndex = m_allObjects.Size();
	for ( Uint32 i=0; i<topObjectIndex; ++i )
	{
		CObject* object = m_allObjects[i];
		if ( object )
		{
			const Uint32 objectFlags = object->GetFlags();

			if ( (!objectFlags || (objectFlags & includedFlags)) && ((objectFlags & excludedFlags) == 0) )
			{
				if (!objectClass || object->IsA(objectClass) )
				{
					allObjects.PushBack( object );
				}
			}
		}
	}
}

void CObjectsMap::AllocateObject( CObject* object, Uint32& outIndex )
{
	Red::Threads::CScopedLock< Mutex > lock( m_objectListMutex );

	// check object map invariant
	RED_FATAL_ASSERT( m_numLiveObjects + m_freeIndices.Size() == (m_allObjects.Size()-1), "Object map corruption" );

	// allocation logic:
	//  - allocate new indices if we have free space in the array
	//  - reuse old indices to prevent resizing of m_allObjects
	//  - as a last resort resize the array

	if ( m_allObjects.Size() < m_allObjects.Capacity() )
	{
		outIndex = m_allObjects.Size();
		m_allObjects.PushBack( object );
	}
	else if ( !m_freeIndices.Empty() )
	{
		outIndex = m_freeIndices.PopBackFast();

		ASSERT( !m_allObjects[ outIndex ] );
		m_allObjects[ outIndex ] = object;
	}
	else // allocate totally new index
	{
		outIndex = m_allObjects.Size();
		m_allObjects.PushBack( object );
	}

	// count alive objects
	m_numLiveObjects += 1;
	RED_FATAL_ASSERT( m_numLiveObjects + m_freeIndices.Size() == (m_allObjects.Size()-1), "Object map corruption" );
}

void CObjectsMap::DeallocateObject( CObject* object, Uint32& outIndex )
{
	Red::Threads::CScopedLock< Mutex > lock( m_objectListMutex );
	DeallocateObjectNoLock( object, outIndex );
}

void CObjectsMap::DeallocateObjectNoLock( CObject* object, Uint32& outIndex )
{
	// count alive objects
	RED_FATAL_ASSERT( m_numLiveObjects > 0, "Object map corruption" );
	RED_FATAL_ASSERT( m_numLiveObjects + m_freeIndices.Size() == (m_allObjects.Size()-1), "Object map corruption" );
	m_numLiveObjects -= 1;

	RED_ASSERT( object );
	RED_ASSERT( outIndex < m_allObjects.Size() );

	// Make sure we are deleting valid object
	RED_ASSERT( object->GetObjectIndex() == outIndex );

	// Reset object
	RED_ASSERT( m_allObjects[ outIndex ] == object );
	m_allObjects[ outIndex ] = NULL;
	RED_UNUSED( object );

	// Push the index back to the pool
	m_freeIndices.PushBack( outIndex );


	// Reset object
	outIndex = 0;
}

void CObjectsMap::Acquire()
{
	m_objectListMutex.Acquire();
}

void CObjectsMap::Release()
{
	m_objectListMutex.Release();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

