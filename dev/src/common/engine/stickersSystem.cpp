/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#ifndef NO_MARKER_SYSTEMS

#include "stickersSystem.h"
#include "../core/depot.h"
#include "../core/configVar.h"
#include "../core/configVarLegacyWrapper.h"
#include "../core/gatheredResource.h"
#include "game.h"
#include "world.h"
#include "dynamicLayer.h"
#include "entity.h"
#include "helpTextComponent.h"

namespace
{
	CGatheredResource resStickerEntity( TXT("engine\\templates\\editor\\sticker.w2ent"), 0 );
}

namespace Config
{
	TConfigVar<Bool>				cvStickerSystemShowOnMap			( "StickerSystem", "ShowOnMap",				true,					eConsoleVarFlag_Save | eConsoleVarFlag_Developer );
	TConfigVar<Bool>				cvStickerSystemAutoSync				( "StickerSystem", "AutoSynchronization",	true,					eConsoleVarFlag_Save | eConsoleVarFlag_Developer );
	TConfigVar<Int32>				cvStickerSystemAutoSyncTime			( "StickerSystem", "AutoSyncTime",			5,						eConsoleVarFlag_Save | eConsoleVarFlag_Developer );
	TConfigVar<String>				cvStickerSystemFilterCondition		( "StickerSystem", "FilterCondition",		TXT(""),				eConsoleVarFlag_Save | eConsoleVarFlag_Developer );
	TConfigVar<Int32>				cvStickerSystemRenderDistance		( "StickerSystem", "RenderDistance",		500,					eConsoleVarFlag_Save | eConsoleVarFlag_Developer );
}

CStickersSystem::CStickersSystem(void) 
	: AbstractMarkerSystem( MST_Sticker )
	, m_autoSync(true)
	, m_autoSyncTime(5)
	, m_showOnMap(true)
	, m_newSticker(nullptr)
	, m_renderDistance(500)
{
	/* intentionally empty */
}

CStickersSystem::~CStickersSystem(void)
{
	if(m_newSticker != nullptr)
	{
		delete m_newSticker;
		m_newSticker = nullptr;
	}
}

void CStickersSystem::Initialize()
{
	/* intentionally empty */
}

void CStickersSystem::Tick( Float timeDelta )
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
				CSticker* sticker = ( *it );
				
				// Create template for entity
				EntitySpawnInfo spawnInfo;
				spawnInfo.m_spawnPosition = sticker->m_position;
				spawnInfo.m_detachTemplate = true;
				spawnInfo.m_template = resStickerEntity.LoadAndGet< CEntityTemplate >();

				// Create flag
				sticker->m_stickerEntity = dynamicLayer->CreateEntitySync( spawnInfo );
				if( sticker->m_stickerEntity != nullptr )
				{
					// set title
					const TDynArray< CComponent* >& components =  sticker->m_stickerEntity->GetComponents();
					for( CComponent* comp : components )
					{
						if( CHelpTextComponent* textComponent = Cast< CHelpTextComponent >( comp ) )
						{
							textComponent->SetText( sticker->m_title );
						}
					}

					// Add basic tags
					TagList tags = sticker->m_stickerEntity->GetTags();
					tags.AddTag(CNAME( LockedObject ));
					tags.AddTag(CNAME( StickerObject ));
					sticker->m_stickerEntity->SetTags( tags );
				}
			}
			m_entitiesToAdd.ClearFast();
		}
	}
}

void CStickersSystem::Shutdown()
{
	SaveSettings();

	if( m_newSticker != nullptr )
	{
		delete m_newSticker;
		m_newSticker = nullptr;
	}
}

void CStickersSystem::OnLoadData()
{
	if( IsConnected() == true )
	{
		// clear points from previous world
		m_stickers.ClearFast();

		// get flags for actually world
		CWorld* world = GGame->GetActiveWorld();
		CFilePath worldPath( world->GetFriendlyName() );
		String worldName = worldPath.GetFileName();
		m_databaseConnector.GetAllStickers( m_stickers, worldName );

		SendRequest( MSRT_UpdateData );
	}
}

void CStickersSystem::OnReleaseData()
{
	RemoveEntietiesFromMap();
	m_stickers.ClearFast();
	m_filteredStickers.ClearFast();

	SendRequest( MSRT_UpdateData );
}

void CStickersSystem::BackgroundUpdate()
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
				SynchronizeStickers();
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
				SortFilteredStickers();
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
	if( m_autoSync == true  && m_autoSyncTime != 0 )
	{
		if( m_lastSyncCounter.GetTimePeriod() > m_autoSyncTime*60 )
		{
			m_requestsQueue.Push( MSRT_SynchronizeData );
		}
	}

	UnlockUpdate();
}

void CStickersSystem::SetNewEntity(CEntity* newEntity)
{
	if(m_newSticker != nullptr)
	{
		m_newSticker->m_stickerEntity = newEntity;
	}
	m_waitingForEntity = false;
}

