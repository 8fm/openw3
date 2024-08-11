	/**
	* Copyright © 2012 CD Projekt Red. All Rights Reserved.
	*/

#include "build.h"

#ifndef NO_MARKER_SYSTEMS

#include "../core/configVarSystem.h"
#include "../core/configVarLegacyWrapper.h"
#include "../core/gatheredResource.h"
#include "game.h"
#include "world.h"
#include "dynamicLayer.h"
#include "layer.h"
#include "entity.h"
#include "clipMap.h"
#include "poiSystem.h"
#include "helpTextComponent.h"

namespace
{
	CGatheredResource GQuest( TXT("engine\\templates\\editor\\markers\\poi\\poi_quest.w2ent"), 0 );
	CGatheredResource GSideQuest( TXT("engine\\templates\\editor\\markers\\poi\\poi_side_quest.w2ent"), 0 );
	CGatheredResource GLandmark( TXT("engine\\templates\\editor\\markers\\poi\\poi_landmark.w2ent"), 0 );
	CGatheredResource GInterior( TXT("engine\\templates\\editor\\markers\\poi\\poi_interior.w2ent"), 0 );
	CGatheredResource GDungeon( TXT("engine\\templates\\editor\\markers\\poi\\poi_dungeon.w2ent"), 0 );
	CGatheredResource GGameplay( TXT("engine\\templates\\editor\\markers\\poi\\poi_gameplay.w2ent"), 0 );
	CGatheredResource GMood( TXT("engine\\templates\\editor\\markers\\poi\\poi_mood.w2ent"), 0 );
	CGatheredResource GCommunity( TXT("engine\\templates\\editor\\markers\\poi\\poi_community.w2ent"), 0 );
	CGatheredResource GRoadSign( TXT("engine\\templates\\editor\\markers\\poi\\poi_road_sign.w2ent"), 0 );
	CGatheredResource GHarbor( TXT("engine\\templates\\editor\\markers\\poi\\poi_harbor.w2ent"), 0 );
	CGatheredResource GSettlement( TXT("engine\\templates\\editor\\markers\\poi\\poi_settlement.w2ent"), 0 );
	CGatheredResource GCutscene( TXT("engine\\templates\\editor\\markers\\poi\\poi_cutscene.w2ent"), 0 );
}

namespace Config
{
	TConfigVar<Bool> cvPoiAutoSynchronization( "PoiSystem", "PoiAutoSynchronization", true, eConsoleVarFlag_Save | eConsoleVarFlag_Developer );
	TConfigVar<Bool> cvPoiShowOnMap( "PoiSystem", "PoiShowOnMap", true, eConsoleVarFlag_Save | eConsoleVarFlag_Developer );
	TConfigVar<Int32> cvPoiAutoSyncTime( "PoiSystem", "PoiAutoSyncTyme", 5, eConsoleVarFlag_Save | eConsoleVarFlag_Developer );
	TConfigVar<String> cvPoiBlackBoxPath( "PoiSystem", "PoiBlackBoxPath", TXT(""), eConsoleVarFlag_Save | eConsoleVarFlag_Developer );
}

CPoiSystem::CPoiSystem() 
	: AbstractMarkerSystem( MST_POI )
	, m_autoSync(true)
	, m_autoSyncTime(5)
	, m_showOnMap(true)
	, m_blackBoxPath(TXT(""))
	, m_newPoint(nullptr)
{
	/* intentionally empty */
}

CPoiSystem::~CPoiSystem()
{
	/* intentionally empty */
}

