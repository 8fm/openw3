/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "pathlibDetailedSurfaceData.h"

#include "pathlibAreaDescription.h"
#include "pathlibTerrainInfo.h"
#include "pathlibWorld.h"

namespace PathLib
{

struct SSmoothOutAlgorithm
{
	struct Field
	{
		Int16				m_x;
		Int16				m_y;
	};

	struct GlobalField : public Field
	{
		Int16				m_tileX;
		Int16				m_tileY;
	};

	struct RegionInfo
	{
		Uint32				m_size;
		Bool				m_isNeigbouringWater;
		Bool				m_isNeigbouringNavmesh;
		Bool				m_isConnectedToNavmesh;

		GlobalField			m_minBoundings;
		GlobalField			m_maxBoundings;

		RegionInfo()
			: m_size( 0 )
			, m_isNeigbouringWater( false )
			, m_isNeigbouringNavmesh( false )
			, m_isConnectedToNavmesh( false )													{}
	};

	struct NeighbourArea 
	{
		TDynArray< Field >	m_field;

		Int16				m_tileX;
		Int16				m_tileY;
	};

	typedef TDynArray< Field >			ProcessingList;

	ProcessingList						m_pendingQueue;
	TDynArray< GlobalField >			m_currentlyProcessedRegion;
	THashMap< AreaId, ProcessingList >	m_neighbouringPendingLists;
	THashSet< AreaId >					m_pendingAreas;

	const CPathLibSettings&				m_globalSettings;
	const CTerrainInfo&					m_info;
	CDetailedSurfaceCollection*			m_surfaceCollection;

	Int32								m_resolution;
	Uint32								m_minFreeRegionSize;
	Uint32								m_minBlockedRegionSize;
	Uint32								m_minNavmeshBoundedRegionSize;
	Uint32								m_maxInterestingSize;


	SSmoothOutAlgorithm( const CPathLibSettings& settings, const CTerrainInfo& info, CDetailedSurfaceCollection* collection )
		: m_globalSettings( settings )
		, m_info( info )
		, m_surfaceCollection( collection )
		, m_resolution( info.GetTilesResolution() )
	{
		Float quadSize = info.GetQuadSize();
		Float quadSizeSqr = quadSize * quadSize;

		m_minFreeRegionSize = Uint32( MCeil( settings.GetTerrainWalkableRegionMinSize() / quadSizeSqr ) );
		m_minBlockedRegionSize = Uint32( MCeil( settings.GetTerrainUnwalkableRegionMinSize() / quadSizeSqr ) );
		m_minNavmeshBoundedRegionSize = Uint32( MCeil( settings.GetTerrainNavmeshSurroundedRegionMinSize() / quadSizeSqr ) );

		m_maxInterestingSize = Max( m_minFreeRegionSize, Max( m_minBlockedRegionSize, m_minNavmeshBoundedRegionSize ) ) + 1;

		m_currentlyProcessedRegion.Reserve( m_maxInterestingSize );
	}

	void FloodFill( CDetailedSurfaceData::FieldData::ESurfaceState surfaceState )
	{
		Int16 tileX = -1;
		Int16 tileY = -1;
		CDetailedSurfaceData* surface = nullptr;
		for ( auto it = m_currentlyProcessedRegion.Begin(), end = m_currentlyProcessedRegion.End(); it != end; ++it )
		{
			GlobalField visited = *it;
			if ( visited.m_tileX != tileX || visited.m_tileY != tileY )
			{
				tileX = visited.m_tileX;
				tileY = visited.m_tileY;
				surface = m_surfaceCollection->GetSurface( m_info.GetTileIdFromCoords( tileX, tileY ) );
			}

			CDetailedSurfaceData::FieldData field = surface->GetField( visited.m_x, visited.m_y );
			field.m_surface = surfaceState;
			surface->SetField( visited.m_x, visited.m_y, field );

		}
	}

