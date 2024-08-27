/**
* Copyright (c) 2018 CD Projekt Red. All Rights Reserved.
*/
#ifndef RED_THREADS_THREAD_LINUXAPI_INL
#define RED_THREADS_THREAD_LINUXAPI_INL
#pragma once

#define INFINITE 0xFFFFFFFF

namespace red
{
	namespace LinuxAPI
	{

//////////////////////////////////////////////////////////////////////////
// Mutex implementation
//////////////////////////////////////////////////////////////////////////
		inline MutexImpl::MutexImpl()
		{
			pthread_mutexattr_t attr;

			REDTHR_PTHREAD_CHECK( ::pthread_mutexattr_init( &attr ) );
			REDTHR_PTHREAD_CHECK( ::pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_RECURSIVE ) );
			REDTHR_PTHREAD_CHECK( ::pthread_mutexattr_setprotocol( &attr, PTHREAD_PRIO_INHERIT ) );
			REDTHR_PTHREAD_CHECK( ::pthread_mutex_init( &m_mutex, &attr ) );
			REDTHR_PTHREAD_CHECK( ::pthread_mutexattr_destroy( &attr ) );
		}

		inline MutexImpl::~MutexImpl()
		{
			::pthread_mutex_destroy( &m_mutex );
		}

		inline void MutexImpl::AcquireImpl()
		{
			::pthread_mutex_lock( &m_mutex );
		}

		inline Bool MutexImpl::TryAcquireImpl()
		{
			Int32 retval = ::pthread_mutex_trylock( &m_mutex );

#ifdef RED_ASSERTS_ENABLED
			RED_ASSERT( retval == 0 || retval == EBUSY );
#else
			RED_VERIFY( retval == 0 || retval == EBUSY );
#endif

			return retval == 0;
		}

		inline void MutexImpl::ReleaseImpl()
		{
			::pthread_mutex_unlock( &m_mutex );
		}

		inline void MutexImpl::SetSpinCountImpl( TSpinCount count )
		{
			// FIXME doesnt seem to be available on debian
			RED_UNUSED( count );
			//REDTHR_PTHREAD_CHECK( ::pthread_mutex_setspinloops_np( &m_mutex, count ) );
		}

		//////////////////////////////////////////////////////////////////////////
		// RWLock implementation
		//////////////////////////////////////////////////////////////////////////
		inline RWLockImpl::RWLockImpl()
		{
			REDTHR_PTHREAD_CHECK( ::pthread_rwlock_init( &m_rwlock, nullptr ) );
		}

		inline RWLockImpl::~RWLockImpl()
		{
			REDTHR_PTHREAD_CHECK( ::pthread_rwlock_destroy( &m_rwlock ) );
		}

		inline void RWLockImpl::AcquireReadSharedImpl()
		{
			REDTHR_PTHREAD_CHECK( ::pthread_rwlock_rdlock( &m_rwlock ) );
		}

		inline void RWLockImpl::AcquireWriteExclusiveImpl()
		{
			REDTHR_PTHREAD_CHECK( ::pthread_rwlock_wrlock( &m_rwlock ) );
		}

		inline void RWLockImpl::ReleaseReadSharedImpl()
		{
			REDTHR_PTHREAD_CHECK( ::pthread_rwlock_unlock( &m_rwlock ) );
		}

		inline void RWLockImpl::ReleaseWriteExclusiveImpl()
		{
			REDTHR_PTHREAD_CHECK( ::pthread_rwlock_unlock( &m_rwlock ) );
		}

		//////////////////////////////////////////////////////////////////////////
		// Semaphore implementation
		//////////////////////////////////////////////////////////////////////////
		inline SemaphoreImpl::SemaphoreImpl( Int32 initialCount, Int32 maximumCount )
		{
			RED_UNUSED(maximumCount);
			RED_SYSTEM_ASSERT( initialCount >= 0 && maximumCount >= 0, "Invalid count arguments" );
			RED_SYSTEM_ASSERT( initialCount <= maximumCount, "Invalid count arguments" );

			const int NO_PROCESS_SHARE = 0;
			REDTHR_SEMA_CHECK( ::sem_init( &m_semaphore, NO_PROCESS_SHARE, initialCount ) );
		}

		inline SemaphoreImpl::~SemaphoreImpl()
		{
			REDTHR_PTHREAD_CHECK( ::sem_destroy( &m_semaphore ) );
		}

		inline void SemaphoreImpl::AcquireImpl()
		{
			//RED_SYSTEM_ASSERT( m_semaphore, "No semaphore" );
			int retval = 0;
			while ( ( retval = ::sem_wait( &m_semaphore ) ) == -1 && ( errno == EINTR || errno == ETIMEDOUT ) )
				continue;

			const int err = errno;

#ifdef RED_ASSERTS_ENABLED
			RED_SYSTEM_ASSERT( retval != -1, "semaphore acquire failed: %s", strerror( err ) );
#endif
		}

		inline Bool SemaphoreImpl::TryAcquireImpl( TTimespec timeoutMs )
		{
			// TODO use timeout
			RED_SYSTEM_ASSERT( timeoutMs != INFINITE, "Infinite timeout specified" );
			int retval = 0;
			while ( ( retval = ::sem_trywait( &m_semaphore ) ) == -1 && (errno == EINTR || errno == ETIMEDOUT))
				continue;

			const int err = errno;

#ifdef RED_ASSERTS_ENABLED
			RED_SYSTEM_ASSERT( retval != -1 || err == EAGAIN, "" );
#endif
			return err != EAGAIN;
		}

