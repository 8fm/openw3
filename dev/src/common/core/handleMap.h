/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "uniPointer.h"
#include "types.h"
#include "typeName.h"
#include "class.h"

/// Base safe handle
class BaseSafeHandle
{
public:
	//! Default constructor
	RED_INLINE BaseSafeHandle()
		: m_handle( NULL )
	{};
	
	//! Init constructor
	BaseSafeHandle( const class IReferencable* object, const CClass* classCheck = nullptr );

	//! Copy constructor
	BaseSafeHandle( const BaseSafeHandle& other, const CClass* classCheck = nullptr );

	//! Move constructor
	BaseSafeHandle( BaseSafeHandle&& other );

	//! Destructor ( needed )
	~BaseSafeHandle();

public:
	//! Copy
	BaseSafeHandle& operator=( const BaseSafeHandle& other );

	//! Move
	BaseSafeHandle& operator=( BaseSafeHandle&& other );

public:
	//! Get the object from the handle
	class IReferencable* Get() const;

	//! Get the object from the handle
	const class IReferencable* GetConst() const;

	//! Set the object
	void Set( const class IReferencable* object, const CClass* classCheck = nullptr );

	//! Clear handle
	void Clear();

	//! Is the handle valid (are we pointing to valid object)
	Bool IsValid() const;

	//! Is the handle lost (was once valid but now it's not)
	Bool IsLost() const;

protected:
	//! Internal handle - the same size on 32 and 64 bit systems
	TUniPointer< class ReferencableInternalHandle >		m_handle;
};

/// Safe handle
template< class T >
class THandle : private BaseSafeHandle
{
public:
	//! Default constructor
	RED_INLINE THandle()
		: BaseSafeHandle()
	{};

	//! Copy constructor
	RED_INLINE THandle( const THandle& other );

	//! Move constructor
	RED_INLINE THandle( THandle&& other );

	//! Initialize from object
	RED_INLINE THandle( const T* object );

public:
	//! Copy
	RED_INLINE THandle& operator=( const THandle& other );

	//! Set new value from pointer
	RED_INLINE THandle& operator=( const T* other );

	//! Move
	RED_INLINE THandle& operator=( THandle&& other )
	{	
		BaseSafeHandle::operator=( std::forward< THandle >(  other ) );
		return *this;
	}

public:
	//! Cast to boolean
	RED_INLINE operator Bool() const
	{
		return IsValid();
	}

	//! Implicit cast to pointer
	RED_INLINE operator T*() const
	{
		return Get();
	}

public:
	//! Get the object from the handle
	RED_INLINE T* Get() const { return (T*)BaseSafeHandle::Get(); }

	//! Get the object from the handle
	RED_INLINE const T* GetConst() const { return (const T*) BaseSafeHandle::Get(); }

	//! Is the handle valid (are we pointing to valid object ?)
	RED_INLINE Bool IsValid() const { return BaseSafeHandle::IsValid(); }

	//! Is the handle lost (was once valid but now it's now)
	RED_INLINE Bool IsLost() const { return BaseSafeHandle::IsLost(); }

	//! Access handle object
	RED_INLINE T* operator->() const
	{
		ASSERT( IsValid(), TXT("Trying to access NULL handle") );
		return Get();
	}

	//! Calculates hash of the handle
	RED_INLINE Uint32 CalcHash() const
	{
		return GetPtrHash( Get() );
	}

public:
	RED_INLINE static const CName& GetTypeName()
	{
		static const CName name( InitName() );
		return name;
	}

	RED_INLINE static const THandle< T >& Null()
	{
		static THandle< T > nullHandle( nullptr );
		return nullHandle;
	}

public:
	// Serialization operator
	RED_INLINE friend IFile& operator<<( IFile& f, THandle< T >& h )
	{
		h.Serialize( f );
		return f;
	}