	void AddToPending( Int16 x, Int16 y, Int16 tileX, Int16 tileY, Bool addAsProcessedRegion, ProcessingList& pendingList )
	{
		GlobalField f;
		f.m_x = x;
		f.m_y = y;
		pendingList.PushBack( f );
		if ( addAsProcessedRegion )
		{
			f.m_tileX = tileX;
			f.m_tileY = tileY;
			m_currentlyProcessedRegion.PushBack( f );
		}
	}

	Bool CheckField( CDetailedSurfaceData::FieldData field, CDetailedSurfaceData::FieldData::ESurfaceState surfaceState, RegionInfo& outRegionInfo )
	{
		if ( field.m_isInstanceConnection )
		{
			outRegionInfo.m_isConnectedToNavmesh = true;
		}

		// check if its marked by instance
		if ( field.m_isMarkedByInstance )
		{
			outRegionInfo.m_isNeigbouringNavmesh = true;
			return false;
		}
		// check if its a water
		if ( field.m_surface == CDetailedSurfaceData::FieldData::ESURFACE_Underwater )
		{
			outRegionInfo.m_isNeigbouringWater = true;
		}
		// check if it was already visited
		if ( field.m_isSmoothed )
		{
			return false;
		}

		// check if surface type match
		if ( field.m_surface != surfaceState )
		{
			return false;
		}

		return true;
	}

	void UpdateRegionBoundings( Int16 tileX, Int16 tileY, Int16 x, Int16 y, RegionInfo& outRegionInfo )
	{
		// minX
		if ( outRegionInfo.m_minBoundings.m_tileX == tileX && outRegionInfo.m_minBoundings.m_x > x )
		{
			outRegionInfo.m_minBoundings.m_x = x;
		}
		else if ( outRegionInfo.m_minBoundings.m_tileX > tileX )
		{
			outRegionInfo.m_minBoundings.m_tileX = tileX;
			outRegionInfo.m_minBoundings.m_x = x;
		}

		// maxX
		if ( outRegionInfo.m_maxBoundings.m_tileX == tileX && outRegionInfo.m_maxBoundings.m_x < x )
		{
			outRegionInfo.m_maxBoundings.m_x = x;
		}
		else if ( outRegionInfo.m_maxBoundings.m_tileX < tileX )
		{
			outRegionInfo.m_maxBoundings.m_tileX = tileX;
			outRegionInfo.m_maxBoundings.m_x = x;
		}

		// minY
		if ( outRegionInfo.m_minBoundings.m_tileY == tileY && outRegionInfo.m_minBoundings.m_y > y )
		{
			outRegionInfo.m_minBoundings.m_y = y;
		}
		else if ( outRegionInfo.m_minBoundings.m_tileY > tileY )
		{
			outRegionInfo.m_minBoundings.m_tileY = tileY;
			outRegionInfo.m_minBoundings.m_y = y;
		}

		// maxY
		if ( outRegionInfo.m_maxBoundings.m_tileY == tileY && outRegionInfo.m_maxBoundings.m_y < y )
		{
			outRegionInfo.m_maxBoundings.m_y = y;
		}
		else if ( outRegionInfo.m_maxBoundings.m_tileY < tileY )
		{
			outRegionInfo.m_maxBoundings.m_tileY = tileY;
			outRegionInfo.m_maxBoundings.m_y = y;
		}
	}

	void ComputeRegionNeighbour( Int16 tileX, Int16 tileY, CDetailedSurfaceData* surfaceData, Int16 x, Int16 y, CDetailedSurfaceData::FieldData::ESurfaceState surfaceState, RegionInfo& outRegionInfo, ProcessingList& pendingList )
	{
		CDetailedSurfaceData::FieldData field = surfaceData->GetField( x, y );
		
		if ( !CheckField( field, surfaceState, outRegionInfo ) )
		{
			return;
		}

		// mark as processed
		field.m_isSmoothed = true;
		surfaceData->SetField( x, y, field );

		// update area boundings
		UpdateRegionBoundings( tileX, tileY, x, y, outRegionInfo );

		// update region size
		++outRegionInfo.m_size;

		// add to pending list
		AddToPending( x, y, tileX, tileY, outRegionInfo.m_size < m_maxInterestingSize, pendingList );
	}

