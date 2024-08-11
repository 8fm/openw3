/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */
#pragma once

/// Render object safe access macros
#define SAFE_COPY( x, y ) { if ( (x) != (y) ) { if ( x ) { (x)->Release(); }; x = y; if ( x ) { (x)->AddRef(); } } }

/// Movement object, ref counted shit
class IMovementObject
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Movement );

private:
	Red::Threads::CAtomic< Int32 >		m_refCount;		//!< Reference count

protected:
	virtual ~IMovementObject();

protected:
	//! Returns true if refcount is zero.
	//  KEEP THIS FUNCTION OUT OF PUBLIC INTERFACE!!!
	Bool IsRefCountZero() const;

	//! Destroys self if reference count is zero.
	//  KEEP THIS FUNCTION OUT OF PUBLIC INTERFACE!!!
	virtual void DestroySelfIfRefCountZero();

public:
	IMovementObject();

	//! Add internal reference
	void AddRef();

	//! Release internal reference
	void Release();
};

