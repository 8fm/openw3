/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "uniPointer.h"
#include "referencable.h"

template< class T >
RED_INLINE THandle<T>::THandle( const THandle<T>& other )
	: BaseSafeHandle( other )
{};

template< class T >
RED_INLINE THandle<T>::THandle( THandle<T>&& other )
	: BaseSafeHandle( std::forward< THandle<T> >( other ) )
{};

template< class T >
RED_INLINE THandle<T>::THandle( const T* object )
	: BaseSafeHandle( (const IReferencable*)object )
{};

template< class T >
RED_INLINE THandle<T>& THandle<T>::operator=( const THandle<T>& other )
{	
	// ctremblay: I have to remove ClassID< T >() for now for the sake of faster build time. We can't forward declare anything because of RTTI system.
	BaseSafeHandle::Set( (const IReferencable*)other.Get() ); 
	return *this;
}

template< class T >
RED_INLINE THandle<T>& THandle<T>::operator=( const T* other )
{	
	BaseSafeHandle::Set( (const IReferencable*)other );
	return *this;
}

template< class T >
RED_INLINE void THandle<T>::Serialize( IFile& f )
{
	if ( f.IsWriter() )
	{
#ifndef RED_FINAL_BUILD
		CHandleSerializationMarker marker;
#endif

		IReferencable* ptr = BaseSafeHandle::Get();
		f.SerializePointer( ClassID<T>(), (void*&) ptr );
	}
	else if ( f.IsReader() )
	{
		IReferencable* ptr = nullptr;
		f.SerializePointer( ClassID<T>(), (void*&) ptr );
		BaseSafeHandle::Set( ptr );
	}
}

RED_INLINE BaseSafeHandle::BaseSafeHandle( const IReferencable* object, const CClass* classCheck /*= nullptr*/ )
	: m_handle( NULL )
{
	if ( object && object->m_internalHandle )
	{
		ReferencableInternalHandle* handle = object->m_internalHandle;
		RED_ASSERT( handle != nullptr );

		// validate handle creation
#ifndef RED_FINAL_BUILD
		if ( !object->OnValidateHandleCreation() )
		{
			WARN_CORE( TXT("Handle to to object 0x%08llX of class '%ls' will not be created."),
				(Uint64)object, object->GetClass()->GetName().AsChar() );

			return;
		}
#endif

/*		// check if object is protected
		if ( handle->IsProtected() )
		{
			WARN_CORE( TXT("Trying to create a handle to protected object 0x%08llX of class '%ls'"),
				(Uint64)object, object->GetClass()->GetName().AsChar() );

			return;
		}*/

		// check class
		if ( classCheck && !object->GetClass()->IsA( classCheck ) )
		{
			WARN_CORE( TXT("Setting object of class '%ls' to handle of type '%ls'. Handle will be reset since this can cause crashes."),
				object->GetClass()->GetName().AsChar(), classCheck->GetName().AsChar() );

			return;
		}

		// valid
		m_handle = handle;
		m_handle->AddRef();
	}
}

RED_FORCE_INLINE BaseSafeHandle::~BaseSafeHandle()
{
	if ( m_handle != nullptr )
	{
		m_handle->Release();
		m_handle = NULL;
	}
}

// This function is call hammered. Keep inline.
RED_INLINE Bool BaseSafeHandle::IsValid() const
{
	return m_handle && ( m_handle->GetPtr() != NULL );
}

RED_INLINE Bool BaseSafeHandle::IsLost() const
{
	return m_handle && ( m_handle->GetPtr() == NULL );
}

RED_INLINE IReferencable* BaseSafeHandle::Get() const
{
	return m_handle ? m_handle->GetPtr() : NULL;
}
