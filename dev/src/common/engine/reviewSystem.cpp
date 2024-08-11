/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#ifndef NO_MARKER_SYSTEMS

#include "reviewSystem.h"
#include "helpTextComponent.h"
#include "renderer.h"
#include "screenshotSystem.h"

#include "game.h"
#include "layer.h"
#include "dynamicLayer.h"
#include "world.h"
#include "bitmapTexture.h"
#include "../core/system.h"
#include "../core/importer.h"
#include "../core/jobGenericJobs.h"
#include "../core/loadingJobManager.h"
#include "../core/configVarLegacyWrapper.h"
#include "../core/feedback.h"
#include "../core/system.h"
#include "../core/gatheredResource.h"
#include "../gpuApiUtils/gpuApiInterface.h"
#include "../gpuApiDX10/gpuApi.h"
#include "baseEngine.h"
#include "component.h"
#include "entity.h"
#include "viewport.h"

namespace
{
	CGatheredResource GOpenedFlag( TXT("engine\\templates\\editor\\markers\\review\\opened_flag.w2ent"), 0 );
	CGatheredResource GFixedFlag( TXT("engine\\templates\\editor\\markers\\review\\fixed_flag.w2ent"), 0 );
	CGatheredResource GClosedFlag( TXT("engine\\templates\\editor\\markers\\review\\closed_flag.w2ent"), 0 );
}

namespace Config
{
	TConfigVar<String>				cvTTProject				( "ReviewSystem", "TTProject",			TXT("test_project3"),	eConsoleVarFlag_Save | eConsoleVarFlag_Developer );
	TConfigVar<Bool>				cvDownloadClosedFlags	( "ReviewSystem", "DownloadClosedFlag", false,					eConsoleVarFlag_Save | eConsoleVarFlag_Developer );
	TConfigVar<Bool>				cvShowOnMap				( "ReviewSystem", "ShowOnMap",			true,					eConsoleVarFlag_Save | eConsoleVarFlag_Developer );
	TConfigVar<Bool>				cvAutoSync				( "ReviewSystem", "AutoSynchronization",true,					eConsoleVarFlag_Save | eConsoleVarFlag_Developer );
	TConfigVar<Int32>				cvAutoSyncTime			( "ReviewSystem", "AutoSyncTime",		5,						eConsoleVarFlag_Save | eConsoleVarFlag_Developer );
	TConfigVar<Int32>				cvFilterCategory		( "ReviewSystem", "FilterCategory",		(Int32)ReviewSearchNone,eConsoleVarFlag_Save | eConsoleVarFlag_Developer );
	TConfigVar<String>				cvFilterCondition		( "ReviewSystem", "FilterCondition",	TXT(""),				eConsoleVarFlag_Save | eConsoleVarFlag_Developer );
}

CBitmapTexture* CReviewFlagComment::GetFlagScreen()
{
	if( this->m_screen.Get() == nullptr )
	{
		CReviewSystem* reviewSystem = static_cast< CReviewSystem* >( GEngine->GetMarkerSystems()->GetSystem( MST_Review ) );
		if( reviewSystem != nullptr )
		{
			reviewSystem->ImportScreenForComment( this );
		}
	}

	return this->m_screen.Get();
}

CReviewSystem::CReviewSystem() 
	: AbstractMarkerSystem( MST_Review )
	, m_autoSync(true)
	, m_downloadClosedFlags(false)
	, m_autoSyncTime(5)
	, m_waitingForEntity(false)
	, m_showOnMap(true)
	, m_filtersAreOn(false)
	, m_newFlag( nullptr )
{
	/* intentionally empty */
	m_ttProject = Config::cvTTProject.Get();
}

CReviewSystem::~CReviewSystem()
{
	/* intentionally empty */
}

void CReviewSystem::Initialize()
{
	// set proper flags
	m_flagEntities[RFS_Opened] = GOpenedFlag.LoadAndGet< CEntityTemplate >();
	m_flagEntities[RFS_Fixed] = GFixedFlag.LoadAndGet< CEntityTemplate >();
	m_flagEntities[RFS_Closed] = GClosedFlag.LoadAndGet< CEntityTemplate >();
	m_flagEntities[RFS_ReOpened] = GOpenedFlag.LoadAndGet< CEntityTemplate >();
}

