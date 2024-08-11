/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "asyncFileAccess.h"

IAsyncFile::IAsyncFile()
	: m_refCount(1)
{
}

void IAsyncFile::AddRef()
{
	m_refCount.Increment();
}

void IAsyncFile::Release()
{
	if ( 0 == m_refCount.Decrement() )
	{
		delete this;
	}
}