Bool CStickersSystem::IsConnected() const
{
	return m_databaseConnector.IsConnected();
}

void CStickersSystem::CreateNewSticker()
{
	if(m_newSticker != nullptr)
	{
		delete m_newSticker;
		m_newSticker = nullptr;
	}

	m_newSticker = new CSticker();
}

Bool CStickersSystem::AddNewSticker()
{
	SendMessage( MSM_SynchronizationStart );

	Bool result = false;
	if(m_newSticker != nullptr)
	{
		// add information about active world
		CWorld* world = GGame->GetActiveWorld();
		CFilePath worldPath(world->GetFriendlyName());
		String worldName = worldPath.GetFileName();
		m_newSticker->m_mapName = worldName;

		// add information about position from entity
		CEntity* stickerEntity = m_newSticker->m_stickerEntity.Get();
		if( stickerEntity == nullptr )
		{
			RED_LOG( TXT("StickerSystem"), TXT("Entity for sticker is not exist") );
			return false;
		}
		m_newSticker->m_position = stickerEntity->GetPosition();

		// add sticker to database
		result = m_databaseConnector.AddNewSticker(*m_newSticker);	

		if(result == true)
		{
			m_stickers.PushBack(*m_newSticker);
			delete m_newSticker;
			m_newSticker = nullptr;
		}
	}

	SendMessage( MSM_SynchronizationEnd );
	SendRequest( MSRT_SynchronizeData );

	return result;
}

Bool CStickersSystem::ModifySticker(CSticker& sticker)
{
	SendMessage( MSM_SynchronizationStart );

	// update position
	CEntity* stickerEntity = sticker.m_stickerEntity.Get();
	if( stickerEntity == nullptr )
	{
		RED_LOG( TXT("StickerSystem"), TXT("Entity for sticker is not exist") );
		return false;
	}
	sticker.m_position = stickerEntity->GetPosition();

	Bool result = m_databaseConnector.ModifySticker(sticker);	

	SendMessage( MSM_SynchronizationEnd );
	SendRequest( MSRT_SynchronizeData );

	return result;
}

Bool CStickersSystem::DeleteSticker(CSticker& sticker)
{
	SendMessage( MSM_SynchronizationStart );

	Bool result = m_databaseConnector.DeleteSticker(sticker);	

	SendMessage( MSM_SynchronizationEnd );
	SendRequest( MSRT_SynchronizeData );

	return result;
}

void CStickersSystem::SetFilters( const TDynArray<Bool>& categories )
{
	SetInternalFilter( categories, m_filterCondition );
}

void CStickersSystem::SetAutoSync(Bool autosync)
{
	m_autoSync = autosync;
	m_lastSyncCounter.ResetTimer();
	Config::cvStickerSystemAutoSync.Set( m_autoSync );
}

void CStickersSystem::SetShowOnMap(Bool showOnMap)
{
	m_showOnMap = showOnMap;
	Config::cvStickerSystemShowOnMap.Set( m_showOnMap );
	SendRequest( MSRT_UpdateData );
}

void CStickersSystem::SetSyncTime(Uint32 time)
{
	m_autoSyncTime = time;
	Config::cvStickerSystemAutoSyncTime.Set( m_autoSyncTime );
}

void CStickersSystem::SetRenderDistance(Uint32 distance)
{
	m_renderDistance = distance;
	Config::cvStickerSystemRenderDistance.Set( m_renderDistance );
}

Bool CStickersSystem::GetAutoSync() const
{
	return m_autoSync;
}

Bool CStickersSystem::GetShowOnMap() const
{
	return m_showOnMap;
}

Uint32 CStickersSystem::GetSyncTime() const
{
	return m_autoSyncTime;
}

Uint32 CStickersSystem::GetRenderDistance() const
{
	return m_renderDistance;
}

CSticker* CStickersSystem::GetNewSticker() const
{
	return m_newSticker;
}

const TDynArray<String>& CStickersSystem::GetStickerCategories() const
{
	return m_stickerCategories;
}

const TDynArray<CSticker*>& CStickersSystem::GetStickers() const
{
	return m_filteredStickers;
}

CSticker* CStickersSystem::GetSticker(Uint32 number) const
{
	if(number < m_filteredStickers.Size())
	{
		return m_filteredStickers[number];
	}
	return nullptr;
}

void CStickersSystem::Connect()
{
	SendMessage( MSM_DatabaseConnectionStart );

	if(m_databaseConnector.Connect() == true)
	{
		m_databaseConnector.GetAllCategories(m_stickerCategories);

		LoadSettings();
		SendMessage( MSM_DatabaseConnected );

		SendRequest( MSRT_ReloadData );
	}
	else
	{
		SendMessage( MSM_DatabaseLostConnection );
	}
}