void CReviewSystem::Tick( Float timeDelta )
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
				if( CReviewFlag* reviewFlag = ( *it ) )
				{
					// Select last comment for flag
					Uint32 lastCommentIndex = reviewFlag->m_comments.Size()-1;
					CReviewFlagComment& comment = reviewFlag->m_comments[lastCommentIndex];
					Uint32 state = comment.m_state-1;

					// Create template for entity
					EntitySpawnInfo spawnInfo;
					spawnInfo.m_spawnPosition = comment.m_flagPosition;
					spawnInfo.m_detachTemplate = true;
					spawnInfo.m_template = m_flagEntities[state];

					// Create flag
					reviewFlag->m_flagEntity = dynamicLayer->CreateEntitySync( spawnInfo );
					if ( reviewFlag->m_flagEntity != nullptr )
					{
						// set title
						const TDynArray< CComponent* >& components =  reviewFlag->m_flagEntity->GetComponents();
						const Uint32 componentCount = components.Size();
						for( Uint32 i=0; i<componentCount; ++i )
						{
							if( components[i]->IsA< CHelpTextComponent >() == true )
							{
								CHelpTextComponent* helpTextComponent = static_cast< CHelpTextComponent* >( components[i] );
								if( helpTextComponent != nullptr )
								{
									helpTextComponent->SetText( reviewFlag->m_summary );
								}
							}
						}

						// Add basic tags
						TagList tags = reviewFlag->m_flagEntity->GetTags();
						tags.AddTag(CNAME( LockedObject ));
						tags.AddTag(CNAME( ReviewFlagObject ));
						reviewFlag->m_flagEntity->SetTags( tags );
					}
				}
			}
			m_entitiesToAdd.ClearFast();
		}
	}
}

void CReviewSystem::Shutdown()
{
	SaveSettings();

	if( m_newFlag != nullptr )
	{
		delete m_newFlag;
		m_newFlag = nullptr;
	}
}

void CReviewSystem::OnLoadData()
{
	if( IsConnected() == true && GGame->GetActiveWorld() )
	{
		// clear flag from previous world
		m_flags.ClearFast();

		// get flags for actually world
		m_databaseConnector.GetAllFlags( m_flags, GGame->GetActiveWorld()->GetDepotPath(), GetTTProject(), m_downloadClosedFlags );

		// save time of last update to ini file
		String worldName = GGame->GetActiveWorld()->GetDepotPath();
		SaveUpdateTimeForMap( worldName );

		SendRequest( MSRT_UpdateData );
	}
}

void CReviewSystem::OnReleaseData()
{
	RemoveEntietiesFromMap();
	m_filteredFlags.Clear();
	m_flags.Clear();

	SendRequest( MSRT_UpdateData );
}

void CReviewSystem::BackgroundUpdate()
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
				SynchronizeFlags();
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
				SortFilteredFlags();
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

void CReviewSystem::SetNewEntity( CEntity* newEntity )
{
	if( m_newFlag != nullptr )
	{
		m_newFlag->m_flagEntity = newEntity;
	}
	m_waitingForEntity = false;
}

Bool CReviewSystem::IsConnected() const
{
	return m_databaseConnector.IsConnected();
}

CReviewFlag* CReviewSystem::CreateNewFlag()
{
	if( m_newFlag != nullptr )
	{
		delete m_newFlag;
		m_newFlag = nullptr;
	}

	m_newFlag = new CReviewFlag();
	return m_newFlag;
}

