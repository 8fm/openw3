/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#ifndef RED_THREADS_THREAD_WINAPI_INL
#define RED_THREADS_THREAD_WINAPI_INL
#pragma once

namespace Red { namespace Threads { namespace WinAPI {

	//////////////////////////////////////////////////////////////////////////
	// Mutex implementation
	//////////////////////////////////////////////////////////////////////////
	inline CMutexImpl::CMutexImpl()
	{
		::InitializeCriticalSection( &m_criticalSection );
	}

	inline CMutexImpl::~CMutexImpl()
	{
		::DeleteCriticalSection( &m_criticalSection );
	}

	inline void CMutexImpl::AcquireImpl()
	{
		::EnterCriticalSection( &m_criticalSection );
	}

	inline Bool CMutexImpl::TryAcquireImpl()
	{
		return ::TryEnterCriticalSection( &m_criticalSection ) != FALSE;
	}

	inline void CMutexImpl::ReleaseImpl()
	{
		::LeaveCriticalSection( &m_criticalSection );
	}

	inline void CMutexImpl::SetSpinCountImpl( TSpinCount count )
	{
		(void)::SetCriticalSectionSpinCount( &m_criticalSection, count );
	}

	//////////////////////////////////////////////////////////////////////////
	// RWLock implementation
	//////////////////////////////////////////////////////////////////////////
	inline CRWLockImpl::CRWLockImpl()
	{
		::InitializeSRWLock( &m_rwlock );
	}

	inline CRWLockImpl::~CRWLockImpl()
	{
		// No RWLock cleanup necessary
	}

	inline void CRWLockImpl::AcquireReadSharedImpl()
	{
		::AcquireSRWLockShared( &m_rwlock );
	}

	inline void CRWLockImpl::AcquireWriteExclusiveImpl()
	{
		::AcquireSRWLockExclusive( &m_rwlock );
	}

	inline void CRWLockImpl::ReleaseReadSharedImpl()
	{
		::ReleaseSRWLockShared( &m_rwlock );
	}

	inline void CRWLockImpl::ReleaseWriteExclusiveImpl()
	{
		::ReleaseSRWLockExclusive( &m_rwlock );
	}

// 	inline Bool CRWLockImpl::TryAcquireReadSharedImpl()
// 	{
// 		return ::TryAcquireSRWLockShared( &m_rwlock ) != FALSE;
// 	}
// 
// 	inline Bool CRWLockImpl::TryAcquireWriteExclusiveImpl()
// 	{
// 		return ::TryAcquireSRWLockExclusive( &m_rwlock ) != FALSE;
// 	}
	
	//////////////////////////////////////////////////////////////////////////
	// Semaphore implementation
	//////////////////////////////////////////////////////////////////////////
	inline CSemaphoreImpl::CSemaphoreImpl( Int32 initialCount, Int32 maximumCount )
	{
		RED_ASSERT( initialCount >= 0 && maximumCount >= 0, TXT("Invalid count arguments") );
		RED_ASSERT( initialCount <= maximumCount, TXT("Invalid count arguments") );

#ifdef RED_PLATFORM_DURANGO
		m_semaphore = ::CreateSemaphoreEx( nullptr, initialCount, maximumCount, nullptr, 0, SYNCHRONIZE | SEMAPHORE_MODIFY_STATE );
#else
		m_semaphore = ::CreateSemaphore( nullptr, initialCount, maximumCount, nullptr );
#endif
		REDTHR_WIN_CHECK( m_semaphore );
	}

	inline CSemaphoreImpl::~CSemaphoreImpl()
	{
		if ( m_semaphore )
		{
			REDTHR_WIN_CHECK( ::CloseHandle( m_semaphore ) );
		}
	}

	inline void CSemaphoreImpl::AcquireImpl()
	{
		RED_ASSERT( m_semaphore, TXT("No semaphore") );
		const DWORD waitResult = ::WaitForSingleObject( m_semaphore, INFINITE );
		REDTHR_WIN_CHECK( waitResult != WAIT_FAILED ); // Get extended error info if applicable
		RED_VERIFY( waitResult == WAIT_OBJECT_0, TXT("Failed to wait") );
	}

	inline Bool CSemaphoreImpl::TryAcquireImpl( TTimespec timeoutMs )
	{
		RED_ASSERT( timeoutMs != INFINITE, TXT("Infinite timeout specified") );
		const DWORD waitResult = ::WaitForSingleObject( m_semaphore, static_cast< DWORD >( timeoutMs ) );
		REDTHR_WIN_CHECK( waitResult != WAIT_FAILED ); // Get extended error info if applicable
		RED_VERIFY( waitResult == WAIT_OBJECT_0 || waitResult == WAIT_TIMEOUT, TXT("Failed to wait") );

		return waitResult == WAIT_OBJECT_0;
	}

	inline void CSemaphoreImpl::ReleaseImpl( Int32 count )
	{
		RED_ASSERT( m_semaphore, TXT("No semaphore") );
		RED_ASSERT( count > 0 );

		REDTHR_WIN_CHECK( ::ReleaseSemaphore( m_semaphore, count, nullptr ) );
	}

	//////////////////////////////////////////////////////////////////////////
	// Condition variable implementation
	//////////////////////////////////////////////////////////////////////////
	inline CConditionVariableImpl::CConditionVariableImpl()
	{
		::InitializeConditionVariable( &m_cond );
	}

	inline CConditionVariableImpl::~CConditionVariableImpl()
	{
		// No cleanup function
	}

	inline void CConditionVariableImpl::WaitImpl( CMutexImpl& mutexImpl )
	{
		REDTHR_WIN_CHECK( ::SleepConditionVariableCS( &m_cond, &mutexImpl.m_criticalSection, INFINITE ) );
	}

	inline void CConditionVariableImpl::WakeAnyImpl()
	{
		::WakeConditionVariable( &m_cond );
	}

	inline void CConditionVariableImpl::WakeAllImpl()
	{
		::WakeAllConditionVariable( &m_cond );
	}

} } } // namespace Red { namespace Threads { namespace WinAPI {

#endif // RED_THREADS_THREAD_WINAPI_INL