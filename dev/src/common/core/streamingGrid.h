#pragma once

#include "idAllocator.h"

//-----------------------

/// Streaming grid - a multi level grid, optimal for storing objects with they own streaming range
/// Optimized for cache performance
class CStreamingGrid
{
public:
	CStreamingGrid( const Uint32 numLevels, const Uint32 numBuckets );
	~CStreamingGrid();

	//! Object index
	typedef Uint32 TGridHash;

	//! Get number of allocated levels
	const Uint32 GetNumLevels() const;

	//! Get number of allowed buckets
	const Uint32 GetMaxBuckets() const;

	//! Get number of allocated buckets
	const Uint32 GetNumBuckets() const;

	//! Get debug information
	void GetDebugInfo( const Uint32 level, struct SStreamingGridDebugData& outData ) const;

	//! Register object in the grid
	TGridHash Register( Uint16 x, Uint16 y, Uint16 z, Uint16 radius, const Uint32 data );

	//! Move object
	TGridHash Move( TGridHash hash, Uint16 newX, Uint16 newY, Uint16 newZ, const Uint32 data );

	//! Unregister object from the grid
	void Unregister( TGridHash hash, const Uint32 data );

	//! Unregister object from the grid using position
	void Unregister( Uint16 x, Uint16 y, Uint16 z, Uint16 radius, const Uint32 data );

	//! Collect entries in range for given position
	//! Entries are returned using provided collector
	void CollectForPoint( Uint16 x, Uint16 y, Uint16 z, class CStreamingGridCollector& outCollector ) const;

	//! Collect entries in 2D area
	//! Entries are returned using provided collector
	void CollectForArea( Uint16 minX, Uint16 minY, Uint16 maxX, Uint16 maxY, class CStreamingGridCollector& outCollector ) const;

private:
	typedef Uint32  TBucketIndex;

#pragma pack(push)
#pragma pack(1)
	struct GridElement // 10 bytes
	{
		Uint16	m_x;			// quantized world space position (X)
		Uint16	m_y;			// quantized world space position (Y)
		Uint16	m_z;			// quantized world space position (Z)
		Uint16	m_radius;		// quantized radius
		Uint32	m_data;			// user data
	};

	struct GridBucket // 64 bytes
	{
		static const Uint32 MAX = 5;

		GridElement		m_elems[MAX];		// elements
		Uint32			m_nextBucket:24;	// next bucket index
		Uint32			m_elemCount:8;		// number of elements in this bucket
	};

	struct GridNode
	{
		TBucketIndex	m_bucket;			// first bucket index
		Uint16			m_bucketCount;		// number of buckets in this node
	};

	struct GridLevel
	{
		GridNode*		m_nodes;
		Uint16			m_levelSize;
		Uint32			m_nodeCount;
	};

	static_assert( sizeof(GridElement) == 12, "Invalid size of grid element, this structure is cache optimized, do not tamper with it" );
	static_assert( sizeof(GridBucket) == 64, "Invalid size of grid bucket, this structure is cache optimized, do not tamper with it" );
	static_assert( sizeof(GridNode) == 6, "Invalid size of grid node, this structure is cache optimized, do not tamper with it" );
#pragma pack(pop)

	// grid levels
	GridLevel*			m_levels;
	Uint32				m_numLevels;

	// all grid cells
	GridNode*			m_nodes;
	Uint32				m_numNodes;

	// grid buckets
	GridBucket*				m_buckets;
	IDAllocatorDynamic		m_bucketIDs;
	Uint32					m_numBuckets;

	// initialize grid levels
	void CreateGrid( const Uint32 numLevels );
};

//-----------------------------

/// Grid data collector
class CStreamingGridCollector
{
public:
	struct Elem
	{
		Uint32	m_elem;
		Uint32	m_distance;
	};

	CStreamingGridCollector( Elem* elements, const Uint32 capacity );

	// sort object by distance
	void Sort();

	RED_FORCE_INLINE void Add( const Uint32 element, const Uint64 distance )
	{
		if ( m_numElements < m_maxElemenets )
		{
			m_elements[ m_numElements ].m_elem = element;
			m_elements[ m_numElements ].m_distance = (Uint32) (distance >> 1); // don't make it to accurate because we can overflow Uint32
			m_numElements += 1;
		}
	}

	RED_FORCE_INLINE const Uint32 Size() const
	{
		return m_numElements;
	}

	RED_FORCE_INLINE const Uint32 operator[]( const Uint32 index ) const
	{
		return m_elements[ index ].m_elem;
	}

private:
	Elem*		m_elements;
	Uint32		m_numElements;
	Uint32		m_maxElemenets;
};

//-----------------------------

/// Grid debug information
struct SStreamingGridDebugData
{
	Uint32		m_numBuckets;			//!< total number of used buckets on this level
	Uint32		m_numCells;				//!< number of used cells on this levle
	Uint32		m_numElements;			//!< number of elements in the cells
	Uint32		m_numWastedElements;	//!< number of elements that are not allocated (not including the last bucket)

	// element information
	struct Elem
	{
		Float	m_x;
		Float	m_y;
		Float	m_r;
	};

	TDynArray< Elem >	m_elems;

	// occupancy histogram
	Uint32					m_gridSize;
	Uint32					m_gridMaxElemCount;
	TDynArray< Uint32 >		m_gridElemCount;

	SStreamingGridDebugData()
		: m_numBuckets(0)
		, m_numCells(0)
		, m_numElements(0)
		, m_numWastedElements(0)
		, m_gridSize(0)
		, m_gridMaxElemCount(0)
	{}
};

//-----------------------------
