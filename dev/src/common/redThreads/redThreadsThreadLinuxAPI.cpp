/**
* Copyright (c) 2018 CD Projekt Red. All Rights Reserved.
*/

#include "redThreadsPlatform.h"

#if defined( RED_THREADS_PLATFORM_LINUX_API )

#include "redThreadsThread.h"

#include "redThreadsThreadLinuxAPI.h"
#include <climits> // for PTHREAD_STACK_MIN

namespace Red { namespace Threads { namespace LinuxAPI {

	//  Or use sched_get_priority_max() etc
	static const Int32 LOWEST_PRIORITY = 1;
	static const Int32 DEFAULT_PRIORITY = 50;
	static const Int32 HIGHEST_PRIORITY = 99;

	const Int32		g_kThreadPriorityIdle = LOWEST_PRIORITY;
	const Int32		g_kThreadPriorityLowest = DEFAULT_PRIORITY - 20;
	const Int32		g_kThreadPriorityBelowNormal = DEFAULT_PRIORITY - 10;
	const Int32		g_kThreadPriorityNormal = DEFAULT_PRIORITY;
	const Int32		g_kThreadPriorityAboveNormal = DEFAULT_PRIORITY + 10;
	const Int32		g_kThreadPriorityHighest = DEFAULT_PRIORITY + 20;
	const Int32		g_kThreadPriorityTimeCritical = HIGHEST_PRIORITY;

	const Int32		g_kDefaultThreadSchedulingPolicy = SCHED_RR;
	const Int32		g_kDefaultThreadPriority = g_kThreadPriorityNormal;

	void InitializeFrameworkImpl( TAffinityMask mainThreadAffinityMask )
	{
		pthread_t mainThread = ::pthread_self();
		sched_param schedParam;
		schedParam.sched_priority = g_kDefaultThreadPriority;

		REDTHR_PTHREAD_CHECK( ::pthread_setschedparam( mainThread, g_kDefaultThreadSchedulingPolicy, &schedParam ) );

		//if ( mainThreadAffinityMask != 0 )
		//{
		//	// call ::pthread_setaffinity_np
		//	RED_ASSERT( mainThreadAffinityMask != 0, TXT( "InitializeFrameworkImpl: mainThreadAffinityMask != 0" ) );
		//}
	}

	void ShutdownFrameworkImpl()
	{
	}

	void YieldCurrentThreadImpl()
	{
		::pthread_yield();
	}

	void SleepOnCurrentThreadImpl( TTimespec sleepTimeInMS )
	{
		const TTimespec microPerMilli = 1000;
		REDTHR_PTHREAD_CHECK( ::usleep( sleepTimeInMS * microPerMilli ) );
	}

	void SetCurrentThreadAffinityImpl( TAffinityMask affinityMask )
	{
		if ( affinityMask != 0 )
		{
			cpu_set_t cpuset;
			CPU_ZERO( &cpuset );
			for ( Uint32 i = 0; i < sizeof( affinityMask ); ++i )
			{
				if ( ( affinityMask & ( 1ULL << i ) ) != 0 )
					CPU_SET( i, &cpuset );
			}
			REDTHR_PTHREAD_CHECK( ::pthread_setaffinity_np( pthread_self(), sizeof( cpuset ), &cpuset ) );
		}
	}

	void SetCurrentThreadNameImpl( const char* threadName )
	{
		const Uint32 LINUX_MAX_THREADBUF_SIZE = 16;
		AnsiChar truncatedThreadName[LINUX_MAX_THREADBUF_SIZE];
		Red::System::StringCopy( truncatedThreadName, threadName, LINUX_MAX_THREADBUF_SIZE );
		REDTHR_PTHREAD_CHECK( ::pthread_setname_np( pthread_self(), truncatedThreadName ) );
	}

	Uint32 GetMaxHardwareConcurrencyImpl()
	{
		// FIXME cores or threads?
		int numCores = sysconf( _SC_NPROCESSORS_ONLN );
		return numCores;
	}

	static void* ThreadEntryFunc( void* userData )
	{
		_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
		_MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);

		CThread* context = static_cast< CThread* >( userData );
		RED_ASSERT( context, TXT("Missing thread context") );

		if ( context )
		{
			SetCurrentThreadNameImpl( context->GetThreadName() );
			context->ThreadFunc();
		}

		return nullptr;
	}

