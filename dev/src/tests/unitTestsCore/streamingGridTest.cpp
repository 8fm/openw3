/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../../common/redMath/random/fastRand.h"
#include "../../common/core/streamingGrid.h"
#include "../../common/core/streamingGridHelpers.h"

//#pragma optimize ("",off)

#define NUM_GRID_LEVELS		8
#define NUM_GRID_BUCKETS	10000

class GridItemsHelper
{
public:
	struct Item
	{
		Int32	m_x;
		Int32	m_y;
		Int32	m_z;
		Int32	m_r;
		Uint32	m_data;
		Uint32	m_hash;
		Bool	m_collected;
		Bool	m_collectedCheck;
	};

	TDynArray< Item >				m_items;
	Red::Math::Random::FastRand		m_rand;

	void Generate( Uint32 numItems, Uint32 minRadius, Uint32 maxRadius )
	{
		m_rand.Seed( 12345 );
		m_items.Resize( numItems );

		for ( Uint32 i=0; i<numItems; ++i )
		{
			auto& item = m_items[i];
			item.m_data = i;
			item.m_hash = 0;
			item.m_x = (Uint16)( m_rand.Get< Uint32 >() * 2 );
			item.m_y = (Uint16)( m_rand.Get< Uint32 >() * 2 );
			item.m_z = (Uint16)( m_rand.Get< Uint32 >() / 50 );

			const Uint64 r = m_rand.Get< Uint32 >();
			const Uint64 dr = (Uint64)(maxRadius - minRadius);
			item.m_r = (Uint16)( minRadius + ( (dr * r) / m_rand.Max< Uint32 >() ) );
		}
	}

	void ResetFlags()
	{
		for ( auto& it : m_items )
		{
			it.m_collectedCheck = false;
			it.m_collected = false;
		}
	}

	void CollectForPoint( const Int32 x, const Int32 y, const Int32 z )
	{
		for ( auto& it : m_items )
		{
			const Int64 dx = (Int64)x - (Int64)it.m_x;
			const Int64 dy = (Int64)y - (Int64)it.m_y;
			const Int64 dz = (Int64)z - (Int64)it.m_z;

			const Int64 d2 = (dx*dx) + (dy*dy) + (dz*dz);
			const Int64 r2 = (Int64)it.m_r * (Int64)it.m_r;

			if ( d2 <= r2 )
			{
				it.m_collectedCheck = true;
			}
			else
			{
				it.m_collectedCheck = false;
			}
		}
	}

	void CollectForArea( const Int32 minX, const Int32 minY, const Int32 maxX, const Int32 maxY )
	{
		for ( auto& it : m_items )
		{
			if ( it.m_x >= minX && it.m_x <= maxX && it.m_y >= minY && it.m_y <= maxY )
			{
				it.m_collectedCheck = true;
			}
			else
			{
				it.m_collectedCheck = false;
			}
		}
	}
};

namespace Helpers
{
	static const Uint32 CountGridElements( const CStreamingGrid& grid )
	{
		Uint32 numTotalElems = 0;
		for ( Uint32 i=0; i<NUM_GRID_LEVELS; ++i )
		{
			SStreamingGridDebugData data;
			grid.GetDebugInfo(i, data);
			numTotalElems += data.m_numElements;
		}
		return numTotalElems;
	}
}

TEST( Names, StreamingGrid_10k_AddRemove )
{
	// generate elements
	GridItemsHelper elems;
	elems.Generate( 10000, 100, 50000 );

	// create grid
	CStreamingGrid grid( NUM_GRID_LEVELS, NUM_GRID_BUCKETS );
	EXPECT_EQ( grid.GetNumLevels(), NUM_GRID_LEVELS );
	EXPECT_EQ( grid.GetMaxBuckets(), NUM_GRID_BUCKETS );

	// insert elements into the grid
	for ( Uint32 i=0; i<elems.m_items.Size(); ++i )
	{
		auto& item = elems.m_items[i];
		const auto hash = grid.Register( item.m_x, item.m_y, item.m_z, item.m_r, item.m_data );
		EXPECT_NE( hash, 0 );
		item.m_hash = hash;
	}

	// check that elements are added correctly
	EXPECT_EQ( Helpers::CountGridElements(grid), elems.m_items.Size() );

	// remove elements from the grid
	for ( Uint32 i=0; i<elems.m_items.Size(); ++i )
	{
		auto& item = elems.m_items[i];
		grid.Unregister( item.m_hash, item.m_data );
	}

	// expect grid to be empty
	EXPECT_EQ( Helpers::CountGridElements(grid), 0 );
}

TEST( Names, StreamingGrid_10k_AddReverseRemove )
{
	// generate elements
	GridItemsHelper elems;
	elems.Generate( 10000, 100, 50000 );

	// create grid
	CStreamingGrid grid( NUM_GRID_LEVELS, NUM_GRID_BUCKETS );
	EXPECT_EQ( grid.GetNumLevels(), NUM_GRID_LEVELS );
	EXPECT_EQ( grid.GetMaxBuckets(), NUM_GRID_BUCKETS );

	// insert elements into the grid
	for ( Uint32 i=0; i<elems.m_items.Size(); ++i )
	{
		auto& item = elems.m_items[i];
		const auto hash = grid.Register( item.m_x, item.m_y, item.m_z, item.m_r, item.m_data );
		EXPECT_NE( hash, 0 );
		item.m_hash = hash;
	}

	// check that elements are added correctly
	EXPECT_EQ( Helpers::CountGridElements(grid), elems.m_items.Size() );

	// remove elements from the grid - IN REVERSE ORDER
	for ( Uint32 i=0; i<elems.m_items.Size(); ++i )
	{
		auto& item = elems.m_items[ (elems.m_items.Size()-1) - i ];
		grid.Unregister( item.m_hash, item.m_data );
	}

	// expect grid to be empty
	EXPECT_EQ( Helpers::CountGridElements(grid), 0 );
}

