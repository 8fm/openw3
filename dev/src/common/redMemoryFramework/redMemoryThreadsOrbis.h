/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#ifndef _RED_MEMORY_FRAMEWORK_THREADS_ORBISAPI_H
#define _RED_MEMORY_FRAMEWORK_THREADS_ORBISAPI_H
#pragma once

#include "redMemoryAssert.h"

namespace Red { namespace MemoryFramework { namespace OrbisAPI {

//////////////////////////////////////////////////////////////////////////
// Orbis-only Adaptive mutex
//	Very important that this is non-reentrant. If it recurses, say goodbye to thread-safety
class CAdaptiveMutexImpl
{
public:
	CAdaptiveMutexImpl();
	~CAdaptiveMutexImpl();
	void		Acquire();
	void		Release();

	class CScopedLockImpl
	{
	private:
		CAdaptiveMutexImpl*				m_mutex;
	public:
		CScopedLockImpl( CAdaptiveMutexImpl *mutex );
		~CScopedLockImpl();
	};
	typedef CScopedLockImpl TScopedLock;

private:
	RED_INLINE Red::System::Bool VerifyResult( Red::System::Int32 error );
	ScePthreadMutex m_mutex;
};

//////////////////////////////////////////////////////////////////////////
// VerifyResult
//	Wraps common error handling for orbis (tests return codes, etc), asserts
RED_INLINE Red::System::Bool CAdaptiveMutexImpl::VerifyResult( Red::System::Int32 error )
{
	RED_MEMORY_ASSERT( error == SCE_OK, "Red::MemoryFramework::OrbisAPI::Mutex function failed with error code: 0x%x", error );
	return error == SCE_OK;
}

//////////////////////////////////////////////////////////////////////////
// CTor
//	Create the mutex
RED_INLINE CAdaptiveMutexImpl::CAdaptiveMutexImpl()
{
	ScePthreadMutexattr attr;
	VerifyResult( scePthreadMutexattrInit( &attr ) );
	VerifyResult( scePthreadMutexattrSettype( &attr, SCE_PTHREAD_MUTEX_ADAPTIVE ) );		// Recursive mutex
	VerifyResult( scePthreadMutexattrSetprotocol( &attr, SCE_PTHREAD_PRIO_INHERIT ) );		// Inherit priority
	VerifyResult( scePthreadMutexInit( &m_mutex, &attr, nullptr ) );						// Initialise the mutex
	VerifyResult( scePthreadMutexattrDestroy( &attr ) );									// Get rid of the attribs
}

//////////////////////////////////////////////////////////////////////////
// DTor
//	Destroy the mutex
RED_INLINE CAdaptiveMutexImpl::~CAdaptiveMutexImpl()
{
	VerifyResult( scePthreadMutexDestroy( &m_mutex ) );
}

//////////////////////////////////////////////////////////////////////////
// Acquire
//
RED_INLINE void CAdaptiveMutexImpl::Acquire()
{
	VerifyResult( scePthreadMutexLock( &m_mutex ) );
}

//////////////////////////////////////////////////////////////////////////
// Release
//
RED_INLINE void CAdaptiveMutexImpl::Release()
{
	VerifyResult( scePthreadMutexUnlock( &m_mutex ) );
}

//////////////////////////////////////////////////////////////////////////
// CScopedLockImpl CTor
//
RED_INLINE CAdaptiveMutexImpl::CScopedLockImpl::CScopedLockImpl( CAdaptiveMutexImpl* mutex )
	: m_mutex( mutex )
{
	RED_MEMORY_ASSERT( m_mutex, "Cannot create mutex wrapper with no implementation" );
	m_mutex->Acquire();
}

//////////////////////////////////////////////////////////////////////////
// CScopedLockImpl DTor
//
RED_INLINE CAdaptiveMutexImpl::CScopedLockImpl::~CScopedLockImpl()
{
	RED_MEMORY_ASSERT( m_mutex, "Mutex wrapper with no implementation" );
	m_mutex->Release();
	m_mutex = nullptr;
}

//////////////////////////////////////////////////////////////////////////
// Orbis Mutex implementation
class CMutexImpl
{
public:
	CMutexImpl();
	~CMutexImpl();

	void		Acquire();
	void		Release();

	class CScopedLockImpl
	{
	private:
		CMutexImpl*				m_mutex;

	public:
		CScopedLockImpl( CMutexImpl *mutex );
		~CScopedLockImpl();
	};
	typedef CScopedLockImpl TScopedLock;

private:
	RED_INLINE Red::System::Bool VerifyResult( Red::System::Int32 error );

	ScePthreadMutex m_mutex;
};

//////////////////////////////////////////////////////////////////////////
// VerifyResult
//	Wraps common error handling for orbis (tests return codes, etc), asserts
RED_INLINE Red::System::Bool CMutexImpl::VerifyResult( Red::System::Int32 error )
{
	RED_MEMORY_ASSERT( error == SCE_OK, "Red::MemoryFramework::OrbisAPI::Mutex function failed with error code: 0x%x", error );
	return error == SCE_OK;
}

//////////////////////////////////////////////////////////////////////////
// CTor
//	Create the mutex
RED_INLINE CMutexImpl::CMutexImpl()
{
	ScePthreadMutexattr attr;
	VerifyResult( scePthreadMutexattrInit( &attr ) );
	VerifyResult( scePthreadMutexattrSettype( &attr, SCE_PTHREAD_MUTEX_RECURSIVE ) );		// Recursive mutex
	VerifyResult( scePthreadMutexattrSetprotocol( &attr, SCE_PTHREAD_PRIO_INHERIT ) );		// Inherit priority
	VerifyResult( scePthreadMutexInit( &m_mutex, &attr, nullptr ) );						// Initialise the mutex
	VerifyResult( scePthreadMutexattrDestroy( &attr ) );									// Get rid of the attribs
}

//////////////////////////////////////////////////////////////////////////
// DTor
//	Destroy the mutex
RED_INLINE CMutexImpl::~CMutexImpl()
{
	VerifyResult( scePthreadMutexDestroy( &m_mutex ) );
}

//////////////////////////////////////////////////////////////////////////
// Acquire
//
inline void CMutexImpl::Acquire()
{
	VerifyResult( scePthreadMutexLock( &m_mutex ) );
}

//////////////////////////////////////////////////////////////////////////
// Release
//
inline void CMutexImpl::Release()
{
	VerifyResult( scePthreadMutexUnlock( &m_mutex ) );
}

//////////////////////////////////////////////////////////////////////////
// CScopedLockImpl CTor
//
inline CMutexImpl::CScopedLockImpl::CScopedLockImpl( CMutexImpl* mutex )
	: m_mutex( mutex )
{
	RED_MEMORY_ASSERT( m_mutex, "Cannot create mutex wrapper with no implementation" );
	m_mutex->Acquire();
}

//////////////////////////////////////////////////////////////////////////
// CScopedLockImpl DTor
//
inline CMutexImpl::CScopedLockImpl::~CScopedLockImpl()
{
	RED_MEMORY_ASSERT( m_mutex, "Mutex wrapper with no implementation" );
	m_mutex->Release();
	m_mutex = nullptr;
}

//////////////////////////////////////////////////////////////////////////
// System-wide threading utils

inline void YieldThread()
{
	::scePthreadYield();
}

inline void FullMemoryBarrier()
{
	_mm_mfence();
}

} } } 

#endif 