void CPoiSystem::Initialize()
{
	// load need entities
	m_pointsEntities[POIC_Quest] = GQuest.LoadAndGet< CEntityTemplate >();
	m_pointsEntities[POIC_SideQuest] = GSideQuest.LoadAndGet< CEntityTemplate >();
	m_pointsEntities[POIC_Landmark] = GLandmark.LoadAndGet< CEntityTemplate >();
	m_pointsEntities[POIC_Interrior] = GInterior.LoadAndGet< CEntityTemplate >();
	m_pointsEntities[POIC_Dungeon] = GDungeon.LoadAndGet< CEntityTemplate >();
	m_pointsEntities[POIC_Gameplay] = GGameplay.LoadAndGet< CEntityTemplate >();
	m_pointsEntities[POIC_Mood] = GMood.LoadAndGet< CEntityTemplate >();
	m_pointsEntities[POIC_Community] = GCommunity.LoadAndGet< CEntityTemplate >();
	m_pointsEntities[POIC_RoadSign] = GRoadSign.LoadAndGet< CEntityTemplate >();
	m_pointsEntities[POIC_Harbor] = GHarbor.LoadAndGet< CEntityTemplate >();
	m_pointsEntities[POIC_Settlement] = GSettlement.LoadAndGet< CEntityTemplate >();
	m_pointsEntities[POIC_Cutscene] = GCutscene.LoadAndGet< CEntityTemplate >();
}

void CPoiSystem::Tick( Float timeDelta )
{
	if( GGame != nullptr && GGame->GetActiveWorld() != nullptr )
	{
		CDynamicLayer* dynamicLayer = GGame->GetActiveWorld()->GetDynamicLayer();

		// remove old flags
		if( m_entitiesToRemove.Size() > 0 )
		{
			for( auto it=m_entitiesToRemove.Begin(); it!=m_entitiesToRemove.End(); ++it )
			{
				CEntity* entity = ( *it );
				dynamicLayer->RemoveEntity( entity );
			}
			m_entitiesToRemove.ClearFast();
		}

		// add new flags
		if( m_entitiesToAdd.Size() > 0 )
		{
			for( auto it=m_entitiesToAdd.Begin(); it!=m_entitiesToAdd.End(); ++it )
			{
				CPointofInterest* point = ( *it );

				if( point->m_category > POIC_Count )
				{
					RED_LOG_WARNING( RED_LOG_CHANNEL( MarkerSystem_POI ), TXT("POI marker has unsupported category. Someone modified categories in BlackBox and forgot to change the information in the database.") );
					continue;
				}

				// Create template for entity
				EntitySpawnInfo spawnInfo;
				spawnInfo.m_spawnPosition = point->m_worldPosition;
				spawnInfo.m_detachTemplate = false;
				spawnInfo.m_template = m_pointsEntities[point->m_category-1];

				// Create flag
				point->m_poiEntity = dynamicLayer->CreateEntitySync( spawnInfo );
				if ( point->m_poiEntity != nullptr )
				{
					// set title
					const TDynArray< CComponent* >& components =  point->m_poiEntity->GetComponents();
					for( CComponent* comp : components )
					{
						if( CHelpTextComponent* textComponent = Cast< CHelpTextComponent >( comp ) )
						{
							textComponent->SetText( point->m_name );
							if( point->m_category == 12 )
							{
								//textComponent->SetTextColor(Color::DARK_MAGENTA);
								textComponent->SetDrawBackground(true);
								textComponent->SetBackgroundColor(Color::DARK_MAGENTA);
							}
						}
					}

					// Add basic tags
					TagList tags = point->m_poiEntity->GetTags();
					tags.AddTag(CNAME( LockedObject ));
					tags.AddTag(CNAME( PointObject ));
					point->m_poiEntity->SetTags( tags );
				}
			}
			m_entitiesToAdd.ClearFast();
		}
	}
}

void CPoiSystem::Shutdown()
{
	SaveSettings();

	if( m_newPoint != nullptr )
	{
		delete m_newPoint;
		m_newPoint = nullptr;
	}
}

void CPoiSystem::OnLoadData()
{
	if(IsConnected() == true)
	{
		// clear points from previous world
		m_points.ClearFast();

		// get points for actually world
		m_databaseConnector.GetAllPoints( m_points, GetActiveLevelId() );

		CalculatePointsPositionToWorld();

		SendRequest( MSRT_UpdateData );
	}
}