Bool CReviewSystem::AddNewFlag( String& screenPath )
{
	SendMessage( MSM_SynchronizationStart );

	Bool result = false;
	if( m_newFlag != nullptr )
	{
		// protection against empty string
		if(m_newFlag->m_linkToVideo.Empty() == true)
		{
			m_newFlag->m_linkToVideo = TXT(" ");
		}

		// catch screen
		if( CreateScreenShot(m_newFlag, &m_newFlag->m_comments[0]) == false )
		{
			SendMessage( MSM_SynchronizationEnd );
			return false;
		}
		screenPath = m_newFlag->m_comments[0].m_pathToScreen;

		result = m_testtrackConnector.AddNewFlag( *m_newFlag );
		if(result == false)
		{
			SendMessage( MSM_SynchronizationEnd );
			return false;
		}

		result = m_databaseConnector.AddNewFlag( *m_newFlag, GetTTProject() );	

		if(result == true)
		{
			// protection against empty string
			m_newFlag->m_linkToVideo.Trim();

			// set update time
			time_t now = time(0);
			m_newFlag->m_lastUpdate = *localtime( &now );

			m_newFlag->m_comments.Clear();
			m_flags.PushBack( *m_newFlag );
			delete m_newFlag;
			m_newFlag = nullptr;
		}
		else
		{
			if( GFileManager->FileExist( m_newFlag->m_comments[0].m_pathToScreen ) == true )
			{
				GFileManager->DeleteFile( m_newFlag->m_comments[0].m_pathToScreen );
			}
		}
	}

	SendMessage( MSM_SynchronizationEnd );
	SendRequest( MSRT_SynchronizeData );

	return result;
}

Bool CReviewSystem::ModifyFlag( CReviewFlag& flag, CReviewFlagComment& comment, Bool makeScreen )
{
	SendMessage( MSM_SynchronizationStart );

	// catch screen
	if( makeScreen == true )
	{
		if( CreateScreenShot( &flag, &comment ) == false )
		{
			SendMessage( MSM_SynchronizationEnd );
			return false;
		}
	}
	else
	{
		// get screen from last comment
		const Uint32 commentCount = flag.m_comments.Size();
		const CReviewFlagComment& lastComment = flag.m_comments[commentCount-1];
		comment.m_pathToScreen = lastComment.m_pathToScreen;
	}

	// update flag in testtrack
	Bool result = false;
	const Uint32 commentCount = flag.m_comments.Size();
	Bool stateIsModified = ( comment.m_state != flag.m_comments[commentCount-1].m_state );
	if( stateIsModified == true )
	{
		result = m_testtrackConnector.ModifyFlag(flag, comment);
	}
	else
	{
		result = m_testtrackConnector.AddComment( flag, comment );
	}

	if( result == false )
	{		
		GFeedback->ShowWarn( TXT("Information about the flag cannot be updated in Test Track. Please change it manually.") );
	}

	// update flag in database
	result = m_databaseConnector.ModifyFlag(flag, comment);
	
	// delete temporary image
	if( result == false )
	{
		if( GFileManager->FileExist(comment.m_pathToScreen) == true )
		{
			GFileManager->DeleteFile(comment.m_pathToScreen);
		}
	}

	SendMessage( MSM_SynchronizationEnd );
	SendRequest( MSRT_SynchronizeData );

	return result;
}

void CReviewSystem::SetFilters( const TDynArray<Bool>& states, const TDynArray<Bool>& types )
{
	SetInternalFilters( states, types, m_filterCategory, m_filterCondition );
}

void CReviewSystem::SetDownloadClosedFlags(Bool download)
{
	if( m_downloadClosedFlags != download )
	{
		m_downloadClosedFlags = download;
		Config::cvDownloadClosedFlags.Set( m_downloadClosedFlags );

		SendRequest( MSRT_ReloadData );
	}
}

void CReviewSystem::SetAutoSync(Bool autosync)
{
	m_autoSync = autosync;
	m_lastSyncCounter.ResetTimer();
	Config::cvAutoSync.Set( m_autoSync );
}

void CReviewSystem::SetShowOnMap(Bool showOnMap)
{
	m_showOnMap = showOnMap;
	Config::cvShowOnMap.Set( m_showOnMap );
	SendRequest( MSRT_UpdateData );
}

void CReviewSystem::SetSyncTime(Uint32 time)
{
	m_autoSyncTime = time;
	Config::cvAutoSyncTime.Set( m_autoSyncTime );
}

