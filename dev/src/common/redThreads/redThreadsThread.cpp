/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "redThreadsThread.h"

#include "../redSystem/crt.h"

namespace Red { namespace Threads {

	void InitializeFramework( TAffinityMask mainThreadAffinityMask /*= 0*/ )
	{
		OSAPI::InitializeFrameworkImpl( mainThreadAffinityMask );
	}

	void ShutdownFramework()
	{
		OSAPI::ShutdownFrameworkImpl();
	}

	void YieldCurrentThread()
	{
		OSAPI::YieldCurrentThreadImpl();
	}

	void SleepOnCurrentThread( TTimespec sleepTimeInMS )
	{
		OSAPI::SleepOnCurrentThreadImpl( sleepTimeInMS );
	}

	SThreadMemParams::SThreadMemParams( TStackSize stackSize /*= OSAPI::g_kDefaultSpawnedThreadStackSize */ )
		: m_stackSize( stackSize )
	{
	}

	void CSpinLock::YieldThread( Uint32 iter )
	{
		if ( iter < 16 )
		{
			/* do nothing - spin lock */
		}
		else
		{
			YieldCurrentThread();
		}
	}
	
	CThread::CThread( const AnsiChar* threadName, 
					  const SThreadMemParams& memParams /*= SThreadMemParams()*/ )
		: m_threadImpl( memParams )
	{
		RED_UNUSED( threadName );
		const size_t nameBufSize = sizeof(m_threadName)/sizeof(m_threadName[0]); // buffer has +1 size for NULL terminator
		RED_ASSERT( threadName, TXT("Thread name cannot be NULL.") );
		RED_ASSERT( Red::System::StringLength(threadName) <= g_kMaxThreadNameLength, TXT("Thread name is larger than common OS supported size") );
		Red::System::StringCopy( m_threadName, threadName, nameBufSize );
	}

	CThread::~CThread()
	{
	}

	void CThread::JoinThread()
	{
		m_threadImpl.JoinThread();
	}

	void CThread::DetachThread()
	{
		m_threadImpl.DetachThread();
	}

	void CThread::InitThread()
	{
		m_threadImpl.InitThread( this );
	}

	void CThread::SetAffinityMask( TAffinityMask mask )
	{
		RED_UNUSED( mask );
		m_threadImpl.SetAffinityMask( mask );
	}

	void CThread::SetPriority( EThreadPriority priority )
	{
		m_threadImpl.SetPriority( priority );
	}

	Bool CThread::operator==( const CThread& rhs ) const
	{
		return m_threadImpl == rhs.m_threadImpl;
	}

} } // namespace Red { namespace Threads {