void CPoiSystem::OnReleaseData()
{
	RemoveEntietiesFromMap();
	m_filteredPoints.Clear();
	m_points.Clear();

	SendRequest( MSRT_UpdateData );
}

void CPoiSystem::BackgroundUpdate()
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_synchronizationMutex );

	LockUpdate();

	while( m_requestsQueue.Empty() == false )
	{
		EMarkerSystemRequestType requestType = m_requestsQueue.Front();

		switch( requestType )
		{
		case MSRT_ReconnectWithDatabase:
			{
				Connect();
			}
			break;
		case MSRT_LoadData:
			if( IsConnected() == true )
			{
				OnLoadData();
			}
			break;
		case MSRT_ReleaseData:
			if( IsConnected() == true )
			{
				OnReleaseData();
			}
			break;
		case MSRT_SynchronizeData:
			if( IsConnected() == true )
			{
				SynchronizePoints();
			}
			break;
		case MSRT_UpdateData:
			if( IsConnected() == true )
			{
				UpdateData();
			}
			break;
		case MSRT_SortData:
			if( IsConnected() == true )
			{
				SortFilteredPoints();
			}
			break;
		case MSRT_ReloadData:
			{
				if( IsConnected() == true )
				{
					OnReleaseData();
					OnLoadData();
				}
			}
			break;
		}

		m_requestsQueue.Pop();
	}

	// synchronization
	if( m_autoSync == true && m_autoSyncTime != 0 )
	{
		if( m_lastSyncCounter.GetTimePeriod() > m_autoSyncTime*60 )
		{
			m_requestsQueue.Push( MSRT_SynchronizeData );
		}
	}

	UnlockUpdate();
}

void CPoiSystem::SetNewEntity(CEntity* newEntity)
{
	if( m_newPoint != nullptr )
	{
		m_newPoint->m_poiEntity = newEntity;
	}
	m_waitingForEntity = false;
}

Bool CPoiSystem::IsConnected() const
{
	return m_databaseConnector.IsConnected();
}

void CPoiSystem::CreateNewPoint()
{
	if( m_newPoint != nullptr )
	{
		delete m_newPoint;
		m_newPoint = nullptr;
	}

	m_newPoint = new CPointofInterest();
}

Bool CPoiSystem::AddNewPoint()
{
	SendMessage( MSM_SynchronizationStart );

	Bool result = false;
	if(m_newPoint != nullptr)
	{
		CalculatePointPositionToBlackBox(*m_newPoint);

		result = m_databaseConnector.AddNewPoint(*m_newPoint);	

		if( result == true )
		{
			m_points.PushBack(*m_newPoint);
			delete m_newPoint;
			m_newPoint = nullptr;
		}
	}

	SendMessage( MSM_SynchronizationEnd );
	SendRequest( MSRT_SynchronizeData );

	return result;
}

Bool CPoiSystem::ModifyPoint(CPointofInterest& point)
{
	SendMessage( MSM_SynchronizationStart );

	CalculatePointPositionToBlackBox(point);
	Bool result = m_databaseConnector.ModifyPoint(point);

	SendMessage( MSM_SynchronizationEnd );
	SendRequest( MSRT_SynchronizeData );

	return result;
}

Bool CPoiSystem::DeletePoint( CPointofInterest& point )
{
	SendMessage( MSM_SynchronizationStart );

	Bool result = m_databaseConnector.DeletePoint( point );

	SendMessage( MSM_SynchronizationEnd );
	SendRequest( MSRT_SynchronizeData );

	return result;
}

void CPoiSystem::SetFilters( const TDynArray<Bool>& categories )
{
	SetInternalFilters( categories, m_filterCondition );
}

void CPoiSystem::SetAutoSync(Bool autosync)
{
	m_autoSync = autosync;
	m_lastSyncCounter.ResetTimer();
	Config::cvPoiAutoSynchronization.Set( m_autoSync );
}

