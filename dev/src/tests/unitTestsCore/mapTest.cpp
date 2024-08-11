#include "build.h"

#include "../../common/core/profilerTypes.h"
#include "../../common/core/sortedmap.h"
#include "../../common/core/hashmap.h"
#include "../../common/core/hashset.h"
#include "hashmap_W2.h"
#include "hashmap_continuous.h"
#define THashMap_CT THashMap_continuous
#include "hash_containers_STL.h"

template < class MAP_TYPE >
void Merge2EqualSize()
{
	MAP_TYPE a;
	MAP_TYPE b;

	MAP_TYPE out;

	a.Insert( 10, 'a' );
	a.Insert( 20, 'a' );
	a.Insert( 30, 'a' );
	a.Insert( 40, 'a' );
	a.Insert( 50, 'a' );
	a.Insert( 60, 'a' );
	a.Insert( 70, 'a' );
	a.Insert( 80, 'a' );
	a.Insert( 90, 'a' );

	b.Insert( 99, 'b' );
	b.Insert( 81, 'b' );
	b.Insert( 72, 'b' );
	b.Insert( 63, 'b' );
	b.Insert( 54, 'b' );
	b.Insert( 45, 'b' );
	b.Insert( 36, 'b' );
	b.Insert( 27, 'b' );
	b.Insert( 18, 'b' );

	MAP_TYPE* sources[ 2 ];

	sources[ 0 ] = &a;
	sources[ 1 ] = &b;

	MAP_TYPE::Merge( out, sources, 2 );

	MAP_TYPE expected;

	expected.Insert( 10, 'a' );
	expected.Insert( 18, 'b' );
	expected.Insert( 20, 'a' );
	expected.Insert( 27, 'b' );
	expected.Insert( 30, 'a' );
	expected.Insert( 36, 'b' );
	expected.Insert( 40, 'a' );
	expected.Insert( 45, 'b' );
	expected.Insert( 50, 'a' );
	expected.Insert( 54, 'b' );
	expected.Insert( 60, 'a' );
	expected.Insert( 63, 'b' );
	expected.Insert( 70, 'a' );
	expected.Insert( 72, 'b' );
	expected.Insert( 80, 'a' );
	expected.Insert( 81, 'b' );
	expected.Insert( 90, 'a' );
	expected.Insert( 99, 'b' );

	ASSERT_EQ( expected.Size(), out.Size() );

	for( auto iter = out.Begin(); iter != out.End(); ++iter )
	{
		AnsiChar value;
		ASSERT_TRUE( expected.Find( iter->m_first, value ) );
		EXPECT_EQ( value, iter->m_second );
	}
}

template < class MAP_TYPE >
void Merge3EqualSize()
{
	MAP_TYPE a;
	MAP_TYPE b;
	MAP_TYPE c;

	MAP_TYPE out;

	a.Insert( 10, 'a' );
	a.Insert( 20, 'a' );
	a.Insert( 30, 'a' );
	a.Insert( 40, 'a' );
	a.Insert( 50, 'a' );
	a.Insert( 60, 'a' );
	a.Insert( 70, 'a' );
	a.Insert( 80, 'a' );
	a.Insert( 90, 'a' );

	b.Insert( 99, 'b' );
	b.Insert( 81, 'b' );
	b.Insert( 72, 'b' );
	b.Insert( 63, 'b' );
	b.Insert( 54, 'b' );
	b.Insert( 45, 'b' );
	b.Insert( 36, 'b' );
	b.Insert( 27, 'b' );
	b.Insert( 18, 'b' );

	c.Insert( 22, 'c' );
	c.Insert( 55, 'c' );
	c.Insert(100, 'c' );
	c.Insert( 11, 'c' );
	c.Insert( 44, 'c' );
	c.Insert( 88, 'c' );
	c.Insert( 33, 'c' );
	c.Insert( 66, 'c' );
	c.Insert( 77, 'c' );

	MAP_TYPE* sources[ 3 ];

	sources[ 0 ] = &a;
	sources[ 1 ] = &b;
	sources[ 2 ] = &c;

	MAP_TYPE::Merge( out, sources, 3 );

	MAP_TYPE expected;

	expected.Insert( 10, 'a' );
	expected.Insert( 11, 'c' );
	expected.Insert( 18, 'b' );
	expected.Insert( 20, 'a' );
	expected.Insert( 22, 'c' );
	expected.Insert( 27, 'b' );
	expected.Insert( 30, 'a' );
	expected.Insert( 33, 'c' );
	expected.Insert( 36, 'b' );
	expected.Insert( 40, 'a' );
	expected.Insert( 44, 'c' );
	expected.Insert( 45, 'b' );
	expected.Insert( 50, 'a' );
	expected.Insert( 54, 'b' );
	expected.Insert( 55, 'c' );
	expected.Insert( 60, 'a' );
	expected.Insert( 63, 'b' );
	expected.Insert( 66, 'c' );
	expected.Insert( 70, 'a' );
	expected.Insert( 72, 'b' );
	expected.Insert( 77, 'c' );
	expected.Insert( 80, 'a' );
	expected.Insert( 81, 'b' );
	expected.Insert( 88, 'c' );
	expected.Insert( 90, 'a' );
	expected.Insert( 99, 'b' );
	expected.Insert(100, 'c' );

	ASSERT_EQ( expected.Size(), out.Size() );

	for( auto iter = out.Begin(); iter != out.End(); ++iter )
	{
		AnsiChar value;
		ASSERT_TRUE( expected.Find( iter->m_first, value ) );
		EXPECT_EQ( value, iter->m_second );
	}
}

