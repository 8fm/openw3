/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "uniPointer.h"

class CClass;
class ISerializable;
class CObject;

/// Generic typed pointer to object
class CPointer
{
protected:
	//! Pointer to object
	TUniPointer< void >	m_data;

	//! Object class
	CClass*				m_class;

public:
	//! Get actual pointed object
	RED_INLINE void* GetPointer() const { return m_data.Get(); }

	//! Get the pointer to stored object pointer
	RED_INLINE void** GetPointerRef() { return m_data.GetRef(); }

	//! Get class of the pointed object (can be fake)
	RED_INLINE CClass* GetClass() const { return m_class; }

	//! Is this a null pointer
	RED_INLINE Bool IsNull() const { return m_data.Get() == NULL; }

public:
	RED_INLINE CPointer();
	RED_INLINE CPointer( void* pointer, CClass* theClass );
	CPointer( ISerializable* object );
	RED_INLINE CPointer( const CPointer& pointer );
	RED_INLINE ~CPointer();

	//! General assignment operator
	RED_INLINE CPointer& operator=( const CPointer& other );
	 
	//! Compare for being equal
	RED_INLINE Bool operator==( const CPointer& other ) const;

	//! Compare for not being equal
	RED_INLINE Bool operator!=( const CPointer& other ) const;

	//! Is this a pointer to an IObject ?
	Bool IsObject() const;

	//! Is this a pointer to an ISerializable object ?
	Bool IsSerializable() const;

	//! Get class of object pointed by the pointer (not the pointer class, works only on ISerializable and above)
	CClass* GetRuntimeClass() const;

	//! Get direct pointer to held IObject, returs NULL if it's not an IObject pointer
	CObject* GetObjectPtr() const;

	//! Get a pointer to held ISerializable, returns NULL if it's not a ISerializable pointer
	ISerializable* GetSerializablePtr() const;

public:
	//! The null pointer
	static CPointer& Null();

protected:
	void initialize( void* ptr, CClass* ptrClass );
	void release();
};

#include "pointer.inl"