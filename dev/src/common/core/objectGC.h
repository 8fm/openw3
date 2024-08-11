/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "countedBool.h"
#include "scopedPtr.h"

/// Basic GC worker implementation
class IObjectGCStategy
{
public:
	virtual ~IObjectGCStategy() {};
	virtual void CollectGarbage( const Bool reportLeaks ) = 0;
};

/// Object GC helper
class IObjectGCHelper
{
public:
	virtual ~IObjectGCHelper() {};

	/// GC is about to be performed, flush stuff
	virtual void OnGCStarting() = 0;
	virtual void OnGCFinished() = 0;
};

/// Manger for internal GC - controls GC updates ticking
class CObjectGC
{
public:
	CObjectGC();
	~CObjectGC();

	// Request a GC now, may not be performed if there are loading tasks
	void Collect();

	// Perform full GC right now, will wait for GC blocking tasks to complete
	void CollectNow();

	// Tick GC, collects garbage in the background - if there's a possibility to run of of memory if performs GC
	void Tick();

	// GC flushes helper shit
	void RegisterHelper( IObjectGCHelper* helper );
	void UnregisterHelper( IObjectGCHelper* helper );

	//! Enable/Disable GC (it won't be called - may lead to OOM)
	RED_INLINE void DisableGC() { m_suppress.Set(); }
	RED_INLINE void EnableGC() { m_suppress.Unset(); }

	//! Are we doing GC right now (TODO: remove)
	RED_INLINE const Bool IsDoingGC() const { return m_duringGC; }

	//! Get GC count
	RED_INLINE const Uint32 GetGCCount() const { return m_totalCount; }

private:
	Red::Threads::CSemaphore	m_lock;					// Internal GC lock (not reentrant)
	volatile Bool				m_duringGC;				// We are doing the GC right now (TODO: remove)
	CountedBool					m_suppress;				// Suppress automatic garbage collecting

	Uint32						m_nextAllowedGCFrame;	// Frame index the next GC is allowed
	Uint32						m_frameIndex;			// Current frame index
	Uint32						m_totalCount;			// Number of garbage collects done so far

	Red::TScopedPtr< IObjectGCStategy >	m_strategy;		// Actual GC strategy to execute

	TDynArray< IObjectGCHelper* >	m_helpers;			// GC helpers
	Red::Threads::CMutex			m_helpersLock;		// Lock for GC helpers array

	// perform GC
	void Perform( const Bool force );

	// should the GC be performed ?
	Bool ShouldPerformGC() const;	

	// latch the job system
	Bool LockJobSystem( const Bool force );
};

/// GC interface, created in CBasEengine
extern CObjectGC* GObjectGC;

