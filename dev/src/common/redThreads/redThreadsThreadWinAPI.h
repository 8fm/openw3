/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#ifndef RED_THREADS_THREAD_WINAPI_H
#define RED_THREADS_THREAD_WINAPI_H
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
namespace Red { namespace Threads { namespace WinAPI {

	void InitializeFrameworkImpl( TAffinityMask mainThreadAffinityMask );
	void ShutdownFrameworkImpl();

} } } // namespace Red { namespace Threads { namespace WinAPI {

//////////////////////////////////////////////////////////////////////////
// Synchronization object implementation decls
//////////////////////////////////////////////////////////////////////////
namespace Red { namespace Threads { namespace WinAPI {

	class CMutexImpl
	{
		friend class CConditionVariableImpl;

	private:
		CRITICAL_SECTION		m_criticalSection;

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
		SRWLOCK		m_rwlock;

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
		HANDLE		m_semaphore;

	protected:
		CSemaphoreImpl( Int32 initialCount, Int32 maximumCount );
		~CSemaphoreImpl();

	protected:
		void		AcquireImpl();
		Bool		TryAcquireImpl( TTimespec timeoutMs );
		void		ReleaseImpl( Int32 count );
	};

	class CConditionVariableImpl
	{
	private:
		CONDITION_VARIABLE			m_cond;

	protected:
		CConditionVariableImpl();
		~CConditionVariableImpl();

	protected:
		void WaitImpl( CMutexImpl& mutexImpl );

		void WakeAnyImpl();
		void WakeAllImpl();
	};

} } } // namespace Red { namespace Threads { namespace WinAPI {

//////////////////////////////////////////////////////////////////////////
// Thread object implementation
//////////////////////////////////////////////////////////////////////////
namespace Red { namespace Threads { namespace WinAPI {

	void YieldCurrentThreadImpl();
	void SleepOnCurrentThreadImpl( TTimespec sleepTimeInMS );

	class CThreadImpl
	{
	private:
		CThread*				m_debug;

	private:
		SThreadMemParams		m_memParams;

	private:
		HANDLE					m_thread;

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
		RED_INLINE Bool			IsValid() const { return m_thread != HANDLE(); }
	};

} } } // namespace Red { namespace Threads { namespace WinAPI {

#include "redThreadsThreadWinAPI.inl"

#endif // RED_THREADS_THREAD_WINAPI_H