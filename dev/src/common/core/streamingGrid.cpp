#include "build.h"
#include "streamingGrid.h"
#include "profiler.h"

/*#undef LOG_CORE

void LOG_CORE( const Char* txt, ... )
{
	Char buf[ 512 ];
	va_list args;

	va_start( args, txt );
	Red::VSNPrintF( buf, 512, txt, args );
	va_end( args );

	OutputDebugStringW( buf );
	OutputDebugStringW( TXT("\n") );
}*/

//#define DEBUG_STREAMING_GRID

//----

CStreamingGridCollector::CStreamingGridCollector( Elem* elements, const Uint32 capacity )
	: m_elements( elements )
	, m_numElements( 0 )
	, m_maxElemenets( capacity )
{}

void CStreamingGridCollector::Sort()
{
	// yes, it's still faster than lambda
	struct
	{
		RED_FORCE_INLINE Bool operator()( const Elem& a, const Elem& b ) const
		{
			return a.m_distance < b.m_distance;
		}
	} cmp;

	::Sort( &m_elements[0], &m_elements[m_numElements], cmp );
}

//----

CStreamingGrid::CStreamingGrid( const Uint32 numLevels, const Uint32 numBuckets )
{
	// allocate buckets
	m_numBuckets = numBuckets;
	m_buckets = (GridBucket*) RED_MEMORY_ALLOCATE_ALIGNED( MemoryPool_Default, MC_EntityManager, sizeof(GridBucket) * numBuckets, 64 );
	m_bucketIDs.Resize( numBuckets );
	LOG_CORE( TXT("Entity streaming grid is using %1.2fKB for (%d buckets)"), 
		(sizeof(GridBucket) * numBuckets) / 1024.0f, numBuckets );

	// allocate indices 0
	m_bucketIDs.Alloc();

	// Initialize the grid
	CreateGrid( numLevels );
}

CStreamingGrid::~CStreamingGrid()
{
	if ( m_buckets )
	{
		RED_MEMORY_FREE( MemoryPool_Default, MC_EntityManager, m_buckets );
		m_buckets = nullptr;
	}

	if ( m_nodes )
	{
		RED_MEMORY_FREE( MemoryPool_Default, MC_EntityManager, m_nodes );
		m_nodes = nullptr;
	}

	if ( m_levels )
	{
		RED_MEMORY_FREE( MemoryPool_Default, MC_EntityManager, m_levels );
		m_levels = nullptr;
	}
}

const Uint32 CStreamingGrid::GetNumLevels() const
{
	return m_numLevels;
}

const Uint32 CStreamingGrid::GetMaxBuckets() const
{
	return m_numBuckets;
}

const Uint32 CStreamingGrid::GetNumBuckets() const
{
	return m_bucketIDs.GetNumAllocated();
}

void CStreamingGrid::GetDebugInfo( const Uint32 level, struct SStreamingGridDebugData& outData ) const
{
	if ( level >= m_numLevels )
	{
		outData.m_numBuckets = 0;
		outData.m_numElements = 0;
		outData.m_numWastedElements = 0;
		outData.m_numCells = 0;
		return;
	}

	// process grid at given level
	const GridLevel& l = m_levels[ level ];
	outData.m_numCells = l.m_nodeCount;

	// grid counts
	outData.m_gridSize = l.m_levelSize;
	outData.m_gridElemCount.Resize( l.m_levelSize*l.m_levelSize );
	outData.m_gridMaxElemCount = 0;

	// count buckets
	for ( Uint32 y=0; y<l.m_levelSize; ++y )
	{
		for ( Uint32 x=0; x<l.m_levelSize; ++x )
		{
			const GridNode& n = l.m_nodes[x + y*l.m_levelSize];

			Uint32 numCellElements = 0;
			TBucketIndex bucketIndex = n.m_bucket;
			while ( bucketIndex != 0 )
			{
				const GridBucket& b = m_buckets[ bucketIndex ];

				// count elements
				outData.m_numBuckets += 1;
				outData.m_numElements += b.m_elemCount;
				numCellElements += b.m_elemCount;

				// extract elements
				for ( Uint32 j=0; j<b.m_elemCount; ++j )
				{
					const GridElement& e = b.m_elems[j];

					// emit debug element
					SStreamingGridDebugData::Elem elemInfo;
					elemInfo.m_x = (Float)e.m_x / 65535.0f;
					elemInfo.m_y = (Float)e.m_y / 65535.0f;
					elemInfo.m_r = (Float)e.m_radius / 65535.0f;
					outData.m_elems.PushBack( elemInfo );
				}

				// next bucket
				bucketIndex = b.m_nextBucket;
			}

			// count wasted elements
			if ( n.m_bucketCount > 1 )
			{
				outData.m_numWastedElements += (n.m_bucketCount*GridBucket::MAX - numCellElements);
			}

			// update histogram
			outData.m_gridElemCount[x + y*l.m_levelSize] = numCellElements;
			if ( numCellElements > outData.m_gridMaxElemCount )
				outData.m_gridMaxElemCount = numCellElements;
		}
	}
}