Bool CReviewSystem::GetDownloadClosedFlags() const
{
	return m_downloadClosedFlags;
}

Bool CReviewSystem::GetAutoSync() const
{
	return m_autoSync;
}

Bool CReviewSystem::GetShowOnMap() const
{
	return m_showOnMap;
}

Uint32 CReviewSystem::GetSyncTime() const
{
	return m_autoSyncTime;
}

CReviewFlag* CReviewSystem::GetNewFlag() const
{
	return m_newFlag;
}

void CReviewSystem::GetStateList( TDynArray< String >&  flagStates )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_synchronizationMutex );
	flagStates.CopyFast( m_flagStates );
}

void CReviewSystem::GetBugTypeList( TDynArray< String >& flagTypes )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_synchronizationMutex );
	flagTypes.CopyFast( m_flagTypes );
}

void CReviewSystem::GetFlags( TDynArray< CReviewFlag* >& filteredFlags )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_synchronizationMutex );
	filteredFlags.CopyFast( m_filteredFlags );
}

CReviewFlag* CReviewSystem::GetFlag(Uint32 number)
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_synchronizationMutex );
	if(number < m_filteredFlags.Size())
	{
		return m_filteredFlags[number];
	}
	return nullptr;
}

void CReviewSystem::Connect()
{
	SendMessage( MSM_DatabaseConnectionStart );
	
	m_initializationErrorMessage.Clear();
	if( m_databaseConnector.Connect( m_initializationErrorMessage ) == true )
	{
		m_databaseConnector.GetFlagStates( m_flagStates );

		Bool result = m_testtrackConnector.Initialize( TXT("Pre-Alpha"), GetTTProject() );
		if( result == false )
		{
			SendMessage( MSM_TestTrackLostConnection );
			return;
		}

		m_testtrackConnector.GetMilestoneList( m_flagMilestones );
		m_testtrackConnector.GetPriorityList( m_flagPriorities );
		m_testtrackConnector.GetBugTypeList( m_flagTypes );
		m_testtrackConnector.GetProjectList( m_projectList );

		LoadSettings();
		SendMessage( MSM_DatabaseConnected );

		SendRequest( MSRT_ReloadData );
	}
	else
	{
		SendMessage( MSM_DatabaseLostConnection );
	}
}

void CReviewSystem::SynchronizeFlags()
{
	SendMessage( MSM_SynchronizationStart );

	// save date of last synchronization for map
	if( GGame == nullptr || GGame->GetActiveWorld() == nullptr )
	{
		SendMessage( MSM_SynchronizationEnd );
		return;
	}
	String worldName = GGame->GetActiveWorld()->GetDepotPath();

	// synchronize flags with database
	String lastUpdateTime;
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("user"), TXT("ReviewSystem"), worldName, lastUpdateTime );
	lastUpdateTime.ReplaceAll(TXT("_"), TXT(" "), true);
	m_databaseConnector.Synchronize(m_flags, worldName, lastUpdateTime);

	// save time of last update to ini file
	SaveUpdateTimeForMap( worldName );

	FilterFlags();
	SortFilteredFlags();

	SendMessage( MSM_SynchronizationEnd );
}

void CReviewSystem::FilterFlags()
{
	RemoveEntietiesFromMap();

	m_filteredFlags.ClearFast();

	if( m_filtersAreOn == true )
	{
		const Uint32 flagCount = m_flags.Size();
		for( Uint32 i=0; i<flagCount; ++i )
		{
			// filter type
			Uint32 type = m_flags[i].m_type;
			if( type < m_flagTypes.Size() )
			{
				if(m_filterTypesValues[type] == false)
				{
					continue;
				}
			}

			// filter state
			Uint32 lastCommentIndex = m_flags[i].m_comments.Size()-1;
			Uint32 state = m_flags[i].m_comments[lastCommentIndex].m_state-1;
			if( state < m_flagStates.Size() )
			{
				if(m_filterStatesValues[state] == false)
				{
					continue;
				}
			}

			// filter by pattern
			if( m_filterCategory == ReviewSearchByTestTrackID )	// TestTrack number
			{
				Int32 testTrackNumber = -1;
				FromString<Int32>( m_filterCondition, testTrackNumber);
				if( testTrackNumber > -1)
				{
					if(m_flags[i].m_testTrackNumber == (Uint32)testTrackNumber)
					{
						m_filteredFlags.PushBack( &m_flags[i] );
					}
					continue;
				}
			}
			if( m_filterCategory == ReviewSearchBySummary )	// Summary
			{
				if(m_flags[i].m_summary.ContainsSubstring(m_filterCondition) == true)
				{
					m_filteredFlags.PushBack(&m_flags[i]);
				}
				continue;
			}

			m_filteredFlags.PushBack(&m_flags[i]);
		}
	}
	else
	{
		const Uint32 flagCount = m_flags.Size();
		for( Uint32 i=0; i<flagCount; ++i )
		{
			m_filteredFlags.PushBack( &m_flags[i] );
		}
	}

	AddEntitiesOnMap();
}

