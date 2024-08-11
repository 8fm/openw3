#include "build.h"

#include "../../common/redSystem/types.h"
#include "../../common/redSystem/timer.h"
#include "../../common/redSystem/hash.h"
#include "../../common/redSystem/utilityTimers.h"

TEST( Types, Sizes )
{
	EXPECT_TRUE( sizeof( Red::System::Int8 )		== 1 );
	EXPECT_TRUE( sizeof( Red::System::Uint8 )		== 1 );
	EXPECT_TRUE( sizeof( Red::System::Int16 )		== 2 );
	EXPECT_TRUE( sizeof( Red::System::Uint16 )	== 2 );
	EXPECT_TRUE( sizeof( Red::System::Int32 )		== 4 );
	EXPECT_TRUE( sizeof( Red::System::Uint32 )	== 4 );
	EXPECT_TRUE( sizeof( Red::System::Int64 )		== 8 );
	EXPECT_TRUE( sizeof( Red::System::Uint64 )	== 8 );

	EXPECT_TRUE( sizeof( Red::System::Float )		== 4 );
	EXPECT_TRUE( sizeof( Red::System::Double )	== 8 );

	EXPECT_TRUE( sizeof( Red::System::UniChar )	== 2 );
	EXPECT_TRUE( sizeof( Red::System::AnsiChar )	== 1 );
}

TEST( Types, Timers )
{
	//Pretty much check to see that they compile and run
	Red::System::Double testDelta = 0.0;
	{
		Red::System::ScopedStopClock timerA( testDelta );

		Red::System::StopClock timerB;

		for( Red::System::Int32 i = 0; i < 64; ++i )
		{
			testDelta = timerB.GetDelta();
		}

		// Not much that we can do other than check to make sure the value was set
		EXPECT_TRUE( testDelta != 0.0 );
			
		// Reset for next check
		testDelta = 0.0f;
	}

	EXPECT_TRUE( testDelta != 0.0 );
}

TEST( Types, DateTimeUTC )
{
	Red::System::DateTime dt;
	Red::System::Clock::GetInstance().GetUTCTime( dt );

	EXPECT_TRUE( dt.GetRaw() != 0u );
	EXPECT_TRUE( dt.GetYear() < 9999u );
	EXPECT_TRUE( dt.GetMonth() < 12u );
	EXPECT_TRUE( dt.GetDay() < 31u );
	EXPECT_TRUE( dt.GetHour() < 24u );
	EXPECT_TRUE( dt.GetMinute() < 60u );
	EXPECT_TRUE( dt.GetSecond() < 60u );
	EXPECT_TRUE( dt.GetMilliSeconds() < 1000u );
}

TEST( Types, DateTimeLocal )
{
	Red::System::DateTime dt;
	Red::System::Clock::GetInstance().GetLocalTime( dt );

	EXPECT_TRUE( dt.GetRaw() != 0u );
	EXPECT_TRUE( dt.GetYear() < 9999u );
	EXPECT_TRUE( dt.GetMonth() < 12u );
	EXPECT_TRUE( dt.GetDay() < 31u );
	EXPECT_TRUE( dt.GetHour() < 24u );
	EXPECT_TRUE( dt.GetMinute() < 60u );
	EXPECT_TRUE( dt.GetSecond() < 60u );
	EXPECT_TRUE( dt.GetMilliSeconds() < 1000u );
}

TEST( Types, Hash )
{
	EXPECT_TRUE( Red::System::CompileTimeHash32( "" ).m_hash == Red::System::CompileTimeHash32( "" ).m_hash );
	EXPECT_TRUE( Red::System::CompileTimeHash32( "" ).m_hash == Red::System::CalculateHash32( "" ) );

	EXPECT_TRUE( Red::System::CompileTimeHash32( "strangeness befalls a summers day in January" ).m_hash == Red::System::CompileTimeHash32( "strangeness befalls a summers day in January" ).m_hash );
	EXPECT_TRUE( Red::System::CompileTimeHash32( "strangeness befalls a summers day in January" ).m_hash == Red::System::CalculateHash32( "strangeness befalls a summers day in January" ) );

	EXPECT_TRUE( Red::System::CompileTimeHash32( "1234567890" ).m_hash != Red::System::CompileTimeHash32( "abcdefghijk" ).m_hash );
	EXPECT_TRUE( Red::System::CompileTimeHash32( "1234567890" ).m_hash != Red::System::CalculateHash32( "abcdefghijk" ) );
}

TEST( Types, PrintfFormats )
{
	Red::System::AnsiChar testBuffer[64] = {'\0'};
	Red::System::Int16 testI16 =	32661;
	Red::System::Uint16 testU16 =	65346u;
	Red::System::Int64 testI64 =	9223372036854774008ll;
	Red::System::Uint64 testU64 =	18446744073709546569ull;
#ifdef RED_ARCH_X86
	Red::System::MemSize testSize_t = 4294962209u;
#elif defined( RED_ARCH_X64 )
	Red::System::MemSize testSize_t = 1099511618025ull;
#endif

	Red::System::SNPrintF( testBuffer, ARRAY_COUNT( testBuffer ), "%" RED_PRId16, testI16 );
	EXPECT_TRUE( Red::System::StringCompare( testBuffer, "32661" ) == 0 );

	Red::System::SNPrintF( testBuffer, ARRAY_COUNT( testBuffer ), "%" RED_PRIx16, testI16 );
	EXPECT_TRUE( Red::System::StringCompare( testBuffer, "7f95" ) == 0 );

	Red::System::SNPrintF( testBuffer, ARRAY_COUNT( testBuffer ), "%" RED_PRIu16, testU16 );
	EXPECT_TRUE( Red::System::StringCompare( testBuffer, "65346" ) == 0 );


	Red::System::SNPrintF( testBuffer, ARRAY_COUNT( testBuffer ), "%" RED_PRId64, testI64 );
	EXPECT_TRUE( Red::System::StringCompare( testBuffer, "9223372036854774008" ) == 0 );

	Red::System::SNPrintF( testBuffer, ARRAY_COUNT( testBuffer ), "%" RED_PRIx64, testI64 );
	EXPECT_TRUE( Red::System::StringCompare( testBuffer, "7ffffffffffff8f8" ) == 0 );

	Red::System::SNPrintF( testBuffer, ARRAY_COUNT( testBuffer ), "%" RED_PRIu64, testU64 );
	EXPECT_TRUE( Red::System::StringCompare( testBuffer, "18446744073709546569" ) == 0 );


	Red::System::SNPrintF( testBuffer, ARRAY_COUNT( testBuffer ), "%" RED_PRIsize_t, testSize_t );
#ifdef RED_ARCH_X86
	EXPECT_TRUE( Red::System::StringCompare( testBuffer, "4294962209" ) == 0 );
#elif defined( RED_ARCH_X64 )
	EXPECT_TRUE( Red::System::StringCompare( testBuffer, "1099511618025" ) == 0 );
#endif
		
	Red::System::SNPrintF( testBuffer, ARRAY_COUNT( testBuffer ), "%" RED_PRIxsize_t, testSize_t );
#ifdef RED_ARCH_X86
	EXPECT_TRUE( Red::System::StringCompare( testBuffer, "ffffec21" ) == 0 );
#elif defined( RED_ARCH_X64 )
	EXPECT_TRUE( Red::System::StringCompare( testBuffer, "ffffffd9e9" ) == 0 );
#endif
		
}
