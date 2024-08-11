/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#ifndef _RED_MEMORY_FRAMEWORK_THREADS_H
#define _RED_MEMORY_FRAMEWORK_THREADS_H
#pragma once

#include "redMemoryFrameworkPlatform.h"
#include "redMemoryThreadsNullAPI.h"

#ifdef RED_MEMORY_FRAMEWORK_PLATFORM_WINDOWS_API
	#include "redMemoryThreadsWinAPI.h"
#elif defined ( RED_MEMORY_FRAMEWORK_PLATFORM_ORBIS_API )
	#include "redMemoryThreadsOrbis.h"
#elif defined( RED_MEMORY_FRAMEWORK_PLATFORM_DURANGO_API )
	#include "redMemoryThreadsDurango.h"
#endif

#include "redMemoryAtomics.h"

namespace Red { namespace MemoryFramework {

///////////////////////////////////////////////////////////////////
// Mutex interface
template < typename TMutexImpl >
class CMutexBase : public TMutexImpl, protected Red::System::NonCopyable
{
public:
	typedef typename TMutexImpl::TScopedLock TScopedLock;
};

///////////////////////////////////////////////////////////////////
// Adaptive mutex interface
template < typename TAdaptiveMutexImpl >
class CAdaptiveMutexBase : public TAdaptiveMutexImpl, protected Red::System::NonCopyable
{
public:
	typedef typename TAdaptiveMutexImpl::TScopedLock TScopedLock;
};

///////////////////////////////////////////////////////////////////
// Mutex implementations
typedef CMutexBase< OSAPI::CMutexImpl > CMutex;
typedef CMutexBase< NullAPI::CMutexImpl > CNullMutex;

#ifdef RED_PLATFORM_ORBIS
	typedef CAdaptiveMutexBase< OSAPI::CAdaptiveMutexImpl > CAdaptiveMutex;
#endif

///////////////////////////////////////////////////////////////////
// Spinlock is common to all platforms
class CSpinlock
{
public:
	RED_INLINE CSpinlock() : m_lock( 0 ) { }
	RED_INLINE void Acquire()
	{
		for ( Uint32 iter = 0; !TryAcquire(); ++iter )
		{
			if( iter > c_minimumIterationsBeforeYield )
			{
				OSAPI::YieldThread();
			}
		}
	}
	RED_INLINE void Release()
	{
		*( Uint32 volatile* )( &m_lock ) = 0;
		OSAPI::FullMemoryBarrier();
	}

	class CScopedLockImpl
	{
	private:
		CSpinlock*				m_spinlock;
	public:
		CScopedLockImpl( CSpinlock *spinlock ) : m_spinlock( spinlock )	{ spinlock->Acquire(); }
		~CScopedLockImpl()	{ m_spinlock->Release(); m_spinlock = nullptr; }
	};
	typedef CScopedLockImpl TScopedLock;

private:
	RED_INLINE Bool TryAcquire()
	{
		Uint32 ret = OSAPI::AtomicExchange32( &m_lock, 1 );
		return (ret == 0);
	}

#ifdef RED_PLATFORM_ORBIS
	int32_t __attribute__((aligned (4) )) m_lock;
#else
	__declspec(align(4)) uint32_t m_lock;
#endif
	static const Uint32 c_minimumIterationsBeforeYield = 16;
};

} } // namespace Red { namespace MemoryFramework {

#endif // RED_MEMORY_FRAMEWORK_THREADS_H