/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "classBuilder.h"

//////////////////////////////////////////////////////////////////////////

class IReferencable;

/// Referencable object flags
enum EHandleFlags
{
	// IReferencable is using reference counting, when the reference count gets to 0 the OnAllHandlesReleased is called
	HF_ObjectReferenceCounting	= FLAG( 1 ),

	// IReferencable object is protected - no handles to it can be created, this usually means that the object is pending deletion
	HF_ObjectProtected			= FLAG( 2 ),

	// Discard/delete object automatically
	HF_AutoDiscard				= FLAG( 3 ),
};

/// Internal handle data
class ReferencableInternalHandle
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_SmallObjects, MC_Handle );

	friend class IReferencable; // HACK!

public:
	ReferencableInternalHandle( IReferencable* object );
	~ReferencableInternalHandle();

	//! Get the object
	RED_INLINE IReferencable* GetPtr() const { return m_object; }

	//! Are the references tracked for the object ?
	RED_INLINE const Bool IsRefCounted() const { return 0 != (m_flags & HF_ObjectReferenceCounting); }

	//! Is this object protected ?
	RED_INLINE const Bool IsProtected() const { return 0 != (m_flags & HF_ObjectProtected); }	

	//! Unbind from object
	void Unbind();

	//! Add the reference for the handle object (weak refs)
	void AddWeakRef();

	//! Release the reference for the handle object (weak refs)
	void ReleaseWeakRef();

	//! Add the strong reference
	void AddRef();

	//! Release the strong reg
	void Release();

	//! Set handle flags
	void SetFlags( const Uint32 flagsToSet );

	//! Clear handle flags
	void ClearFlags( const Uint32 flagsToClear );

private:
	// number of references for the handle data itself
	Red::Threads::CAtomic< Int32 >	m_weakRefCount; // keeps this object alive

	// number of the strong references to the handled object
	Red::Threads::CAtomic< Int32 >	m_strongRefCount; // keeps the pointed object alive

	// flags - describe how to handle different situations
	volatile Uint32					m_flags;

	// pointer to the actual object, can be NULL if handle got discarded (invalidated)
	IReferencable*					m_object;
	
	// object original class
	const CClass*					m_class;
};

//////////////////////////////////////////////////////////////////////////

/// Referencable is an object that can have a THandle reference created
/// THandle represents a reference to the object and can be either a weak reference or strog (refcounted) one
/// In all cases when the object is deleted or then the DiscardHandles() is called all the handles become invalid
class IReferencable
{
	DECLARE_RTTI_SIMPLE_CLASS_WITH_POOL( IReferencable, MemoryPool_Default, MC_Referencable );

	friend class BaseSafeHandle;

public:
	IReferencable();
	IReferencable(const IReferencable&);
	virtual ~IReferencable();
	IReferencable& operator=(const IReferencable&);

	//! Is the handle reference counted
	const Bool IsHandleReferenceCounted() const;

	//! Is the handle protected ?
	const Bool IsHandleProtected() const;

	//! Discard and handles to this object now
	void DiscardHandles();

	//! Enable full reference counting on the object, once the last 
	//! reference to the object is lost the OnAllHandlesReleased will be called.
	//! Warning - it's illegal to call DiscardHandles on an object with enabled reference counting
	//! Warning - once enabled cannot be disabled
	void EnableReferenceCounting( const Bool enable = true );

	//! Enable/Disable auto discard
	void EnableAutomaticDiscard( const Bool enable = true );

	//! Enable handle protection - no new THandles will be allowed for this object
	//! This usually means that the object is going to be deleted soon
	void EnableHandleProtection();

	//! Disable handle protection - new THandles can be created
	//! This usually means that the object is safe to be used again
	void DisableHandleProtection();

	//! Called when all THandles to this IReferencable are gone
	virtual void OnAllHandlesReleased();

	//! Validate handle creation for this object - NOT USED IN FINAL
	virtual Bool OnValidateHandleCreation() const;

private:
	//! Internal handle to the object - Initially created in the constructor of IReferencable
	//! Zeroed and Released in the destructor. May live longer if the are external references (active handles).	
	ReferencableInternalHandle*		m_internalHandle;
};

BEGIN_ABSTRACT_CLASS_RTTI( IReferencable );
END_CLASS_RTTI();

RED_INLINE void ReferencableInternalHandle::AddWeakRef()
{
	m_weakRefCount.Increment();
}

RED_INLINE void ReferencableInternalHandle::ReleaseWeakRef()
{
	// check if the last handle is being destroyed
	if ( 0 == m_weakRefCount.Decrement() )
	{
		delete this;
	}
}

RED_INLINE void ReferencableInternalHandle::AddRef()
{
	AddWeakRef();

	m_strongRefCount.Increment();
}

RED_FORCE_INLINE void ReferencableInternalHandle::Release()
{
	// call custom callback function
	if ( m_strongRefCount.Decrement() == 0 )
	{
		if ( m_flags & HF_ObjectReferenceCounting && nullptr != m_object )
		{
			m_flags &= ~HF_ObjectReferenceCounting;
			m_object->OnAllHandlesReleased();
		}
	}

	ReleaseWeakRef();
}