TEST( Names, StreamingGrid_10k_TestPoints )
{
	// generate elements
	GridItemsHelper elems;
	elems.Generate( 10000, 100, 50000 );

	// create grid
	CStreamingGrid grid( NUM_GRID_LEVELS, NUM_GRID_BUCKETS );
	EXPECT_EQ( grid.GetNumLevels(), NUM_GRID_LEVELS );
	EXPECT_EQ( grid.GetMaxBuckets(), NUM_GRID_BUCKETS );

	// insert elements into the grid
	for ( Uint32 i=0; i<elems.m_items.Size(); ++i )
	{
		auto& item = elems.m_items[i];
		const auto hash = grid.Register( item.m_x, item.m_y, item.m_z, item.m_r, item.m_data );
		EXPECT_NE( hash, 0 );
		item.m_hash = hash;
	}

	// check that elements are added correctly
	EXPECT_EQ( Helpers::CountGridElements(grid), elems.m_items.Size() );

	// query test
	Red::Math::Random::FastRand testRand;
	for ( Uint32 i=0; i<500; ++i )
	{
		// test points, radius is different with each test
		const Uint16 testPosX = (testRand.Get<Uint32>() * 65535) / testRand.Max<Uint32>();
		const Uint16 testPosY = (testRand.Get<Uint32>() * 65535) / testRand.Max<Uint32>();

		// query the grid
		TStreamingGridCollectorStack< 10005 > collector;
		grid.CollectForPoint( testPosX, testPosY, 0, collector );

		// mark collected item
		elems.ResetFlags();
		for ( Uint32 i=0; i<collector.Size(); ++i )
		{
			const Uint32 data = collector[i];
			auto& it = elems.m_items[ data ];
			EXPECT_EQ( it.m_data, data );
			EXPECT_EQ( it.m_collected, false );
			it.m_collected = true;
		}
				
		// do check
		elems.CollectForPoint( testPosX, testPosY, 0 );

		// make sure the query results are the same
		for ( Uint32 i=0; i<elems.m_items.Size(); ++i )
		{
			auto& item = elems.m_items[i];
			EXPECT_EQ( item.m_collected, item.m_collectedCheck );
			if ( item.m_collected != item.m_collectedCheck )
				break;
		}
	}
}

TEST( Names, StreamingGrid_10k_TestArea )
{
	// generate elements
	GridItemsHelper elems;
	elems.Generate( 10000, 100, 50000 );

	// create grid
	CStreamingGrid grid( NUM_GRID_LEVELS, NUM_GRID_BUCKETS );
	EXPECT_EQ( grid.GetNumLevels(), NUM_GRID_LEVELS );
	EXPECT_EQ( grid.GetMaxBuckets(), NUM_GRID_BUCKETS );

	// insert elements into the grid
	for ( Uint32 i=0; i<elems.m_items.Size(); ++i )
	{
		auto& item = elems.m_items[i];
		const auto hash = grid.Register( item.m_x, item.m_y, item.m_z, item.m_r, item.m_data );
		EXPECT_NE( hash, 0 );
		item.m_hash = hash;
	}

	// check that elements are added correctly
	EXPECT_EQ( Helpers::CountGridElements(grid), elems.m_items.Size() );

	// query test
	Red::Math::Random::FastRand testRand;
	for ( Uint32 i=0; i<500; ++i )
	{
		// 4 test coordinates
		const Uint16 a = (testRand.Get<Uint32>() * 65535) / testRand.Max<Uint32>();
		const Uint16 b = (testRand.Get<Uint32>() * 65535) / testRand.Max<Uint32>();
		const Uint16 c = (testRand.Get<Uint32>() * 65535) / testRand.Max<Uint32>();
		const Uint16 d = (testRand.Get<Uint32>() * 65535) / testRand.Max<Uint32>();

		// compute test box
		const Uint16 minX = Min< Uint16 >( a, b );
		const Uint16 maxX = Max< Uint16 >( a, b );
		const Uint16 minY = Min< Uint16 >( c, d );
		const Uint16 maxY = Max< Uint16 >( c, d );

		// query the grid
		TStreamingGridCollectorStack< 10005 > collector;
		grid.CollectForArea( minX, minY, maxX, maxY, collector );

		// mark collected item
		elems.ResetFlags();
		for ( Uint32 i=0; i<collector.Size(); ++i )
		{
			const Uint32 data = collector[i];
			auto& it = elems.m_items[ data ];
			EXPECT_EQ( it.m_data, data );
			EXPECT_EQ( it.m_collected, false );
			it.m_collected = true;
		}

		// do check
		elems.CollectForArea( minX, minY, maxX, maxY );

		// make sure the query results are the same
		for ( Uint32 i=0; i<elems.m_items.Size(); ++i )
		{
			auto& item = elems.m_items[i];
			EXPECT_EQ( item.m_collected, item.m_collectedCheck );
			if ( item.m_collected != item.m_collectedCheck )
				break;
		}
	}
}

