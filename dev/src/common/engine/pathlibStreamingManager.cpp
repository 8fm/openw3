#include "build.h"
#include "pathlibStreamingManager.h"

#include "../core/loadingJobManager.h"

#include "pathlibStreamingJob.h"
#include "pathlibTerrain.h"
#include "pathlibNavmeshArea.h"
#include "pathlibInstanceMap.h"
#include "pathlibTaskPostStreaming.h"
#include "pathlibTaskUnloadStreamingItems.h"
#include "world.h"
#include "baseEngine.h"

namespace PathLib
{

///////////////////////////////////////////////////////////////////////////////
// CStreamingManager::CBundleLoadedCallback
///////////////////////////////////////////////////////////////////////////////
CStreamingManager::CStreamingManager( CPathLibWorld& pathlib )
	: m_pathlib( pathlib )
	, m_currentJob( NULL )
	, m_currentJobIsCompleted( false )
	, m_versionStamp( 0 )
	//, m_celsInRow( 0xffff )
	, m_currentPlayerX( 0xffff )
	, m_currentPlayerY( 0xffff )
	, m_streamingLock( 0 )
	, m_streamedWorldBox( Vector( 0.f, 0.f, -FLT_MAX ), Vector( 0.f, 0.f, FLT_MAX ) )
	, m_referencePositionValid( false )
{
}
CStreamingManager::~CStreamingManager()
{
	PATHLIB_ASSERT( !m_currentJob );
}

void CStreamingManager::Initialize()
{
	const CTerrainInfo& terrainInfo = m_pathlib.GetTerrainInfo();
	const CPathLibSettings& globalSettings = m_pathlib.GetGlobalSettings();

	Float tileSize = terrainInfo.GetTileSize();
	Float desiredStreamingRange = globalSettings.GetDesiredStreamingRange();

	m_streamingRadius = Int32( desiredStreamingRange / tileSize + 0.9f );	// 'almost' ceil
	m_streamingRadius = Clamp( m_streamingRadius, Int32( 1 ), Int32( MAX_STREAMING_RADIUS ) );

	m_streamingRow = m_streamingRadius * 2 + 1;
	m_streamingSlots.Resize( m_streamingRow * m_streamingRow );

}
void CStreamingManager::Shutdown()
{
	if ( m_currentJob )
	{
		m_currentJob->ShutdownRequest();
		
		while ( !m_currentJob->HasEnded() )
		{
			Red::Threads::SleepOnCurrentThread( 1 );
		}

		m_currentJob->Release();
		m_currentJob = NULL;
	}

	m_streamingSlots.Clear();

	m_currentPlayerX = 0xffff;
	m_currentPlayerY = 0xffff;
}

void CStreamingManager::UpdateStreamedWorldBox()
{
	const CTerrainInfo& setup = m_pathlib.GetTerrainInfo();
	Vector2 terrainCorner = setup.GetTerrainCorner();
	Float tileSize = setup.GetTileSize();
	Float x = Float( m_currentPlayerX );
	Float y = Float( m_currentPlayerY );
	const Float r = Float( m_streamingRadius );

	// streamed world box Z boundings are always cool. So we just play with x,y coords.

	m_streamedWorldBox.Min.AsVector2().Set(
		terrainCorner.X + (x - r) * tileSize,
		terrainCorner.Y + (y - r) * tileSize );

	m_streamedWorldBox.Max.AsVector2().Set(
		terrainCorner.X + (x + r + 1.f) * tileSize,
		terrainCorner.X + (y + r + 1.f) * tileSize );
}

void CStreamingManager::SetReferencePosition( const Vector& position )
{
	m_referencePosition = position;
	m_referencePositionValid = true;
}
void CStreamingManager::ForceUnloadAllPendingItems()
{
	// force-unload all currently pending areas
	if ( !m_pendingToUnload.Empty() )
	{
		for ( auto it = m_pendingToUnload.Begin(), end = m_pendingToUnload.End(); it != end; ++it )
		{
			it->ForceImmediateUnstream();
		}
	}
}

void CStreamingManager::Tick()
{
	// update streaming slots
	if ( !m_referencePositionValid )
	{
		return;
	}

	UpdatePosition( m_referencePosition );

	// do unloading as it is quite easy to do
	if ( !m_pendingToUnload.Empty() )
	{
		IStreamingItem::List::Head unloadList;

		EngineTime time = GEngine->GetRawEngineTime();
		for ( auto it = m_pendingToUnload.Begin(), end = m_pendingToUnload.End(); it != end; )
		{
			IStreamingItem& item = *it;
			if ( item.GetUnstreamRequestTime() < time )
			{
				it = it.Erase();

				unloadList.ListInsert( item );
			}
			else
			{
				++it;
			}
		}

		if ( !unloadList.Empty() )
		{
			CTaskManager& taskManager = *m_pathlib.GetTaskManager();
			CTaskUnloadStreamingItems* task = new CTaskUnloadStreamingItems( taskManager, *this, Move( unloadList ) );
			taskManager.AddTask( task );
			task->Release();
		}
	}

	// update current job
	if ( m_currentJob && m_currentJob->HasEnded() )
	{
		m_currentJob->Release();
		m_currentJob = nullptr;
	}

	// start a new job
	if ( !m_pendingToLoad.Empty() && m_currentJob == nullptr &&  m_streamingLock.GetValue() == 0 )
	{
		// spawn new job
		m_currentJob = new CStreamingJob( Move( m_pendingToLoad ), this );
		m_currentJobIsCompleted = false;

		ASSERT( m_pendingToLoad.Empty() );

		m_currentJob->PreLoadSync();

		SJobManager::GetInstance().Issue( m_currentJob );
	}
}

Uint32 CStreamingManager::TileCoordsToStreamingSlotIndex( Int32 x, Int32 y )
{
	Int32 row = m_streamingRow;
	Int32 shiftX = m_currentPlayerX % row;
	Int32 shiftY = m_currentPlayerY % row;
	Int32 indexX = x - m_currentPlayerX + (row/2);
	Int32 indexY = y - m_currentPlayerY + (row/2);
	if ( indexX >= 0 && indexX < row && indexY >= 0 && indexY < row )
	{
		return ((indexY + shiftY)%row) * row + ((indexX + shiftX)%row);
	}
	return 0xffffffff;
}


void CStreamingManager::ComputeStreamingSlot( Int32 tileX, Int32 tileY, StreamingSlot& streamingSlot, Bool forceSync )
{
	const CTerrainInfo& info = m_pathlib.GetTerrainInfo();
	if ( info.IsInitialized() )
	{
		CTerrainAreaDescription* terrainArea = m_pathlib.GetTerrainAreaDescription( info.GetTileIdFromCoords( tileX, tileY ) );
		if ( terrainArea )
		{
			// get all instance areas connected
			if ( forceSync )
			{
				// TODO: Synchronous
				streamingSlot.PushBack( IStreamingItem::CStreamingRequest( terrainArea, *this ) );
			}
			else
			{
				streamingSlot.PushBack( IStreamingItem::CStreamingRequest( terrainArea, *this ) );
			}
		}
	}


	const TDynArray< AreaId >& instancesList = m_pathlib.GetInstanceMap()->GetMapCelAt( tileX, tileY ).GetInstancesList();
	for ( auto it = instancesList.Begin(), end = instancesList.End(); it != end; ++it )
	{
		CNavmeshAreaDescription* naviArea = m_pathlib.GetInstanceAreaDescription( *it );
		PATHLIB_ASSERT( naviArea );
		if ( naviArea )
		{
			if ( forceSync )
			{
				// TODO: Synchronous
				streamingSlot.PushBack( IStreamingItem::CStreamingRequest( naviArea, *this ) );
			}
			else
			{
				streamingSlot.PushBack( IStreamingItem::CStreamingRequest( naviArea, *this ) );
			}
		}
	}

}

void CStreamingManager::UpdatePosition( const Vector& v )
{
	// streaming logic implementation
	Int16 x, y;
	const CInstanceMap& instanceMap = *m_pathlib.GetInstanceMap();
	instanceMap.GetCelCoordsAtPosition( v.AsVector2(), x, y );

	if ( x == m_currentPlayerX && y == m_currentPlayerY )
	{
		return;
	}
	Int32 tilesInRow = instanceMap.GetCelsInRow();

	Int32 diffX = x - m_currentPlayerX;
	Int32 diffY = y - m_currentPlayerY;

	m_currentPlayerX = x;
	m_currentPlayerY = y;
	
	Bool forceUnstream = Abs( diffX ) > 1 || Abs( diffY ) > 1;

	// force unstream currently pending areas
	if ( !forceUnstream )
	{
		ForceUnloadAllPendingItems();
	}

	if ( Abs( diffX ) >= m_streamingRow || Abs( diffY ) >= m_streamingRow )
	{
		for ( Uint32 slotIdx = 0, slotCount = m_streamingSlots.Size(); slotIdx < slotCount; ++slotIdx )
		{
			m_streamingSlots[ slotIdx ].ClearFast();
		}
		Int32 minX = Max( m_currentPlayerX - m_streamingRadius, 0 );
		Int32 minY = Max( m_currentPlayerY - m_streamingRadius, 0 );

		Int32 maxX = Min( m_currentPlayerX + m_streamingRadius, tilesInRow-1 );
		Int32 maxY = Min( m_currentPlayerY + m_streamingRadius, tilesInRow-1 );

		for ( Int32 tileY = minY; tileY <= maxY; ++tileY )
		{
			for ( Int32 tileX = minX; tileX <= maxX; ++tileX )
			{
				ComputeStreamingSlot( tileX, tileY, m_currentPlayerX == tileX && m_currentPlayerY == tileY );
			}
		}
	}
	else
	{
		for ( Int32 celY = -m_streamingRadius; celY <= m_streamingRadius; ++celY )
		{
			Bool skipY = Abs( celY + diffY ) <= m_streamingRadius;
			Int32 tileY = m_currentPlayerY + celY;
			for ( Int32 celX = -m_streamingRadius; celX <= m_streamingRadius; ++celX )
			{
				if ( skipY )
				{
					// check skipX
					if ( Abs( celX + diffX ) <= m_streamingRadius )
					{
						// reuse current data
						continue;
					}
				}
				Int32 tileX = m_currentPlayerX + celX;

				Int32 streamingSlotIdx = TileCoordsToStreamingSlotIndex( tileX, tileY );
				m_streamingSlots[ streamingSlotIdx ].ClearFast();

				// world boundings test
				if ( tileY < 0 || tileY >= tilesInRow || tileX < 0 || tileX >= tilesInRow )
					continue;

				ComputeStreamingSlot( tileX, tileY, m_streamingSlots[ streamingSlotIdx ], celX == 0 && celY == 0 );
			}
		}
	}

	// unstream all non-requested areas
	if ( forceUnstream )
	{
		ForceUnloadAllPendingItems();
	}

	UpdateStreamedWorldBox();
}

void CStreamingManager::OnAreaDynamicallyAdded( CNavmeshAreaDescription* area )
{
	// Dynamic area creation. Basically editor only code.

	Int16 minX, minY, maxX, maxY;
	const CInstanceMap& instanceMap = *m_pathlib.GetInstanceMap();
	const Box& areaBBox = area->GetBBox();
	instanceMap.GetCelCoordsAtPosition( areaBBox.Min, minX, minY );
	instanceMap.GetCelCoordsAtPosition( areaBBox.Max, maxX, maxY );

	for ( Int32 tileY = minY; tileY <= maxY; ++tileY )
	{
		if ( Abs( tileY - m_currentPlayerY ) > m_streamingRadius )
		{
			continue;
		}
		for ( Int32 tileX = minX; tileX <= maxX; ++tileX )
		{
			if ( Abs( tileX - m_currentPlayerX ) > m_streamingRadius )
			{
				continue;
			}
			Int32 streamingSlotIdx = TileCoordsToStreamingSlotIndex( tileX, tileY );
			m_streamingSlots[ streamingSlotIdx ].PushBack( IStreamingItem::CStreamingRequest( area, *this ) );
		}
	}
}
void CStreamingManager::OnAreaDynamicallyRemoved( CNavmeshAreaDescription* area )
{
	// Dynamic area removal. Editor only code (so stability over performance).

	for ( Uint32 streamingSlotIdx = 0, slotCount = m_streamingSlots.Size(); streamingSlotIdx < slotCount; ++streamingSlotIdx )
	{
		for ( auto it = m_streamingSlots[ streamingSlotIdx ].Begin(), end = m_streamingSlots[ streamingSlotIdx ].End(); it != end; ++it )
		{
			if ( (*it) == area )
			{
				m_streamingSlots[ streamingSlotIdx ].Erase( it );
				break;
			}
		}
	}
}
void CStreamingManager::OnAreaDynamicallyUpdated( CNavmeshAreaDescription* area )
{
	OnAreaDynamicallyRemoved( area );
	OnAreaDynamicallyAdded( area );
}

void CStreamingManager::AddPendingToLoadItem( IStreamingItem& item )
{
	m_pendingToLoad.ListInsert( item );
}
void CStreamingManager::AddPendingToUnloadItem( IStreamingItem& item )
{
	m_pendingToUnload.ListInsert( item );
}

void CStreamingManager::MarkJobCompleted()
{
	m_currentJobIsCompleted = true;
}

void CStreamingManager::AttachStreamedItems( IStreamingItem::List::Head&& list )
{
	m_loadedItems.ListJoin( Move( list ) );

	++m_versionStamp;
}

};			// namespace PathLib
