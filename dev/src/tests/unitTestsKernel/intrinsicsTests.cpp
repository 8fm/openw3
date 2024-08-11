#include "build.h"
#include <cmath>
#include "../../common/redSystem/bitUtils.h"

#ifdef RED_PLATFORM_WIN64
	#define LITERAL_64( x )		x ## ull	// 64 bit int on win64 is unsigned long long
#elif defined RED_PLATFORM_ORBIS
	#define LITERAL_64( x )		x ## ul		// 64 bit int on Orbis is unsigned long
#elif defined RED_PLATFORM_DURANGO
	#define LITERAL_64( x )		x ## ull	
#else
	#define LITERAL_64( x )		x
#endif


TEST( Intrinsics, BSR )
{
	Red::System::Uint32 zero32 = 0u;
	EXPECT_TRUE( Red::System::BitUtils::BitScanReverse( zero32 ) == 0u );
	for( Red::System::Uint32 bit=0; bit<31; ++bit )
	{
		Red::System::Uint32 testValue = 1u << bit;
		EXPECT_TRUE( Red::System::BitUtils::BitScanReverse( testValue ) == bit );
	}
	EXPECT_TRUE( Red::System::BitUtils::BitScanReverse( 30u ) == 4u );
		

#ifdef RED_ARCH_X64
	Red::System::Uint64 zero64 = 0u;
	EXPECT_TRUE( Red::System::BitUtils::BitScanReverse( zero64 ) == 0u );
	for( Red::System::Uint64 bit=0; bit<63; ++bit )
	{
		Red::System::Uint64 testValue = 1ull << bit;
		EXPECT_TRUE( Red::System::BitUtils::BitScanReverse( testValue ) == bit );
	}

	EXPECT_TRUE( Red::System::BitUtils::BitScanReverse( LITERAL_64( 0x0000000000000110 ) ) == 8u );
#endif
}

TEST( Intrinsics, BSF )
{
	Red::System::Uint32 zero32 = 0u;
	EXPECT_TRUE( Red::System::BitUtils::BitScanForward( zero32 ) == 0u );
	for( Red::System::Uint32 bit=0; bit<31; ++bit )
	{
		Red::System::Uint32 testValue = 1u << bit;
		EXPECT_TRUE( Red::System::BitUtils::BitScanForward( testValue ) == bit );
	}
	EXPECT_TRUE( Red::System::BitUtils::BitScanForward( 30u ) == 1u );

#ifdef RED_ARCH_X64
	Red::System::Uint64 zero64 = 0u;
	EXPECT_TRUE( Red::System::BitUtils::BitScanForward( zero64 ) == 0u );
	for( Red::System::Uint64 bit=0; bit<63; ++bit )
	{
		Red::System::Uint64 testValue = 1ull << bit;
		EXPECT_TRUE( Red::System::BitUtils::BitScanForward( testValue ) == bit );
	}
	EXPECT_TRUE( Red::System::BitUtils::BitScanForward( LITERAL_64( 0x0000000000000110 ) ) == 4u );
#endif
}

TEST( Intrinsics, FastLog2 )
{
#define CHECK_LOG_2(x)		{																			\
							size_t l2slow = (size_t)floor( log((double)x) / log((double)2 ) );		\
							size_t l2fast = (size_t)Red::System::BitUtils::Log2( x );				\
							EXPECT_EQ( l2slow, l2fast );												\
						}
		
	CHECK_LOG_2(5u);
	CHECK_LOG_2(542u);
	CHECK_LOG_2(845312u);
	CHECK_LOG_2(84021u);
	CHECK_LOG_2(1u);
	CHECK_LOG_2(3574u);
	CHECK_LOG_2(512u);
	CHECK_LOG_2(266546179u);
	CHECK_LOG_2(((Red::System::Uint32)-1));
#ifdef RED_ARCH_X64
	CHECK_LOG_2(LITERAL_64(2380938509385));
	CHECK_LOG_2(LITERAL_64(235798575));
	CHECK_LOG_2(LITERAL_64(583280705985));
	CHECK_LOG_2(LITERAL_64(385629862195));
	CHECK_LOG_2(LITERAL_64(7622755));
	CHECK_LOG_2(LITERAL_64(620162952362));
	CHECK_LOG_2(LITERAL_64(2872169));
	CHECK_LOG_2(LITERAL_64(75));
#endif
}