CStreamingGrid::TGridHash CStreamingGrid::Register( Uint16 x, Uint16 y, Uint16 z, Uint16 radius, const Uint32 data )
{
	// calculate on which level we should put the entry
	Int32 level = m_numLevels - 1;
	while ( level > 0 )
	{
		const Int32 levelShift = 15 - level;
		const Int32 levelRadius = 1 << levelShift; // level0 grid has 2x2 cells

		if ( radius < levelRadius )
			break;

		--level;
	}

	// calculate cell index in the entry
	const Int32 levelShift = 15 - level;
	const Int32 levelCount = 2 << level;
	const Uint32 cellIndex = (x >> levelShift) + (y >> levelShift) * levelCount;

	// validation
	RED_FATAL_ASSERT( level < (Int32)m_numLevels, "Invalid quantization result" );
	RED_FATAL_ASSERT( cellIndex < m_levels[level].m_nodeCount, "Invalid quantization result" );

	// add to cell, try to add to an existing bucket first
	GridNode* node = m_levels[level].m_nodes + cellIndex;
	TBucketIndex bucketIndex = node->m_bucket;
	while ( bucketIndex != 0 )
	{
		GridBucket* b = &m_buckets[ bucketIndex ];
		if ( b->m_elemCount < GridBucket::MAX )
		{
			// allocate element in bucket
			GridElement* e = &b->m_elems[ b->m_elemCount ];
			b->m_elemCount += 1;

			// store element values
			e->m_x = x;
			e->m_y = y;
			e->m_z = z;
			e->m_radius = radius;
			e->m_data = data;

#ifdef DEBUG_STREAMING_GRID
			LOG_CORE( TXT("GRID: Added 0x%X at bucket %d pos [%d,%d,%d], rad %d, level %d, elem %d"), 
				data, bucketIndex, x, y, z, radius, level, b->m_elemCount );
#endif

			// return object ID - bucket index
			return bucketIndex;
		}

		// try in next node
		bucketIndex = b->m_nextBucket;
	}

	// no free space, create new bucket
	bucketIndex = m_bucketIDs.Alloc();
	if ( bucketIndex == 0 )
	{
		ERR_CORE( TXT("Initial bucket array was to small, resizing...") );
		//Uint32 oldBucketSize = m_numBuckets * sizeof( GridBucket );
		m_numBuckets *= 2;
		m_bucketIDs.Resize( m_numBuckets );

		//void * newBuckets = RED_MEMORY_ALLOCATE_ALIGNED( MemoryPool_Default, MC_EntityManager, sizeof(GridBucket) * m_numBuckets, 64 );
		//Red::System::MemoryCopy( newBuckets, m_buckets, oldBucketSize );
		//RED_MEMORY_FREE( MemoryPool_Default, MC_EntityManager, m_buckets );
		//m_buckets = (GridBucket*)newBuckets;

		m_buckets = (GridBucket*) RED_MEMORY_REALLOCATE_ALIGNED( MemoryPool_Default, m_buckets, MC_EntityManager, sizeof(GridBucket) * m_numBuckets, 16 );

		// reallocate index
		bucketIndex = m_bucketIDs.Alloc();
		RED_FATAL_ASSERT( bucketIndex != 0, "Failed to allocate bucket Id even after resizeing" );
	}

	// Store in new bucket
	GridBucket* b = &m_buckets[ bucketIndex ];
	GridElement* e = &b->m_elems[0];

	// store element values
	e->m_x = x;
	e->m_y = y;
	e->m_z = z;
	e->m_radius = radius;
	e->m_data = data;

	// update node
	b->m_elemCount = 1;
	b->m_nextBucket = node->m_bucket;
	node->m_bucket = bucketIndex;
	node->m_bucketCount += 1;

#ifdef DEBUG_STREAMING_GRID
	LOG_CORE( TXT("GRID: Added 0x%X at bucket %d pos [%d,%d,%d], rad %d, level %d, elem %d"), 
		data, bucketIndex, x, y, z, radius, level, b->m_elemCount );
#endif

	// bucket index is the grid hash
	return bucketIndex;
}