void CReviewSystem::SaveUpdateTimeForMap(String& mapName)
{
	time_t now = time(0);
	tm* localTime = localtime(&now);
	String dateTime = ToString(localTime->tm_year+1900)+TXT("-")+ToString(localTime->tm_mon+1)+TXT("-")+ToString(localTime->tm_mday)+TXT("_");
	dateTime += ToString(localTime->tm_hour)+TXT(":")+ToString(localTime->tm_min)+TXT(":")+ToString(localTime->tm_sec);

	SConfig::GetInstance().GetLegacy().WriteParam( TXT("user"), TXT("ReviewSystem"), mapName.AsChar(), dateTime );

	m_lastSyncCounter.ResetTimer();
}

Bool CReviewSystem::ImportScreenForComment( CReviewFlagComment* comment )
{
	String importFileExt = CFilePath( comment->m_pathToScreen ).GetExtension();
	IImporter* importer = IImporter::FindImporter( ClassID< CBitmapTexture >(), importFileExt );
	if ( !importer )
	{
		GFeedback->ShowError( TXT("There is no importer to import source data from '%ls'. Make source source data is there."), comment->m_pathToScreen.AsChar() );
		return false;
	}

	// Reimport file
	IImporter::ImportOptions options;
	options.m_existingResource = nullptr;
	options.m_parentObject = nullptr;
	options.m_sourceFilePath = comment->m_pathToScreen;
	comment->m_screen = nullptr;
	comment->m_screen = SafeCast<CBitmapTexture>(importer->DoImport( options ));

	return comment->m_screen.Get() != nullptr;
}