		inline void SemaphoreImpl::ReleaseImpl( Int32 count )
		{
			RED_SYSTEM_ASSERT( /*m_semaphore ||*/ count > 0, "No semaphore" );

			for ( Int32 j = 0; j < count; ++j )
			{
				REDTHR_SEMA_CHECK( ::sem_post( &m_semaphore ) );
			}
		}

		inline const void* SemaphoreImpl::GetOSHandleImpl() const
		{
			// TODO clang doesnt like this cast
			return nullptr;//(const void*)m_semaphore;
		}

		//////////////////////////////////////////////////////////////////////////
		// Condition variable implementation
		//////////////////////////////////////////////////////////////////////////
		inline ConditionVariableImpl::ConditionVariableImpl()
		{
			REDTHR_PTHREAD_CHECK( ::pthread_cond_init( &m_cond, nullptr ) );
		}

		inline ConditionVariableImpl::~ConditionVariableImpl()
		{
			REDTHR_PTHREAD_CHECK( ::pthread_cond_destroy( &m_cond ) );
		}

		inline void ConditionVariableImpl::WaitImpl( MutexImpl& mutexImpl )
		{
			REDTHR_PTHREAD_CHECK( ::pthread_cond_wait( &m_cond, &mutexImpl.m_mutex ) );
		}

		inline void ConditionVariableImpl::WaitWithTimeoutImpl( MutexImpl& mutexImpl, TTimespec timeoutMs )
		{
			// seems pthread_cond_timedwait is not relative 
			REDTHR_PTHREAD_CHECK( timeoutMs < 1000 );

			struct timeval now;
			::gettimeofday( &now, NULL );

			struct timespec timeToWait;
			timeToWait.tv_sec = now.tv_sec;
			timeToWait.tv_nsec = ( now.tv_usec + 1000UL * timeoutMs ) * 1000UL;

			REDTHR_PTHREAD_CHECK( ::pthread_cond_timedwait( &m_cond, &mutexImpl.m_mutex, &timeToWait ) );
		}

		inline void ConditionVariableImpl::WakeAnyImpl()
		{
			REDTHR_PTHREAD_CHECK( ::pthread_cond_signal( &m_cond ) );
		}

		inline void ConditionVariableImpl::WakeAllImpl()
		{
			REDTHR_PTHREAD_CHECK( ::pthread_cond_broadcast( &m_cond ) );
		}

		//////////////////////////////////////////////////////////////////////////
		// Manual reset event implementation
		//////////////////////////////////////////////////////////////////////////
		inline ManualResetEventImpl::ManualResetEventImpl( Bool startSignalled )
		{
			pthread_mutexattr_t attr;

			REDTHR_PTHREAD_CHECK( ::pthread_mutexattr_init( &attr ) );
			REDTHR_PTHREAD_CHECK( ::pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_RECURSIVE ) );
			REDTHR_PTHREAD_CHECK( ::pthread_mutexattr_setprotocol( &attr, PTHREAD_PRIO_INHERIT ) );
			REDTHR_PTHREAD_CHECK( ::pthread_mutex_init( &m_mutex, &attr ) );
			REDTHR_PTHREAD_CHECK( ::pthread_mutexattr_destroy( &attr ) );

			REDTHR_PTHREAD_CHECK( ::pthread_cond_init( &m_cond, nullptr ) );

			m_triggered = false;
			if ( startSignalled )
				SetEventImpl();
		}

		inline ManualResetEventImpl::~ManualResetEventImpl()
		{
			::pthread_mutex_destroy( &m_mutex );
			REDTHR_PTHREAD_CHECK( ::pthread_cond_destroy( &m_cond ) );
		}

		inline void ManualResetEventImpl::SetEventImpl()
		{
			::pthread_mutex_lock( &m_mutex );
			m_triggered = true;
			REDTHR_PTHREAD_CHECK( ::pthread_cond_signal( &m_cond ) );
			::pthread_mutex_unlock( &m_mutex );
		}

		inline void ManualResetEventImpl::ResetEventImpl()
		{
			::pthread_mutex_lock( &m_mutex );
			m_triggered = false;
			::pthread_mutex_unlock( &m_mutex );
		}

		inline void ManualResetEventImpl::WaitImpl()
		{
			::pthread_mutex_lock( &m_mutex );
			while ( !m_triggered )
			{
				::pthread_cond_wait( &m_cond, &m_mutex );
			}
			::pthread_mutex_unlock( &m_mutex );
		}

		inline Bool ManualResetEventImpl::TryWaitImpl( TTimespec timeoutMS )
		{
			REDTHR_PTHREAD_CHECK( timeoutMS != INFINITE );

			// seems pthread_cond_timedwait is not relative 
			REDTHR_PTHREAD_CHECK( timeoutMS < 1000 );

			struct timeval now;
			::gettimeofday( &now, NULL );

			struct timespec timeToWait;
			timeToWait.tv_sec = now.tv_sec;
			timeToWait.tv_nsec = ( now.tv_usec + 1000UL * timeoutMS ) * 1000UL;

			::pthread_mutex_lock( &m_mutex );
			Int32 result = ::pthread_cond_timedwait( &m_cond, &m_mutex, &timeToWait );
			::pthread_mutex_unlock( &m_mutex );

			return result == 0;
		}

		inline const void* ManualResetEventImpl::GetOSHandleImpl() const
		{
			// TODO
			return nullptr;
		}

	}
} // namespace red { namespace WinAPI {

#endif // RED_THR