void CStreamingGrid::Unregister( TGridHash hash, const Uint32 data )
{
	RED_FATAL_ASSERT( hash != 0, "Trying to unregister invalid object from the grid" );
	RED_FATAL_ASSERT( hash < m_bucketIDs.GetCapacity(), "Trying to unregister invalid object from the grid" );

	// find the object in the bucket
	GridBucket& b = m_buckets[ hash ];
	RED_FATAL_ASSERT( b.m_elemCount > 0, "Hash pointing to empty bucket" );
	for ( Uint32 i=0; i < b.m_elemCount; ++i )
	{
		if ( b.m_elems[i].m_data == data )
		{
			// cleanup the last element
			b.m_elemCount -= 1;

#ifdef DEBUG_STREAMING_GRID
			const auto& e = b.m_elems[i];
			LOG_CORE( TXT("GRID: Removed 0x%X at bucket %d pos [%d,%d,%d], rad %d, elem %d/%d"), 
				data, hash, e.m_x, e.m_y, e.m_z, e.m_radius, i, b.m_elemCount );
#endif

			// shift
			if ( i != b.m_elemCount )
			{
				b.m_elems[i] = b.m_elems[b.m_elemCount];
			}

			// cleanup last element
			b.m_elems[b.m_elemCount].m_data = 0;
			b.m_elems[b.m_elemCount].m_x = 0;
			b.m_elems[b.m_elemCount].m_y = 0;
			b.m_elems[b.m_elemCount].m_z = 0;
			b.m_elems[b.m_elemCount].m_radius = 0;

			// last element removed from bucket ?
			if ( b.m_elemCount == 0 )
			{
				// TODO: find the related grid node
				// TODO: unlink the bucked from the node linked list
				// TODO: release the bucket to the pool
				// m_bucketIDs.Release( hash );
			}

			// deleted
			return;
		}
	}

	// value not found in bucket
	RED_FATAL( "Object not found in the streaming grid" );
}

