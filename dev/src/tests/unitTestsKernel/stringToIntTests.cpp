#include "build.h"

#include "../../common/redSystem/types.h"
#include "../../common/redSystem/crt.h"
#include "../../common/redSystem/numericalLimits.h"

#include <errno.h>

#include <limits>

#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

TEST( StringToIntegerConversions, NumericLimits )
{
	EXPECT_EQ( std::numeric_limits< Red::System::Int32 >::min(), Red::System::NumericLimits< Red::System::Int32 >::Min() );
	EXPECT_EQ( std::numeric_limits< Red::System::Int32 >::max(), Red::System::NumericLimits< Red::System::Int32 >::Max() );
	EXPECT_EQ( std::numeric_limits< Red::System::Uint32 >::min(), Red::System::NumericLimits< Red::System::Uint32 >::Min() );
	EXPECT_EQ( std::numeric_limits< Red::System::Uint32 >::max(), Red::System::NumericLimits< Red::System::Uint32 >::Max() );
}

TEST( StringToIntegerConversions, WideConvertZero )
{
	Red::System::Int32 result = 1;
	EXPECT_TRUE( Red::System::StringToInt( result, L"0", nullptr, Red::System::BaseTen ) );

	EXPECT_EQ( 0, result );
}

TEST( StringToIntegerConversions, WideConvertZeroNegSign )
{
	Red::System::Int32 result = 1;
	EXPECT_TRUE( Red::System::StringToInt( result, L"-0", nullptr, Red::System::BaseTen ) );

	EXPECT_EQ( 0, result );
}

TEST( StringToIntegerConversions, WideConvertZeroPosSign )
{
	Red::System::Int32 result = 1;
	EXPECT_TRUE( Red::System::StringToInt( result, L"+0", nullptr, Red::System::BaseTen ) );

	EXPECT_EQ( 0, result );
}

TEST( StringToIntegerConversions, WideConvertInt32Min )
{
	Red::System::Int32 result = 1;
	EXPECT_TRUE( Red::System::StringToInt( result, L"-2147483648", nullptr, Red::System::BaseTen ) );

	EXPECT_EQ( Red::System::NumericLimits< Red::System::Int32 >::Min(), result );
}

TEST( StringToIntegerConversions, WideConvertInt32Max )
{
	Red::System::Int32 result = 1;
	EXPECT_TRUE( Red::System::StringToInt( result, L"2147483647", nullptr, Red::System::BaseTen ) );

	EXPECT_EQ( Red::System::NumericLimits< Red::System::Int32 >::Max(), result );
}

//////////////////////////////////////////////////////////////////////////

TEST( StringToIntegerConversions, AnsiConvertZero )
{
	Red::System::Int32 result = 1;
	EXPECT_TRUE( Red::System::StringToInt( result, "0", nullptr, Red::System::BaseTen ) );

	EXPECT_EQ( 0, result );
}

TEST( StringToIntegerConversions, AnsiConvertZeroNegSign )
{
	Red::System::Int32 result = 1;
	EXPECT_TRUE( Red::System::StringToInt( result, "-0", nullptr, Red::System::BaseTen ) );

	EXPECT_EQ( 0, result );
}

TEST( StringToIntegerConversions, AnsiConvertZeroPosSign )
{
	Red::System::Int32 result = 1;
	EXPECT_TRUE( Red::System::StringToInt( result, "+0", nullptr, Red::System::BaseTen ) );

	EXPECT_EQ( 0, result );
}

TEST( StringToIntegerConversions, AnsiConvertInt32Min )
{
	Red::System::Int32 result = 1;
	EXPECT_TRUE( Red::System::StringToInt( result, "-2147483648", nullptr, Red::System::BaseTen ) );

	EXPECT_EQ( Red::System::NumericLimits< Red::System::Int32 >::Min(), result );
}

TEST( StringToIntegerConversions, AnsiConvertInt32Max )
{
	Red::System::Int32 result = 1;
	EXPECT_TRUE( Red::System::StringToInt( result, "2147483647", nullptr, Red::System::BaseTen ) );

	EXPECT_EQ( Red::System::NumericLimits< Red::System::Int32 >::Max(), result );
}

//////////////////////////////////////////////////////////////////////////

TEST( StringToIntegerConversions, WideConvertInt32Overflow )
{
	Red::System::Int32 result = 1;
	EXPECT_TRUE( !Red::System::StringToInt( result, L"9223372036854775807", nullptr, Red::System::BaseTen ) );

	EXPECT_EQ( ERANGE, errno );
	EXPECT_EQ( Red::System::NumericLimits< Red::System::Int32 >::Max(), result );
}

TEST( StringToIntegerConversions, WideConvertInt32Underflow )
{
	Red::System::Int32 result = 1;
	EXPECT_TRUE( !Red::System::StringToInt( result, L"-9223372036854775808", nullptr, Red::System::BaseTen ) );

	EXPECT_EQ( ERANGE, errno );
	EXPECT_EQ( Red::System::NumericLimits< Red::System::Int32 >::Min(), result );
}

TEST( StringToIntegerConversions, WideConvertUint32Overflow )
{
	Red::System::Uint32 result = 1;
	EXPECT_TRUE( !Red::System::StringToInt( result, L"0xFFFFFFFFFFFFFFFF", nullptr, Red::System::BaseSixteen ) );

	EXPECT_EQ( ERANGE, errno );
	EXPECT_EQ( Red::System::NumericLimits< Red::System::Uint32 >::Max(), result );
}

//////////////////////////////////////////////////////////////////////////

