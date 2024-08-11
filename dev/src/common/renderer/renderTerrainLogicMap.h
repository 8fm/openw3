/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CRenderProxy_TerrainChunk;

// Defines
#define MAX_TERRAIN_CHUNKS_PER_TILE		32
#define MAX_TERRAIN_TILES_IN_WORLD		8
#define MAX_TERRAIN_LOGIC_UNIT			( MAX_TERRAIN_CHUNKS_PER_TILE * MAX_TERRAIN_TILES_IN_WORLD )

/// Mapping of terrain neightbours
class CRenderTerrainLogicMap
{
private:
	CRenderProxy_TerrainChunk*		m_chunks[ MAX_TERRAIN_LOGIC_UNIT ][ MAX_TERRAIN_LOGIC_UNIT ];

public:
	CRenderTerrainLogicMap();
	~CRenderTerrainLogicMap();

	//! Add chunk to map
	void AddChunk( CRenderProxy_TerrainChunk* chunk, Int32 logicX, Int32 logicY, Int32 logicSize );

	//! Remove chunk from map
	void RemoveChunk( CRenderProxy_TerrainChunk* chunk, Int32 logicX, Int32 logicY, Int32 logicSize );

public:
	//! Get terrain chunk at given logic position
	CRenderProxy_TerrainChunk* GetChunk( Int32 logicX, Int32 logicY );
};