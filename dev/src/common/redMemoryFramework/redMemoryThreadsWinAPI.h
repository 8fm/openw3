/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#ifndef _RED_MEMORY_FRAMEWORK_THREADS_WINAPI_INL
#define _RED_MEMORY_FRAMEWORK_THREADS_WINAPI_INL
#pragma once

#include "redMemoryAssert.h"
#include "../redSystem/os.h"

//////////////////////////////////////////////////////////////////////////
// Windows Mutex implementation

namespace Red { namespace MemoryFramework { namespace WinAPI {

class CMutexImpl
{
private:
	static const DWORD		c_criticalSectionSpinCount = 256;		// Use a spin-lock on windows
	CRITICAL_SECTION		m_criticalSection;

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
	::InitializeCriticalSection( &m_criticalSection );
	::SetCriticalSectionSpinCount( &m_criticalSection, c_criticalSectionSpinCount );
}

inline CMutexImpl::~CMutexImpl()
{
	::DeleteCriticalSection( &m_criticalSection );
}

inline void CMutexImpl::Acquire()
{
	::EnterCriticalSection( &m_criticalSection );
}

inline void CMutexImpl::Release()
{
	::LeaveCriticalSection( &m_criticalSection );
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
	(void)::SwitchToThread();
}

inline void FullMemoryBarrier()
{
	_ReadWriteBarrier(); 
	MemoryBarrier();
}

} } } // namespace Red { namespace MemoryFramework { namespace WinAPI {

#endif // RED_MEMORY_FRAMEWORK_THREADS_WIN32_INL