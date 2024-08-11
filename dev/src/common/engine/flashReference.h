/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

//////////////////////////////////////////////////////////////////////////
// IFlashReference
//////////////////////////////////////////////////////////////////////////
class IFlashReference
{
private:
	Int32	m_refCount;		//!< Reference count. Not thread safe.

protected:
	virtual ~IFlashReference();

protected:
	virtual void OnDestroy() {}

public:
	IFlashReference();

	//! Add internal reference
	Int32 AddRef();

	//! Release internal reference
	Int32 Release();

public:
	// Memory allocation overloading
	void *operator new( size_t size );
	void operator delete( void *ptr );
};