	// Serialization function
	RED_INLINE void Serialize( IFile& f );

private:
	static CName InitName()
	{
		Char typeName[ RED_NAME_MAX_LENGTH ];

		const Char* innerTypeName = TTypeName<T>::GetTypeName().AsChar();
		Red::System::SNPrintF( typeName, ARRAY_COUNT(typeName), TXT("handle:%ls"), innerTypeName );

		return CName( typeName );
	}
};

//! Equality operator
template< class LeftType, class RightType >
RED_FORCE_INLINE static const Bool operator==( const THandle< LeftType >& lh, const THandle< RightType >& rh )
{
	return lh.Get() == rh.Get();
}

//! Inequality operator
template< class LeftType, class RightType >
RED_FORCE_INLINE static const Bool operator!=( const THandle< LeftType >& lh, const THandle< RightType >& rh )
{
	return lh.Get() != rh.Get();
}

//! Pointer order operator
template< class LeftType, class RightType >
RED_FORCE_INLINE static const Bool operator<( const THandle< LeftType >& lh, const THandle< RightType >& rh )
{
	return (char*)lh.Get() < (char*)rh.Get();
}

//! Equality operator with pointer on the right side
template< class LeftType, class RightType >
RED_FORCE_INLINE static const Bool operator==( const THandle< LeftType >& lh, RightType* rh )
{
	return lh.Get() == rh;
}

//! Inequality operator with pointer on the right side
template< class LeftType, class RightType >
RED_FORCE_INLINE static const Bool operator!=( const THandle< LeftType >& lh, RightType* rh )
{
	return lh.Get() != rh;
}

//! Equality operator with pointer on the left side
template< class LeftType, class RightType >
RED_FORCE_INLINE static const Bool operator==( LeftType* lh, const THandle< RightType>& rh )
{
	return lh == rh.Get();
}

//! Inequality operator with pointer on the left side
template< class LeftType, class RightType >
RED_FORCE_INLINE static const Bool operator!=( LeftType* lh, const THandle< RightType>& rh )
{
	return lh != rh.Get();
}

// Handle cast
template< class _DestType, class _SrcType >
const THandle< _DestType > Cast( const THandle< _SrcType >& srcObj )
{
	if ( !srcObj.IsValid() )
		return NULL;

	CClass *destTypeClass = _DestType::GetStaticClass();
	CClass *srcTypeClass = _SrcType::GetStaticClass();
	CClass *srcClass = srcObj->GetClass();

	_SrcType* srcObjNonConst = const_cast< _SrcType* >( srcObj.Get() );

	void *actualSrcObj = srcClass->CastFrom( srcTypeClass, srcObjNonConst );
	return THandle< _DestType >( static_cast<_DestType*>( srcClass->CastTo( destTypeClass, actualSrcObj ) ) );
}

// Handle safe cast
template< class _DestType, class _SrcType >
THandle< _DestType > SafeCast( const THandle< _SrcType >& srcObj )
{
	ASSERT( srcObj && srcObj->template IsA< _DestType >() );
	return Cast<_DestType>( srcObj );
}

//! Helper for removing empty handles from array
template< class T, enum EMemoryClass memoryClass, RED_CONTAINER_POOL_TYPE memoryPool >
void RemoveEmptyPointers( TDynArray< THandle< T >, memoryClass, memoryPool >& ar )
{
	// Remove all empty elements from array
	const Int32 size = (Int32) ar.Size();
	for ( Int32 i=size-1; i>=0; --i )
	{
		if ( !ar[i] )
		{
			ar.Erase( ar.Begin() + i );
		}
	}

	// Shrink to reduce memory
	ar.Shrink();
}

#ifndef RED_FINAL_BUILD
/// Handle serialization debug helper
class CHandleSerializationMarker
{
public:
	static Bool st_isInHandleSerialization;

private:
	Bool m_previousFlag;

public:
	CHandleSerializationMarker()
	{
		m_previousFlag = st_isInHandleSerialization;
		st_isInHandleSerialization = true;
	}

	~CHandleSerializationMarker()
	{
		st_isInHandleSerialization = m_previousFlag;
	}
};
#endif

// Inlined part of handle
#include "handleMap.h.inl"