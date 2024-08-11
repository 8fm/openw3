/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "pointer.h"
#include "object.h"

namespace
{
	CPointer TheNullPointer;
}

CPointer& CPointer::Null()
{
	return TheNullPointer;
}

CPointer::CPointer( ISerializable* object )
	: m_class( NULL )
{
	if ( NULL != object )
	{
		initialize( object, object->GetClass() );
	}
}

Bool CPointer::IsObject() const
{
	return m_class && m_class->IsObject();
}

Bool CPointer::IsSerializable() const
{
	return m_class && m_class->IsSerializable();
}

CClass* CPointer::GetRuntimeClass() const
{
	if ( m_data )
	{
		// Use runtime class from serializable
		ISerializable* serializable = GetSerializablePtr();
		if ( NULL != serializable )
		{
			return serializable->GetClass();
		}

		// Use pointer class
		return m_class;
	}

	// no class known
	return NULL;
}

CObject* CPointer::GetObjectPtr() const
{
	if ( m_class && m_class->IsObject() )
	{
		return m_class->CastTo< CObject >( m_data.Get() );
	}

	return NULL;
}

ISerializable* CPointer::GetSerializablePtr() const
{
	if ( m_class && m_class->IsSerializable() )
	{
		return m_class->CastTo< ISerializable >( m_data.Get() );
	}

	return NULL;
}

void CPointer::initialize( void* ptr, CClass* ptrClass )
{
	if ( m_data != ptr )
	{
		// Different pointer is being set
		m_class = ptrClass;
		m_data = ptr;
	}
	else
	{
		// Change the class only, pointer is the same
		m_class = ptrClass;
	}
}

void CPointer::release()
{
	// Cleanup
	m_data = NULL;
	m_class = NULL;
}