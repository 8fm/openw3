/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "renderObject.h"

IRenderObject::~IRenderObject()
{
	RED_FATAL_ASSERT( m_refCount.GetValue() == 0, "Debug this. NOW!" );
}

IRenderObject::IRenderObject()
	: m_refCount( 1 )
{
}

Bool IRenderObject::IsRefCountZero() const
{
	return 0 == m_refCount.GetValue();
}

void IRenderObject::DestroySelfIfRefCountZero()
{
	ASSERT( IsRefCountZero() && "Yeah, for any additional tests you have to overload this function :)" );
	delete this;
}

void IRenderObject::AddRef()
{
	m_refCount.Increment();
}

void IRenderObject::Release()
{
	RED_FATAL_ASSERT( m_refCount.GetValue() > 0, "Releasing IRenderObject too many times" );
	if ( m_refCount.Decrement() == 0 )
	{
		DestroySelfIfRefCountZero();
	}
}

Int32 IRenderObject::GetRefCount() const
{
	return m_refCount.GetValue();
}