void CReviewSystem::SortFilteredFlags()
{
	switch( m_category )
	{
	case ReviewCreationDate:
		{
			struct LocalSorterByCreationDate
			{
				static Bool RED_FORCE_INLINE Sort( const CReviewFlag* flag1, const CReviewFlag* flag2 )
				{
					tm time1 = flag1->m_lastUpdate;
					time1.tm_year -= 1900;
					tm time2 = flag2->m_lastUpdate;
					time2.tm_year -= 1900;

					double seconds =  difftime( mktime( &time1 ), mktime( &time2 ) );

					return (seconds < 0);
				}
			};
			Sort( m_filteredFlags.Begin(), m_filteredFlags.End(), LocalSorterByCreationDate::Sort );
		}
		break;
	case ReviewSummary:
		{
			struct LocalSorterBySummary
			{
				static Bool RED_FORCE_INLINE Sort( const CReviewFlag* flag1, const CReviewFlag* flag2 )
				{
					return Red::System::StringCompareNoCase( flag1->m_summary.AsChar(), flag2->m_summary.AsChar() ) < 0;
				}
			};
			Sort( m_filteredFlags.Begin(), m_filteredFlags.End(), LocalSorterBySummary::Sort );
		}
		break;
	case ReviewState:
		{
			struct LocalSorterByState
			{
				static Bool RED_FORCE_INLINE Sort( const CReviewFlag* flag1, const CReviewFlag* flag2 )
				{
					Uint32 state1 = flag1->m_comments[flag1->m_comments.Size()-1].m_state;
					Uint32 state2 = flag2->m_comments[flag2->m_comments.Size()-1].m_state;

					return (state1 < state2);
				}
			};
			Sort( m_filteredFlags.Begin(), m_filteredFlags.End(), LocalSorterByState::Sort );
		}
		break;
	case ReviewType:
		{
			struct LocalSorterByType
			{
				static Bool RED_FORCE_INLINE Sort( const CReviewFlag* flag1, const CReviewFlag* flag2 )
				{
					return (flag1->m_type < flag2->m_type);
				}
			};
			Sort( m_filteredFlags.Begin(), m_filteredFlags.End(), LocalSorterByType::Sort );
		}
		break;
	case ReviewPriority:
		{
			struct LocalSorterByPriority
			{
				static Bool RED_FORCE_INLINE Sort( const CReviewFlag* flag1, const CReviewFlag* flag2 )
				{
					Uint32 priority1 = flag1->m_comments[flag1->m_comments.Size()-1].m_priority;
					Uint32 priority2 = flag2->m_comments[flag2->m_comments.Size()-1].m_priority;

					return (priority1 < priority2);
				}
			};
			Sort( m_filteredFlags.Begin(), m_filteredFlags.End(), LocalSorterByPriority::Sort );
		}
		break;
	}

	if( m_sortingOrder == MSSO_Descending )
	{
		// invert position
		const Uint32 flagCount = m_filteredFlags.Size();
		for( Uint32 i=0; i<flagCount/2; ++i )
		{
			CReviewFlag* tempFlag = m_filteredFlags[i];
			m_filteredFlags[i] = m_filteredFlags[flagCount-i-1];
			m_filteredFlags[flagCount-i-1] = tempFlag;
		}
	}

	SendMessage( MSM_DataAreSorted );
}


#define USE_UBER_SCREENSHOT_IN_MARKER

#ifdef USE_UBER_SCREENSHOT_IN_MARKER
struct SReviewFlagScreenshotAttacher
{
	CReviewFlagComment* m_comment;
	Bool				m_finished;

	Bool operator== ( const SReviewFlagScreenshotAttacher& other ) const { return m_comment == other.m_comment; }

	void OnFinished( const String& path, const void* buffer, Bool status, const String& msg )
	{
		ASSERT( GFileManager->FileExist( path ) );
		GFileManager->SetFileReadOnly( path, false );
		m_comment->m_pathToScreen = path;
		m_finished = true;
	}
};
#endif // USE_UBER_SCREENSHOT_IN_MARKER

Bool CReviewSystem::CreateScreenShot( CReviewFlag* flag, CReviewFlagComment* comment )
{
	CWorld* world = GGame->GetActiveWorld();
	CFilePath worldPath( world->GetDepotPath() );
	String worldName = worldPath.GetFileName();

	// Format screenshot name
	Red::System::DateTime dt;
	Red::System::Clock::GetInstance().GetLocalTime( dt );
	String time = String::Printf( TXT("%d%d%d"), dt.GetHour(), dt.GetMinute(), dt.GetSecond() );
	String modifiedSummary = flag->m_summary;
	modifiedSummary.ReplaceAll( TXT(" "), TXT("_") );

	// create subdirectory
	String directoryPath = String::Printf( TXT("\\\\cdprs-ttpscreens\\TTP_Screens\\%d_%d_%d\\"), dt.GetDay() + 1, dt.GetMonth() + 1, dt.GetYear() );
	String subdirectoryPath = String::Printf( TXT("%s%s\\"), directoryPath.AsChar(), worldName.AsChar() );
	String screenFullPath = String::Printf( TXT("%s%s_%s.png"), subdirectoryPath.AsChar(), modifiedSummary.AsChar(), time.AsChar() );
	GSystemIO.CreateDirectory( directoryPath.AsChar() );
	GSystemIO.CreateDirectory( subdirectoryPath.AsChar() );

	// prepare parameters and callback
	IViewport* viewport = GGame->GetViewport();
	SScreenshotParameters parameters( viewport->GetFullWidth(), viewport->GetFullHeight(), screenFullPath.AsChar(), SF_PNG, 4, 90.0f, SRF_PlainScreenshot );

#ifdef USE_UBER_SCREENSHOT_IN_MARKER
	SReviewFlagScreenshotAttacher screenshotAttacher;
	screenshotAttacher.m_finished = false;
	screenshotAttacher.m_comment = comment;
	Functor4< void, const String&, const void*, Bool, const String& > funct4( &screenshotAttacher, &SReviewFlagScreenshotAttacher::OnFinished );

	SScreenshotSystem::GetInstance().RequestScreenshot( parameters, SCF_SaveToDisk | SCF_UseProvidedFilename, SRF_PlainScreenshot, funct4 );
	SScreenshotSystem::GetInstance().Flush();

	ASSERT( screenshotAttacher.m_finished );
#else
	GRender->Flush();
	SScreenshotSystem::GetInstance().TakeSimpleScreenshotNow( parameters, SCF_SaveToDisk | SCF_UseProvidedFilename );
	ASSERT( GFileManager->FileExist( screenFullPath ) );
	GFileManager->SetFileReadOnly( screenFullPath, false );
	comment->m_pathToScreen = screenFullPath;
	
#endif // USE_UBER_SCREENSHOT_IN_MARKER
	return true;
}

