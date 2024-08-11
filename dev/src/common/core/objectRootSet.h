/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CObject;

#include "hashmap.h"

/// Root set - used only for GC
class CObjectsRootSet
{
public:
	CObjectsRootSet();
	~CObjectsRootSet();

	// Add object to root set (ref counted internally, thread safe)
	void Add( CObject* object );

	// Remove object from root set (thread safe)
	void Remove( CObject* object );

	// Get number of root set references for given object
	const Uint32 GetCount( CObject* object ) const;

	// Get number of objects in the root set
	RED_INLINE const Uint32 GetSize() const { return m_roots.Size(); }

	// Visit all root set objects (thread safe)
	template< typename Visitor >
	RED_INLINE void VisitRootSet( Visitor& visitor )
	{
		Red::Threads::CScopedLock< Mutex > lock(m_lock);

		for ( auto it = m_roots.Begin(); it != m_roots.End(); ++it )
		{
			RED_ASSERT( it->m_second > 0 );
			if ( !visitor( it->m_first ) )
				break;
		}
	}

	// Visit all root set objects (thread safe)
	template< typename Visitor >
	RED_INLINE void VisitRootSet( const Visitor& visitor )
	{
		Red::Threads::CScopedLock< Mutex > lock(m_lock);

		for ( auto it = m_roots.Begin(); it != m_roots.End(); ++it )
		{
			RED_ASSERT( it->m_second > 0 );
			if ( !visitor( it->m_first ) )
				break;
		}
	}

private:
	typedef THashMap< CObject*, Uint32 >			TRootSet;
	typedef Red::Threads::CMutex					Mutex;

	mutable Mutex			m_lock;			// Access mutex for the root set
	TRootSet				m_roots;		// List of object roots ( reachability sources for GC )
};

// Objects root set
// Initialized in the CBaseEngine
extern CObjectsRootSet* GObjectsRootSet;