void CStreamingGrid::Unregister( Uint16 x, Uint16 y, Uint16 z, Uint16 radius, const Uint32 data )
{
	// calculate on which level we should put the entry
	Int32 level = m_numLevels - 1;
	while ( level > 0 )
	{
		const Int32 levelShift = 15 - level;
		const Int32 levelRadius = 1 << levelShift; // level0 grid has 2x2 cells

		if ( radius < levelRadius )
			break;

		--level;
	}

	// calculate cell index in the entry
	const Int32 levelShift = 15 - level;
	const Int32 levelCount = 2 << level;
	const Uint32 cellIndex = (x >> levelShift) + (y >> levelShift) * levelCount;

	// validation
	RED_FATAL_ASSERT( level < (Int32)m_numLevels, "Invalid quantization result" );
	RED_FATAL_ASSERT( cellIndex < m_levels[level].m_nodeCount, "Invalid quantization result" );

	// add to cell, try to add to an existing bucket first
	GridNode* node = m_levels[level].m_nodes + cellIndex;
	TBucketIndex bucketIndex = node->m_bucket;
	while ( bucketIndex != 0 )
	{
		GridBucket& b = m_buckets[ bucketIndex ];
		for ( Uint32 i=0; i<b.m_elemCount; ++i )
		{
			GridElement& e = b.m_elems[ i ];
			if ( e.m_data == data )
			{
				// cleanup the last element
				b.m_elemCount -= 1;

#ifdef DEBUG_STREAMING_GRID
				const auto& e = b.m_elems[i];
				LOG_CORE( TXT("GRID: Removed 0x%X at bucket %d pos [%d,%d,%d], rad %d, elem %d/%d"), 
					data, hash, e.m_x, e.m_y, e.m_z, e.m_radius, i, b.m_elemCount );
#endif

				// shift
				if ( i != b.m_elemCount )
				{
					b.m_elems[i] = b.m_elems[b.m_elemCount];
				}

				// cleanup last element
				b.m_elems[b.m_elemCount].m_data = 0;
				b.m_elems[b.m_elemCount].m_x = 0;
				b.m_elems[b.m_elemCount].m_y = 0;
				b.m_elems[b.m_elemCount].m_z = 0;
				b.m_elems[b.m_elemCount].m_radius = 0;

				// last element removed from bucket ?
				if ( b.m_elemCount == 0 )
				{
					// TODO: find the related grid node
					// TODO: unlink the bucked from the node linked list
					// TODO: release the bucket to the pool
					// m_bucketIDs.Release( hash );
				}

				// deleted
				return;
			}
		}

		// try in next bucket
		bucketIndex = b.m_nextBucket;
	}

	// value not found in bucket
	RED_FATAL( "Object not found in the streaming grid" );
}

CStreamingGrid::TGridHash CStreamingGrid::Move( TGridHash hash, Uint16 x, Uint16 y, Uint16 z, const Uint32 data )
{
	RED_FATAL_ASSERT( hash != 0, "Trying to move entry not registered in the grid" );

	// find existing element
	GridElement* existingData = nullptr;
	if ( hash != 0 )
	{
		RED_FATAL_ASSERT( hash < m_bucketIDs.GetCapacity(), "Trying to unregister invalid object from the grid" );

		// find the object in the bucket
		GridBucket& b = m_buckets[ hash ];
		for ( Uint32 i=0; i < b.m_elemCount; ++i )
		{
			GridElement& e = b.m_elems[i];
			if ( e.m_data == data )
			{
				existingData = &e;
				break;
			}
		}
	}

	// try to reuse existing element (only allowed if radius is the same)
	if ( existingData == nullptr )
	{
		RED_FATAL( "Object not found in the streaming grid" );
		return 0;
	}

	// calculate on which level we should put the entry
	Int32 level = m_numLevels - 1;
	const Int32 radius = existingData->m_radius;
	while ( level > 0 )
	{
		const Int32 levelShift = 15 - level;
		const Int32 levelRadius = 1 << levelShift; // level0 grid has 2x2 cells

		if ( radius < levelRadius )
			break;

		--level;
	}

	// are we in the same grid cell ?
	const Int32 levelShift = 15 - level;
	const Uint32 oldCX = (existingData->m_x) >> levelShift;
	const Uint32 oldCY = (existingData->m_y) >> levelShift;
	const Uint32 newCX = (x) >> levelShift;
	const Uint32 newCY = (y) >> levelShift;
	if ( oldCX == newCX && oldCY == newCY )
	{
#ifdef DEBUG_STREAMING_GRID
		const auto& e = *existingData;
		LOG_CORE( TXT("GRID: Updated 0x%X at bucket %d pos [%d,%d,%d], rad %d"), 
			data, hash, e.m_x, e.m_y, e.m_z, e.m_radius );
#endif

		// in the same cell, just update the location
		existingData->m_x = x;
		existingData->m_y = y;
		existingData->m_z = z;
		return hash;
	}

	// not in the same cell, cleanup 
	Unregister( hash, data );

	// register in new place
	return Register( x, y, z, radius, data );
}

