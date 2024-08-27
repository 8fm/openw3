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
namespace red
{

	class Thread;

} // namespace red

  //////////////////////////////////////////////////////////////////////////
  // Synchronization object implementation decls
  //////////////////////////////////////////////////////////////////////////
namespace red
{
	namespace LinuxAPI
	{

		class MutexImpl
		{
			friend class ConditionVariableImpl;

		private:
			pthread_mutex_t m_mutex;

		protected:
			MutexImpl();
			~MutexImpl();

		protected:
			void		AcquireImpl();
			Bool		TryAcquireImpl();
			void		ReleaseImpl();
			void		SetSpinCountImpl( TSpinCount count );
		};

		class RWLockImpl
		{
		private:
			pthread_rwlock_t m_rwlock;

		protected:
			RWLockImpl();
			~RWLockImpl();

		protected:
			void AcquireReadSharedImpl();
			void AcquireWriteExclusiveImpl();

			void ReleaseReadSharedImpl();
			void ReleaseWriteExclusiveImpl();

			// 		Bool TryAcquireReadSharedImpl();
			// 		Bool TryAcquireWriteExclusiveImpl();
		};

		class SemaphoreImpl
		{
		private:
			sem_t		m_semaphore;

		protected:
			SemaphoreImpl( Int32 initialCount, Int32 maximumCount );
			~SemaphoreImpl();

		protected:
			void		AcquireImpl();
			Bool		TryAcquireImpl( TTimespec timeoutMs );
			void		ReleaseImpl( Int32 count );
			const void* GetOSHandleImpl() const;
		};

		class ConditionVariableImpl
		{
		private:
			pthread_cond_t 				m_cond;

		protected:
			ConditionVariableImpl();
			~ConditionVariableImpl();

		protected:
			void WaitImpl( MutexImpl& mutexImpl );
			void WaitWithTimeoutImpl( MutexImpl& mutexImpl, TTimespec timeoutMs );

			void WakeAnyImpl();
			void WakeAllImpl();
		};

		class ManualResetEventImpl
		{
		private:
			pthread_mutex_t m_mutex;
			pthread_cond_t m_cond;
			bool m_triggered;

		protected:
			explicit ManualResetEventImpl( Bool startSignalled );
			~ManualResetEventImpl();

		protected:
			void SetEventImpl();
			void ResetEventImpl();
			void WaitImpl();
			Bool TryWaitImpl( TTimespec timeoutMs );
			const void* GetOSHandleImpl() const;
		};

	}
} // namespace red { namespace LinuxAPI {

 //////////////////////////////////////////////////////////////////////////
 // Thread object implementation
 //////////////////////////////////////////////////////////////////////////
namespace red
{
	namespace LinuxAPI
	{

		void YieldCurrentThreadImpl();
		void SleepOnCurrentThreadImpl( TTimespec sleepTimeInMS );
		void SetCurrentThreadAffinityImpl( TAffinityMask affinityMask );
		void SetCurrentThreadNameImpl( const char* threadName );
		Uint32 GetMaxHardwareConcurrencyImpl();

		class ThreadImpl
		{
		private:
			Thread*				m_debug;

		private:
			ThreadMemParams		m_memParams;

		private:
			pthread_t			m_thread;

		public:
			ThreadImpl( const ThreadMemParams& memParams );
			~ThreadImpl();

			void					InitThread( Thread* context );
			void					JoinThread();
			void					DetachThread();

		public:
			void					SetAffinityMask( Uint64 mask );
			void					SetPriority( EThreadPriority priority );

		public:
			Bool					operator==( const ThreadImpl& rhs ) const;
			RED_INLINE Bool			IsValid() const { return m_thread != pthread_t(); }
		};

	}
} // namespace red { namespace LinuxAPI {

#include "redThreadsThreadLinuxAPI.inl"

#endif // RED_THREADS_THREAD_LINUXAPI_H
