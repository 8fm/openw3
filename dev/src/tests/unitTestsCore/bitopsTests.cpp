#include "build.h"

#include "../../common/redSystem/bitUtils.h"

TEST( BitOps, CLZ_ZeroMask )
{
	Uint32 leadingZeros = Red::System::BitUtils::CountLeadingZeros<Uint32>( 0 );
	EXPECT_EQ( 0u, leadingZeros );
}

TEST( BitOps, CLZ_FullMask )
{
	Uint32 leadingZeros = Red::System::BitUtils::CountLeadingZeros<Uint32>( 0xFFFFFFFFU );
	EXPECT_EQ( 0u, leadingZeros );
}

TEST( BitOps, CLZ_MSBZero )
{
	Uint32 leadingZeros = Red::System::BitUtils::CountLeadingZeros<Uint32>( 0x7FFFFFFFU );
	EXPECT_EQ( 1u, leadingZeros );
}

TEST( BitOps, CLZ_LSBZero )
{
	Uint32 leadingZeros = Red::System::BitUtils::CountLeadingZeros<Uint32>( 0xFFFFFFFEU );
	EXPECT_EQ( 0u, leadingZeros );
}

TEST( BitOps, CLZ_LowSet )
{
	Uint32 leadingZeros = Red::System::BitUtils::CountLeadingZeros<Uint32>( 0xFFFFU );
	EXPECT_EQ( 16u, leadingZeros );
}

TEST( BitOps, CLZ_Highet )
{
	Uint32 leadingZeros = Red::System::BitUtils::CountLeadingZeros<Uint32>( 0xFFFF0000U );
	EXPECT_EQ( 0u, leadingZeros );
}

TEST( BitOps, CLZ_LowSetPlusOneHigh ) // test an uneven amount of bits set
{
	Uint32 leadingZeros = Red::System::BitUtils::CountLeadingZeros<Uint32>( 0x1FFFFU );
	EXPECT_EQ( 15u, leadingZeros );
}

TEST( BitOps, CLZ_LowSetMinusOneHigh ) // test an uneven amount of bits set
{
	Uint32 leadingZeros = Red::System::BitUtils::CountLeadingZeros<Uint32>( 0x7FFFU );
	EXPECT_EQ( 17u, leadingZeros );
}

TEST( BitOps, CLZ_MaxPow2 )
{
	Uint32 leadingZeros = Red::System::BitUtils::CountLeadingZeros<Uint32>( 0x80000000U );
	EXPECT_EQ( 0u, leadingZeros );
}

TEST( BitOps, CLZ_One )
{
	Uint32 leadingZeros = Red::System::BitUtils::CountLeadingZeros<Uint32>( 1U );
	EXPECT_EQ( 31u, leadingZeros );
}

#ifdef RED_ARCH_X64
TEST( BitOps, CLZ_ZeroMaskx64 )
{
	Uint64 leadingZeros = Red::System::BitUtils::CountLeadingZeros<Uint64>( 0ULL );
	EXPECT_EQ( 0u, leadingZeros );
}

TEST( BitOps, CLZ_FullMaskx64 )
{
	Uint64 leadingZeros = Red::System::BitUtils::CountLeadingZeros<Uint64>( 0xFFFFFFFFFFFFFFFFULL );
	EXPECT_EQ( 0u, leadingZeros );
}

TEST( BitOps, CLZ_MSBZerox64 )
{
	Uint64 leadingZeros = Red::System::BitUtils::CountLeadingZeros<Uint64>( 0x7FFFFFFFFFFFFFFFULL );
	EXPECT_EQ( 1u, leadingZeros );
}

TEST( BitOps, CLZ_LSBZerox64 )
{
	Uint64 leadingZeros = Red::System::BitUtils::CountLeadingZeros<Uint64>( 0xFFFFFFFFFFFFFFFEULL );
	EXPECT_EQ( 0u, leadingZeros );
}

TEST( BitOps, CLZ_LowSetx64 )
{
	Uint64 leadingZeros = Red::System::BitUtils::CountLeadingZeros<Uint64>( 0xFFFFFFFFULL );
	EXPECT_EQ( 32u, leadingZeros );
}

TEST( BitOps, CLZ_Highetx64 )
{
	Uint64 leadingZeros = Red::System::BitUtils::CountLeadingZeros<Uint64>( 0xFFFFFFFF00000000ULL );
	EXPECT_EQ( 0u, leadingZeros );
}

TEST( BitOps, CLZ_LowSetPlusOneHighx64 ) // test an uneven amount of bits set
{
	Uint32 leadingZeros = static_cast< Uint32 >( Red::System::BitUtils::CountLeadingZeros<Uint64>( 0x1FFFFFFFFULL ) );
	EXPECT_EQ( 31u, leadingZeros );
}

TEST( BitOps, CLZ_LowSetMinusOneHighx64 ) // test an uneven amount of bits set
{
	Uint32 leadingZeros = static_cast< Uint32 >( Red::System::BitUtils::CountLeadingZeros<Uint64>( 0x7FFFFFFFULL ) );
	EXPECT_EQ( 33u, leadingZeros );
}

TEST( BitOps, CLZ_MaxPow2x64 )
{
	Uint32 leadingZeros = static_cast< Uint32 >( Red::System::BitUtils::CountLeadingZeros<Uint64>( 0x8000000000000000ULL ) );
	EXPECT_EQ( 0u, leadingZeros );
}

TEST( BitOps, CLZ_Onex64 )
{
	Uint32 leadingZeros = static_cast< Uint32 >( Red::System::BitUtils::CountLeadingZeros<Uint64>( 1ULL ) );
	EXPECT_EQ( 63u, leadingZeros );
}
#endif
