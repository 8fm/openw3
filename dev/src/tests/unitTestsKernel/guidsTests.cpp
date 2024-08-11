#include "build.h"

#include "../../common/redSystem/guid.h"

TEST( Guids, Creation )
{
	Red::System::GUID a;
	Red::System::GUID b( 1, 2, 3, 4 );

	// GUID creation is only valid on windows
#if defined( RED_PLATFORM_WIN32 ) || defined( RED_PLATFORM_WIN64 )
	Red::System::GUID c = Red::System::GUID::Create();
#endif

	EXPECT_TRUE( a != b );

#if defined( RED_PLATFORM_WIN32 ) || defined( RED_PLATFORM_WIN64 )
	EXPECT_TRUE( a != c );
	EXPECT_TRUE( b != c );
#endif

	EXPECT_TRUE( a.IsZero() );
	EXPECT_TRUE( a == Red::System::GUID::ZERO );
}

TEST( Guids, Copy )
{
	Red::System::Char buffer[ 1024 ];

#if defined( RED_PLATFORM_WIN32 ) || defined( RED_PLATFORM_WIN64 )
	Red::System::GUID a = Red::System::GUID::Create();
#else
	Red::System::GUID a( 1, 2, 3, 4 );
#endif

	a.ToString( buffer, ARRAY_COUNT( buffer ) );
	Red::System::GUID b = Red::System::GUID::Create( buffer );

	EXPECT_TRUE( a == b );
}