void CPoiSystem::SetShowOnMap(Bool showOnMap)
{
	m_showOnMap = showOnMap;
	Config::cvPoiShowOnMap.Set( m_showOnMap );
	SendRequest( MSRT_UpdateData );
}

void CPoiSystem::SetSyncTime(Uint32 time)
{
	m_autoSyncTime = time;
	Config::cvPoiAutoSyncTime.Set( m_autoSyncTime );
}

void CPoiSystem::SetBlackBoxPath(const String& blackBoxPath)
{
	m_blackBoxPath = blackBoxPath;
	Config::cvPoiBlackBoxPath.Set( m_blackBoxPath );
}

Bool CPoiSystem::GetAutoSync() const
{
	return m_autoSync;
}

Bool CPoiSystem::GetShowOnMap() const
{
	return m_showOnMap;
}

Uint32 CPoiSystem::GetSyncTime() const
{
	return m_autoSyncTime;
}

String CPoiSystem::GetBlackBoxPath() const
{
	return m_blackBoxPath;
}

CPointofInterest* CPoiSystem::GetNewPoint() const
{
	return m_newPoint;
}

const TDynArray<String>& CPoiSystem::GetPointCategories() const
{
	return m_pointCategories;
}

const TDynArray<CPointofInterest*>& CPoiSystem::GetPoints() const
{
	return m_filteredPoints;
}

CPointofInterest* CPoiSystem::GetPoint(Uint32 number) const
{
	if(number < m_filteredPoints.Size())
	{
		return m_filteredPoints[number];
	}
	return nullptr;
}

void CPoiSystem::Connect()
{
	SendMessage( MSM_DatabaseConnectionStart );

	if( m_databaseConnector.Connect() == true )
	{
		m_databaseConnector.GetAllLevels( m_pointLevels );
		m_databaseConnector.GetAllCategories( m_pointCategories );

		LoadSettings();
		SendMessage( MSM_DatabaseConnected );

		SendRequest( MSRT_ReloadData );
	}
	else
	{
		SendMessage( MSM_DatabaseLostConnection );
	}
}

void CPoiSystem::SynchronizePoints()
{
	SendMessage( MSM_SynchronizationStart );

	// save date of last synchronization for map
	if( GGame == nullptr || GGame->GetActiveWorld() == nullptr )
	{
		SendMessage( MSM_SynchronizationEnd );
		return;
	}
	// save date of last synchronization for map
	String worldName = GGame->GetActiveWorld()->GetDepotPath();

	// mark entities to delete before synchronizing with database as it clears all points and doesn't restore their entities
	RemoveEntietiesFromMap();
	
	// synchronize point with database
	m_databaseConnector.Synchronize( m_points, GetActiveLevelId() );
	
	FilterPoints();
	CalculatePointsPositionToWorld();
	m_lastSyncCounter.ResetTimer();

	SendMessage( MSM_SynchronizationEnd );
}

void CPoiSystem::FilterPoints()
{
	RemoveEntietiesFromMap();

	m_filteredPoints.ClearFast();

	const Uint32 pointCount = m_points.Size();
	for(Uint32 i=0; i<pointCount; ++i)
	{
		// filter type
		Uint32 type = m_points[i].m_category-1;
		if( type < m_pointCategories.Size() )
		{
			if(m_filterCategoryValues[type] == false)
			{
				continue;
			}
		}

		if(m_points[i].m_name.ContainsSubstring(m_filterCondition) == true)
		{
			m_filteredPoints.PushBack(&m_points[i]);
		}
	}

	AddEntitiesOnMap();
}