template < class MAP_TYPE >
void Merge3DifferentSize()
{
	MAP_TYPE a;
	MAP_TYPE b;
	MAP_TYPE c;

	MAP_TYPE out;

	a.Insert( 10, 'a' );
	a.Insert( 20, 'a' );
	a.Insert( 30, 'a' );
	a.Insert( 40, 'a' );
	a.Insert( 50, 'a' );

	b.Insert( 72, 'b' );
	b.Insert( 63, 'b' );
	b.Insert( 54, 'b' );
	b.Insert( 45, 'b' );
	b.Insert( 36, 'b' );
	b.Insert( 27, 'b' );
	b.Insert( 18, 'b' );

	c.Insert( 22, 'c' );
	c.Insert( 55, 'c' );
	c.Insert(100, 'c' );
	c.Insert( 11, 'c' );
	c.Insert( 44, 'c' );
	c.Insert( 88, 'c' );
	c.Insert( 33, 'c' );
	c.Insert( 66, 'c' );
	c.Insert( 77, 'c' );

	MAP_TYPE* sources[ 3 ];

	sources[ 0 ] = &a;
	sources[ 1 ] = &b;
	sources[ 2 ] = &c;

	MAP_TYPE::Merge( out, sources, 3 );

	MAP_TYPE expected;

	expected.Insert( 10, 'a' );
	expected.Insert( 11, 'c' );
	expected.Insert( 18, 'b' );
	expected.Insert( 20, 'a' );
	expected.Insert( 22, 'c' );
	expected.Insert( 27, 'b' );
	expected.Insert( 30, 'a' );
	expected.Insert( 33, 'c' );
	expected.Insert( 36, 'b' );
	expected.Insert( 40, 'a' );
	expected.Insert( 44, 'c' );
	expected.Insert( 45, 'b' );
	expected.Insert( 50, 'a' );
	expected.Insert( 54, 'b' );
	expected.Insert( 55, 'c' );
	expected.Insert( 63, 'b' );
	expected.Insert( 66, 'c' );
	expected.Insert( 72, 'b' );
	expected.Insert( 77, 'c' );
	expected.Insert( 88, 'c' );
	expected.Insert(100, 'c' );

	ASSERT_EQ( expected.Size(), out.Size() );

	for( auto iter = out.Begin(); iter != out.End(); ++iter )
	{
		AnsiChar value;
		ASSERT_TRUE( expected.Find( iter->m_first, value ) );
		EXPECT_EQ( value, iter->m_second );
	}
}

template < class MAP_TYPE >
void Merge2SameIndices()
{
	MAP_TYPE a;
	MAP_TYPE b;

	MAP_TYPE out;

	a.Insert( 10, 'a' );
	a.Insert( 20, 'a' );
	a.Insert( 30, 'a' );

	b.Insert( 10, 'b' );
	b.Insert( 20, 'b' );
	b.Insert( 30, 'b' );

	MAP_TYPE* sources[ 2 ];

	sources[ 0 ] = &a;
	sources[ 1 ] = &b;

	MAP_TYPE::Merge( out, sources, 2 );

	MAP_TYPE expected;

	expected.Insert( 10, 'a' );
	expected.Insert( 20, 'a' );
	expected.Insert( 30, 'a' );

	ASSERT_EQ( expected.Size(), out.Size() );

	for( auto iter = out.Begin(); iter != out.End(); ++iter )
	{
		AnsiChar value;
		ASSERT_TRUE( expected.Find( iter->m_first, value ) );
		EXPECT_EQ( value, iter->m_second );
	}
}