void CStreamingGrid::CollectForPoint( Uint16 testX, Uint16 testY, Uint16 testZ, CStreamingGridCollector& outCollector ) const
{
	PC_SCOPE( StreamingGridCollectPoint );

	// visit each level, start from the bottom
	Int32 maxLevel = (m_numLevels-1);
	Int32 cellX = testX >> (15 - maxLevel);
	Int32 cellY = testY >> (15 - maxLevel);
	for ( Int32 level = maxLevel; level >= 0; --level )
	{
		const auto* nodes = m_levels[level].m_nodes;

		// number of cells in this level
		const Int32 levelSize = 2 << level;
		RED_FATAL_ASSERT( cellX < levelSize, "Cell coordinate out of range (%d, max=%d, level=%d)", cellX, levelSize, level );
		RED_FATAL_ASSERT( cellY < levelSize, "Cell coordinate out of range (%d, max=%d, level=%d)", cellY, levelSize, level );

		// visit cell and touching cells (3x3 grid)
		// we know from the construction of the grid that we don't have to check any further
		const Int32 minX = Max< Int32 >( 0, cellX - 1 );
		const Int32 minY = Max< Int32 >( 0, cellY - 1 );
		const Int32 maxX = Min< Int32 >( cellX + 1, levelSize-1 );
		const Int32 maxY = Min< Int32 >( cellY + 1, levelSize-1 );

		// debug stuff
#ifdef DEBUG_STREAMING_GRID
		LOG_CORE( TXT("Level[%d]: [%d,%d]-[%d,%d]"), level, minX, minY, maxX, maxY );
#endif

		// scan buckets in each node
		for ( Int32 y=minY; y<=maxY; ++y )
		{
			for ( Int32 x=minX; x<=maxX; ++x )
			{
				const Int32 cellIndex = x + (y*levelSize);

				TBucketIndex bucketIndex = nodes[ cellIndex ].m_bucket;
				while ( bucketIndex != 0 )
				{
					const GridBucket& bucket = m_buckets[ bucketIndex ];

					// debug stuff
#ifdef DEBUG_STREAMING_GRID
					LOG_CORE( TXT("  Cell[%d,%d]: %d, bucket %d, elems %d"), x, y, cellIndex, bucketIndex, bucket.m_elemCount );
#endif

					// test elements, all elements from this bucket are in cache, dirt cheap test
					for ( Uint32 c=0; c<bucket.m_elemCount; ++c )
					{
						const GridElement& e = bucket.m_elems[c];

						// calculate squared distance
						const Int64 dx = (Int64)e.m_x - (Int64)testX;
						const Int64 dy = (Int64)e.m_y - (Int64)testY;
						const Int64 dz = (Int64)e.m_z - (Int64)testZ;
						const Int64 d2 = (dx*dx) + (dy*dy) + (dz*dz);

						// validate distance - should not be bigger than two times the cell size
						const Int32 cellSize = 2 * ( (1<<15) >> level );
						RED_FATAL_ASSERT( abs(dx) <= cellSize, "DX is larger than cell size (%d>%d)", dx, cellSize );
						RED_FATAL_ASSERT( abs(dy) <= cellSize, "DY is larger than cell size (%d>%d)", dy, cellSize );
						RED_UNUSED(cellSize);

						// check
						const Int64 r2 = (Int64)e.m_radius * (Int64)e.m_radius;
						if ( d2 <= r2 )
						{
							outCollector.Add( e.m_data, d2 );
						}
					}

					// advance to next bucket
					bucketIndex = bucket.m_nextBucket;
				}
			}
		}

		// adjust cell pos for the upper level
		cellX /= 2;
		cellY /= 2;
	}

	// we should end up in 0,0 cell
	RED_FATAL_ASSERT( cellX == 0, "Cell counting problem" );
	RED_FATAL_ASSERT( cellY == 0, "Cell counting problem" );
}

