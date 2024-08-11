/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#include "build.h"
#include "../../common/redSystem/error.h" 
#include "logMock.h"

using namespace Red::System;
using namespace testing;

namespace Red
{
namespace System
{
namespace Error
{
	class ErrorHandlerMock : public Red::System::Error::Handler
	{
	public:
		MOCK_METHOD1( WriteDump, Bool(const Char*filename) );
		MOCK_METHOD3( GetCallstack, Uint32 ( Char* output, Uint32 outputSize, Uint32 skipFrames  ) );
		MOCK_METHOD2( GetCallstack, void(  Callstack& stack, Uint32 skipFrames ) );
		MOCK_METHOD3( GetCallstack, void( Callstack& stack, const Internal::ThreadId& threadId, Uint32 skipFrames ) );
		MOCK_METHOD4( GetCallstack, Uint32( Char* output, Uint32 outputSize, const Internal::ThreadId& threadId, Uint32 skipFrames ) );
		MOCK_METHOD2( EnumerateThreads, Uint32 ( Internal::ThreadId* threadIds, Uint32 maxThreads ) );
		MOCK_METHOD5( HandleUserChoice, void ( EAssertAction chosenAction, const Char* cppFile, Uint32 line, const Char* expression, const Char* details ) );
		MOCK_CONST_METHOD0( GetCommandline, const Char*() );
		MOCK_CONST_METHOD0( IsDebuggerPresent, Bool() );
		
		MOCK_METHOD6( GetDebugInformation, Uint32 ( Char* output, Uint32 outputSize, Uint32 flags, Uint32 skipCurrentThreadFrames, CNameHash callbackTag, const Internal::ThreadId& specifiedThread ) );

		MOCK_METHOD5( Assert, EAssertAction ( const Char * message, const Char* cppFile, Uint32 line, const Char* expression, Uint32 skipFrames ) );
	};

#ifdef RED_PLATFORM_WINPC
	LONG WINAPI HandleException( EXCEPTION_POINTERS* )
	{
		return EXCEPTION_EXECUTE_HANDLER;
	}
#endif

	struct ErrorFixture : ::testing::Test
	{
		virtual void SetUp()
		{
			m_globalErrorHandler = Error::Handler::GetInstance();
			m_globalLogManager = &Log::Manager::GetInstance();
			Error::Handler::SetInternalInstance( &errorHandlerMock );	
			Log::Manager::SetInternalInstance( &logMock );
			DefaultValue< EAssertAction >::Set(AA_Break);
		#ifdef RED_PLATFORM_WINPC
			exceptionHandler = SetUnhandledExceptionFilter( &HandleException );
		#endif
		}

		virtual void TearDown()
		{
			Error::Handler::SetInternalInstance( m_globalErrorHandler );
			Log::Manager::SetInternalInstance( m_globalLogManager );
		#ifdef RED_PLATFORM_WINPC
			SetUnhandledExceptionFilter( exceptionHandler );
		#endif	
		}

		void Boom()
		{
			RED_FATAL_ASSERT( 0, "I'm Dead" );
		}

		void NoBoom()
		{
			RED_FATAL_ASSERT( 1, "I'm Alive" );
		}

		ErrorHandlerMock errorHandlerMock;
		LogMock logMock;

		Error::Handler * m_globalErrorHandler;
		Log::Manager * m_globalLogManager;

	#ifdef RED_PLATFORM_WINPC
		LPTOP_LEVEL_EXCEPTION_FILTER exceptionHandler;
	#endif
	};


	TEST_F( ErrorFixture, Assert_False_condition_break_process )
	{
		EXPECT_DEATH_IF_SUPPORTED( Boom(), "" );
	}

	TEST_F( ErrorFixture, Assert_false_condition_handle_info_through_Error_Handler )
	{

#if GTEST_HAS_DEATH_TEST
		if( ::testing::internal::InDeathTestChild() )
		{
			EXPECT_CALL( errorHandlerMock, Assert(_,_,_,_,_) )
				.Times(1)
				.WillOnce( Return( AA_Break ) );
		}
#endif 

		EXPECT_DEATH_IF_SUPPORTED( Boom(), "" );
	}

	TEST_F( ErrorFixture, Assert_True_condition_do_not_break_process )
	{
		EXPECT_CALL( errorHandlerMock, Assert(_,_,_,_,_) )
			.Times( 0 );

		NoBoom();
	}

	TEST_F( ErrorFixture, Warning_true_condition_do_not_write_to_log )
	{
		EXPECT_CALL( logMock, IsEnabled() )
			.Times( 0 );

		EXPECT_NO_FATAL_FAILURE( RED_WARNING( 1, "No Warning here!" ) );
	}

	TEST_F( ErrorFixture, Warning_false_condition_do_not_write_to_log_if_log_is_not_enabled )
	{
		EXPECT_CALL( errorHandlerMock, GetDebugInformation( _, _, _, _, _, _ ) )
			.Times( 1 );

		EXPECT_CALL( logMock, IsEnabled() )
			.Times( 1 )
			.WillOnce( Return( false ) );

		EXPECT_CALL( logMock, Write(_) )
			.Times( 0 );

		EXPECT_NO_FATAL_FAILURE( RED_WARNING( 0, "Warning here!" ) );
	}

	TEST_F( ErrorFixture, Warning_false_condition_do_write_to_log_if_log_is_enabled )
	{
		EXPECT_CALL( errorHandlerMock, GetDebugInformation( _, _, _, _, _, _ ) )
			.Times( 1 );

		EXPECT_CALL( logMock, IsEnabled() )
			.Times( 1 )
			.WillOnce( Return( true ) );

		EXPECT_CALL( logMock, Write(_) )
			.Times( 1 );

		EXPECT_NO_FATAL_FAILURE( RED_WARNING( 0, "Warning here!" ) );
	}

	TEST_F( ErrorFixture, Warning_Once_true_condition_do_not_write_to_log )
	{
		EXPECT_CALL( logMock, IsEnabled() )
			.Times( 0 );

		EXPECT_NO_FATAL_FAILURE( RED_WARNING_ONCE( 1, "No Warning here!" ) );
	}

	TEST_F( ErrorFixture, Warning_Once_false_condition_do_write_to_log )
	{
		EXPECT_CALL( logMock, IsEnabled() )
			.Times( 1 )
			.WillOnce( Return( true ) );

		EXPECT_CALL( logMock, Write(_) )
			.Times( 1 );

		EXPECT_NO_FATAL_FAILURE( RED_WARNING_ONCE( 0, "Warning here!" ) );
	}

	TEST_F( ErrorFixture, Warning_Once_false_condition_multiple_time_write_only_once_to_log )
	{
		EXPECT_CALL( logMock, IsEnabled() )
			.Times( 1 )
			.WillOnce( Return( true ) );

		EXPECT_CALL( logMock, Write(_) )
			.Times( 1 );

		for( int count = 0, end = 3; count != end; ++count )
		{
			EXPECT_NO_FATAL_FAILURE( RED_WARNING_ONCE( 0, "Warning here!" ) );
		}		
	}
}
}
}