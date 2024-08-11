/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderTerrainLogicMap.h"

CRenderTerrainLogicMap::CRenderTerrainLogicMap()
{
	Red::System::MemoryZero( m_chunks, sizeof(m_chunks) );
}

CRenderTerrainLogicMap::~CRenderTerrainLogicMap()
{
}

void CRenderTerrainLogicMap::AddChunk( CRenderProxy_TerrainChunk* chunk, Int32 logicX, Int32 logicY, Int32 logicSize )
{
	// Check size
	ASSERT( logicX >= 0 && logicX + logicSize <= MAX_TERRAIN_LOGIC_UNIT );
	ASSERT( logicY >= 0 && logicY + logicSize <= MAX_TERRAIN_LOGIC_UNIT );

	// Set crap
	for ( Int32 y=0; y<logicSize; ++y )
	{
		for ( Int32 x=0; x<logicSize; ++x )
		{
			CRenderProxy_TerrainChunk*& slot = m_chunks[ y + logicY ][ x + logicX ];
			ASSERT( slot == NULL );
			slot = chunk;
		}
	}
}

void CRenderTerrainLogicMap::RemoveChunk( CRenderProxy_TerrainChunk* chunk, Int32 logicX, Int32 logicY, Int32 logicSize )
{
	// Check size
	ASSERT( logicX >= 0 && logicX + logicSize <= MAX_TERRAIN_LOGIC_UNIT );
	ASSERT( logicY >= 0 && logicY + logicSize <= MAX_TERRAIN_LOGIC_UNIT );

	// Set crap
	for ( Int32 y=0; y<logicSize; ++y )
	{
		for ( Int32 x=0; x<logicSize; ++x )
		{
			CRenderProxy_TerrainChunk*& slot = m_chunks[ y + logicY ][ x + logicX ];
			ASSERT( slot == chunk );
			slot = NULL;
		}
	}
}

CRenderProxy_TerrainChunk* CRenderTerrainLogicMap::GetChunk( Int32 logicX, Int32 logicY )
{
	if ( logicX >= 0 && logicY >= 0 && logicX < MAX_TERRAIN_LOGIC_UNIT && logicY < MAX_TERRAIN_LOGIC_UNIT )
	{
		CRenderProxy_TerrainChunk*& slot = m_chunks[ logicY ][ logicX ];
		return slot;
	}

	return NULL;
}
