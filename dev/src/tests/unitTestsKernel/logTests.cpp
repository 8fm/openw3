/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#include "build.h"
#include "../../common/redSystem/log.h"
#include "logMock.h"

using namespace testing;

namespace Red
{
namespace System
{
namespace Log
{
	class OutputDeviceMock : public OutputDevice
	{
	public:
		MOCK_METHOD1( Write, void( const Message & message ) );
		MOCK_METHOD0( Flush, void() );
		MOCK_CONST_METHOD0( IsSafeToCallOnCrash, Bool() );
	};

	bool operator==( const Message & left, const Message & right )
	{
		return left.priority == right.priority
			&& left.channelHash == right.channelHash
			&& StringCompare( left.channelText, right.channelText ) == 0;
	}

	struct LogManagerFixture : ::testing::Test
	{
		virtual void SetUp()
		{
			m_globalInstance = &Manager::GetInstance();
			Manager::SetInternalInstance( &logMock );
		}

		virtual void TearDown()
		{
			Manager::SetInternalInstance( m_globalInstance );
		}

		LogMock logMock;
		Manager * m_globalInstance;
	};


	TEST_F( LogManagerFixture, Log_macro_do_not_write_if_is_not_enabled )
	{
		EXPECT_CALL( logMock, IsEnabled() )
			.Times( 1 )
			.WillOnce( Return( false ) );

		EXPECT_CALL( logMock, Write(_) )
			.Times( 0 );

		RED_LOG( RED_LOG_CHANNEL( UnitTest ), TXT( "This is a Test" ) );
	}

	TEST_F( LogManagerFixture, Log_macro_do_write_if_is_enabled )
	{
		EXPECT_CALL( logMock, IsEnabled() )
			.Times( 1 )
			.WillOnce( Return( true ) );

		Message message;
		message.priority = P_Information;
		message.channelHash = Red::CNameHash( "UnitTest" );
		message.channelText = TXT( "UnitTest" );

		EXPECT_CALL( logMock, Write( message ) )
			.Times( 1 );

		RED_LOG( RED_LOG_CHANNEL( UnitTest ), TXT( "This is a Test" ) );
	}

	TEST_F( LogManagerFixture, Log_Error_macro_do_not_write_if_is_not_enabled )
	{
		EXPECT_CALL( logMock, IsEnabled() )
			.Times( 1 )
			.WillOnce( Return( false ) );

		EXPECT_CALL( logMock, Write(_) )
			.Times( 0 );

		RED_LOG_ERROR( RED_LOG_CHANNEL( UnitTest ), TXT( "This is a Test" ) );
	}

	TEST_F( LogManagerFixture, Log_Error_macro_do_write_if_is_enabled )
	{
		EXPECT_CALL( logMock, IsEnabled() )
			.Times( 1 )
			.WillOnce( Return( true ) );

		Message message;
		message.priority = P_Error;
		message.channelHash = Red::CNameHash( "UnitTest" );
		message.channelText = TXT( "UnitTest" );

		EXPECT_CALL( logMock, Write( message ) )
			.Times( 1 );

		RED_LOG_ERROR( RED_LOG_CHANNEL( UnitTest ), TXT( "This is a Test" ) );
	}

	TEST_F( LogManagerFixture, Log_Warning_macro_do_not_write_if_is_not_enabled )
	{
		EXPECT_CALL( logMock, IsEnabled() )
			.Times( 1 )
			.WillOnce( Return( false ) );

		EXPECT_CALL( logMock, Write(_) )
			.Times( 0 );

		RED_LOG_WARNING( RED_LOG_CHANNEL( UnitTest ), TXT( "This is a Test" ) );
	}

	TEST_F( LogManagerFixture, Log_Warning_macro_do_write_if_is_enabled )
	{
		EXPECT_CALL( logMock, IsEnabled() )
			.Times( 1 )
			.WillOnce( Return( true ) );

		Message message;
		message.priority = P_Warning;
		message.channelHash = Red::CNameHash( "UnitTest" );
		message.channelText = TXT( "UnitTest" );

		EXPECT_CALL( logMock, Write( message ) )
			.Times( 1 );

		RED_LOG_WARNING( RED_LOG_CHANNEL( UnitTest ), TXT( "This is a Test" ) );
	}

	TEST_F( LogManagerFixture, Log_Spam_macro_do_not_write_if_is_not_enabled )
	{
		EXPECT_CALL( logMock, IsEnabled() )
			.Times( 1 )
			.WillOnce( Return( false ) );

		EXPECT_CALL( logMock, Write(_) )
			.Times( 0 );

		RED_LOG_SPAM( RED_LOG_CHANNEL( UnitTest ), TXT( "This is a Test" ) );
	}

	TEST_F( LogManagerFixture, Log_Spam_macro_do_write_if_is_enabled )
	{
		EXPECT_CALL( logMock, IsEnabled() )
			.Times( 1 )
			.WillOnce( Return( true ) );

		Message message;
		message.priority = P_Spam;
		message.channelHash = Red::CNameHash( "UnitTest" );
		message.channelText = TXT( "UnitTest" );

		EXPECT_CALL( logMock, Write( message ) )
			.Times( 1 );

		RED_LOG_SPAM( RED_LOG_CHANNEL( UnitTest ), TXT( "This is a Test" ) );
	}

	struct LogFixture : testing::Test
	{
		LogFixture()
			: m_globalInstance( nullptr )
			, outputDeviceMock_1( nullptr )
			, outputDeviceMock_2( nullptr )
		{
			message.priority = P_Information;
			message.channelHash = Red::CNameHash( "UnitTest" );
			message.channelText = TXT( "UnitTest" );
		}