void CPoiSystem::CalculatePointsPositionToWorld()
{
	Uint32 pointCount = m_points.Size();
	for(Uint32 i=0; i<pointCount; ++i)
	{
		Vector poiPosInWorld;
		CWorld* world = GGame->GetActiveWorld();
		if ( world != nullptr )
		{
			CClipMap* clipMap = world->GetTerrain();
			if ( clipMap != nullptr )
			{
				SClipmapParameters params;
				clipMap->GetClipmapParameters( &params );

				Vector terrainCorner = clipMap->GetTerrainCorner();
				Uint32 levelTileCount = m_pointLevels[GetActiveLevelId()-1].m_tileCount;
				Uint32 levelTileSize = m_pointLevels[GetActiveLevelId()-1].m_tileSize;
				Float x = (Float) m_points[i].m_coordinateX / ( levelTileCount * levelTileSize );
				Float y = 1.0f - ( (Float) m_points[i].m_coordinateY / ( levelTileCount * levelTileSize ));
				poiPosInWorld = Vector( params.terrainSize*x + terrainCorner.X, 
					params.terrainSize*y + terrainCorner.Y, 0 );

				if ( m_points[i].m_snappedToTerrain )
				{
					clipMap->GetHeightForWorldPositionSync( poiPosInWorld, 0, poiPosInWorld.Z );
				}
				else
				{
					poiPosInWorld.Z = m_points[i].m_worldPosition.Z;
				}
				m_points[i].m_worldPosition = poiPosInWorld;
			}
		}
	}
}

void CPoiSystem::CalculatePointPositionToBlackBox(CPointofInterest& point)
{
	point.m_worldPosition = point.m_poiEntity->GetPosition();

	CWorld* world = GGame->GetActiveWorld();
	if ( world )
	{
		CClipMap* clipMap = world->GetTerrain();
		if ( clipMap )
		{
			SClipmapParameters params;
			clipMap->GetClipmapParameters( &params );
			Vector terrainCorner = clipMap->GetTerrainCorner();

			Vector poiPosInWorld = point.m_worldPosition;
			poiPosInWorld.X =  (poiPosInWorld.X - terrainCorner.X) / params.terrainSize;
			poiPosInWorld.Y =  (poiPosInWorld.Y - terrainCorner.Y) / params.terrainSize;

			Uint32 levelTileCount = m_pointLevels[GetActiveLevelId()-1].m_tileCount;
			Uint32 levelTileSize = m_pointLevels[GetActiveLevelId()-1].m_tileSize;
			Uint32 x = (Uint32)( poiPosInWorld.X * levelTileCount * levelTileSize );
			Uint32 y = (Uint32)( (1.0f - poiPosInWorld.Y) * levelTileCount * levelTileSize );

			point.m_coordinateX = x;
			point.m_coordinateY = y;
		}
	}
}

Int32 CPoiSystem::GetActiveLevelId() const
{
	CWorld* world = GGame->GetActiveWorld();
	CFilePath worldPath(world->GetFriendlyName());
	String worldName = worldPath.GetFileName();

	// hack related with BlackBox names ...
	if(worldName == TXT("skellige"))
	{
		worldName = TXT("skellige_islands");
	}
	else if(worldName == TXT("prolog_village"))
	{
		worldName = TXT("prologue_village");
	}

	for(Uint32 i=0; i<m_pointLevels.Size(); ++i)
	{
		if(worldName == m_pointLevels[i].m_levelName)
		{
			return (i+1);
		}
	}
	return -1;
}

void CPoiSystem::SortFilteredPoints()
{
	switch( m_sortCategory )
	{
	case POISC_Title:
		{
			struct LocalSorterByTitle
			{
				static RED_INLINE Bool Sort( const CPointofInterest* poi1, const CPointofInterest* poi2 )
				{
					return Red::System::StringCompareNoCase( poi1->m_name.AsChar(), poi2->m_name.AsChar() ) < 0;
				}
			};
			Sort( m_filteredPoints.Begin(), m_filteredPoints.End(), LocalSorterByTitle::Sort );
		}
		break;
	case POISC_Category:
		{
			struct LocalSorterByCategory
			{
				static RED_INLINE Bool Sort( const CPointofInterest* poi1, const CPointofInterest* poi2 )
				{
					return ( poi1->m_category < poi2->m_category );
				}
			};
			Sort( m_filteredPoints.Begin(), m_filteredPoints.End(), LocalSorterByCategory::Sort );
		}
		break;
	}

	if( m_sortOrder == MSSO_Descending )
	{
		// invert position
		const Uint32 pointCount = m_filteredPoints.Size();
		for( Uint32 i=0; i<pointCount/2; ++i )
		{
			CPointofInterest* tempPoint = m_filteredPoints[i];
			m_filteredPoints[i] = m_filteredPoints[pointCount-i-1];
			m_filteredPoints[pointCount-i-1] = tempPoint;
		}
	}

	SendMessage( MSM_DataAreSorted );
}

