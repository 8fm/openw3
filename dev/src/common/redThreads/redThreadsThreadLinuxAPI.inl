/**
* Copyright (c) 2018 CD Projekt Red. All Rights Reserved.
*/
#ifndef RED_THREADS_THREAD_LINUXAPI_INL
#define RED_THREADS_THREAD_LINUXAPI_INL
#pragma once

#define INFINITE 0xFFFFFFFF

namespace Red { namespace Threads { namespace LinuxAPI {

	//////////////////////////////////////////////////////////////////////////
	// Mutex implementation
	//////////////////////////////////////////////////////////////////////////
	inline CMutexImpl::CMutexImpl()
	{
		pthread_mutexattr_t attr;

		REDTHR_PTHREAD_CHECK( ::pthread_mutexattr_init( &attr ) );
		REDTHR_PTHREAD_CHECK( ::pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_RECURSIVE ) );
		REDTHR_PTHREAD_CHECK( ::pthread_mutexattr_setprotocol( &attr, PTHREAD_PRIO_INHERIT ) );
		REDTHR_PTHREAD_CHECK( ::pthread_mutex_init( &m_mutex, &attr ) );
		REDTHR_PTHREAD_CHECK( ::pthread_mutexattr_destroy( &attr ) );
	}

	inline CMutexImpl::~CMutexImpl()
	{
		::pthread_mutex_destroy( &m_mutex );
	}

	inline void CMutexImpl::AcquireImpl()
	{
		::pthread_mutex_lock( &m_mutex );
	}

	inline Bool CMutexImpl::TryAcquireImpl()
	{
		Int32 retval = ::pthread_mutex_trylock( &m_mutex );

#ifdef RED_ASSERTS_ENABLED
		RED_ASSERT( retval == 0 || retval == EBUSY );
#else
		RED_VERIFY( retval == 0 || retval == EBUSY );
#endif

		return retval == 0;
	}

	inline void CMutexImpl::ReleaseImpl()
	{
		::pthread_mutex_unlock( &m_mutex );
	}

	inline void CMutexImpl::SetSpinCountImpl( TSpinCount count )
	{
		// FIXME doesnt seem to be available on debian
		RED_UNUSED( count );
		//REDTHR_PTHREAD_CHECK( ::pthread_mutex_setspinloops_np( &m_mutex, count ) );
	}

	//////////////////////////////////////////////////////////////////////////
	// RWLock implementation
	//////////////////////////////////////////////////////////////////////////
	inline CRWLockImpl::CRWLockImpl()
	{
		REDTHR_PTHREAD_CHECK( ::pthread_rwlock_init( &m_rwlock, nullptr ) );
	}

	inline CRWLockImpl::~CRWLockImpl()
	{
		REDTHR_PTHREAD_CHECK( ::pthread_rwlock_destroy( &m_rwlock ) );
	}

	inline void CRWLockImpl::AcquireReadSharedImpl()
	{
		REDTHR_PTHREAD_CHECK( ::pthread_rwlock_rdlock( &m_rwlock ) );
	}

	inline void CRWLockImpl::AcquireWriteExclusiveImpl()
	{
		REDTHR_PTHREAD_CHECK( ::pthread_rwlock_wrlock( &m_rwlock ) );
	}

	inline void CRWLockImpl::ReleaseReadSharedImpl()
	{
		REDTHR_PTHREAD_CHECK( ::pthread_rwlock_unlock( &m_rwlock ) );
	}

	inline void CRWLockImpl::ReleaseWriteExclusiveImpl()
	{
		REDTHR_PTHREAD_CHECK( ::pthread_rwlock_unlock( &m_rwlock ) );
	}

	//////////////////////////////////////////////////////////////////////////
	// Semaphore implementation
	//////////////////////////////////////////////////////////////////////////
	inline CSemaphoreImpl::CSemaphoreImpl( Int32 initialCount, Int32 maximumCount )
	{
		RED_UNUSED(maximumCount);
		RED_ASSERT( initialCount >= 0 && maximumCount >= 0, TXT("Invalid semaphore count") );
		RED_ASSERT( initialCount <= maximumCount, TXT("Invalid semaphore count") );

		const int NO_PROCESS_SHARE = 0;
		REDTHR_SEMA_CHECK( ::sem_init( &m_semaphore, NO_PROCESS_SHARE, initialCount ) );
	}

	inline CSemaphoreImpl::~CSemaphoreImpl()
	{
		REDTHR_PTHREAD_CHECK( ::sem_destroy( &m_semaphore ) );
	}

	inline void CSemaphoreImpl::AcquireImpl()
	{
		//RED_ASSERT( m_semaphore, TXT("No semaphore") );
		int retval = 0;
		while ( ( retval = ::sem_wait( &m_semaphore ) ) == -1 && ( errno == EINTR || errno == ETIMEDOUT ) )
			continue;

		const int err = errno;

#ifdef RED_ASSERTS_ENABLED
		RED_ASSERT( retval != -1, TXT("semaphore acquire failed: %s"), strerror( err ) );
#endif
	}

	inline Bool CSemaphoreImpl::TryAcquireImpl( TTimespec timeoutMs )
	{
		// TODO use timeout
		RED_ASSERT( timeoutMs != INFINITE, TXT("Infinite timeout specified") );
		int retval = 0;
		while ( ( retval = ::sem_trywait( &m_semaphore ) ) == -1 && (errno == EINTR || errno == ETIMEDOUT))
			continue;

		const int err = errno;

#ifdef RED_ASSERTS_ENABLED
		RED_ASSERT( retval != -1 || err == EAGAIN, TXT("") );
#endif
		return err != EAGAIN;
	}

	inline void CSemaphoreImpl::ReleaseImpl( Int32 count )
	{
		RED_ASSERT( /*m_semaphore ||*/ count > 0, TXT("No semaphore") );

		for ( Int32 j = 0; j < count; ++j )
		{
			REDTHR_SEMA_CHECK( ::sem_post( &m_semaphore ) );
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// Condition variable implementation
	//////////////////////////////////////////////////////////////////////////
	inline CConditionVariableImpl::CConditionVariableImpl()
	{
		REDTHR_PTHREAD_CHECK( ::pthread_cond_init( &m_cond, nullptr ) );
	}

	inline CConditionVariableImpl::~CConditionVariableImpl()
	{
		REDTHR_PTHREAD_CHECK( ::pthread_cond_destroy( &m_cond ) );
	}

	inline void CConditionVariableImpl::WaitImpl( CMutexImpl& mutexImpl )
	{
		REDTHR_PTHREAD_CHECK( ::pthread_cond_wait( &m_cond, &mutexImpl.m_mutex ) );
	}

	inline void CConditionVariableImpl::WakeAnyImpl()
	{
		REDTHR_PTHREAD_CHECK( ::pthread_cond_signal( &m_cond ) );
	}

	inline void CConditionVariableImpl::WakeAllImpl()
	{
		REDTHR_PTHREAD_CHECK( ::pthread_cond_broadcast( &m_cond ) );
	}

} } } // namespace Red { namespace Threads { namespace LinuxAPI {

#endif // RED_THREADS_THREAD_LINUXAPI_INL
