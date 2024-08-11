/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

/// Grid coordinates
struct CMergedWorldGeometryGridCoordinates
{
	DECLARE_RTTI_STRUCT( CMergedWorldGeometryGridCoordinates );

public:
	Int16	m_x;
	Int16	m_y;

public:
	CMergedWorldGeometryGridCoordinates(); // -1,-1 - invalid
	CMergedWorldGeometryGridCoordinates( const Int16 x, const Int16 y );
	CMergedWorldGeometryGridCoordinates( const CMergedWorldGeometryGridCoordinates& other );

	RED_INLINE CMergedWorldGeometryGridCoordinates& operator=( const CMergedWorldGeometryGridCoordinates& other )
	{
		m_x = other.m_x;
		m_y = other.m_y;
		return *this;
	}

	RED_INLINE const Bool operator==( const CMergedWorldGeometryGridCoordinates& other ) const
	{
		return (m_x == other.m_x) && (m_y == other.m_y);
	}

	RED_INLINE const Bool operator!=( const CMergedWorldGeometryGridCoordinates& other ) const
	{
		return (m_x != other.m_x) || (m_y != other.m_y);
	}

	RED_INLINE const Bool operator<( const CMergedWorldGeometryGridCoordinates& other ) const
	{
		if ( m_x < other.m_x ) return true;
		if ( m_x > other.m_x ) return false;
		return m_y < other.m_y;
	}

	RED_INLINE const Bool IsValid() const
	{
		return (m_x >= -1) && (m_y >= 0);
	}

	RED_INLINE const Uint32 CalcHash() const
	{
		return (Uint32)m_x | ((Uint32)m_y << 16);
	}
};

BEGIN_CLASS_RTTI( CMergedWorldGeometryGridCoordinates );
	PROPERTY_RO( m_x, TXT("X coordinate in grid space") );
	PROPERTY_RO( m_y, TXT("Y coordinate in grid space") );
END_CLASS_RTTI();

/// Entity representing a merged area in the world for which automatic geometry was created
class CMergedWorldGeometryEntity : public CEntity
{
	DECLARE_ENGINE_CLASS( CMergedWorldGeometryEntity, CEntity, 0 );

public:
	CMergedWorldGeometryEntity();

	// Setup entity for given region
	void Setup( const Uint64 sourceDataHash, const Box& worldBounds, const CMergedWorldGeometryGridCoordinates& gridCoordinates );

	// Accumulat stats about payload data
	void AccumulatePayloadStats( const Uint32 dataSize, const Uint32 numTriangles, const Uint32 numVertices );

	// Get source data hash
	RED_INLINE const Uint64 GetSourceDataHash() const { return m_sourceDataHash; }

	// Get world space bounds of the region for which this data was generated
	RED_INLINE const Box& GetWorldBounds() const { return m_worldBounds; }

	// Get internal grid coordinates
	RED_INLINE const CMergedWorldGeometryGridCoordinates& GetGridCoordinates() const { return m_gridCoordinates; }

	// Get size of the runtime payload in this area
	RED_INLINE const Uint32 GetPayloadDataSize() const { return m_statsDataSize; }

	// Get number of renderable triangles in this area
	RED_INLINE const Uint32 GetPayloadTriangleCount() const { return m_statsNumTriangles; }

	// Get number of renderable vertices in this area
	RED_INLINE const Uint32 GetPayloadVertexCount() const { return m_statsNumVertices; }

private:
	Uint64									m_sourceDataHash;			//!< Hash of the source data stored in the region (so we can skip computing it)
	Box										m_worldBounds;				//!< World bounds of the region stored here
	CMergedWorldGeometryGridCoordinates		m_gridCoordinates;			//!< Coordinates within the grid

	Uint32									m_statsDataSize;			//!< Size of the runtime payload in this area
	Uint32									m_statsNumTriangles;		//!< Number of renderable triangles in this area
	Uint32									m_statsNumVertices;			//!< Number of renderable vertices in this area
};

BEGIN_CLASS_RTTI( CMergedWorldGeometryEntity );
	PARENT_CLASS( CEntity );
	PROPERTY_RO( m_sourceDataHash, TXT("Hash of the source data stored in the region (so we can skip computing it)" ) );
	PROPERTY_RO( m_worldBounds, TXT("World bounds of the region stored here" ) );
	PROPERTY_RO( m_gridCoordinates, TXT("Coordinates within the grid" ) );
	PROPERTY_RO( m_statsDataSize, TXT("Size of the runtime payload in this area") );
	PROPERTY_RO( m_statsNumTriangles, TXT("Number of renderable triangles in this area") );
	PROPERTY_RO( m_statsNumVertices, TXT("Number of renderable vertices in this area") );
END_CLASS_RTTI();