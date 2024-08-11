#include "build.h"

#include "../../common/redMath/random/random.h"
#include "../../common/redMath/random/standardRand.h"

using namespace Red::System::Random;
using namespace Red::Math::Random;
using namespace Red::System;

Generator< StandardRand > m_generator;

//////////////////////////////////////////////////////////////////////////
// Standard Rand

template< typename TRandType >
void CheckRand( int iterations )
{
	for( int i = 0 ; i < iterations; ++i )
	{
		TRandType value = m_generator.Get< TRandType >();

		// When requesting a range, value will be 0 <= result <= RAND_MAX
		EXPECT_LE( value, m_generator.Max< TRandType >() );
	}
}

template< typename TRandType >
void CheckRand( int iterations, TRandType max )
{
	for( int i = 0 ; i < iterations; ++i )
	{
		TRandType value = m_generator.Get< TRandType >( max );

		// When requesting a range, value will be 0 <= result < max
		EXPECT_LT( value, max );
	}
}

template< typename TRandType >
void CheckRand( int iterations, TRandType max, TRandType min )
{
	for( int i = 0 ; i < iterations; ++i )
	{
		TRandType result = m_generator.Get< TRandType >( min, max );

		// When requesting a range, value will be min <= result < max
		EXPECT_LT( result, max );
		EXPECT_GE( result, min );
	}
}

//////////////////////////////////////////////////////////////////////////
// Uint8

TEST( Random, StandardRandUint8 )
{
	CheckRand< Uint8 >( 1024 );
}

TEST( Random, StandardRandUint8Max )
{
	CheckRand< Uint8 >( 1024, 50 );
}

TEST( Random, StandardRandUint8RangeMaxMin )
{
	CheckRand< Uint8 >( 1024, 100, 50 );
}

//////////////////////////////////////////////////////////////////////////
// Uint16

TEST( Random, StandardRandUint16 )
{
	CheckRand< Uint16 >( 1024 );
}

TEST( Random, StandardRandUint16Max )
{
	CheckRand< Uint16 >( 1024, 512 );
}

TEST( Random, StandardRandUint16RangeMaxMin )
{
	CheckRand< Uint16 >( 1024, 1024, 512 );
}

//////////////////////////////////////////////////////////////////////////
// Uint32

TEST( Random, StandardRandUint32 )
{
	CheckRand< Uint32 >( 2048 );
}

TEST( Random, StandardRandUint32Max )
{
	CheckRand< Uint32 >( 2048, 1024 );
}

TEST( Random, StandardRandUint32RangeMaxMin )
{
	CheckRand< Uint32 >( 2048, 2048, 1024 );
}

//////////////////////////////////////////////////////////////////////////
// Uint64

TEST( Random, StandardRandUint64 )
{
	CheckRand< Uint64 >( 2048 );
}

TEST( Random, StandardRandUint64Max )
{
	CheckRand< Uint64 >( 2048, 1024 );
}

TEST( Random, StandardRandUint64RangeMaxMin )
{
	CheckRand< Uint64 >( 2048, 2048, 1024 );
}

//////////////////////////////////////////////////////////////////////////
// Float

TEST( Random, StandardRandFloat )
{
	CheckRand< Float >( 2048 );
}

TEST( Random, StandardRandFloatMax )
{
	CheckRand< Float >( 2048, 0.5f );
}

TEST( Random, StandardRandFloatRangeMaxMin )
{
	CheckRand< Float >( 2048, 0.7f, 0.3f );
}

//////////////////////////////////////////////////////////////////////////
// Seed tests

template< typename T, Uint32 arraySize >
RED_INLINE void SeedFill( SeedValue seed, T* values )
{
	m_generator.Seed( seed );

	for( Uint32 i = 0; i < arraySize; ++i )
	{
		values[ i ] = m_generator.Get< T >();
	}
}

template< typename T, Uint32 arraySize >
RED_INLINE void SeedTest()
{
	SeedValue seed = m_generator.Get< SeedValue >();

	T a[ arraySize ];
	T b[ arraySize ];

	SeedFill< T, arraySize >( seed, a );
	SeedFill< T, arraySize >( seed, b );

	EXPECT_EQ( a, b );
}

TEST( Random, StandardRandSeedTestUint8 )
{
	SeedTest< Uint8, 512 >();
}

TEST( Random, StandardRandSeedTestUint16 )
{
	SeedTest< Uint16, 1024 >();
}

TEST( Random, StandardRandSeedTestUint32 )
{
	SeedTest< Uint32, 2048 >();
}

TEST( Random, StandardRandSeedTestUint64 )
{
	SeedTest< Uint64, 2048 >();
}