TEST( StringToIntegerConversions, AnsiConvertInt32Overflow )
{
	Red::System::Int32 result = 1;
	EXPECT_TRUE( !Red::System::StringToInt( result, "9223372036854775807", nullptr, Red::System::BaseTen ) );

	EXPECT_EQ( ERANGE, errno );
	EXPECT_EQ( Red::System::NumericLimits< Red::System::Int32 >::Max(), result );
}

TEST( StringToIntegerConversions, AnsiConvertInt32Underflow )
{
	Red::System::Int32 result = 1;
	EXPECT_TRUE( !Red::System::StringToInt( result, "-9223372036854775808", nullptr, Red::System::BaseTen ) );

	EXPECT_EQ( ERANGE, errno );
	EXPECT_EQ( Red::System::NumericLimits< Red::System::Int32 >::Min(), result );
}

TEST( StringToIntegerConversions, AnsiConvertUint32Overflow )
{
	Red::System::Uint32 result = 1;
	EXPECT_TRUE( !Red::System::StringToInt( result, "0xFFFFFFFFFFFFFFFF", nullptr, Red::System::BaseSixteen ) );

	EXPECT_EQ( ERANGE, errno );
	EXPECT_EQ( Red::System::NumericLimits< Red::System::Uint32 >::Max(), result );
}

//////////////////////////////////////////////////////////////////////////

TEST( StringToIntegerConversions, WideConvertNoInt32 )
{
	Red::System::Int32 result = 1;
	EXPECT_TRUE( Red::System::StringToInt( result, L"abc", nullptr, Red::System::BaseTen ) );

	EXPECT_EQ( 0, result );
	EXPECT_EQ( 0, errno );
}

TEST( StringToIntegerConversions, AnsiConvertNoInt32 )
{
	Red::System::Int32 result = 1;
	EXPECT_TRUE( Red::System::StringToInt( result, "abc", nullptr, Red::System::BaseTen ) );

	EXPECT_EQ( 0, result );
	EXPECT_EQ( 0, errno );
}

//////////////////////////////////////////////////////////////////////////

TEST( StringToIntegerConversions, WideConvertInt32LeadingWhitespace )
{
	Red::System::Int32 result = 1;
	EXPECT_TRUE( Red::System::StringToInt( result, L"     123", nullptr, Red::System::BaseTen ) );

	EXPECT_EQ( 123, result );
	EXPECT_EQ( 0, errno );
}

TEST( StringToIntegerConversions, WideConvertInt32TrailingWhitespace )
{
	Red::System::Int32 result = 1;
	EXPECT_TRUE( Red::System::StringToInt( result, L"123    ", nullptr, Red::System::BaseTen ) );

	EXPECT_EQ( 123, result );
	EXPECT_EQ( 0, errno );
}

TEST( StringToIntegerConversions, WideConvertInt32LeadingTrailingWhitespace )
{
	Red::System::Int32 result = 1;
	EXPECT_TRUE( Red::System::StringToInt( result, L"    123    ", nullptr, Red::System::BaseTen ) );

	EXPECT_EQ( 123, result );
	EXPECT_EQ( 0, errno );
}

TEST( StringToIntegerConversions, WideConvertInt32WhitespaceOther )
{
	Red::System::Int32 result = 1;
	EXPECT_TRUE( Red::System::StringToInt( result, L"\n\t\v\r 123 \n\t\v\r  ", nullptr, Red::System::BaseTen ) );

	EXPECT_EQ( 123, result );
	EXPECT_EQ( 0, errno );
}


TEST( StringToIntegerConversions, WideConvertInt32TrailingNonNumbers )
{
	Red::System::Int32 result = 1;
	EXPECT_TRUE( Red::System::StringToInt( result, L"123abc", nullptr, Red::System::BaseTen ) );

	EXPECT_EQ( 123, result );
	EXPECT_EQ( 0, errno );
}

TEST( StringToIntegerConversions, WideConvertInt32TrailingNonNumbersHex )
{
	Red::System::Int32 result = 1;
	EXPECT_TRUE( Red::System::StringToInt( result, L"0x123abcZabba", nullptr, Red::System::BaseSixteen ) );

	EXPECT_EQ( 0x123abc, result );
	EXPECT_EQ( 0, errno );
}

TEST( StringToIntegerConversions, WideConvertIn32LeadingNonNumbers )
{
	Red::System::Int32 result = 1;
	EXPECT_TRUE( Red::System::StringToInt( result, L"zzz123", nullptr, Red::System::BaseTen ) );

	EXPECT_EQ( 0, result );
	EXPECT_EQ( 0, errno );
}

TEST( StringToIntegerConversions, WideConvertInt32PaddedZero )
{
	Red::System::Int32 result = 1;
	EXPECT_TRUE( Red::System::StringToInt( result, L"000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000", nullptr, Red::System::BaseTen ) );

	EXPECT_EQ( 0, result );
	EXPECT_EQ( 0, errno );
}

TEST( StringToIntegerConversions, WideConvertInt32PaddedZeroOtherSimple )
{
	Red::System::Int32 result = 1;
	EXPECT_TRUE( Red::System::StringToInt( result, L"0123", nullptr, Red::System::BaseTen ) );

	EXPECT_EQ( 123, result );
	EXPECT_EQ( 0, errno );
}

TEST( StringToIntegerConversions, WideConvertInt32PaddedZeroOther )
{
	Red::System::Int32 result = 1;
	EXPECT_TRUE( Red::System::StringToInt( result, L"000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000123", nullptr, Red::System::BaseTen ) );

	EXPECT_EQ( 123, result );
	EXPECT_EQ( 0, errno );
}

