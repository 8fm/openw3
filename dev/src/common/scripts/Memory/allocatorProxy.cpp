/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#include "allocatorProxy.h"

namespace Red
{
	AllocatorProxy::AllocatorProxy( ReallocatorFunc afunc, FreeFunc ffunc )
	:	m_realloc( afunc )
	,	m_free( ffunc )
	{

	}

	AllocatorProxy::AllocatorProxy( const AllocatorProxy* other )
	:	m_realloc( other->m_realloc )
	,	m_free( other->m_free )
	{

	}

	AllocatorProxy::AllocatorProxy( const AllocatorProxy& other )
	:	m_realloc( other.m_realloc )
	,	m_free( other.m_free )
	{

	}

	AllocatorProxy::~AllocatorProxy()
	{

	}
}
