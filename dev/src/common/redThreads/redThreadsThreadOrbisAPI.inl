/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#ifndef RED_THREADS_THREAD_ORBISAPI_INL
#define RED_THREADS_THREAD_ORBISAPI_INL
#pragma once

#include <pthread_np.h>

namespace Red { namespace Threads { namespace OrbisAPI {

	//////////////////////////////////////////////////////////////////////////
	// Mutex implementation
	//////////////////////////////////////////////////////////////////////////
	inline CMutexImpl::CMutexImpl()
	{
		ScePthreadMutexattr attr;
		REDTHR_SCE_CHECK( ::scePthreadMutexattrInit( &attr ) );
		REDTHR_SCE_CHECK( ::scePthreadMutexattrSettype( &attr, SCE_PTHREAD_MUTEX_RECURSIVE ) );
		REDTHR_SCE_CHECK( ::scePthreadMutexattrSetprotocol( &attr, SCE_PTHREAD_PRIO_INHERIT ) );
		REDTHR_SCE_CHECK( ::scePthreadMutexInit( &m_mutex, &attr, nullptr ) );
		REDTHR_SCE_CHECK( ::scePthreadMutexattrDestroy( &attr ) );
	}

	inline CMutexImpl::~CMutexImpl()
	{
		REDTHR_SCE_CHECK( ::scePthreadMutexDestroy( &m_mutex ) );
	}

	inline void CMutexImpl::AcquireImpl()
	{
		REDTHR_SCE_CHECK( ::scePthreadMutexLock( &m_mutex ) );
	}

	inline Bool CMutexImpl::TryAcquireImpl()
	{
		Int32 retval = ::scePthreadMutexTrylock( &m_mutex );

#ifdef RED_ASSERTS_ENABLED
		RED_ASSERT( retval == SCE_OK || retval == SCE_KERNEL_ERROR_EBUSY )
#else
		RED_VERIFY( retval == SCE_OK || retval == SCE_KERNEL_ERROR_EBUSY );
#endif

		return retval == SCE_OK;
	}

	inline void CMutexImpl::ReleaseImpl()
	{
		REDTHR_SCE_CHECK( ::scePthreadMutexUnlock( &m_mutex ) );
	}

	inline void CMutexImpl::SetSpinCountImpl( TSpinCount count )
	{
		RED_UNUSED( count );
		// No op for now, as can only use SCE wrappers as of the latest PS4 SDK.
		// Possibility to use an adaptive mutex instead, but then can't rely on it
		// being recursive...
		//REDTHR_SCE_CHECK( ::pthread_mutex_setspinloops_np( &m_mutex, count ) );
	}

	//////////////////////////////////////////////////////////////////////////
	// RWLock implementation
	//////////////////////////////////////////////////////////////////////////
	inline CRWLockImpl::CRWLockImpl()
	{
		REDTHR_SCE_CHECK( ::scePthreadRwlockInit( &m_rwlock, nullptr, nullptr ) );
	}

	inline CRWLockImpl::~CRWLockImpl()
	{
		REDTHR_SCE_CHECK( ::scePthreadRwlockDestroy( &m_rwlock ) );
	}

	inline void CRWLockImpl::AcquireReadSharedImpl()
	{
		// In the case of a reading lock, if SCE_KERNEL_ERROR_EBUSY then we've reached the max number of reading threads
		// TBD: Practical limit and when we'll start busy waiting.
		
#if 0
		int lockResult = SCE_KERNEL_ERROR_EAGAIN;
		do
		{
			lockResult = ::scePthreadRwlockRdlock( &m_rwlock );
			RED_ASSERT( lockResult == SCE_OK || lockResult == SCE_KERNEL_ERROR_EAGAIN, TXT("Acquiring read lock failed: 0x%08X"), lockResult );
		}
		while ( lockResult == SCE_KERNEL_ERROR_EAGAIN );
#endif

		// Can't even hit SCE_KERNEL_ERROR_EAGAIN - run out of spawnable threads first
		REDTHR_SCE_CHECK( ::scePthreadRwlockRdlock( &m_rwlock ) );
	}

	inline void CRWLockImpl::AcquireWriteExclusiveImpl()
	{
		REDTHR_SCE_CHECK( ::scePthreadRwlockWrlock( &m_rwlock ) );
	}

	inline void CRWLockImpl::ReleaseReadSharedImpl()
	{
		REDTHR_SCE_CHECK( ::scePthreadRwlockUnlock( &m_rwlock ) );
	}