void CPoiSystem::UpdateData()
{
	FilterPoints();
	SortFilteredPoints();

	SendMessage( MSM_DataAreUpdated );
}

void CPoiSystem::SetSortingSettings( EPOISortCategory category, EMarkerSystemSortOrder order )
{
	m_sortCategory = category;
	m_sortOrder = order;

	SendRequest( MSRT_SortData );
}

void CPoiSystem::RemoveEntietiesFromMap()
{
	for( auto it=m_points.Begin(); it!=m_points.End(); ++it )
	{
		CPointofInterest& point = ( *it );
		if( point.m_poiEntity != nullptr )
		{
			m_entitiesToRemove.PushBackUnique( point.m_poiEntity );
			point.m_poiEntity = nullptr;
		}
	}
}

void CPoiSystem::AddEntitiesOnMap()
{
	if( m_showOnMap == true )
	{
		for( auto it=m_points.Begin(); it!=m_points.End(); ++it )
		{
			CPointofInterest& point = ( *it );
			m_entitiesToAdd.PushBack( &point );
		}
	}
}

THandle< CEntityTemplate > CPoiSystem::GetPointTemplate( enum EPOICategory category ) const
{
	if( category < POIC_Count )
	{
		return m_pointsEntities[category];
	}
	return nullptr;
}

void CPoiSystem::SetInternalFilters( const TDynArray<Bool>& categories, const String& filterCondition )
{
	m_filterCategoryValues.CopyFast( categories );
	m_filterCondition = filterCondition;

	SendRequest( MSRT_UpdateData );
}

void CPoiSystem::SetSearchFilter( const String& condition )
{
	SetInternalFilters( m_filterCategoryValues, condition );
}

void CPoiSystem::LoadSettings()
{
	// read settings from ini file
	m_showOnMap = Config::cvPoiShowOnMap.Get();
	m_autoSync = Config::cvPoiAutoSynchronization.Get();
	m_autoSyncTime = Config::cvPoiAutoSyncTime.Get();
	m_blackBoxPath = Config::cvPoiBlackBoxPath.Get();

	const Uint32 typeCount = m_pointCategories.Size();
	m_filterCategoryValues.ResizeFast( typeCount );
	for( Uint32 i=0; i<typeCount; ++i )
	{
		m_filterCategoryValues[i] = true;
		SConfig::GetInstance().GetLegacy().ReadParam( TXT("user"), TXT("ReviewSystem"), m_pointCategories[i], m_filterCategoryValues[i] );
	}
}

void CPoiSystem::SaveSettings()
{
	// read settings from ini file
	Config::cvPoiShowOnMap.Set( m_showOnMap );
	Config::cvPoiAutoSynchronization.Set( m_autoSync );
	Config::cvPoiAutoSyncTime.Set( m_autoSyncTime );
	Config::cvPoiBlackBoxPath.Set( m_blackBoxPath );

	const Uint32 typeCount = m_pointCategories.Size();
	for( Uint32 i=0; i<typeCount; ++i )
	{
		SConfig::GetInstance().GetLegacy().WriteParam( TXT("user"), TXT("ReviewSystem"), m_pointCategories[i].AsChar(), m_filterCategoryValues[i] );
	}
}

#endif	// NO_MARKER_SYSTEMS