	void ComputeRegionAtTile( Int16 tileX, Int16 tileY, CDetailedSurfaceData* surfaceData, CDetailedSurfaceData::FieldData::ESurfaceState surfaceState, RegionInfo& outRegionInfo )
	{
		const Int32 tilesInRow = m_info.GetTilesInRow();

		AreaId leftAreaId, rightAreaId, upAreaId, downAreaId;
		leftAreaId = rightAreaId = upAreaId = downAreaId = INVALID_AREA_ID;

		CDetailedSurfaceData* leftSurface, *rightSurface, *upSurface, *downSurface;
		leftSurface = rightSurface = upSurface = downSurface = nullptr;

		// common function to determine processing list
		auto funGetAreaAndSurface = [ this ] ( Int16 tileX, Int16 tileY, AreaId& outAreaId, CDetailedSurfaceData* outSurface )
		{
			outAreaId = m_info.GetTileIdFromCoords( tileX, tileY );
			outSurface = m_surfaceCollection->GetSurface( outAreaId );
		};

		// determine left list
		if ( tileX > 0 )
		{
			funGetAreaAndSurface( tileX-1, tileY, leftAreaId, leftSurface );
		}
		// determine right list
		if ( tileX < tilesInRow-1 )
		{
			funGetAreaAndSurface( tileX+1, tileY, rightAreaId, rightSurface );
		}
		// determine up list
		if ( tileY > 0 )
		{
			funGetAreaAndSurface( tileX, tileY-1, upAreaId, upSurface );
		}
		// determine down list
		if ( tileY < tilesInRow-1 )
		{
			funGetAreaAndSurface( tileX, tileY+1, downAreaId, downSurface );
		}

		do 
		{
			Field currentField = m_pendingQueue.PopBackFast();

			Int16 x = currentField.m_x;
			Int16 y = currentField.m_y;
			
			// left neighbour (x-1)
			if ( x > 0 )
			{
				ComputeRegionNeighbour( tileX, tileY, surfaceData, x-1, y, surfaceState, outRegionInfo, m_pendingQueue );
			}
			else if ( leftSurface )
			{
				// pass test to left tile
				ComputeRegionNeighbour( tileX-1, tileY, leftSurface, m_resolution-1, y, surfaceState, outRegionInfo, m_neighbouringPendingLists[ leftAreaId ] );
			}

			// right neigbour (x+1)
			if ( x < m_resolution-1 )
			{
				ComputeRegionNeighbour( tileX, tileY, surfaceData, x+1, y, surfaceState, outRegionInfo, m_pendingQueue );
			}
			else if ( rightSurface )
			{
				// pass test to right tile
				ComputeRegionNeighbour( tileX+1, tileY, rightSurface, 0, y, surfaceState, outRegionInfo, m_neighbouringPendingLists[ rightAreaId ] );
			}

			// lower neighbour (y-1)
			if ( y > 0 )
			{
				ComputeRegionNeighbour( tileX, tileY, surfaceData, x, y-1, surfaceState, outRegionInfo, m_pendingQueue );
			}
			else if ( downSurface )
			{
				// pass test to tile down
				ComputeRegionNeighbour( tileX, tileY-1, downSurface, x, m_resolution-1, surfaceState, outRegionInfo, m_neighbouringPendingLists[ downAreaId ] );
			}

			// upper neighbour (y+1)
			if ( y < m_resolution-1 )
			{
				ComputeRegionNeighbour( tileX, tileY, surfaceData, x, y+1, surfaceState, outRegionInfo, m_pendingQueue );
			}
			else if ( upSurface)
			{
				// pass test to tile up
				ComputeRegionNeighbour( tileX, tileY+1, upSurface, x, 0, surfaceState, outRegionInfo, m_neighbouringPendingLists[ upAreaId ] );
			}

		} while ( !m_pendingQueue.Empty() );

		auto funAddPaendingArea = [ this ] ( PathLib::AreaId areaId )
		{
			if ( !( areaId == INVALID_AREA_ID || m_neighbouringPendingLists[ areaId ].Empty() ) )
			{
				m_pendingAreas.Insert( areaId );
			}
		};

		funAddPaendingArea( leftAreaId );
		funAddPaendingArea( rightAreaId );
		funAddPaendingArea( upAreaId );
		funAddPaendingArea( downAreaId );
	}

