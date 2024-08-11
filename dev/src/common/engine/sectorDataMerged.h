#pragma once

#include "../core/hashmap.h"
#include "../core/idAllocator.h"
#include "../core/bitset.h"

#include "sectorDataGlobals.h"
#include "sectorData.h"

class CStreamingGrid;
class CStreamingPositionQuantizer;

/// Runtime helper class for merging sector data
class CSectorDataMerged : public CSectorData
{
	DECLARE_RTTI_SIMPLE_CLASS( CSectorDataMerged );
	NO_DEFAULT_CONSTRUCTOR( CSectorDataMerged );

public:
	CSectorDataMerged( const Float worldSize, const Bool hasPrefetchData );
	~CSectorDataMerged();

	/// Append data from another sector into this big blob, mark them with unique sector ID
	/// NOTE: there should be no streaming happening during this call, this is expensive call, do not do it to often
	/// returns internal ID of the sector inside the streaming system
	Uint32 AppendSectorData( const CSectorData* data );

	/// Remove data from merged sector block
	/// NOTE: there should be no streaming happening during this call, this is expensive call, do not do it to often, 
	/// NOTE: we loose space every time we unload data, the lost space is NEVER reused ATM (we don't need that for W3 because we load all the layers)
	void RemoveSectorData( const Uint32 id );

	//! Collect entries in range for given position
	//! Entries are returned using provided collector
	void CollectForPoint( const Vector& point, class CStreamingGridCollector& outCollector ) const;

	//! Collect entries in 3D object area
	//! Entries are returned using provided collector
	void CollectForAreas( const Box* areas, const Uint32 numAreas, class CStreamingGridCollector& outCollector ) const;

	//! Collect all resources referenced at given location and radius
	void CollectResourcesFromLocation( const Vector& point, const Float radius, TDynArray< SectorData::PackedResourceRef >& outResourceIndices ) const;

private:
	IDAllocator< SectorData::MAX_SECTORS >		m_sectorFreeIndices;	// free indices for the sector masks
	IDAllocator< SectorData::MAX_OBJECTS >		m_objectFreeIndices;	// free indices in the object list
	
	THashMap< Uint64, Uint32 >					m_resourceMap;			// global resource map

	// streaming grid support
	Float										m_worldSize;
	CStreamingGrid*								m_grid;
	CStreamingPositionQuantizer*				m_quantizer;

	// do we have prefetch data
	Bool										m_hasPrefetchData;

	// calculate object radius
	const Float CalcRadius( const SectorData::PackedObject& object ) const;
};

BEGIN_CLASS_RTTI( CSectorDataMerged );
	PARENT_CLASS( CSectorData );
END_CLASS_RTTI();