void CReviewSystem::UpdateData()
{
	FilterFlags();
	SortFilteredFlags();

	SendMessage( MSM_DataAreUpdated );
}

void CReviewSystem::SetSortingSettings( EReviewSortCategory category, EMarkerSystemSortOrder order )
{
	m_category = category;
	m_sortingOrder = order;

	SendRequest( MSRT_SortData );
}

void CReviewSystem::RemoveEntietiesFromMap()
{
	for( auto it=m_flags.Begin(); it!=m_flags.End(); ++it )
	{
		CReviewFlag& reviewFlag = ( *it );
		if( reviewFlag.m_flagEntity != nullptr )
		{
			m_entitiesToRemove.PushBackUnique( reviewFlag.m_flagEntity );
			m_entitiesToAdd.Remove( &reviewFlag );
			reviewFlag.m_flagEntity = nullptr;
		}
	}
}

void CReviewSystem::AddEntitiesOnMap()
{
	if( m_showOnMap == true )
	{
		for( auto it=m_flags.Begin(); it!=m_flags.End(); ++it )
		{
			CReviewFlag& reviewFlag = ( *it );
			m_entitiesToAdd.PushBackUnique( &reviewFlag );
		}
	}
}

THandle< CEntityTemplate > CReviewSystem::GetFlagTemplate( enum EReviewFlagState flagState ) const
{
	if( flagState < RFS_Count )
	{
		return m_flagEntities[flagState];
	}
	return nullptr;
}

void CReviewSystem::GetStateValues( TDynArray< Bool >& stateValues )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_synchronizationMutex );
	stateValues.CopyFast( m_filterStatesValues );
}

void CReviewSystem::GetTypesValues( TDynArray< Bool >& typeValues )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_synchronizationMutex );
	typeValues.CopyFast( m_filterTypesValues );
}

EReviewSearchType CReviewSystem::GetSearchType() const
{
	return m_filterCategory;
}

String CReviewSystem::GetSearchCondition() const
{
	return m_filterCondition;
}

void CReviewSystem::LoadSettings()
{
	// read settings from ini file

	m_downloadClosedFlags = Config::cvDownloadClosedFlags.Get();
	m_showOnMap = Config::cvShowOnMap.Get();
	m_autoSync = Config::cvAutoSync.Get();
	m_autoSyncTime = Config::cvAutoSyncTime.Get();

	// filters
	m_filterCategory = (EReviewSearchType)Config::cvFilterCategory.Get();
	m_filterCondition = Config::cvFilterCondition.Get();
	
	const Uint32 typeCount = m_flagTypes.Size();
	m_filterTypesValues.ResizeFast( typeCount );
	for( Uint32 i=0; i<typeCount; ++i )
	{
		m_filterTypesValues[i] = true;
		SConfig::GetInstance().GetLegacy().ReadParam( TXT("user"), TXT("ReviewSystem"), m_flagTypes[i], m_filterTypesValues[i] );
	}

	const Uint32 stateCount = m_flagStates.Size();
	m_filterStatesValues.ResizeFast( stateCount );
	for( Uint32 i=0; i<stateCount; ++i )
	{
		m_filterStatesValues[i] = true;
		SConfig::GetInstance().GetLegacy().ReadParam( TXT("user"), TXT("ReviewSystem"), m_flagStates[i], m_filterStatesValues[i] );
	}
}

