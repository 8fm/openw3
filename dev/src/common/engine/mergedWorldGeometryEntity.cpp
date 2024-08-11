/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "mergedWorldGeometryEntity.h"

IMPLEMENT_ENGINE_CLASS( CMergedWorldGeometryEntity );
IMPLEMENT_ENGINE_CLASS( CMergedWorldGeometryGridCoordinates );

//---------------

CMergedWorldGeometryGridCoordinates::CMergedWorldGeometryGridCoordinates()
	: m_x( -1 )
	, m_y( -1 )
{
}

CMergedWorldGeometryGridCoordinates::CMergedWorldGeometryGridCoordinates( const Int16 x, const Int16 y )
	: m_x( x )
	, m_y( y )
{
}

CMergedWorldGeometryGridCoordinates::CMergedWorldGeometryGridCoordinates( const CMergedWorldGeometryGridCoordinates& other )
	: m_x( other.m_x )
	, m_y( other.m_y )
{
}

//---------------

CMergedWorldGeometryEntity::CMergedWorldGeometryEntity()
	: m_sourceDataHash(0)
	, m_statsDataSize(0)
	, m_statsNumTriangles(0)
	, m_statsNumVertices(0)
{
}

void CMergedWorldGeometryEntity::Setup( const Uint64 sourceDataHash, const Box& worldBounds, const CMergedWorldGeometryGridCoordinates& gridCoordinates )
{
	m_sourceDataHash = sourceDataHash;
	m_worldBounds = worldBounds;
	m_gridCoordinates = gridCoordinates;

	m_statsDataSize = 0;
	m_statsNumTriangles = 0;
	m_statsNumVertices = 0;

	// the distance is calculated in a flat mode
	//SetStaticFlag( ESF_FlatStremaingDistance, true );
}

void CMergedWorldGeometryEntity::AccumulatePayloadStats( const Uint32 dataSize, const Uint32 numTriangles, const Uint32 numVertices )
{
	m_statsDataSize += dataSize;
	m_statsNumTriangles += numTriangles;
	m_statsNumVertices += numVertices;
}

//---------------
