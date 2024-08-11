/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "terrainEditWorkingBuffers.h"
#include "../../common/engine/terrainTile.h"
#include "../../common/engine/terrainUtils.h"


Uint32 STerrainWorkingBufferHeight::GetTileResolution( CTerrainTile* tile )
{
	return tile->GetResolution();
}

void STerrainWorkingBufferHeight::CopyFromTile( CTerrainTile* tile, Float* buffer )
{
	size_t numTexels = tile->GetResolution() * tile->GetResolution();

	const Uint16* tileHeights = tile->GetLevelSyncHM( 0 );
	for ( Uint32 i = 0; i < numTexels; ++i )
	{
		buffer[ i ] = ( Float )tileHeights[ i ];
	}
}

void STerrainWorkingBufferHeight::CopyRegionFromTile( CTerrainTile* tile, Float* buffer, const Rect& rect )
{
	Uint32 res = tile->GetResolution();
	const Uint16* tileBuff = tile->GetLevelSyncHM( 0 );
	for ( Int32 y = rect.m_top; y < rect.m_bottom; ++y )
	{
		for ( Int32 x = rect.m_left; x < rect.m_right; ++x )
		{
			buffer[ x + y * res ] = tileBuff[ x + y * res ];
		}
	}
}

void STerrainWorkingBufferHeight::CopyRegionToTile( CTerrainTile* tile, Float* buffer, const Rect& rect )
{
	Uint32 res = tile->GetResolution();
	Uint16* tileBuff = tile->GetLevelWriteSyncHM( 0 );
	for ( Int32 y = rect.m_top; y < rect.m_bottom; ++y )
	{
		Float* floatPtr = buffer + y * res + rect.m_left;
		Uint16* tilePtr = tileBuff + y * res + rect.m_left;
		for ( Int32 x = rect.m_left; x < rect.m_right; ++x )
		{
			// Clamp to valid range. We write back to the working buffer so that height sculpting works predictably -- raising until it
			// maxes out, and then lowering, will lower the clamped height instead of the out-of-range values that can't be seen.
			Float clamped = Clamp( *floatPtr, 0.0f, 65535.0f );
			*floatPtr = clamped;
			*tilePtr = ( Uint16 )clamped;

			++tilePtr;
			++floatPtr;
		}
	}
}