void CStreamingGrid::CollectForArea( Uint16 minX, Uint16 minY, Uint16 maxX, Uint16 maxY, class CStreamingGridCollector& outCollector ) const
{
	PC_SCOPE( StreamingGridCollectArea );

	// visit each level, start from the bottom
	Int32 maxLevel = (m_numLevels-1);
	Int32 cellMinX = minX >> (15 - maxLevel);
	Int32 cellMinY = minY >> (15 - maxLevel);
	Int32 cellMaxX = maxX >> (15 - maxLevel);
	Int32 cellMaxY = maxY >> (15 - maxLevel);
	for ( Int32 level = maxLevel; level >= 0; --level )
	{
		const auto* nodes = m_levels[level].m_nodes;

		// number of cells in this level
		const Int32 levelSize = 2 << level;
		RED_FATAL_ASSERT( cellMinX < levelSize, "Cell coordinate out of range (%d, max=%d, level=%d)", cellMinX, levelSize, level );
		RED_FATAL_ASSERT( cellMinY < levelSize, "Cell coordinate out of range (%d, max=%d, level=%d)", cellMinY, levelSize, level );
		RED_FATAL_ASSERT( cellMaxX < levelSize, "Cell coordinate out of range (%d, max=%d, level=%d)", cellMaxX, levelSize, level );
		RED_FATAL_ASSERT( cellMaxY < levelSize, "Cell coordinate out of range (%d, max=%d, level=%d)", cellMaxY, levelSize, level );

		// scan buckets in each node
		for ( Int32 y=cellMinY; y<=cellMaxY; ++y )
		{
			for ( Int32 x=cellMinX; x<=cellMaxX; ++x )
			{
				const Int32 cellIndex = x + (y*levelSize);

				TBucketIndex bucketIndex = nodes[ cellIndex ].m_bucket;
				while ( bucketIndex != 0 )
				{
					const GridBucket& bucket = m_buckets[ bucketIndex ];

					// test elements, all elements from this bucket are in cache, dirt cheap test
					for ( Uint32 c=0; c<bucket.m_elemCount; ++c )
					{
						const GridElement& e = bucket.m_elems[c];

						// test proxy position
						if ( e.m_x >= minX && e.m_x <= maxX && e.m_y >= minY && e.m_y <= maxY )
						{
							outCollector.Add( e.m_data, 0 );
						}
					}

					// advance to next bucket
					bucketIndex = bucket.m_nextBucket;
				}
			}
		}

		// adjust cell pos for the upper level
		cellMinX /= 2;
		cellMinY /= 2;
		cellMaxX /= 2;
		cellMaxY /= 2;
	}

	// we should end up in 0,0 cell
	RED_FATAL_ASSERT( cellMinX == 0, "Cell counting problem" );
	RED_FATAL_ASSERT( cellMinY == 0, "Cell counting problem" );
	RED_FATAL_ASSERT( cellMaxX == 0, "Cell counting problem" );
	RED_FATAL_ASSERT( cellMaxY == 0, "Cell counting problem" );
}

void CStreamingGrid::CreateGrid( const Uint32 numLevels )
{
	// calculate the total cell count
	Uint32 numTotalCells = 0;
	for ( Uint32 i=0; i<numLevels; ++i )
	{
		const Uint32 gridSize = 2 << i; // level0 - 2x2, level1- 4x4, etc
		numTotalCells += gridSize * gridSize;
	}

	// preallocate memory for nodes
	m_numNodes = numTotalCells;
	m_nodes = (GridNode*) RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_EntityManager, sizeof(GridNode) * m_numNodes );
	Red::MemoryZero( m_nodes, sizeof(GridNode) * m_numNodes );

	// preallocate memory for grid levels
	m_numLevels = numLevels;
	m_levels = (GridLevel*) RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_EntityManager, sizeof(GridLevel) * m_numLevels );

	// setup levels
	numTotalCells = 0;
	for ( Uint32 i=0; i<numLevels; ++i )
	{
		const Uint32 gridSize = 2 << i; // level0 - 2x2, level1- 4x4, etc

		m_levels[i].m_levelSize = (Uint16)gridSize;
		m_levels[i].m_nodeCount = gridSize * gridSize;
		m_levels[i].m_nodes = &m_nodes[ numTotalCells ];

		numTotalCells += gridSize * gridSize;
	}

	// stats
	LOG_CORE( TXT("Entity streaming grid is using %1.2fKB (%d levels, %d cells)"), 
		(sizeof(GridNode) * m_numNodes) / 1024.0f, numLevels, numTotalCells );
}