	inline void CRWLockImpl::ReleaseWriteExclusiveImpl()
	{
		REDTHR_SCE_CHECK( ::scePthreadRwlockUnlock( &m_rwlock ) );
	}

// 	inline Bool CRWLockImpl::TryAcquireReadSharedImpl()
// 	{
// 		// In the case of a reading lock, if SCE_KERNEL_ERROR_EBUSY then we've reached the max number of reading threads
// 		const int lockResult = ::scePthreadRwlockTryrdlock( &m_rwlock );
// 		RED_ASSERT( lockResult == SCE_OK || lockResult == SCE_KERNEL_ERROR_EBUSY || lockResult == SCE_KERNEL_ERROR_EAGAIN, TXT("Trying to acquire read lock failed: 0x%08X"), lockResult );
// 
// 		return lockResult == SCE_OK;
// 	}
// 
// 	inline Bool CRWLockImpl::TryAcquireWriteExclusiveImpl()
// 	{
// 		// If in the case of a writing lock, SCE_KERNEL_ERROR_EBUSY means it's already exclusively locked (and recursive write locks are not supported)
// 		const int lockResult = ::scePthreadRwlockTrywrlock( &m_rwlock );
// 		RED_ASSERT( lockResult == SCE_OK || lockResult == SCE_KERNEL_ERROR_EBUSY, TXT("Trying to acquire write lock failed: 0x%08X"), lockResult );
// 
// 		return lockResult == SCE_OK;
// 	}

	//////////////////////////////////////////////////////////////////////////
	// Semaphore implementation
	//////////////////////////////////////////////////////////////////////////
	inline CSemaphoreImpl::CSemaphoreImpl( Int32 initialCount, Int32 maximumCount )
	{
		RED_ASSERT( initialCount >= 0 && maximumCount >= 0, TXT("Invalid semaphore count") );
		RED_ASSERT( initialCount <= maximumCount, TXT("Invalid semaphore count") );
		REDTHR_SCE_CHECK( ::sceKernelCreateSema( &m_semaphore, "sema", SCE_KERNEL_SEMA_ATTR_TH_FIFO, initialCount, maximumCount, nullptr ) );
	}

	inline CSemaphoreImpl::~CSemaphoreImpl()
	{
		REDTHR_SCE_CHECK( ::sceKernelDeleteSema( m_semaphore ) );
	}

	inline void CSemaphoreImpl::AcquireImpl()
	{
		REDTHR_SCE_CHECK( ::sceKernelWaitSema( m_semaphore, 1, nullptr ) );
	}

	inline Bool CSemaphoreImpl::TryAcquireImpl( TTimespec timeoutMs )
	{
		if ( timeoutMs == 0 )
		{
			const int pollResult = ::sceKernelPollSema( m_semaphore, 1 );
			RED_ASSERT( pollResult == SCE_OK || pollResult == SCE_KERNEL_ERROR_EBUSY, TXT("Polling semaphore failed") );
			return pollResult == SCE_OK;	
		}

		const TTimespec microPerMilli = 1000;
		TTimespec waitTime = timeoutMs * microPerMilli;
		const int waitResult = ::sceKernelWaitSema( m_semaphore, 1, &waitTime );
		RED_ASSERT( waitResult == SCE_OK || waitResult == SCE_KERNEL_ERROR_ETIMEDOUT, TXT("Waiting on semaphore failed") );
		return waitResult == SCE_OK;
	}

	inline void CSemaphoreImpl::ReleaseImpl( Int32 count )
	{
		RED_ASSERT( count > 0 );
		REDTHR_SCE_CHECK( ::sceKernelSignalSema( m_semaphore, count ) );
	}

	//////////////////////////////////////////////////////////////////////////
	// Condition variable implementation
	//////////////////////////////////////////////////////////////////////////
	inline CConditionVariableImpl::CConditionVariableImpl()
	{
		REDTHR_SCE_CHECK( ::scePthreadCondInit( &m_cond, nullptr, nullptr ) );
	}

	inline CConditionVariableImpl::~CConditionVariableImpl()
	{
		REDTHR_SCE_CHECK( ::scePthreadCondDestroy( &m_cond ) );
	}

	inline void CConditionVariableImpl::WaitImpl( CMutexImpl& mutexImpl )
	{
		REDTHR_SCE_CHECK( ::scePthreadCondWait( &m_cond, &mutexImpl.m_mutex ) );
	}

	inline void CConditionVariableImpl::WakeAnyImpl()
	{
		REDTHR_SCE_CHECK( ::scePthreadCondSignal( &m_cond ) );
	}

	inline void CConditionVariableImpl::WakeAllImpl()
	{
		REDTHR_SCE_CHECK( ::scePthreadCondBroadcast( &m_cond ) );
	}

} } } // namespace Red { namespace Threads { namespace OrbisAPI {

#endif // RED_THREADS_THREAD_ORBISAPI_INL