	void ComputeRegion( Int16 baseTileX, Int16 baseTileY, CDetailedSurfaceData* surfaceData, Int16 x, Int16 y, CDetailedSurfaceData::FieldData::ESurfaceState surfaceState, RegionInfo& outRegionInfo )
	{
		// setup output
		outRegionInfo.m_minBoundings.m_tileX = baseTileX;
		outRegionInfo.m_minBoundings.m_tileY = baseTileY;
		outRegionInfo.m_minBoundings.m_x = x;
		outRegionInfo.m_minBoundings.m_y = y;

		outRegionInfo.m_maxBoundings.m_tileX = baseTileX;
		outRegionInfo.m_maxBoundings.m_tileY = baseTileY;
		outRegionInfo.m_maxBoundings.m_x = x;
		outRegionInfo.m_maxBoundings.m_y = y;

		{
			// accumulate info from initial quad
			CDetailedSurfaceData::FieldData fieldData = surfaceData->GetField( x, y );
			
			if ( fieldData.m_isInstanceConnection )
			{
				outRegionInfo.m_isConnectedToNavmesh = true;
			}

			outRegionInfo.m_size = 1;
		}
		
		
		// add initial node to pending list 
		AddToPending( x, y, baseTileX, baseTileY, true, m_pendingQueue );

		ComputeRegionAtTile( baseTileX, baseTileY, surfaceData, surfaceState, outRegionInfo );

		while ( !m_pendingAreas.Empty() )
		{
			auto begin = m_pendingAreas.Begin();
			AreaId areaToProcess = *begin;
			m_pendingAreas.Erase( begin );

			// find area processing list
			auto itFind = m_neighbouringPendingLists.Find( areaToProcess );
			// check if processing list is indeed build
			if ( itFind == m_neighbouringPendingLists.End() )
			{
				continue;
			}
			// move events to current processing list
			m_pendingQueue = Move( itFind->m_second );
			m_neighbouringPendingLists.Erase( itFind );

			// check if we have any nodes to process
			if ( m_pendingQueue.Empty() )
			{
				continue;
			}
			
			Int32 processingTileX, processingTileY;
			m_info.GetTileCoordsFromId( areaToProcess, processingTileX, processingTileY );

			CDetailedSurfaceData* surfaceData = m_surfaceCollection->GetSurface( areaToProcess );

			ComputeRegionAtTile( processingTileX, processingTileY, surfaceData, surfaceState, outRegionInfo );
		}
	}

