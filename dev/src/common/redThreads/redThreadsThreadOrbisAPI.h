/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#ifndef RED_THREADS_THREAD_ORBISAPI_H
#define RED_THREADS_THREAD_ORBISAPI_H
#pragma once

#include "redThreadsPlatform.h"

//////////////////////////////////////////////////////////////////////////
// Forward declarations
//////////////////////////////////////////////////////////////////////////
namespace Red { namespace Threads {

	class CThread;

} } // namespace Red { namespace Threads {

//////////////////////////////////////////////////////////////////////////
// Namespace functions
//////////////////////////////////////////////////////////////////////////
namespace Red { namespace Threads { namespace OrbisAPI {

	void InitializeFrameworkImpl( TAffinityMask mainThreadAffinityMask );
	void ShutdownFrameworkImpl();

} } } // namespace Red { namespace Threads { namespace OrbisAPI {

//////////////////////////////////////////////////////////////////////////
// Synchronization object implementation decls
//////////////////////////////////////////////////////////////////////////
namespace Red { namespace Threads { namespace OrbisAPI {

	class CMutexImpl
	{
		friend class CConditionVariableImpl;

	private:
		ScePthreadMutex				m_mutex;

	protected:
		CMutexImpl();
		~CMutexImpl();

		void		AcquireImpl();
		Bool		TryAcquireImpl();
		void		ReleaseImpl();
		void		SetSpinCountImpl( TSpinCount count );
	};

	class CRWLockImpl
	{
	private:
		ScePthreadRwlock			m_rwlock;

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
		SceKernelSema			m_semaphore;

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
		ScePthreadCond 				m_cond;

	protected:
		CConditionVariableImpl();
		~CConditionVariableImpl();

	protected:
		void WaitImpl( CMutexImpl& mutexImpl );

		void WakeAnyImpl();
		void WakeAllImpl();
	};

} } } // namespace Red { namespace Threads { namespace OrbisAPI {

//////////////////////////////////////////////////////////////////////////
// Thread object implementation
//////////////////////////////////////////////////////////////////////////
namespace Red { namespace Threads { namespace OrbisAPI {

	void YieldCurrentThreadImpl();
	void SleepOnCurrentThreadImpl( TTimespec sleepTimeInMS );

	class CThreadImpl
	{
	private:
		SThreadMemParams		m_memParams;

	private:
		ScePthread				m_thread;	

	public:
								CThreadImpl( const SThreadMemParams& memParams );
								~CThreadImpl();

		void					InitThread( CThread* context );
		void					JoinThread();
		void					DetachThread();

	public:

		void					SetAffinityMask( TAffinityMask mask );
		void					SetPriority( EThreadPriority priority );

	public:
		Bool					operator==( const CThreadImpl& rhs ) const;

	private:
		inline Bool				IsValid() const { return m_thread != ScePthread(); }
	};

} } } // namespace Red { namespace Threads { namespace OrbisAPI {

#include "redThreadsThreadOrbisAPI.inl"

#endif // RED_THREADS_THREAD_ORBISAPI_H