template < class MAP_TYPE >
void Merge2StringKeys()
{
	MAP_TYPE a;
	MAP_TYPE b;

	MAP_TYPE out;

	a.Insert( TXT( "1" ), 'a' );
	a.Insert( TXT( "3" ), 'a' );
	a.Insert( TXT( "5" ), 'a' );
	a.Insert( TXT( "8" ), 'a' );

	b.Insert( TXT( "2" ), 'b' );
	b.Insert( TXT( "4" ), 'b' );
	a.Insert( TXT( "6" ), 'b' );
	a.Insert( TXT( "8" ), 'b' );

	MAP_TYPE* sources[ 2 ];

	sources[ 0 ] = &a;
	sources[ 1 ] = &b;

	MAP_TYPE::Merge( out, sources, 2 );

	MAP_TYPE expected;

	expected.Insert( TXT( "1" ), 'a' );
	expected.Insert( TXT( "2" ), 'b' );
	expected.Insert( TXT( "3" ), 'a' );
	expected.Insert( TXT( "4" ), 'b' );
	expected.Insert( TXT( "5" ), 'a' );
	expected.Insert( TXT( "6" ), 'b' );
	expected.Insert( TXT( "8" ), 'a' );

	ASSERT_EQ( expected.Size(), out.Size() );

	for( auto iter = out.Begin(); iter != out.End(); ++iter )
	{
		AnsiChar value;
		ASSERT_TRUE( expected.Find( iter->m_first, value ) );
		EXPECT_EQ( value, iter->m_second );
	}
}

// Sorted map

TEST( SortedMap, Merge2EqualSize )
{
	Merge2EqualSize< TSortedMap< Uint32, AnsiChar > >();
}

TEST( SortedMap, Merge3EqualSize )
{
	Merge3EqualSize< TSortedMap< Uint32, AnsiChar > >();
}

TEST( SortedMap, Merge3DifferentSize )
{
	Merge3DifferentSize< TSortedMap< Uint32, AnsiChar > >();
}

TEST( SortedMap, Merge2SameIndices )
{
	Merge2SameIndices< TSortedMap< Uint32, AnsiChar > >();
}

TEST( SortedMap, Merge2StringKeys )
{
	Merge2StringKeys< TSortedMap< String, AnsiChar > >();
}

// Utils

template < typename K, typename V >
RED_FORCE_INLINE V GetPairValue( const std::pair< K, V >& pair )
{
	return pair.second;
}

template < typename K, typename V >
RED_FORCE_INLINE V GetPairValue( const TPair< K, V >& pair )
{
	return pair.m_second;
}

// All maps combined - correctness and performance tests

struct SHashContainerProfileResult
{
	Uint32 m_memoryOverhead;
	Double m_insertTime;
	Double m_findTime;
	Double m_eraseTime;
	Double m_iterateWholeTime;
	Double m_iterate10PercentTime;

	SHashContainerProfileResult()
		: m_memoryOverhead( 0 )
		, m_insertTime( 0.0 )
		, m_findTime( 0.0 )
		, m_eraseTime( 0.0 )
		, m_iterateWholeTime( 0.0 )
		, m_iterate10PercentTime( 0.0 )
	{}
};

template < class MAP_TYPE, class KEY_TYPE >
void MapPerformanceTest( const TDynArray< KEY_TYPE >& keys, Uint32 keysCount, Uint32 numTests, SHashContainerProfileResult& stats )
{
	stats.m_insertTime = 0.0f;
	stats.m_findTime = 0.0f;
	stats.m_eraseTime = 0.0f;
	stats.m_iterateWholeTime = 0.0f;
	stats.m_iterate10PercentTime = 0.0f;

	for ( Uint32 test = 0; test < numTests; ++test )
	{
		MAP_TYPE map;
		map.Reserve( keysCount );

		{
			CTimeCounter timer;
			for ( Uint32 i = 0; i < keysCount; ++i )
			{
				map.Insert( keys[ i ], 0 );
			}
			stats.m_insertTime += timer.GetTimePeriodMS();
		}

		stats.m_memoryOverhead = map.GetInternalMemSize() - sizeof( TPair< KEY_TYPE, Uint32 > ) * keysCount;

		{
			CTimeCounter timer;
			for ( Uint32 i = 0; i < keysCount; ++i )
			{
				const Bool exists = map.KeyExist( keys [ i ] );
				ASSERT_TRUE( exists );
			}
			stats.m_findTime += timer.GetTimePeriodMS();
		}

		{
			CTimeCounter timer;
			Uint32 sum = 0;
			for ( auto it = map.Begin(), end = map.End(); it != end; ++it )
			{
				sum += GetPairValue( *it );
			}
			stats.m_iterateWholeTime += timer.GetTimePeriodMS();
		}

		const Uint32 size90Percent = keysCount * 9 / 10;
		{
			CTimeCounter timer;
			for ( Uint32 i = 0; i < size90Percent; ++i )
			{
				const Bool removed = map.Erase( keys[ i ] );
				ASSERT_TRUE( removed );
			}
			stats.m_eraseTime += timer.GetTimePeriodMS();
		}

		{
			CTimeCounter timer;
			Uint32 sum = 0;
			for ( auto it = map.Begin(), end = map.End(); it != end; ++it )
			{
				sum += GetPairValue( *it );
			}
			stats.m_iterate10PercentTime += timer.GetTimePeriodMS();
		}

		{
			CTimeCounter timer;
			for ( Uint32 i = size90Percent; i < keysCount; ++i )
			{
				const Bool removed = map.Erase( keys[ i ] );
				ASSERT_TRUE( removed );
			}
			stats.m_eraseTime += timer.GetTimePeriodMS();
		}
	}
}

