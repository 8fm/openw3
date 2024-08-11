/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#ifndef RED_THREADS_THREAD_INL
#define RED_THREADS_THREAD_INL
#pragma once

namespace Red { namespace Threads {

	//////////////////////////////////////////////////////////////////////////
	// Mutex implementation
	//////////////////////////////////////////////////////////////////////////
	inline CMutex::CMutex()
	{
	}

	inline CMutex::~CMutex()
	{
	}

	inline void CMutex::Acquire()
	{
		Base::AcquireImpl();
	}
	
	inline Bool CMutex::TryAcquire()
	{
		return Base::TryAcquireImpl();
	}

	inline void CMutex::Release()
	{
		Base::ReleaseImpl();
	}

	inline void CMutex::SetSpinCount( TSpinCount count )
	{
		Base::SetSpinCountImpl( count );
	}

	inline CNullMutex::CNullMutex()
	{
	}

	inline CNullMutex::~CNullMutex()
	{
	}

	inline void CNullMutex::Acquire()
	{

	}

	inline Bool CNullMutex::TryAcquire()
	{
		return true;
	}

	inline void CNullMutex::Release()
	{
	}

	inline void CNullMutex::SetSpinCount( TSpinCount count )
	{
		RED_UNUSED( count );
	}

	//////////////////////////////////////////////////////////////////////////
	// Scoped lock implementation
	//////////////////////////////////////////////////////////////////////////
	template < typename TAcquireRelease > inline CScopedLock< TAcquireRelease >::CScopedLock( TAcquireRelease& syncObject )
		: m_syncObjectRef( syncObject )
	{
		m_syncObjectRef.Acquire();
	}

	template < typename TAcquireRelease > inline CScopedLock< TAcquireRelease >::~CScopedLock()
	{
		m_syncObjectRef.Release();
	}

	//////////////////////////////////////////////////////////////////////////
	// Shared scoped lock implementation
	//////////////////////////////////////////////////////////////////////////
	template < typename TAcquireReleaseShared > inline CScopedSharedLock< TAcquireReleaseShared >::CScopedSharedLock( TAcquireReleaseShared& syncObject )
		: m_syncObjectRef( syncObject )
	{
		m_syncObjectRef.AcquireShared();
	}

	template < typename TAcquireReleaseShared > inline CScopedSharedLock< TAcquireReleaseShared >::~CScopedSharedLock()
	{
		m_syncObjectRef.ReleaseShared();
	}

	//////////////////////////////////////////////////////////////////////////
	// RWLock implementation
	//////////////////////////////////////////////////////////////////////////
	inline CRWLock::CRWLock()
	{
	}

	inline CRWLock::~CRWLock()
	{
	}

	inline void CRWLock::AcquireReadShared()
	{
		Base::AcquireReadSharedImpl();
	}

	inline void CRWLock::AcquireWriteExclusive()
	{
		Base::AcquireWriteExclusiveImpl();
	}

	inline void CRWLock::ReleaseReadShared()
	{
		Base::ReleaseReadSharedImpl();
	}

	inline void CRWLock::ReleaseWriteExclusive()
	{
		Base::ReleaseWriteExclusiveImpl();
	}

// 	inline Bool CRWLock::TryAcquireReadShared()
// 	{
// 		return Base::TryAcquireReadSharedImpl();
// 	}
// 
// 	inline Bool CRWLock::TryAcquireWriteExclusive()
// 	{
// 		return Base::TryAcquireWriteExclusiveImpl();
// 	}

	//////////////////////////////////////////////////////////////////////////
	// Semaphore implementation
	//////////////////////////////////////////////////////////////////////////
	inline CSemaphore::CSemaphore( Int32 initialCount, Int32 maximumCount )
		: Base( initialCount, maximumCount )
	{
	}

	inline void CSemaphore::Acquire()
	{
		Base::AcquireImpl();
	}

