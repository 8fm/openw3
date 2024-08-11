/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "redThreadsPlatform.h"

#if defined( RED_THREADS_PLATFORM_ORBIS_API )

#include "redThreadsThread.h"
#include "redThreadsThreadOrbisAPI.h"

#include <pthread_np.h>

namespace Red { namespace Threads { namespace OrbisAPI {

	// Priority *numerical values* are inverted. E.g., default-10 has higher priority than default+10.
	const Int32		g_kThreadPriorityIdle = SCE_KERNEL_PRIO_FIFO_LOWEST;
	const Int32		g_kThreadPriorityLowest = SCE_KERNEL_PRIO_FIFO_DEFAULT + 20;
	const Int32		g_kThreadPriorityBelowNormal = SCE_KERNEL_PRIO_FIFO_DEFAULT + 10;
	const Int32		g_kThreadPriorityNormal = SCE_KERNEL_PRIO_FIFO_DEFAULT;
	const Int32		g_kThreadPriorityAboveNormal = SCE_KERNEL_PRIO_FIFO_DEFAULT - 10;
	const Int32		g_kThreadPriorityHighest = SCE_KERNEL_PRIO_FIFO_DEFAULT - 20;
	const Int32		g_kThreadPriorityTimeCritical = SCE_KERNEL_PRIO_FIFO_HIGHEST;

	const Int32		g_kDefaultThreadSchedulingPolicy = SCE_KERNEL_SCHED_RR;
	const Int32		g_kDefaultThreadPriority = g_kThreadPriorityNormal;

	void InitializeFrameworkImpl( TAffinityMask mainThreadAffinityMask )
	{
		ScePthread mainThread = ::scePthreadSelf();
		SceKernelSchedParam schedParam;
		schedParam.sched_priority = g_kDefaultThreadPriority;

		// As of SDK 0.920 the main thread's inherited policy is SCE_KERNEL_SCHED_FIFO, so we change it here to our desired default.
		REDTHR_SCE_CHECK( ::scePthreadSetschedparam( mainThread, g_kDefaultThreadSchedulingPolicy, &schedParam ) );

		if ( mainThreadAffinityMask != 0 )
		{
			REDTHR_SCE_CHECK( ::scePthreadSetaffinity( mainThread, mainThreadAffinityMask ) );
		}
	}

	void ShutdownFrameworkImpl()
	{
	}

	void YieldCurrentThreadImpl()
	{
		::scePthreadYield();
	}

	void SleepOnCurrentThreadImpl( TTimespec sleepTimeInMS )
	{
		const TTimespec microPerMilli = 1000;
		REDTHR_SCE_CHECK( ::sceKernelUsleep( sleepTimeInMS * microPerMilli ) );
	}

	static void* ThreadEntryFunc( void *userData )
	{
		CThread* context = static_cast< CThread* >( userData );
		RED_ASSERT( context, TXT("Missing thread context") );
		if ( context )
		{
			context->ThreadFunc();
		}

		return nullptr;
	}

	namespace // anonymous
	{
		Int32 priorityLUT[] =
		{
			g_kThreadPriorityIdle,
			g_kThreadPriorityLowest,
			g_kThreadPriorityBelowNormal,
			g_kThreadPriorityNormal,
			g_kThreadPriorityAboveNormal,
			g_kThreadPriorityHighest,
			g_kThreadPriorityTimeCritical,
		};
	}

	CThreadImpl::CThreadImpl( const SThreadMemParams& memParams )
		: m_memParams( memParams )
		, m_thread()
	{
	}

	void CThreadImpl::InitThread( CThread* context )
	{
		const TStackSize stackSize = m_memParams.m_stackSize;

		RED_ASSERT( context, TXT("No thread context specified") );
		RED_ASSERT( m_memParams.m_stackSize >= PTHREAD_STACK_MIN, TXT("Stack size is too small") );

		ScePthreadAttr attr;
		REDTHR_SCE_CHECK( ::scePthreadAttrInit( &attr ) );

		// Stack params
		//////////////////////////////////////////////////////////////////////////
		REDTHR_SCE_CHECK( ::scePthreadAttrSetstacksize( &attr, stackSize ) );
		//REDTHR_SCE_CHECK( ::scePthreadAttrSetguardsize( &attr, PAGE_SIZE ) );

		// Run settings
		//////////////////////////////////////////////////////////////////////////
		SceKernelSchedParam schedParam;
		schedParam.sched_priority = g_kDefaultThreadPriority;
		REDTHR_SCE_CHECK( ::scePthreadAttrSetinheritsched( &attr, SCE_PTHREAD_EXPLICIT_SCHED ) );
		REDTHR_SCE_CHECK( ::scePthreadAttrSetschedpolicy( &attr, g_kDefaultThreadSchedulingPolicy ) );
		REDTHR_SCE_CHECK( ::scePthreadAttrSetschedparam( &attr, &schedParam ) );
		REDTHR_SCE_CHECK( ::scePthreadAttrSetdetachstate( &attr, SCE_PTHREAD_CREATE_JOINABLE ) );

		// Create the thread
		//////////////////////////////////////////////////////////////////////////
		const AnsiChar* threadName = context->GetThreadName();
		REDTHR_SCE_CHECK( ::scePthreadCreate( &m_thread, &attr, ThreadEntryFunc, context, threadName ) );

		// Cleanup
		//////////////////////////////////////////////////////////////////////////
		REDTHR_SCE_CHECK( ::scePthreadAttrDestroy( &attr ) );
	}

	CThreadImpl::~CThreadImpl()
	{
		// Could detach the thread, but since ThreadFunc belongs to
		// CThread we're more likely in some messed up state.
		RED_ASSERT( !IsValid(), TXT("Programmer error - manage thread lifetimes properly: thread object reached base destructor without a JoinThread() or DetachThread()") );
	}

	void CThreadImpl::JoinThread()
	{
		RED_ASSERT( IsValid() );
		if ( IsValid() )
		{
			REDTHR_SCE_CHECK( ::scePthreadJoin( m_thread, nullptr ) );
			m_thread = ScePthread();
		}
	}

	void CThreadImpl::DetachThread()
	{
		RED_ASSERT( IsValid() );
		if ( IsValid() )
		{
			REDTHR_SCE_CHECK( ::scePthreadDetach( m_thread ) );
			m_thread = ScePthread();
		}
	}

	void CThreadImpl::SetAffinityMask( TAffinityMask mask )
	{
		RED_ASSERT( IsValid() );
		if ( IsValid() )
		{
			REDTHR_SCE_CHECK( ::scePthreadSetaffinity( m_thread, static_cast< SceKernelCpumask >( mask ) ) );
		}
	}

	void CThreadImpl::SetPriority( EThreadPriority priority )
	{
		RED_ASSERT( IsValid() );
		if ( IsValid() )
		{
			REDTHR_SCE_CHECK( ::scePthreadSetprio( m_thread, priorityLUT[ priority ] ) );
		}
	}

	Bool CThreadImpl::operator==( const CThreadImpl& rhs ) const
	{
		RED_ASSERT( IsValid() );
		if ( IsValid() )
		{
			return ::scePthreadEqual( m_thread, rhs.m_thread ) != 0;
		}
		return false;
	}

} } } // namespace Red { namespace Threads { namespace OrbisAPI {

#endif // RED_THREADS_PLATFORM_ORBIS_API