template < class SET_TYPE, class KEY_TYPE >
void SetPerformanceTest( const TDynArray< KEY_TYPE >& keys, Uint32 keysCount, Uint32 numTests, SHashContainerProfileResult& stats )
{
	stats.m_insertTime = 0.0f;
	stats.m_findTime = 0.0f;
	stats.m_eraseTime = 0.0f;
	stats.m_iterateWholeTime = 0.0f;
	stats.m_iterate10PercentTime = 0.0f;

	for ( Uint32 test = 0; test < numTests; ++test )
	{
		SET_TYPE set;
		set.Reserve( keysCount );

		{
			CTimeCounter timer;
			for ( Uint32 i = 0; i < keysCount; ++i )
			{
				set.Insert( keys[ i ] );
			}
			stats.m_insertTime += timer.GetTimePeriodMS();
		}

		stats.m_memoryOverhead = set.GetInternalMemSize() - sizeof( KEY_TYPE ) * keysCount;

		{
			CTimeCounter timer;
			for ( Uint32 i = 0; i < keysCount; ++i )
			{
				const Bool exists = set.Exist( keys[ i ] );
				ASSERT_TRUE( exists );
			}
			stats.m_findTime += timer.GetTimePeriodMS();
		}

		{
			CTimeCounter timer;
			Uint32 numEqual = 0;
			KEY_TYPE dummyValue = KEY_TYPE();
			for ( auto it = set.Begin(), end = set.End(); it != end; ++it )
			{
				if ( dummyValue == *it )
				{
					++numEqual;
				}
			}
			stats.m_iterateWholeTime += timer.GetTimePeriodMS();
		}

		const Uint32 size90Percent = keysCount * 9 / 10;
		{
			CTimeCounter timer;
			for ( Uint32 i = 0; i < size90Percent; ++i )
			{
				const Bool removed = set.Erase( keys[ i ] );
				ASSERT_TRUE( removed );
			}
			stats.m_eraseTime += timer.GetTimePeriodMS();
		}

		{
			CTimeCounter timer;
			Uint32 numEqual = 0;
			KEY_TYPE dummyValue = KEY_TYPE();
			for ( auto it = set.Begin(), end = set.End(); it != end; ++it )
			{
				if ( dummyValue == *it )
				{
					++numEqual;
				}
			}
			stats.m_iterate10PercentTime += timer.GetTimePeriodMS();
		}

		{
			CTimeCounter timer;
			for ( Uint32 i = size90Percent; i < keysCount; ++i )
			{
				const Bool removed = set.Erase( keys[ i ] );
				ASSERT_TRUE( removed );
			}
			stats.m_eraseTime += timer.GetTimePeriodMS();
		}
	}
}

