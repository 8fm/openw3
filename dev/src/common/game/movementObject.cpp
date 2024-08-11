/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

IMovementObject::~IMovementObject()
{
	ASSERT( m_refCount.GetValue() == 0 );
}

IMovementObject::IMovementObject()
	: m_refCount( 1 )
{
}

Bool IMovementObject::IsRefCountZero() const
{
	return 0 == m_refCount.GetValue();
}

void IMovementObject::DestroySelfIfRefCountZero()
{
	ASSERT( IsRefCountZero() && "Yeah, for any additional tests you have to overload this function :)" );
	delete this;
}

void IMovementObject::AddRef()
{
	m_refCount.Increment();
}

void IMovementObject::Release()
{
	ASSERT( m_refCount.GetValue() > 0 );
	if ( m_refCount.Decrement() == 0 )
	{
		DestroySelfIfRefCountZero();
	}
}