void CReviewSystem::SaveSettings()
{
	// write settings from ini file
	Config::cvDownloadClosedFlags.Set( m_downloadClosedFlags );
	Config::cvShowOnMap.Set( m_showOnMap );
	Config::cvAutoSync.Set( m_autoSync );
	Config::cvAutoSyncTime.Set( m_autoSyncTime );

	// filters
	Config::cvFilterCategory.Set( m_filterCategory );
	Config::cvFilterCondition.Set( m_filterCondition );

	const Uint32 typeCount = m_flagTypes.Size();
	for( Uint32 i=0; i<typeCount; ++i )
	{
		SConfig::GetInstance().GetLegacy().WriteParam( TXT("user"), TXT("ReviewSystem"), m_flagTypes[i].AsChar(), m_filterTypesValues[i] );
	}

	const Uint32 stateCount = m_flagStates.Size();
	for( Uint32 i=0; i<stateCount; ++i )
	{
		SConfig::GetInstance().GetLegacy().WriteParam( TXT("user"), TXT("ReviewSystem"), m_flagStates[i].AsChar(), m_filterStatesValues[i] );
	}
}

void CReviewSystem::GetProjectList( TDynArray< String >& projectList )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_synchronizationMutex );
	projectList.CopyFast( m_projectList );
}

void CReviewSystem::GetMilestoneList( TDynArray< String >& flagMilestones )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_synchronizationMutex );
	flagMilestones.CopyFast( m_flagMilestones );
}

void CReviewSystem::GetPriorityList( TDynArray< String >& flagPriorities )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_synchronizationMutex );
	flagPriorities.CopyFast( m_flagPriorities );
}

String CReviewSystem::GetDefaultProjectName() const
{
	return m_testtrackConnector.GetDefaultProjectName();
}

String CReviewSystem::GetDefaultMilestoneName() const
{
	return m_testtrackConnector.GetDefaultMilestoneName();
}

void CReviewSystem::SetDefaultProjectName( const String& projectName )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_synchronizationMutex );

	Bool result = m_testtrackConnector.SwitchProject( projectName );
	if( result == false )
	{
		SendMessage( MSM_TestTrackLostConnection );
		return;
	}

	m_ttProject = projectName;
	m_testtrackConnector.GetMilestoneList( m_flagMilestones );
	m_testtrackConnector.GetPriorityList( m_flagPriorities );
	m_testtrackConnector.GetBugTypeList( m_flagTypes );
	m_testtrackConnector.GetProjectList( m_projectList );
}

void CReviewSystem::SetDefaultMilestoneName( const String& milestoneName )
{
	m_testtrackConnector.SetProjectMilestone( milestoneName );
}

void CReviewSystem::SetSearchFilter( EReviewSearchType searchType, const String& condition )
{
	SetInternalFilters( m_filterStatesValues, m_filterTypesValues, searchType, condition );
}

void CReviewSystem::SetInternalFilters( const TDynArray<Bool>& states, const TDynArray<Bool>& types, EReviewSearchType searchType, const String& condition )
{
	m_filterStatesValues.CopyFast( states );
	m_filterTypesValues.CopyFast( types );
	m_filterCategory = searchType;
	m_filterCondition = condition;
	m_filtersAreOn = true;

	SendRequest( MSRT_UpdateData );
}

const String CReviewSystem::GetTTProject()
{
	return m_ttProject;
}

Bool CReviewSystem::TryToLoadToTTP( const String& user, const String& password )
{
	return m_testtrackConnector.ConnectToTTP( user, password, GetTTProject() );
}

#endif // NO_MARKER_SYSTEMS