		virtual void SetUp()
		{
			m_globalInstance = &Manager::GetInstance();
			Manager::SetInternalInstance( &logManager );
		}

		virtual void TearDown()
		{
			delete outputDeviceMock_1;
			delete outputDeviceMock_2;
			
			Manager::SetInternalInstance( m_globalInstance );
		}

		void CreateAndRegisterDevice_1() 
		{
			// base class constructor call Log singleton Register function. Yeah I know ... this suck
			outputDeviceMock_1 = new OutputDeviceMock();
		}

		void CreateAndRegisterDevice_2()
		{
			// base class constructor call Log singleton Register function. Yeah I know ... this suck
			outputDeviceMock_2 = new OutputDeviceMock();
		}

		Manager * m_globalInstance;
		Manager logManager;
		OutputDeviceMock * outputDeviceMock_1;
		OutputDeviceMock * outputDeviceMock_2;
		Message message;
	};

	TEST( Log, Write_do_nothing_if_no_device_registered )
	{
		Manager logManager;
		EXPECT_NO_FATAL_FAILURE( logManager.Write( Message() ) );
	}

	TEST( Log, Flush_do_nothing_if_no_device_registered )
	{
		Manager logManager;
		EXPECT_NO_FATAL_FAILURE( logManager.Flush() );
	}

	TEST_F( LogFixture, Flush_call_Flush_on_single_registered_OutputDevice )
	{
		CreateAndRegisterDevice_1();

		EXPECT_CALL( *outputDeviceMock_1, Flush() )
			.Times( 1 );

		logManager.Flush();
	}

	TEST_F( LogFixture, Flush_do_not_call_Flush_on_single_unregistered_OutputDevice )
	{
		CreateAndRegisterDevice_1();

		logManager.Unregister( outputDeviceMock_1 );

		EXPECT_CALL( *outputDeviceMock_1, Flush() )
			.Times( 0 );

		logManager.Flush();
	}

	TEST_F( LogFixture, Flush_call_Flush_on_multiple_registered_OutputDevice )
	{
		CreateAndRegisterDevice_1();
		CreateAndRegisterDevice_2();

		EXPECT_CALL( *outputDeviceMock_1, Flush() )
			.Times( 1 );

		EXPECT_CALL( *outputDeviceMock_2, Flush() )
			.Times( 1 );

		logManager.Flush();
	}
	
	TEST_F( LogFixture, Flush_do_not_call_Flush_on_unregistered_OutputDevice )
	{
		CreateAndRegisterDevice_1();
		CreateAndRegisterDevice_2();

		logManager.Unregister( outputDeviceMock_1 );

		EXPECT_CALL( *outputDeviceMock_1, Flush() )
			.Times( 0 );

		EXPECT_CALL( *outputDeviceMock_2, Flush() )
			.Times( 1 );

		logManager.Flush();
	}

	TEST_F( LogFixture, Write_call_write_on_single_registered_OutputDevice )
	{
		CreateAndRegisterDevice_1();

		EXPECT_CALL( *outputDeviceMock_1, Write( message ) )
			.Times( 1 );

		logManager.Write( message );
	}

	TEST_F( LogFixture, Write_do_not_call_write_if_all_unregistered_OutputDevice )
	{
		CreateAndRegisterDevice_1();
		logManager.Unregister( outputDeviceMock_1 );

		EXPECT_CALL( *outputDeviceMock_1, Write( message ) )
			.Times( 0 );

		logManager.Write( message );
	}

	TEST_F( LogFixture, Write_call_write_on_multiple_registered_OutputDevice )
	{
		CreateAndRegisterDevice_1();
		CreateAndRegisterDevice_2();

		EXPECT_CALL( *outputDeviceMock_1, Write( message ) )
			.Times( 1 );

		EXPECT_CALL( *outputDeviceMock_2, Write( message ) )
			.Times( 1 );

		logManager.Write( message );
	}

	TEST_F( LogFixture, Write_do_not_call_write_on_unregistered_OutputDevice )
	{
		CreateAndRegisterDevice_1();
		CreateAndRegisterDevice_2();

		logManager.Unregister( outputDeviceMock_1 );

		EXPECT_CALL( *outputDeviceMock_1, Write( message ) )
			.Times( 0 );

		EXPECT_CALL( *outputDeviceMock_2, Write( message ) )
			.Times( 1 );

		logManager.Write( message );
	}

	TEST_F( LogFixture, Write_do_call_write_on_OutputDevice_if_in_crash_mode_and_safe )
	{
		CreateAndRegisterDevice_1();

		EXPECT_CALL( *outputDeviceMock_1, IsSafeToCallOnCrash() )
			.Times( 1 )
			.WillOnce( Return( true ) );

		EXPECT_CALL( *outputDeviceMock_1, Write( message ) )
			.Times( 1 );

		logManager.SetCrashModeActive( true );
		logManager.Write( message );
	}

	TEST_F( LogFixture, Write_do_not_call_write_on_OutputDevice_if_in_crash_mode_but_not_safe )
	{
		CreateAndRegisterDevice_1();

		EXPECT_CALL( *outputDeviceMock_1, IsSafeToCallOnCrash() )
			.Times( 1 )
			.WillOnce( Return( false ) );

		EXPECT_CALL( *outputDeviceMock_1, Write( message ) )
			.Times( 0 );

		logManager.SetCrashModeActive( true );
		logManager.Write( message );
	}
}
}
}
