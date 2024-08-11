/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

/// Connection between sockets in graph
class CGraphConnection : public ISerializable
{
	DECLARE_RTTI_SIMPLE_CLASS_WITH_POOL( CGraphConnection, MemoryPool_SmallObjects, MC_Graph );

protected:
	CGraphSocket*		m_source;			//!< Connection source
	CGraphSocket*		m_destination;		//!< Connection destination
	Bool				m_inactive;			//!< Is connection active

public:
	// Is connection valid ?
	RED_INLINE Bool IsValid() const { return m_source!=NULL && m_destination!=NULL; }

	// Is connection active ?
	RED_INLINE Bool IsActive() const { return !m_inactive; }

	// Set active state
	RED_INLINE void SetActive( Bool active ) { m_inactive = !active; }

	// Get source
	RED_INLINE CGraphSocket* GetSource( Bool includeHidden = false ) const { return ( !m_inactive || includeHidden ) ? m_source : nullptr; }

	// Get destination
	RED_INLINE CGraphSocket* GetDestination( Bool includeHidden = false ) const { return ( !m_inactive || includeHidden ) ? m_destination : nullptr; }

public:
	CGraphConnection();
	CGraphConnection( CGraphSocket* source, CGraphSocket* destination );

	//! Ignore during GC to save time (temporary flag until GC is removed)
	virtual const Bool CanIgnoreInGC() const { return true; }
};

BEGIN_CLASS_RTTI( CGraphConnection );
	PARENT_CLASS( ISerializable );
	PROPERTY( m_source );
	PROPERTY( m_destination );
	PROPERTY( m_inactive );
END_CLASS_RTTI();