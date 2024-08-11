/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../../common/engine/terrainTypes.h"


// Working buffers will expire after 30 minutes of not being used.
#define TERRAIN_WORKING_BUFFER_MAX_TTL ( 30 * 60 )
// In total, working buffers of one type can use up to 64MB. If this is reached, least recently used buffers are destroyed.
#define TERRAIN_WORKING_BUFFER_MAX_DATA_SIZE ( 64 * 1024 * 1024 )


class CTerrainTile;



// While editing the terrain, touched tiles are copied into a temporary buffer, to allow higher precision or access
// to uncompressed data. These buffers are kept around for the duration of the tool, unless they timeout (enough
// time passes without any edits to the tile), or they take up too much memory.
template< typename BufferDetails >
class CTerrainWorkingBufferManager
{
public:
	typedef typename BufferDetails::WorkingDataType WorkingDataType;

private:
	struct SWorkingBuffer
	{
		Float ttl;
		WorkingDataType* buffer;
		size_t bufferSize;

		SWorkingBuffer()
			: ttl( 0.0f )
			, buffer( nullptr )
			, bufferSize( 0 )
		{}
	};

	typedef THashMap< CTerrainTile*, SWorkingBuffer > BuffersMap;

	BuffersMap	m_workingBuffers;
	size_t		m_workingBuffersDataSize;

public:

	CTerrainWorkingBufferManager()
		: m_workingBuffersDataSize( 0 )
	{}


	WorkingDataType* GetBufferForTile( CTerrainTile* tile )
	{
		SWorkingBuffer* workingBuff = m_workingBuffers.FindPtr( tile );
		if ( !workingBuff )
		{
			SWorkingBuffer newWorkingBuff;
			Uint32 res = BufferDetails::GetTileResolution( tile );
			newWorkingBuff.bufferSize = res * res * sizeof( WorkingDataType );

			// If adding this buffer will go over our size budget, free an old one.
			while ( m_workingBuffersDataSize + newWorkingBuff.bufferSize >= TERRAIN_WORKING_BUFFER_MAX_DATA_SIZE )
			{
				RemoveOldest();
			}

			newWorkingBuff.buffer = ( WorkingDataType* )RED_MEMORY_ALLOCATE( MemoryPool_TerrainEditor, MC_Editor, newWorkingBuff.bufferSize );

			LoadFromTile( tile, newWorkingBuff );

			m_workingBuffers.Set( tile, newWorkingBuff );
			m_workingBuffersDataSize += newWorkingBuff.bufferSize;

			workingBuff = m_workingBuffers.FindPtr( tile );
		}

		ASSERT( workingBuff && workingBuff->buffer, TXT("WorkingBuffer not added, or doesn't have a buffer... how can this be?") );
		if ( !workingBuff ) return nullptr;

		workingBuff->ttl = TERRAIN_WORKING_BUFFER_MAX_TTL;

		return workingBuff->buffer;
	}


	void CopyToTile( CTerrainTile* tile, const Rect& rect )
	{
		SWorkingBuffer* workingBuff = m_workingBuffers.FindPtr( tile );
		if ( !workingBuff ) return;

		BufferDetails::CopyRegionToTile( tile, workingBuff->buffer, rect );
	}


	// Only does anything if that tile already has a working buffer.
	void CopyFromTile( CTerrainTile* tile, const Rect& rect )
	{
		SWorkingBuffer* workingBuff = m_workingBuffers.FindPtr( tile );
		if ( !workingBuff ) return;

		BufferDetails::CopyRegionFromTile( tile, workingBuff->buffer, rect );
	}


	void ProcessTimeouts( Float dt )
	{
		TDynArray< CTerrainTile*, MC_Temporary > tilesToRemove;
		for ( BuffersMap::iterator it = m_workingBuffers.Begin(); it != m_workingBuffers.End(); ++it )
		{
			SWorkingBuffer& buff = it->m_second;
			buff.ttl -= dt;
			if ( buff.ttl <= 0 )
			{
				tilesToRemove.PushBack( it->m_first );
			}
		}
		// Remove the tiles that we freed.
		for ( Uint32 i = 0; i < tilesToRemove.Size(); ++i )
		{
			Remove( tilesToRemove[ i ] );
		}
	}

	void RemoveOldest()
	{
		// Find oldest.
		CTerrainTile* bestTileToRemove = NULL;
		Float bestTileTTL = 0;
		for ( BuffersMap::iterator it = m_workingBuffers.Begin(); it != m_workingBuffers.End(); ++it )
		{
			if ( !bestTileToRemove || it->m_second.ttl < bestTileTTL )
			{
				bestTileToRemove = it->m_first;
				bestTileTTL = it->m_second.ttl;
			}
		}
		ASSERT( bestTileToRemove, TXT("Couldn't find an oldest FloatHeightBuffer to remove! This is bad, but we're gonna try to prevent an infinite loop by pretending...") );
		if ( !bestTileToRemove )
		{
			// Just lower the data size a bit, so we aren't stuck.
			m_workingBuffersDataSize -= 256 * 256 * sizeof( WorkingDataType );
			return;
		}

		Remove( bestTileToRemove );
	}

	void ClearAll()
	{
		for ( BuffersMap::iterator it = m_workingBuffers.Begin(); it != m_workingBuffers.End(); ++it )
		{
			RED_MEMORY_FREE( MemoryPool_TerrainEditor, MC_Editor, it->m_second.buffer );
		}
		m_workingBuffers.ClearFast();
		m_workingBuffersDataSize = 0;
	}

	void Remove( CTerrainTile* tile )
	{
		BuffersMap::iterator it = m_workingBuffers.Find( tile );
		if ( it != m_workingBuffers.End() )
		{
			RED_MEMORY_FREE( MemoryPool_TerrainEditor, MC_Editor, it->m_second.buffer );
			m_workingBuffersDataSize -= it->m_second.bufferSize;

			m_workingBuffers.Erase( it );
		}
	}


	void LoadFromTile( CTerrainTile* tile, SWorkingBuffer& buffer )
	{
		BufferDetails::CopyFromTile( tile, buffer.buffer );
	}


	void RefreshFromTerrain()
	{
		for ( BuffersMap::iterator it = m_workingBuffers.Begin(); it != m_workingBuffers.End(); ++it )
		{
			LoadFromTile( it->m_first, it->m_second );
		}
	}
};




struct STerrainWorkingBufferHeight
{
	typedef Float WorkingDataType;

	static Uint32 GetTileResolution( CTerrainTile* tile );
	static void CopyFromTile( CTerrainTile* tile, Float* buffer );
	static void CopyRegionFromTile( CTerrainTile* tile, Float* buffer, const Rect& rect );
	static void CopyRegionToTile( CTerrainTile* tile, Float* buffer, const Rect& rect );
};