	void ProcessSurface( Int16 tileX, Int16 tileY, CDetailedSurfaceData* surfaceData )
	{
		// process all quads
		for ( Int16 y = 0; y < m_resolution; ++y )
		{
			for ( Int16 x = 0; x < m_resolution; ++x )
			{
				CDetailedSurfaceData::FieldData field = surfaceData->GetField( x, y );

				// check if quad was alread processed
				if ( field.m_isSmoothed )
				{
					continue;
				}

				// mark as processed
				field.m_isSmoothed = true;
				surfaceData->SetField( x, y, field );

				// ignore terrain slope info under instances
				if ( !field.m_isMarkedByInstance )
				{
					if ( field.m_surface == CDetailedSurfaceData::FieldData::ESURFACE_Ok || field.m_surface == CDetailedSurfaceData::FieldData::ESURFACE_Slope )
					{
						RegionInfo regionInfo;
						ComputeRegion( tileX, tileY, surfaceData, x, y, field.m_surface, regionInfo );

						Bool floodFill = false;
						if ( field.m_surface ==  CDetailedSurfaceData::FieldData::ESURFACE_Ok )
						{
							if ( !regionInfo.m_isConnectedToNavmesh )
							{
								if ( regionInfo.m_isNeigbouringNavmesh )
								{
									// navmesh bounded region
									if ( regionInfo.m_size <= m_minNavmeshBoundedRegionSize )
									{
										floodFill = true;
									}
								}
								else if ( regionInfo.m_size <= m_minFreeRegionSize )
								{
									floodFill = true;
								}
							}
						}
						else
						{
							// unwalkable bounded region
							if ( regionInfo.m_size <= m_minBlockedRegionSize )
							{
								floodFill = true;
							}
						}

						if ( floodFill )
						{
							FloodFill( field.m_surface == CDetailedSurfaceData::FieldData::ESURFACE_Ok ? CDetailedSurfaceData::FieldData::ESURFACE_Slope : CDetailedSurfaceData::FieldData::ESURFACE_Ok );
						}

						m_currentlyProcessedRegion.ClearFast();
					}
				}
			}
		}
	}

	void Process()
	{
		// process all tiles
		Uint32 tilesInRow = m_info.GetTilesInRow();
		for ( Uint16 tileX = 0; tileX < tilesInRow; ++tileX )
		{
			for ( Uint16 tileY = 0; tileY < tilesInRow; ++tileY )
			{
				AreaId areaId = m_info.GetTileIdFromCoords( tileX, tileY );
				CDetailedSurfaceData* surfaceData = m_surfaceCollection->GetSurface( areaId );
				ProcessSurface( tileX, tileY, surfaceData );

				PATHLIB_LOG( TXT("Completed smoothing for %d %d."), tileX, tileY );
			}
		}
	}

};


////////////////////////////////////////////////////////////////////////////
// CDetailedSurfaceData
////////////////////////////////////////////////////////////////////////////
CDetailedSurfaceData::CDetailedSurfaceData()
	: m_resolution( 0xbaada555 )
{

}

void CDetailedSurfaceData::Initialize( Uint32 resolution )
{
	m_resolution = resolution;

	m_data.Resize( resolution * resolution );
	Red::System::MemoryZero( m_data.Data(), m_data.DataSize() );
	//m_data.SetZero();
}

void CDetailedSurfaceData::ClearProcessingFlag()
{
	for( Uint32 i = 0, n = m_resolution*m_resolution; i != n; ++i )
	{
		m_data[ i ].m_isSmoothed = false;
	}
}

void CDetailedSurfaceData::GlobalSmoothOutProcess( CPathLibWorld& pathlib, CDetailedSurfaceCollection* collection )
{
	const CPathLibSettings& settings = pathlib.GetGlobalSettings();
	const CTerrainInfo& info = pathlib.GetTerrainInfo();
	SSmoothOutAlgorithm algorithm( settings, info, collection );
	algorithm.Process();
}

////////////////////////////////////////////////////////////////////////////
// CDetailedSurfaceCollection
////////////////////////////////////////////////////////////////////////////

void CDetailedSurfaceCollection::Initialize( CPathLibWorld& pathlib )
{
	const PathLib::CTerrainInfo& terrainInfo = pathlib.GetTerrainInfo();

	Uint32 tilesCount = terrainInfo.GetTilesCount();
	Uint32 tileResolution = terrainInfo.GetTilesResolution();
	m_surfaceData.Resize( tilesCount );

	for ( auto it = m_surfaceData.Begin(), end = m_surfaceData.End(); it != end; ++it )
	{
		it->Initialize( tileResolution );
	}

}
void CDetailedSurfaceCollection::Clear()
{
	m_surfaceData.Clear();
}

CDetailedSurfaceData* CDetailedSurfaceCollection::GetSurface( AreaId areaId )
{
	return &m_surfaceData[ areaId & CAreaDescription::ID_MASK_INDEX ];
}

};			// namespace PathLib

