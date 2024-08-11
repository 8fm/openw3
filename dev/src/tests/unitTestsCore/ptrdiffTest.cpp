#include "build.h"

#include "../../common/core/util.h"

	// Doesn't handle pointers outside the 32-bit range, only to be used on ptrdiffs.

TEST( PtrDiff, PtrToUint32_Zero )
{
	void* p = nullptr;

	Uint32 i = PtrDiffToUint32( p );
	EXPECT_EQ( 0u, i ); // Technically doesn't have to be zero, but let's at least test the assumption that it is.
}

TEST( PtrDiff, PtrToUint32_One )
{
	void* p = reinterpret_cast< void* >( 0x1u );
	Uint32 i = PtrDiffToUint32( p );
	EXPECT_EQ( 0x1u, i );
}

TEST( PtrDiff, PtrToUint32_0xffffffff )
{
	void* p = reinterpret_cast< void* >( 0xffffffffu );
	Uint32 i = PtrDiffToUint32( p );
	EXPECT_EQ( 0xffffffffu, i );
}

TEST( PtrDiff, PtrToUint32_0xdeadbeef )
{
	void* p = reinterpret_cast< void* >( 0xdeadbeefu );
	Uint32 i = PtrDiffToUint32( p );
	EXPECT_EQ( 0xdeadbeefu, i );
}

TEST( PtrDiff, PtrToInt32_Zero )
{
	void* p = nullptr;

	Int32 i = PtrDiffToInt32( p );
	EXPECT_EQ( 0, i ); // Technically doesn't have to be zero, but let's at least test the assumption that it is.
}
TEST( PtrDiff, PtrToInt32_One )
{
	void* p = reinterpret_cast< void* >( 0x1 );
	Int32 i = PtrDiffToInt32( p );
	EXPECT_EQ( 0x1, i );
}

TEST( PtrDiff, PtrToInt32_0xffffffff )
{
	void* p = reinterpret_cast< void* >( 0xffffffff );
	Int32 i = PtrDiffToUint32( p );
	EXPECT_EQ( -1, i );
}

TEST( PtrDiff, PtrToInt32_0xdeadbeef )
{
	void* p = reinterpret_cast< void* >( 0xdeadbeef );
	Uint32 i = PtrDiffToUint32( p );
	EXPECT_EQ( 0xdeadbeef, i );
}