	CThreadImpl::CThreadImpl( const SThreadMemParams& memParams )
		: m_memParams( memParams )
		, m_thread()
		, m_debug( nullptr )
	{
	}

	void CThreadImpl::InitThread( CThread* context )
	{
		RED_ASSERT( !IsValid(), TXT("Thread already created!") );
		if ( IsValid() )
		{
			return;
		}

		m_debug = context;

		const TStackSize stackSize = m_memParams.m_stackSize;

		RED_ASSERT( context, TXT("No thread context specified") );
		RED_ASSERT( m_memParams.m_stackSize >= PTHREAD_STACK_MIN, TXT("Stack size is too small") );

		pthread_attr_t attr;
		REDTHR_PTHREAD_CHECK( ::pthread_attr_init( &attr ) );

		// Stack params
		REDTHR_PTHREAD_CHECK( ::pthread_attr_setstacksize( &attr, stackSize ) );
		//REDTHR_PTHREAD_CHECK( ::pthread_attr_setguardsize( &attr, PAGE_SIZE ) );

		// Run settings
		sched_param schedParam;
		schedParam.sched_priority = g_kDefaultThreadPriority;
		//		REDTHR_PTHREAD_CHECK( ::pthread_attr_setinheritsched( &attr, PTHREAD_EXPLICIT_SCHED ) );
		// 		REDTHR_PTHREAD_CHECK( ::pthread_attr_setschedpolicy( &attr, g_kDefaultThreadSchedulingPolicy ) );
		// 		REDTHR_PTHREAD_CHECK( ::pthread_attr_setschedparam( &attr, &schedParam ) );
		//		REDTHR_PTHREAD_CHECK( ::pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_JOINABLE ) );

		// Create the thread
		REDTHR_PTHREAD_CHECK( ::pthread_create( &m_thread, &attr, ThreadEntryFunc, context ) );

		// Cleanup
		REDTHR_PTHREAD_CHECK( ::pthread_attr_destroy( &attr ) );
	}

	CThreadImpl::~CThreadImpl()
	{
		// Could detach the thread, but since ThreadFunc belongs to
		// Thread we're more likely in some messed up state.

		AnsiChar dbgBuf[256];
		Red::System::StringConvert( dbgBuf, m_debug ? m_debug->GetThreadName() : "<no context", 256 );

		RED_ASSERT( !IsValid(), TXT("Programmer error - manage thread lifetimes properly in thread '%s': thread object reached base destructor without a JoinThread() or DetachThread()"), dbgBuf );
	}

	void CThreadImpl::JoinThread()
	{
		REDTHR_ASSERT( IsValid() );
		if ( IsValid() )
		{
			REDTHR_PTHREAD_CHECK( ::pthread_join( m_thread, nullptr ) );
			m_thread = pthread_t();
		}
	}

	void CThreadImpl::DetachThread()
	{
		REDTHR_ASSERT( IsValid() );
		if ( IsValid() )
		{
			REDTHR_PTHREAD_CHECK( ::pthread_detach( m_thread ) );
			m_thread = pthread_t();
		}
	}

	void CThreadImpl::SetAffinityMask( Uint64 mask )
	{
		// Catch usage mistakes
		REDTHR_ASSERT( IsValid() );

		if ( IsValid() && mask != 0 )
		{
			cpu_set_t cpuset;
			CPU_ZERO( &cpuset );
			for ( Uint32 i = 0; i < sizeof( mask ); ++i )
			{
				if ( ( mask & ( 1ULL << i ) ) != 0 )
					CPU_SET( i, &cpuset );
			}
			REDTHR_PTHREAD_CHECK( ::pthread_setaffinity_np( m_thread, sizeof( cpuset ), &cpuset ) );
		}
	}

	void CThreadImpl::SetPriority( EThreadPriority priority )
	{
		REDTHR_ASSERT( IsValid() );
		if ( IsValid() )
		{
			//REDTHR_PTHREAD_CHECK( ::pthread_setschedprio( m_thread, priorityLUT[ priority ] ) );
		}
	}

	Bool CThreadImpl::operator==( const CThreadImpl& rhs ) const
	{
		REDTHR_ASSERT( IsValid() );
		if ( IsValid() )
		{
			return ::pthread_equal( m_thread, rhs.m_thread ) != 0;
		}
		return false;
	}

} } } // namespace Red { namespace Threads { namespace LinuxAPI {

#endif // RED_THREADS_PLATFORM_LINUX_API