void CStickersSystem::SynchronizeStickers()
{
	SendMessage( MSM_SynchronizationStart );

	// save date of last synchronization for map
	if( GGame == nullptr || GGame->GetActiveWorld() == nullptr )
	{
		SendMessage( MSM_SynchronizationEnd );
		return;
	}

	// synchronize point with database
	CWorld* world = GGame->GetActiveWorld();
	CFilePath worldPath(world->GetFriendlyName());
	String worldName = worldPath.GetFileName();
	m_databaseConnector.Synchronize( m_stickers, worldName );

	m_lastSyncCounter.ResetTimer();

	FilterPoints();

	SendMessage( MSM_SynchronizationEnd );
}

void CStickersSystem::FilterPoints()
{
	RemoveEntietiesFromMap();

	m_filteredStickers.ClearFast();

	const Uint32 stickerCount = m_stickers.Size();
	for(Uint32 i=0; i<stickerCount; ++i)
	{
		// filter type
		Uint32 type = m_stickers[i].m_type-1;
		if(m_filterCategoryValues[type] == false)
		{
			continue;
		}

		if(m_stickers[i].m_title.ContainsSubstring(m_filterCondition) == true)
		{
			m_filteredStickers.PushBack(&m_stickers[i]);
		}
	}

	AddEntitiesOnMap();
}

void CStickersSystem::SortFilteredStickers()
{
	struct StickerSorterByTitle
	{
		static RED_INLINE Bool Sort( const CSticker* sticker1, const CSticker* sticker2 )
		{
			return Red::System::StringCompareNoCase( sticker1->m_title.AsChar(), sticker2->m_title.AsChar() ) < 0;
		}
	};

	Sort( m_filteredStickers.Begin(), m_filteredStickers.End(), StickerSorterByTitle::Sort );

	SendMessage( MSM_DataAreSorted );
}

void CStickersSystem::UpdateData()
{
	FilterPoints();
	SortFilteredStickers();

	SendMessage( MSM_DataAreUpdated );
}

void CStickersSystem::AddEntitiesOnMap()
{
	if( m_showOnMap == true )
	{
		for( auto it=m_stickers.Begin(); it!=m_stickers.End(); ++it )
		{
			CSticker& sticker = ( *it );
			m_entitiesToAdd.PushBack( &sticker );
		}
	}
}

void CStickersSystem::RemoveEntietiesFromMap()
{
	for( auto it=m_stickers.Begin(); it!=m_stickers.End(); ++it )
	{
		CSticker& sticker = ( *it );
		if( sticker.m_stickerEntity != nullptr )
		{
			m_entitiesToRemove.PushBack( sticker.m_stickerEntity );
			sticker.m_stickerEntity = nullptr;
		}
	}
}

THandle< CEntityTemplate > CStickersSystem::GetStickerTemplate() const
{
	return resStickerEntity.LoadAndGet< CEntityTemplate >();
}

void CStickersSystem::SetInternalFilter( const TDynArray<Bool>& categories, const String& filterCondition )
{
	m_filterCategoryValues.CopyFast( categories );
	m_filterCondition = filterCondition;

	SendRequest( MSRT_UpdateData );
}

void CStickersSystem::SetSearchFilter( const String& condition )
{
	SetInternalFilter( m_filterCategoryValues, condition );
}

void CStickersSystem::LoadSettings()
{
	// read settings from ini file
	m_showOnMap = Config::cvStickerSystemShowOnMap.Get();
	m_autoSync = Config::cvStickerSystemAutoSync.Get();
	m_autoSyncTime = Config::cvStickerSystemAutoSyncTime.Get();

	// filters
	m_filterCondition = Config::cvStickerSystemFilterCondition.Get();

	const Uint32 categoryCount = m_stickerCategories.Size();
	m_filterCategoryValues.ResizeFast( categoryCount );
	for( Uint32 i=0; i<categoryCount; ++i )
	{
		m_filterCategoryValues[i] = true;
		SConfig::GetInstance().GetLegacy().ReadParam( TXT("user"), TXT("StickerSystem"), m_stickerCategories[i], m_filterCategoryValues[i] );
	}
}

void CStickersSystem::SaveSettings()
{
	// write settings from ini file
	Config::cvStickerSystemShowOnMap.Set( m_showOnMap );
	Config::cvStickerSystemAutoSync.Set( m_autoSync );
	Config::cvStickerSystemAutoSyncTime.Set( m_autoSyncTime );

	// filters
	Config::cvStickerSystemFilterCondition.Set( m_filterCondition );

	const Uint32 categoryCount = m_stickerCategories.Size();
	for( Uint32 i=0; i<categoryCount; ++i )
	{
		SConfig::GetInstance().GetLegacy().WriteParam( TXT("user"), TXT("StickerSystem"), m_stickerCategories[i].AsChar(), m_filterCategoryValues[i] );
	}
}

#endif	// NO_MARKER_SYSTEMS