// ctremblay this is too heavy. Break quick incremental unit test philosophy. 
// I'm not saying those tests should not exist, but I need to add a feature in the Unit Test framework to flag those heavy test and only run them when requested.
#if 0 
TEST( MapsAndSets, BatchOperations )
{
	SHashContainerProfileResult stats;

#define BEGIN_BATCH_TEST_GROUP( container_type, key_type ) \
	{ \
		typedef key_type KeyType; \
		const Uint32 maxKeys = keys##key_type.Size(); \
		const auto& keys = keys##key_type; \
		fprintf( stdout, " Testing %s %ss\n", #key_type, #container_type ); \
		fprintf( stdout, "----------------------------------------------------------------------------\n" ); \
		fprintf( stdout, "         Memory Overhead     Insert    Find      Erase     Iter      Iter10%%\n" ); \
		fprintf( stdout, "----------------------------------------------------------------------------\n" ); \
		Uint32 testCount = 10; \
		for ( Uint32 keysCount = 10; keysCount <= maxKeys; keysCount *= 10 ) \
		{ \
			fprintf( stdout, "%d elements:\n", keysCount ); \
			fprintf( stdout, "----------------------------------------------------------------------------\n" ); \

#define END_BATCH_TEST_GROUP() \
			fprintf( stdout, "============================================================================\n\n" ); \
		} \
	}

#define BATCH_MAP_TEST( map_type ) \
	MapPerformanceTest< map_type< KeyType, Uint32 > >( keys, keysCount, testCount, stats ); \
	fprintf( stdout, "%16s:%7u%10.3f%10.3f%10.3f%10.3f%10.3f\n", #map_type, stats.m_memoryOverhead, stats.m_insertTime, stats.m_findTime, stats.m_eraseTime, stats.m_iterateWholeTime, stats.m_iterate10PercentTime );

#define BATCH_SET_TEST( set_type ) \
	SetPerformanceTest< set_type< KeyType > >( keys, keysCount, testCount, stats ); \
	fprintf( stdout, "%16s:%7u%10.3f%10.3f%10.3f%10.3f%10.3f\n", #set_type, stats.m_memoryOverhead, stats.m_insertTime, stats.m_findTime, stats.m_eraseTime, stats.m_iterateWholeTime, stats.m_iterate10PercentTime );

	// Generate sets of keys

	CStandardRand random;
	TDynArray< Uint32 > keysUint32( 10000 );
	keysUint32[ 0 ] = 1;
	for ( Uint32 i = 1; i < keysUint32.Size(); ++i )
	{
		keysUint32[ i ] = keysUint32[ i - 1 ] + random.Get< Uint32 >( 1, 100 );
	}

	TDynArray< CGUID > keysCGUID( 1000 );
	for ( Uint32 i = 0; i < keysCGUID.Size(); ++i )
	{
		keysCGUID[ i ] = CGUID( random.Get< Uint32 >(), i * 2, i * 3, i * 4 );
	}

	TDynArray< String > keysString( 1000 );
	for ( Uint32 i = 0; i < keysString.Size(); ++i )
	{
		keysString[ i ] = String::Printf( TXT("%d Number %d"), random.Get< Uint32 >(), i );
	}

	// Test maps

	BEGIN_BATCH_TEST_GROUP( map, Uint32 )
		BATCH_MAP_TEST( STLMap );
		BATCH_MAP_TEST( STLUnorderedMap );
		BATCH_MAP_TEST( TSortedMap );
		BATCH_MAP_TEST( THashMap_W2 );
		BATCH_MAP_TEST( THashMap_CT );
		BATCH_MAP_TEST( THashMap );
	END_BATCH_TEST_GROUP()

	BEGIN_BATCH_TEST_GROUP( map, CGUID )
		BATCH_MAP_TEST( STLMap );
		BATCH_MAP_TEST( STLUnorderedMap );
		BATCH_MAP_TEST( TSortedMap );
		BATCH_MAP_TEST( THashMap_W2 );
		BATCH_MAP_TEST( THashMap_CT );
		BATCH_MAP_TEST( THashMap );
	END_BATCH_TEST_GROUP()

	BEGIN_BATCH_TEST_GROUP( map, String )
		BATCH_MAP_TEST( STLMap );
		BATCH_MAP_TEST( STLUnorderedMap );
		//BATCH_MAP_TEST( TSortedMap ); <- to slow for debugging
		BATCH_MAP_TEST( THashMap_W2 );
		BATCH_MAP_TEST( THashMap_CT );
		BATCH_MAP_TEST( THashMap );
	END_BATCH_TEST_GROUP()

	// Test sets

	BEGIN_BATCH_TEST_GROUP( set, Uint32 )
		BATCH_SET_TEST( STLSet );
		BATCH_SET_TEST( STLUnorderedSet );
		BATCH_SET_TEST( THashSet );
	END_BATCH_TEST_GROUP()

	BEGIN_BATCH_TEST_GROUP( set, CGUID )
		BATCH_SET_TEST( STLSet );
		BATCH_SET_TEST( STLUnorderedSet );
		BATCH_SET_TEST( THashSet );
	END_BATCH_TEST_GROUP()

	BEGIN_BATCH_TEST_GROUP( set, String )
		BATCH_SET_TEST( STLSet );
		BATCH_SET_TEST( STLUnorderedSet );
		BATCH_SET_TEST( THashSet );
	END_BATCH_TEST_GROUP()
}

#endif 
