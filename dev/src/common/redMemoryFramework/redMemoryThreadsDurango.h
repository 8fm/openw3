/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#ifndef _RED_MEMORY_FRAMEWORK_THREADS_DURANGO_H
#define _RED_MEMORY_FRAMEWORK_THREADS_DURANGO_H
#pragma once

#include "redMemoryAssert.h"

namespace Red { namespace MemoryFramework { namespace DurangoAPI {

//////////////////////////////////////////////////////////////////////////
// Mutex implementation
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
	CRITICAL_SECTION		m_criticalSection;
	static const DWORD		c_criticalSectionSpinCount = 256;		// Adaptive critical section spin count
};

//////////////////////////////////////////////////////////////////////////
// CTor
//	Create the mutex
RED_INLINE CMutexImpl::CMutexImpl()
{
	::InitializeCriticalSection( &m_criticalSection );
	::SetCriticalSectionSpinCount( &m_criticalSection, c_criticalSectionSpinCount );
}

//////////////////////////////////////////////////////////////////////////
// DTor
//	Destroy the mutex
RED_INLINE CMutexImpl::~CMutexImpl()
{
	::DeleteCriticalSection( &m_criticalSection );
}

//////////////////////////////////////////////////////////////////////////
// Acquire
//
inline void CMutexImpl::Acquire()
{
	::EnterCriticalSection( &m_criticalSection );
}

//////////////////////////////////////////////////////////////////////////
// Release
//
inline void CMutexImpl::Release()
{
	::LeaveCriticalSection( &m_criticalSection );
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
	(void)::SwitchToThread();
}

inline void FullMemoryBarrier()
{
	_ReadWriteBarrier(); 
	MemoryBarrier();
}

} } } 

#endif 