	inline Bool CSemaphore::TryAcquire( TTimespec timeoutMS /*=0*/ )
	{
		return Base::TryAcquireImpl( timeoutMS );
	}

	inline void CSemaphore::Release( Int32 count /*=1*/ )
	{
		Base::ReleaseImpl( count );
	}

	//////////////////////////////////////////////////////////////////////////
	// Condition variable implementation
	//////////////////////////////////////////////////////////////////////////
	inline CConditionVariable::CConditionVariable()
	{
	}

	inline CConditionVariable::~CConditionVariable()
	{
	}

	inline void CConditionVariable::Wait( CMutex& mutex )
	{
		return Base::WaitImpl( mutex );
	}

	inline void CConditionVariable::WakeAll()
	{
		Base::WakeAllImpl();
	}

	inline void CConditionVariable::WakeAny()
	{
		Base::WakeAnyImpl();
	}

	//////////////////////////////////////////////////////////////////////////
	// Basic spin lock implementation
	//////////////////////////////////////////////////////////////////////////
	inline CSpinLock::CSpinLock( const Bool isInitiallyAquired /*= false*/ )
		: m_spinLock( isInitiallyAquired ? 1 : 0 )
	{
	}

	inline Bool CSpinLock::TryAcquire()
	{
		Uint32 ret = AtomicOps::Exchange32( &m_spinLock, 1 );
		RED_THREADS_MEMORY_BARRIER(); // just for safety - may be not needed
		return (ret == 0);
	}

	inline void CSpinLock::Acquire()
	{
		for ( Uint32 iter = 0; !TryAcquire(); ++iter )
		{
			YieldThread( iter );
		}
	}

	inline void CSpinLock::Release()
	{
		*( Uint32 volatile* )( &m_spinLock ) = 0;
		RED_THREADS_MEMORY_BARRIER(); // just for safety - may be not needed
	}

	//////////////////////////////////////////////////////////////////////////
	// Basic light weight mutex implementation
	//////////////////////////////////////////////////////////////////////////

	inline CLightMutex::CLightMutex()
		: m_threadLock( 0 )
		, m_counter( 0 )
		, m_recursion( 0 )
		, m_spinLock( true ) // initially acquired!
	{}

	inline void CLightMutex::Acquire()
	{
		const Uint32 currentThreadID = GetCurrentThreadId();
		if ( AtomicOps::Increment32( &m_counter ) > 1 ) // recursive entry and/or second entry
		{
			// we are calling this from other thread
			if ( m_threadLock != currentThreadID )
			{
				m_spinLock.Acquire();
			}
		}

		// acquire lock
		m_threadLock = currentThreadID;
		++m_recursion;
	}

	inline void CLightMutex::Release()
	{
		// NOTE: we assume we own the lock when calling this function
		const int32_t recursion = --m_recursion;
		if ( 0 == recursion )
			m_threadLock = 0;

		// release the lock only if there was a distinct possibility we were waiting for it
		if ( AtomicOps::Decrement32( &m_counter ) > 0 )
		{
			// if recusrion==0 than we are still holding some locks but non on this thread
			if ( recursion == 0 )
				m_spinLock.Release();
		}
	}

	inline Uint32 CLightMutex::GetCurrentThreadId()
	{
#if defined( RED_PLATFORM_WIN32 ) || defined( RED_PLATFORM_WIN64 ) || defined( RED_PLATFORM_DURANGO )
		return static_cast< Uint32 >( ::GetCurrentThreadId() );
#elif defined( RED_PLATFORM_ORBIS )
		return static_cast< Uint32 >( ::scePthreadGetthreadid() );
#else
# error Unsupported platform!
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Thread object implementation
	//////////////////////////////////////////////////////////////////////////

	inline const AnsiChar* CThread::GetThreadName() const
	{
		return m_threadName;
	}

} } // namespace Red { namespace Threads {

#endif // RED_THREADS_THREAD_INL