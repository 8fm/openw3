/**
* Copyright (c) 2013 CD Projekt Red. All Rights Reserved.
*/
#ifndef _RED_MEMORY_FRAMEWORK_THREADS_LINUX_H
#define _RED_MEMORY_FRAMEWORK_THREADS_LINUX_H
#pragma once

#include "redMemoryAssert.h"
#include "../redSystem/os.h"

//////////////////////////////////////////////////////////////////////////
// Linux Mutex implementation

namespace Red { namespace MemoryFramework { namespace LinuxAPI {

class CMutexImpl
{
private:
	pthread_mutex_t				m_thread_mutex;

public:
	class CScopedLockImpl
	{
	private:
		CMutexImpl*				m_mutex;

	public:
		CScopedLockImpl( CMutexImpl *mutex );
		~CScopedLockImpl();
	};

	typedef CScopedLockImpl TScopedLock;

public:
	CMutexImpl();
	~CMutexImpl();

	void		Acquire();
	void		Release();
};

//////////////////////////////////////////////////////////////////////////

inline CMutexImpl::CMutexImpl()
{
	pthread_mutexattr_t attr;

	::pthread_mutexattr_init( &attr );
	::pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_RECURSIVE );
	::pthread_mutexattr_setprotocol( &attr, PTHREAD_PRIO_INHERIT );
	::pthread_mutex_init( &m_thread_mutex, &attr );
	::pthread_mutexattr_destroy( &attr );
}

inline CMutexImpl::~CMutexImpl()
{
	::pthread_mutex_destroy( &m_thread_mutex );
}

inline void CMutexImpl::Acquire()
{
	::pthread_mutex_lock( &m_thread_mutex );
}

inline void CMutexImpl::Release()
{
	::pthread_mutex_unlock( &m_thread_mutex );
}

//////////////////////////////////////////////////////////////////////////

inline CMutexImpl::CScopedLockImpl::CScopedLockImpl( CMutexImpl* mutex )
	: m_mutex( mutex )
{
	RED_MEMORY_ASSERT( m_mutex, "Cannot create mutex wrapper with no implementation" );
	m_mutex->Acquire();
}

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
	::pthread_yield();
}

inline void FullMemoryBarrier()
{
	_mm_mfence();
}

} } } // namespace Red { namespace MemoryFramework { namespace WinAPI {

#endif // _RED_MEMORY_FRAMEWORK_THREADS_LINUX_H
