/**
* Copyright (c) 2013 CD Projekt Red. All Rights Reserved.
*/
#ifndef RED_THREADS_THREAD_LINUXAPI_H
#define RED_THREADS_THREAD_LINUXAPI_H
#pragma once

#include "redThreadsPlatform.h"

#include <pthread.h>
#include <semaphore.h>

//////////////////////////////////////////////////////////////////////////
// Forward declarations
//////////////////////////////////////////////////////////////////////////
namespace Red { namespace Threads {

	class CThread;

} } // namespace Red { namespace Threads {

//////////////////////////////////////////////////////////////////////////
// Namespace functions
//////////////////////////////////////////////////////////////////////////
namespace Red { namespace Threads { namespace LinuxAPI {

void InitializeFrameworkImpl( TAffinityMask mainThreadAffinityMask );
void ShutdownFrameworkImpl();

} } } // namespace Red { namespace Threads { namespace LinuxAPI {

//////////////////////////////////////////////////////////////////////////
// Synchronization object implementation decls
//////////////////////////////////////////////////////////////////////////
namespace Red { namespace Threads { namespace LinuxAPI {

	class CMutexImpl
	{
		friend class CConditionVariableImpl;

	private:
		pthread_mutex_t m_mutex;

	protected:
		CMutexImpl();
		~CMutexImpl();

	protected:
		void		AcquireImpl();
		Bool		TryAcquireImpl();
		void		ReleaseImpl();
		void		SetSpinCountImpl( TSpinCount count );
	};

	class CRWLockImpl
	{
	private:
		pthread_rwlock_t m_rwlock;

	protected:
		CRWLockImpl();
		~CRWLockImpl();

	protected:
		void AcquireReadSharedImpl();
		void AcquireWriteExclusiveImpl();

		void ReleaseReadSharedImpl();
		void ReleaseWriteExclusiveImpl();

// 		Bool TryAcquireReadSharedImpl();
// 		Bool TryAcquireWriteExclusiveImpl();
	};

	class CSemaphoreImpl
	{
	private:
		sem_t		m_semaphore;

	protected:
		CSemaphoreImpl( Int32 initialCount, Int32 maximumCount );
		~CSemaphoreImpl();

		void		AcquireImpl();
		Bool		TryAcquireImpl( TTimespec timeoutMs );
		void		ReleaseImpl( Int32 count );
	};

	class CConditionVariableImpl
	{
	private:
		pthread_cond_t 				m_cond;

	protected:
		CConditionVariableImpl();
		~CConditionVariableImpl();

	protected:
		void WaitImpl( CMutexImpl& mutexImpl );

		void WakeAnyImpl();
		void WakeAllImpl();
	};

} } } // namespace Red { namespace Threads { namespace LinuxAPI {

//////////////////////////////////////////////////////////////////////////
// Thread object implementation
//////////////////////////////////////////////////////////////////////////
namespace Red { namespace Threads { namespace LinuxAPI {

	void YieldCurrentThreadImpl();
	void SleepOnCurrentThreadImpl( TTimespec sleepTimeInMS );

	class CThreadImpl
	{
	private:
		CThread*				m_debug;

	private:
		SThreadMemParams		m_memParams;

	private:
		pthread_t				m_thread;

	public:
		CThreadImpl( const SThreadMemParams& memParams );
		~CThreadImpl();

		void					InitThread( CThread* context );
		void					JoinThread();
		void					DetachThread();

	public:
		void					SetAffinityMask( Uint64 mask );
		void					SetPriority( EThreadPriority priority );

	public:
		Bool					operator==( const CThreadImpl& rhs ) const;

	private:
		RED_INLINE Bool			IsValid() const { return m_thread != pthread_t(); }
	};

} } } // namespace Red { namespace Threads { namespace LinuxAPI {

#include "redThreadsThreadLinuxAPI.inl"

#endif // RED_THREADS_THREAD_LINUXAPI_H
