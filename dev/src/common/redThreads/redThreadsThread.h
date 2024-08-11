/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#ifndef RED_THREADS_THREAD_H
#define RED_THREADS_THREAD_H
#pragma once

#include "redThreadsPlatform.h"
#include "redThreadsCommon.h"
#include "redThreadsAtomic.h"
#include "redThreadsRedSystem.h"

namespace Red { namespace Threads {

//////////////////////////////////////////////////////////////////////////
// Thread initialization parameters
//////////////////////////////////////////////////////////////////////////
struct SThreadMemParams
{
	TStackSize m_stackSize;

	SThreadMemParams( TStackSize stackSize = OSAPI::g_kDefaultSpawnedThreadStackSize );
};

} }  // namespace Red { namespace Threads {

//////////////////////////////////////////////////////////////////////////
// Namespace functions
//////////////////////////////////////////////////////////////////////////
namespace Red { namespace Threads {

	void InitializeFramework( TAffinityMask mainThreadAffinityMask = 0 );
	void ShutdownFramework();

} } // namespace Red { namespace Threads {

//////////////////////////////////////////////////////////////////////////
// Synchronization object decls
//////////////////////////////////////////////////////////////////////////
#if defined( RED_THREADS_PLATFORM_WINDOWS_API )
#	include "redThreadsThreadWinAPI.h"
#elif defined( RED_THREADS_PLATFORM_ORBIS_API )
#	include "redThreadsThreadOrbisAPI.h"
#else
#	error No thread implementation for platform
#endif

namespace Red { namespace Threads {

	template< typename TAcquireRelease >
	class CScopedLock
	{
		REDTHR_NOCOPY_CLASS( CScopedLock );

	private:
		TAcquireRelease& m_syncObjectRef;

	public:
		CScopedLock( TAcquireRelease& syncObject );
		~CScopedLock();
	};

	template< typename TAcquireReleaseShared >
	class CScopedSharedLock
	{
		REDTHR_NOCOPY_CLASS( CScopedSharedLock );

	private:
		TAcquireReleaseShared& m_syncObjectRef;

	public:
		CScopedSharedLock( TAcquireReleaseShared& syncObject );
		~CScopedSharedLock();
	};

	class CMutex : private OSAPI::CMutexImpl
	{
		REDTHR_NOCOPY_CLASS( CMutex );

		friend class CConditionVariable;

	private:
		typedef OSAPI::CMutexImpl Base;

	public:
		CMutex();
		~CMutex();

	public:
		void Acquire();
		Bool TryAcquire();
		void Release();
		void SetSpinCount( TSpinCount count );
	};

	// NOTE: not reentrant
	class CSpinLock
	{
		REDTHR_NOCOPY_CLASS( CSpinLock );

	public:
		CSpinLock( const Bool isInitiallyAquired = false );

		Bool TryAcquire();
		void Acquire();
		void Release();

	private:
		AtomicOps::TAtomic32	m_spinLock;

		static void YieldThread( Uint32 iter );
	};

	// Cheap light weight mutex, sizeof() = 16, can yield
	class CLightMutex
	{
		REDTHR_NOCOPY_CLASS( CLightMutex );

	public:
		CLightMutex();

		void Acquire();
		void Release();

	private:
		AtomicOps::TAtomic32	m_threadLock;
		AtomicOps::TAtomic32	m_counter;
		Uint32					m_recursion;
		CSpinLock				m_spinLock;

		static Uint32 GetCurrentThreadId();
	};

	class CNullMutex
	{
		REDTHR_NOCOPY_CLASS( CNullMutex );

	public:
		CNullMutex();
		~CNullMutex();

	public:
		void Acquire();
		Bool TryAcquire();
		void Release();
		void SetSpinCount( TSpinCount count );
	};

	// Note: Don't use recursively or upgrade locks from read to write (or vice versa) - thin API wrapper at the moment.
	class CRWLock : private OSAPI::CRWLockImpl
	{
		REDTHR_NOCOPY_CLASS( CRWLock );
		
	private:
		typedef OSAPI::CRWLockImpl Base;

	public:
		CRWLock();
		~CRWLock();

	public:
		void Acquire() { AcquireWriteExclusive(); }
		void Release() { ReleaseWriteExclusive(); }
		void AcquireShared() { AcquireReadShared(); }
		void ReleaseShared() { ReleaseReadShared(); }

		void AcquireReadShared();
		void AcquireWriteExclusive();

		void ReleaseReadShared();
		void ReleaseWriteExclusive();

//		Not supported on Vista
// 		Bool TryAcquireReadShared();
// 		Bool TryAcquireWriteExclusive();
	};

	class CSemaphore : private OSAPI::CSemaphoreImpl
	{
		REDTHR_NOCOPY_CLASS( CSemaphore );

	private:
		typedef OSAPI::CSemaphoreImpl Base;

	public:
		CSemaphore( Int32 initialCount, Int32 maximumCount );

	public:
		void		Acquire();
		Bool		TryAcquire( TTimespec timeoutMs = 0 );
		void		Release( Int32 count = 1);
	};

	class CConditionVariable : private OSAPI::CConditionVariableImpl
	{
		REDTHR_NOCOPY_CLASS( CConditionVariable );

	private:
		typedef OSAPI::CConditionVariableImpl Base;

	public:
		CConditionVariable();
		~CConditionVariable();

	public:
		void Wait( CMutex& mutex );
		void WakeAll();
		void WakeAny();
	};

} } // namespace Red { namespace Threads {

//////////////////////////////////////////////////////////////////////////
// Thread object decl
//////////////////////////////////////////////////////////////////////////
namespace Red { namespace Threads {

	const size_t g_kMaxThreadNameLength = 31; //!< Maximum supported debugger thread name length (exluding a null terminator).

	void YieldCurrentThread();
	void SleepOnCurrentThread( TTimespec sleepTimeInMS );

	class CThread
	{
		REDTHR_NOCOPY_CLASS( CThread );

	private:
		typedef OSAPI::CThreadImpl CThreadImpl;

	private:
		CThreadImpl			m_threadImpl;
		AnsiChar			m_threadName[ g_kMaxThreadNameLength + 1 ];

	public:
							CThread( const AnsiChar* threadName, const SThreadMemParams& memParams = SThreadMemParams() );
 		virtual				~CThread();
		void				JoinThread();
		void				DetachThread();

		// Not called from the constructor since ThreadFunc is virtual.
		void				InitThread();

	public:
		virtual void		ThreadFunc() = 0;

	public:
		const AnsiChar*		GetThreadName() const;

	public:
		void				SetAffinityMask( TAffinityMask mask );
		void				SetPriority( EThreadPriority priority );

	public:
		Bool				operator==( const CThread& rhs ) const;
	};

} } // namespace Red { namespace Threads {

#if defined( RED_THREADS_PLATFORM_WINDOWS_API )
#	include "redThreadsThreadWinAPI.inl"
#elif defined( RED_THREADS_PLATFORM_ORBIS_API )
#	include "redThreadsThreadOrbisAPI.inl"
#else
#	error No thread implementation for platform
#endif

#include "redThreadsThread.inl"

#endif // RED_THREADS_THREAD_H