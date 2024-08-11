/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "renderObjectPtr.h"

/// Render object safe access macros
#define SAFE_COPY( x, y ) { if ( (x) != (y) ) { if ( x ) { (x)->Release(); }; x = y; if ( x ) { (x)->AddRef(); } } }

/// RenderObject operator new / delete overrides
#define DECLARE_RENDER_OBJECT_MEMORYCLASS( memclass )																		\
	public:																													\
		void *operator new( size_t size )	{ return RED_MEMORY_ALLOCATE_ALIGNED( MemoryPool_Default, memclass, size, 16 ); }	\
		void operator delete( void *ptr )	{ RED_MEMORY_FREE( MemoryPool_Default, memclass, ptr ); }

/// RenderObject operator new / delete overrides
#define DECLARE_RENDER_OBJECT_MEMORYPOOL( memPool, memclass )																\
	public:																													\
	void *operator new( size_t size )	{ return RED_MEMORY_ALLOCATE_ALIGNED( memPool, memclass, size, 16 ); }	\
	void operator delete( void *ptr )	{ RED_MEMORY_FREE( memPool, memclass, ptr ); }

/// Render object, ref counted shit
class IRenderObject
{
	DECLARE_RENDER_OBJECT_MEMORYCLASS( MC_RenderObjects )

protected:
	Red::Threads::CAtomic< Int32 >		m_refCount;		//!< Reference count

protected:
	virtual ~IRenderObject();

protected:
	//! Returns true if refcount is zero.
	//  KEEP THIS FUNCTION OUT OF PUBLIC INTERFACE!!!
	Bool IsRefCountZero() const;

	//! Destroys self if reference count is zero.
	//  KEEP THIS FUNCTION OUT OF PUBLIC INTERFACE!!!
	virtual void DestroySelfIfRefCountZero();

public:
	IRenderObject();

	//! Add internal reference
	void AddRef();

	//! Release internal reference
	virtual void Release();

	Int32 GetRefCount() const; // ctremblay HACK do not use. DO NOT USE.  It will be removed shortly. 
};

typedef TRenderObjectPtr< IRenderObject > RenderObjectHandle;

