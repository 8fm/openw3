/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CObject;

#include "hashmap.h"
#include "object.h"

// Global list of objects - SLOW - try not to use
// Especially do not use the GetAllObjects() stuff - try to use VisitAllObjects
class CObjectsMap
{
public:
	CObjectsMap();
	RED_MOCKABLE ~CObjectsMap();
	
	//! Allocate an index and generation for object
	RED_MOCKABLE void AllocateObject( CObject* object, Uint32& outIndex );

	//! Deallocate object
	RED_MOCKABLE void DeallocateObject( CObject* object, Uint32& outIndex );
	void DeallocateObjectNoLock( CObject* object, Uint32& outIndex ); // GC ONLY

	void Acquire();
	void Release();

public:
	static const Uint32 DEFAULT_INCLUSION_FLAGS = ~0U;
	static const Uint32 DEFAULT_EXCLUSION_FLAGS = OF_Discarded | OF_DefaultObject;

	RED_INLINE const Uint32 GetNumLiveObjects() const
	{
		return m_numLiveObjects;
	}

	RED_INLINE const Uint32 GetMaxObjectIndex() const
	{
		return m_allObjects.Size();
	}

	template< class T >
	RED_INLINE void GetAllObjects( TDynArray< T* >& allObjects, const Uint32 includedFlags = DEFAULT_INCLUSION_FLAGS, const Uint32 excludedFlags = DEFAULT_EXCLUSION_FLAGS ) const
	{
		GetAllObjects( ClassID< T >(), ( TDynArray< CObject* >& ) allObjects, includedFlags, excludedFlags );
	}

	template< class T, typename Visitor >
	RED_INLINE void VisitAllObjects( const Visitor& visitor, const Uint32 includedFlags = DEFAULT_INCLUSION_FLAGS, const Uint32 excludedFlags = DEFAULT_EXCLUSION_FLAGS ) const
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
					if ( object->IsA<T>() )
					{
						if ( !visitor( object, i ) )
							break;
					}
				}
			}
		}
	}

	template< typename Visitor >
	RED_INLINE void VisitAllObjectsNoFilter( const Visitor& visitor ) const
	{
		Red::Threads::CScopedLock< Mutex > lock( m_objectListMutex );

		const Uint32 topObjectIndex = m_allObjects.Size();
		for ( Uint32 i=0; i<topObjectIndex; ++i )
		{
			CObject* object = m_allObjects[i];
			if ( object )
			{
				visitor( object, i );
			}
		}
	}

	// generaic and SAFE object map visitor helper class
	class ObjectVisitor : public Red::NonCopyable
	{
	public:
		ObjectVisitor( CObjectsMap* objectMap );
		~ObjectVisitor();

	protected:
		CObjectsMap*	m_objectMap;
	};

	// helper class that allows SAFE discard of objects
	class ObjectDiscarder : public ObjectVisitor
	{
	public:
		RED_INLINE void operator()( const Uint32 objectIndex ) const
		{
			CObject* object = m_objectMap->m_allObjects[ objectIndex ];
			if ( object ) // CAN HAPPEN
			{
				object->Discard();
			}
		}

		RED_INLINE ObjectDiscarder( CObjectsMap* objectMap )
			: ObjectVisitor( objectMap )
		{}

		RED_INLINE ~ObjectDiscarder()
		{}
	};

	// helper class that allows SAFE iteration over object map indices
	class ObjectIndexer : public ObjectVisitor
	{
	public:
		RED_INLINE CObject* GetObject( const Uint32 objectIndex )
		{
			if ( objectIndex < m_objectMap->m_allObjects.Size() )
				return m_objectMap->m_allObjects[ objectIndex ];

			return nullptr;
		}

		RED_INLINE ObjectIndexer( CObjectsMap* objectMap )
			: ObjectVisitor( objectMap )
		{}

		RED_INLINE ~ObjectIndexer()
		{}
	};


private:
	friend class BaseObjectIterator;
	friend class CGarbageCollector;

	typedef TDynArray< CObject*, MC_ObjectMap >		TObjectMap;
	typedef TDynArray< Uint32, MC_ObjectMap >		TFreeIndices;

	typedef Red::Threads::CMutex					Mutex;
	typedef Red::Threads::CAtomic< Int32 >			AtomicInt;

	TObjectMap				m_allObjects;			// List of all objects in the engine
	TFreeIndices			m_freeIndices;			// List of free object indices
	Uint32					m_numLiveObjects;		// Alive objects

	mutable Mutex			m_objectListMutex;		// Access mutex for object & discard list
	AtomicInt				m_objectLockCount;		// Lock count for object & discard list

	// internal lock/unlock (used by the object iterator)
	void LockForIteration();
	void UnlockForIteration();

	// iteration helper, returns false if END was reached
	Bool Iterate( Int32& currentIndex, CObject*& currentObject, const CClass* classFilter, const Uint32 includedFlags, const Uint32 excludedFlags ) const;

	// internal function to get all object
	void GetAllObjects( CClass* objectClass, TDynArray< CObject* >& allObjects, const Uint32 includedFlags /*= DEFAULT_INCLUSION_FLAGS*/, const Uint32 excludedFlags /*= DEFAULT_EXCLUSION_FLAGS*/ ) const;
};

// Singleton type access to CObjectMap
// Initialized in the CBaseEngine
extern CObjectsMap* GObjectsMap;
