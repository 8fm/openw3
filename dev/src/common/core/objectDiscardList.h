/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CObject;

#include "hashmap.h"

// Global list of objects - DEPRECATED AND UNSAFE - try not to use
// Especially do not use the GetAllObjects() stuff
class CObjectsDiscardList
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_ObjectMap );

public:
	CObjectsDiscardList();
	~CObjectsDiscardList();

	//! Get maximum number of object we can discard in one go
	RED_INLINE const Uint32 GetCapacity() const { return m_maxObjects; }

	//! Add object to the discard list, thread safe
	//! NOTE: objects beyond our max object count will NOT be discaded (memory leak)
	void Add( CObject* object );
	void AddNoLock( CObject * object ); // GC ONLY

	//! Process pending discards, should be called once per frame from CBaseEngine::Tick
	//! If you fancy you may call it manually
	//! It's also automatically called after garbage collection
	void ProcessList( const Bool forceDiscardNow = false );

	void Acquire();
	void Release();

private:
	typedef Red::Threads::AtomicOps::TAtomic32 AtomicInt;
	typedef Red::Threads::CMutex Mutex;

	class List
	{
		DECLARE_CLASS_MEMORY_ALLOCATOR( MC_ObjectMap );

	public:
		CObject**		m_ptr;
		Int32			m_count;	// ctremblay No need to be atomic, it's protected by lock already ....

		List( const Uint32 maxCount );
		~List();
	};

	List*			m_mainList;	// List of objects to discard
	List*			m_tempList;	// Temporary list

	Mutex			m_lock; // TODO: can be removed 

	Uint32			m_maxObjects;		// Maximum number of object in the LIST
	AtomicInt		m_pendingRequest;	// Pending request to discard the object

	void InternalProcessDiscardList();
};

// Objects discard list singleton
// Initialized in the CBaseEngine
extern CObjectsDiscardList* GObjectsDiscardList;
