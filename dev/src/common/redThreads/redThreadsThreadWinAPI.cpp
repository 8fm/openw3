/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "redThreadsPlatform.h"

#if defined( RED_THREADS_PLATFORM_WINDOWS_API )

#include "redThreadsThread.h"

#include "redThreadsThreadWinAPI.h"

#include <process.h>

namespace Red { namespace Threads { namespace WinAPI {

	static void SetThreadName( const AnsiChar* threadName );

	void InitializeFrameworkImpl( TAffinityMask mainThreadAffinityMask )
	{	
		if ( mainThreadAffinityMask != 0 )
		{
			REDTHR_WIN_CHECK( ::SetThreadAffinityMask( GetCurrentThread(), mainThreadAffinityMask ) );
		}

#if defined( RED_PLATFORM_DURANGO )
		// The "Main Thread" is already taken by the platform startup thread
		SetThreadName( "Game Thread");
#endif
	}

	void ShutdownFrameworkImpl()
	{
	}

	void YieldCurrentThreadImpl()
	{
		(void)::SwitchToThread();
	}

	void SleepOnCurrentThreadImpl( TTimespec sleepTimeInMS )
	{
		::Sleep( sleepTimeInMS );
	}

	static void SetThreadName( const AnsiChar* threadName )
	{
#pragma pack(push,8)
		typedef struct tagTHREADNAME_INFO
		{
			DWORD dwType;		// Must be 0x1000.
			LPCSTR szName;		// Pointer to name (in user addr space).
			DWORD dwThreadID;	// Thread ID (-1=caller thread).
			DWORD dwFlags;		// Reserved for future use, must be zero.
		} THREADNAME_INFO;
#pragma pack(pop)

		if ( ! threadName )
		{
			return;
		}

		// Some creepy Sleep, it was on MSDN too
		// DB: Was on MSDN, but now not. Seems rather dodgy all in all, so leaving it here for now.
		Sleep(10);

		THREADNAME_INFO info;
		info.dwType = 0x1000;
		info.szName = threadName;
		info.dwThreadID = (DWORD) -1;
		info.dwFlags = 0;

		__try
		{
			RaiseException( 0x406D1388, 0, sizeof(info)/sizeof(ULONG_PTR), (ULONG_PTR*)&info );
		}
		__except(EXCEPTION_EXECUTE_HANDLER)
		{
		}
	}

#ifdef RED_PLATFORM_DURANGO
	typedef DWORD TThreadRetval;
#else
	typedef Uint32 TThreadRetval;
#endif

	static TThreadRetval RED_STDCALL ThreadEntryFunc( void* userData )
	{
		CThread* context = static_cast< CThread* >( userData );
		RED_ASSERT( context, TXT("Missing thread context") );
		
		if ( context )
		{
			SetThreadName( context->GetThreadName() );
			context->ThreadFunc();
		}

		return 0;
	}

	namespace // anonymous
	{
		Int32 priorityLUT[] =
		{
			THREAD_PRIORITY_IDLE,
			THREAD_PRIORITY_LOWEST,
			THREAD_PRIORITY_BELOW_NORMAL,
			THREAD_PRIORITY_NORMAL,
			THREAD_PRIORITY_ABOVE_NORMAL,
			THREAD_PRIORITY_HIGHEST,
			THREAD_PRIORITY_TIME_CRITICAL,
		};
	}

	CThreadImpl::CThreadImpl( const SThreadMemParams& memParams )
		: m_memParams( memParams )
		, m_thread()
		, m_debug( nullptr )
	{
	}

	void CThreadImpl::InitThread( CThread* context )
	{
		RED_ASSERT( ! IsValid(), TXT("Thread already created!") );
		if ( IsValid() )
		{
			return;
		}

		m_debug = context;

		const TStackSize stackSize = m_memParams.m_stackSize;

		RED_ASSERT( context, TXT("No thread context specified") );
		const Uint32 flags = STACK_SIZE_PARAM_IS_A_RESERVATION;

#ifdef RED_PLATFORM_DURANGO
		// "There are some known issues with using CRT with CreateThread as you have noted in MSDN page. We are aware of this issue and will address it in future XDK releases."
		// https://forums.xboxlive.com/AnswerPage.aspx?qid=cb580b0c-4bc1-4fce-87fa-5efa3f165d6d&tgt=1
		m_thread = ::CreateThread( nullptr, stackSize, ThreadEntryFunc, context, flags, nullptr );
#else
		m_thread = reinterpret_cast< HANDLE >( ::_beginthreadex( nullptr, stackSize, ThreadEntryFunc, context, flags, nullptr ) );
#endif
		RED_ASSERT( m_thread, TXT("Failed to create thread") );

		Int32 priority = priorityLUT[ TP_Normal ];
		REDTHR_WIN_CHECK( ::SetThreadPriority( m_thread, priority ) );
	}

	CThreadImpl::~CThreadImpl()
	{
		// Could detach the thread, but since ThreadFunc belongs to
		// CThread we're more likely in some messed up state.

		Char dbgBuf[256];
		Red::System::StringConvert( dbgBuf, m_debug ? m_debug->GetThreadName() : "<no context", 256 );

		RED_ASSERT( !IsValid(), TXT("Programmer error - manage thread lifetimes properly in thread '%ls': thread object reached base destructor without a JoinThread() or DetachThread()"), dbgBuf );
	}

	void CThreadImpl::JoinThread()
	{
		REDTHR_ASSERT( IsValid() );
		if ( IsValid() )
		{
			const DWORD waitResult = ::WaitForSingleObject( m_thread, INFINITE );
			REDTHR_WIN_CHECK( waitResult != WAIT_FAILED ); // Get extended error info if applicable
			RED_VERIFY( waitResult == WAIT_OBJECT_0, TXT("Failed to wait for thread") );
			REDTHR_WIN_CHECK( ::CloseHandle( m_thread ) );
			m_thread = HANDLE();
		}
	}

	void CThreadImpl::DetachThread()
	{
		REDTHR_ASSERT( IsValid() );
		if ( IsValid() )
		{
			REDTHR_WIN_CHECK( ::CloseHandle( m_thread ) );
			m_thread = HANDLE();
		}
	}

	void CThreadImpl::SetAffinityMask( Uint64 mask )
	{
		REDTHR_ASSERT( IsValid() );
		if ( IsValid() )
		{
			REDTHR_WIN_CHECK( ::SetThreadAffinityMask( m_thread, static_cast< DWORD_PTR >( mask ) ) );		
		}
	}

	void CThreadImpl::SetPriority( EThreadPriority priority )
	{
		REDTHR_ASSERT( IsValid() );
		if ( IsValid() )
		{
			REDTHR_WIN_CHECK( ::SetThreadPriority( m_thread, priorityLUT[priority] ) );
		}
	}

	Bool CThreadImpl::operator==( const CThreadImpl& rhs ) const
	{
		REDTHR_ASSERT( IsValid() );
		if ( IsValid() )
		{
			return ::GetThreadId( m_thread ) == ::GetThreadId( rhs.m_thread );
		}
		return false;
	}

} } } // namespace Red { namespace Threads { namespace WinAPI {

#endif // RED_THREADS_PLATFORM_WINDOWS_API
