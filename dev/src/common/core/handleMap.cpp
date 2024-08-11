/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "handleMap.h"
#include "referencable.h"

#ifndef RED_FINAL_BUILD
	Bool CHandleSerializationMarker::st_isInHandleSerialization = false;
#endif

BaseSafeHandle::BaseSafeHandle( const BaseSafeHandle& other, const CClass* classCheck /*= nullptr*/ )
	: m_handle( NULL )
{
	ReferencableInternalHandle* handle = other.m_handle.Get();
	if ( handle )
	{
		// validate handle creation
#ifndef RED_FINAL_BUILD
		if ( handle->GetPtr() && !handle->GetPtr()->OnValidateHandleCreation() )
		{
			WARN_CORE( TXT("Handle to to object 0x%08llX of class '%ls' will not be created."),
				(Uint64)handle->GetPtr(), handle->GetPtr()->GetClass()->GetName().AsChar() );

			return;
		}
#endif

		/*// check if object is protected
		if ( handle->IsProtected() )
		{
			WARN_CORE( TXT("Trying to copy a handle to protected object 0x%08llX"),
				(Uint64)handle->GetPtr() );

			return;
		}*/

		// check class
		if ( classCheck && handle->GetPtr() && !handle->GetPtr()->GetClass()->IsA( classCheck ) )
		{
			WARN_CORE( TXT("Setting object of class '%ls' to handle of type '%ls'. Handle will be reset since this can cause crashes."),
				handle->GetPtr()->GetClass()->GetName().AsChar(), 
				classCheck->GetName().AsChar() );

			return;
		}

		// valid
		m_handle = handle;
		m_handle->AddRef();
	}
}

BaseSafeHandle::BaseSafeHandle( BaseSafeHandle&& other )
{
	m_handle = other.m_handle;
	other.m_handle = nullptr;
}

const IReferencable* BaseSafeHandle::GetConst() const
{
	return m_handle ? m_handle->GetPtr() : NULL;
}

void BaseSafeHandle::Set( const IReferencable* object, const CClass* classCheck /*= nullptr*/ )
{
	// Old handle
	ReferencableInternalHandle* oldHandle = m_handle.Get();
	ReferencableInternalHandle* newHandle = object ? object->m_internalHandle : nullptr;

	// Set new handle
	if ( object )
	{
		ReferencableInternalHandle* newHandle = object->m_internalHandle;
		RED_ASSERT( newHandle );

		// check the class
		if ( classCheck )
		{
			if ( !object->GetClass()->IsA( classCheck ) )
			{
				WARN_CORE( TXT("Setting object of class '%ls' to handle of type '%ls'. Handle will be reset since this can cause crashes."),
					object->GetClass()->GetName().AsChar(), classCheck->GetName().AsChar() );

				newHandle = nullptr;
			}
		}

		// validate handle creation
#ifndef RED_FINAL_BUILD
		if ( !object->OnValidateHandleCreation() )
		{
			WARN_CORE( TXT("Handle to to object 0x%08llX of class '%ls' will not be created."),
				(Uint64)object, object->GetClass()->GetName().AsChar() );

			return;
		}
#endif

		/*// check handle protection
		if ( newHandle->IsProtected() )
		{
			WARN_CORE( TXT("Trying to set handle to protected object 0x%08llX"),
				(Uint64)newHandle->GetPtr() );

			newHandle = nullptr;
		}*/
	}

	// Swap
	if ( newHandle != oldHandle )
	{
		m_handle = newHandle;

		if (newHandle)
			newHandle->AddRef();

		if (oldHandle)
			oldHandle->Release();
	}
}

void BaseSafeHandle::Clear()
{
	if ( m_handle != nullptr )
	{
		m_handle->Release();
		m_handle = NULL;
	}
}

BaseSafeHandle& BaseSafeHandle::operator=( const BaseSafeHandle& other )
{
	// Do not assign from self
	if ( m_handle == other.m_handle )
		return *this;

	ReferencableInternalHandle* oldHandle = m_handle.Get();
	ReferencableInternalHandle* newHandle = other.m_handle.Get();

	// validate handle creation
#ifndef RED_FINAL_BUILD
	if ( newHandle && newHandle->GetPtr() && !newHandle->GetPtr()->OnValidateHandleCreation() )
	{
		WARN_CORE( TXT("Handle to to object 0x%08llX of class '%ls' will not be created."),
			(Uint64)newHandle->GetPtr(), newHandle->GetPtr()->GetClass()->GetName().AsChar() );

		newHandle = nullptr;
	}
#endif

	// Trying to access protected handle
	/*if ( newHandle && newHandle->IsProtected() )
	{
		WARN_CORE( TXT("Trying to set handle to protected object 0x%08llX"),
			(Uint64)newHandle->GetPtr() );

		newHandle = nullptr;
	}*/

	// Swap
	if ( newHandle != oldHandle )
	{
		m_handle = newHandle;

		if (newHandle)
			newHandle->AddRef();

		if (oldHandle)
			oldHandle->Release();
	}

	return *this;
}

BaseSafeHandle& BaseSafeHandle::operator=( BaseSafeHandle&& other )
{
	if ( &other == this )
	{
		return * this;
	}

	if ( m_handle )
	{
		m_handle->Release();
	}

	m_handle = other.m_handle;
	other.m_handle = NULL;

	// Pass through
	return *this;
}
