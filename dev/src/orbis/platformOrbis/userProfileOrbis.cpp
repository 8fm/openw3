/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "userProfileOrbis.h"
#include "inputDeviceManagerOrbis.h"
#include "orbisApiCall.h"
#include "orbisSystemDialog.h"

#include "../../common/game/gameSaver.h"

#include "../../common/engine/baseEngine.h"

#include "../../common/engine/gameSaveManager.h"

#include "../../common/core/memoryFileReader.h"
#include "../../common/core/memoryFileWriter.h"
#include "../../common/core/xmlReader.h"

#include <libsysmodule.h>
#include <system_service.h>
#include <user_service.h>

#include "userProfileTasks.h"

// Achievements
#pragma comment( lib, "libSceNpTrophy_stub_weak.a" )

// Game [Rich] Presence
#include <libhttp.h>
#include <libssl.h>
#include <np/np_webapi.h>

#pragma comment( lib, "libSceNpManager_stub_weak.a" )
#pragma comment( lib, "libSceNet_stub_weak.a" )
#pragma comment( lib, "libSceSsl_stub_weak.a" )
#pragma comment( lib, "libSceHttp_stub_weak.a" )
#pragma comment( lib, "libSceNpWebApi_stub_weak.a" )

// Base include for all sce dialogs
#include <common_dialog.h>
#pragma comment( lib, "libSceCommonDialog_stub_weak.a" )

// Help / web browser
#include <web_browser_dialog.h>

// User profile dialog
#include <np_profile_dialog.h>

// Error dialog
#include <error_dialog.h>
#include <np_commerce_dialog.h>

// Screenshots
#pragma comment( lib, "libSceScreenShot_stub_weak.a" )

CUserProfileManagerOrbis::CUserProfileManagerOrbis()
	: m_userId( SCE_USER_SERVICE_USER_ID_INVALID )
	, m_netPoolId( -1 )
	, m_sslContextId( -1 )
	, m_httpContextId( -1 )
	, m_webApiContextId( -1 )
	, m_initialized( false )
	, m_hasOnlinePermission( false )
	, m_presenceInitializationStage( PI_Uninitialized )
	, m_initializedLoadSaveInfoIndex( -1 )
	, m_saveSystemInitialized( false )
	, m_saveInfosUpdateState( LIST_NeedsUpdate )
	, m_displayingOutOfDiskSpaceError( false )
	, m_displayingBrokenSaveDataError( false )
	, m_displayingSaveDataErrorCode( false )
	, m_displayingCorruptedSaveOverwriteMessage( false )
	, m_userSettingsExists( false )
	, m_shouldMountSettingsForReading( false )
	, m_loadSaveReadyEventSent( false )
	, m_lockedUserActions( false )
	, m_screenshotReadingRequestStatus( SCR_NotRequested )
	, m_screenshotReadingRequestInfoIndex( -1 )
	, m_screenshotBuffer( DefaultDataBufferAllocator(), CGameSaver::SCREENSHOT_BUFFER_SIZE )
	, m_realScreenshotSize( 0 )
	, m_ignoreSaveUserSettingsRequest( true )
{
	// ****
	// This have been moved to the constructor because of "save settings in profile" feature.
	// User settings are accessed (and needed) at a quite early stage, before we call Initialize().
	// That's why we need save ststem to be functional before Initialize(), and that's why I move this stuff here:
	InitializeDebugTitleId();

	ORBIS_SYS_CALL( sceUserServiceInitialize( nullptr ) );
	ORBIS_SYS_CALL( sceCommonDialogInitialize() );

	if ( false == InitializeSaveSystem() )
	{
		HALT("SaveData library cannot be initialized. DEBUG THIS!");
	}
	// ****

	// PUT InGamePresence [Presence APIs]
	// It is recommended that the frequency by which this API is called be kept within 45 times/15 minutes.
	m_minTimeBetweenRichPresenceUpdates = 20.0f;
}

CUserProfileManagerOrbis::~CUserProfileManagerOrbis()
{
	ShutdownSaveSystem();

	ORBIS_SYS_CALL( sceUserServiceTerminate() );
}

Bool CUserProfileManagerOrbis::Initialize()
{
	ORBIS_SYS_CALL_RET( sceSysmoduleLoadModule( SCE_SYSMODULE_NP_TROPHY ) );
	ORBIS_SYS_CALL_RET( sceSysmoduleLoadModule( SCE_SYSMODULE_SCREEN_SHOT ) );
	
	if( !InitializeGamePresence() )
	{
		ShutdownGamePresence();
	}

	CInputDeviceManagerOrbis* inputManager = static_cast< CInputDeviceManagerOrbis* >( GEngine->GetInputDeviceManager() );

	if( inputManager )
	{
		inputManager->EnableUserSelection( !m_disableInput );
	}

	m_initialized = true;

	return true;
}

void CUserProfileManagerOrbis::Update()
{
	if( m_initialized )
	{
		Int32 success = SCE_OK;

		do
		{
			SceUserServiceEvent event;
			success = sceUserServiceGetEvent( &event );

			if( success == SCE_OK )
			{
				if( event.userId == m_userId )
				{
					if( event.eventType == SCE_USER_SERVICE_EVENT_TYPE_LOGOUT )
					{
						OnActiveUserLost();
					}
				}
				else if( m_userId == SCE_USER_SERVICE_USER_ID_INVALID )
				{
					// We're in promiscuous mode
					if( event.eventType == SCE_USER_SERVICE_EVENT_TYPE_LOGIN )
					{
						// Restart promiscuous mode so that we add the new controller to the list of those getting updated
						SetActiveUserPromiscuous();
					}
				}
			}

		} while( success == SCE_OK );

		RED_FATAL_ASSERT( success == SCE_USER_SERVICE_ERROR_NO_EVENT, "Something went wrong with the ps4 user message pump" );

		if( m_systemDialog && !( m_systemDialog->IsShown() ) )
		{
			if( m_systemDialog->Shutdown() )
			{
				delete m_systemDialog;
				m_systemDialog = nullptr;
			}
		}

		UpdateSaveSystem();

		CUserProfileManager::Update();
	}
}

Bool CUserProfileManagerOrbis::Shutdown()
{
	ShutdownGamePresence();

	ShutdownTrophySystem();

	ORBIS_SYS_CALL_RET( sceSysmoduleUnloadModule( SCE_SYSMODULE_NP_TROPHY ) );

	return true;
}

Bool CUserProfileManagerOrbis::InitializeDebugTitleId()
{
#ifndef RED_FINAL_BUILD

	static const SceNpTitleId NP_TITLE_ID =
	{
		{
		'A','A','A','A','1','2','3','4','5','_','0','0','\0'
		}
	};

	static const SceNpTitleSecret s_npTitleSecret =
	{
		{
			0xc5, 0x54, 0xc0, 0xcd, 0x15, 0xc9, 0xd4, 0x45,
			0x19, 0x67, 0x09, 0xc3, 0x65, 0xab, 0xf6, 0xc3,
			0x7b, 0xda, 0xe3, 0x79, 0xd1, 0xba, 0xc6, 0x2f,
			0xc9, 0x79, 0xf6, 0x8e, 0xc2, 0x75, 0x45, 0x7c,
			0xcb, 0x25, 0x2a, 0xd2, 0x28, 0x05, 0xc7, 0x8a,
			0xc4, 0x36, 0x6f, 0x1f, 0xa6, 0x50, 0x0d, 0x1f,
			0xbc, 0x84, 0x9e, 0x9b, 0xad, 0xde, 0xaf, 0x7c,
			0x11, 0x65, 0x54, 0x68, 0x18, 0xef, 0x43, 0x3f,
			0x42, 0x70, 0x06, 0x76, 0x0c, 0x06, 0x7b, 0x40,
			0x1f, 0xa1, 0x27, 0x90, 0x27, 0xba, 0xd6, 0xb6,
			0xbf, 0xe6, 0x3c, 0x1f, 0x3e, 0xe4, 0x79, 0x1e,
			0xe8, 0xa4, 0x1c, 0xac, 0x3f, 0xea, 0x86, 0x0a,
			0xcd, 0x6a, 0xea, 0xd9, 0xf7, 0x1d, 0xb1, 0xc9,
			0x53, 0x6e, 0x34, 0x76, 0xbe, 0xad, 0xa6, 0x63,
			0x3a, 0xaa, 0x87, 0x77, 0x61, 0x48, 0xd7, 0x79,
			0xd9, 0xe7, 0xb9, 0xe1, 0xc6, 0x53, 0x20, 0x80
		}
	};

	ORBIS_SYS_CALL_RET( sceNpSetNpTitleId( &NP_TITLE_ID, &s_npTitleSecret ) );

#endif // RED_FINAL_BUILD

	return true;
}

Bool CUserProfileManagerOrbis::CheckPermissions()
{
	COrbisPermissionCheckTask* permissionTask = new( CTask::Root ) COrbisPermissionCheckTask( m_userId, m_hasOnlinePermission );

	RED_FATAL_ASSERT( GTaskManager, "Task manager has not been initialised yet" );
	GTaskManager->Issue( *permissionTask, TSP_Normal, TSG_Service );

	// Fire and forget
	permissionTask->Release();

	return true;
}

Bool CUserProfileManagerOrbis::InitializeTrophySystem()
{
	SceNpServiceLabel serviceLabel = 0;
	ORBIS_SYS_CALL_RET( sceNpTrophyCreateContext( &m_trophyContext, m_userId, serviceLabel, 0 ) );

	COrbisTrophyContextRegistrationTask* registrationTask = new( CTask::Root ) COrbisTrophyContextRegistrationTask( m_userId, m_trophyContext );

	RED_FATAL_ASSERT( GTaskManager, "Task manager has not been initialised yet" );
	GTaskManager->Issue( *registrationTask, TSP_Normal, TSG_Service );

	// Fire and forget
	registrationTask->Release();

	return true;
}

Bool CUserProfileManagerOrbis::ShutdownTrophySystem()
{
	ORBIS_SYS_CALL_RET( sceNpTrophyDestroyContext( m_trophyContext ) );

	return true;
}

Bool CUserProfileManagerOrbis::InitializeGamePresence()
{
	RED_FATAL_ASSERT( m_presenceInitializationStage == PI_Uninitialized, "Game presence already initialised!" );

	ORBIS_SYS_CALL_RET( m_netPoolId = sceNetPoolCreate( "Game Presence HTTP", 16 * 1024, 0 ) );
	m_presenceInitializationStage = PI_NetPool;

	ORBIS_SYS_CALL_RET( m_sslContextId = sceSslInit( 256 * 1024 ) );
	m_presenceInitializationStage = PI_SSL;

	ORBIS_SYS_CALL_RET( m_httpContextId = sceHttpInit( m_netPoolId, m_sslContextId, 64 * 1024 ) );
	m_presenceInitializationStage = PI_HTTP;

	ORBIS_SYS_CALL_RET( m_webApiContextId = sceNpWebApiInitialize( m_httpContextId, 1024 * 1024 ) );
	m_presenceInitializationStage = PI_WebApi;

	return true;
}

Bool CUserProfileManagerOrbis::ShutdownGamePresence()
{
	switch( m_presenceInitializationStage )
	{
	case PI_WebApi:
		ORBIS_SYS_CALL_RET( sceNpWebApiTerminate( m_webApiContextId ) );
		m_webApiContextId = -1;
		m_presenceInitializationStage = PI_HTTP;

	case PI_HTTP:
		ORBIS_SYS_CALL_RET( sceHttpTerm( m_httpContextId ) );
		m_httpContextId = -1;
		m_presenceInitializationStage = PI_SSL;

	case PI_SSL:
		ORBIS_SYS_CALL_RET( sceSslTerm( m_sslContextId ) );
		m_sslContextId = -1;
		m_presenceInitializationStage = PI_NetPool;

	case PI_NetPool:
		ORBIS_SYS_CALL_RET( sceNetPoolDestroy( m_netPoolId ) );
		m_netPoolId = -1;
		m_presenceInitializationStage = PI_Uninitialized;
	}

	return true;
}

void CUserProfileManagerOrbis::UnlockAchievement( const CName& name )
{
	SceNpTrophyId sceId = SCE_NP_TROPHY_INVALID_TROPHY_ID;
	if( m_nameToTrophyIdMap.Find( name, sceId ) )
	{
		COrbisTrophyUnlockTask* unlockTask = new( CTask::Root ) COrbisTrophyUnlockTask( m_userId, m_trophyContext, sceId );

		RED_FATAL_ASSERT( GTaskManager, "Task manager has not been initialised yet" );
		GTaskManager->Issue( *unlockTask, TSP_Normal, TSG_Service );

		// Fire and forget
		unlockTask->Release();
	}
}

void CUserProfileManagerOrbis::GetAllAchievements( TDynArray< CName >& achievements ) const
{
	const Uint32 numAchievements = m_nameToTrophyIdMap.Size();
	achievements.Reserve( numAchievements );

	for( auto i = m_nameToTrophyIdMap.Begin(); i != m_nameToTrophyIdMap.End(); ++i )
	{
		achievements.PushBack( i->m_first );
	}
}

Bool CUserProfileManagerOrbis::IsAchievementMapped( const CName& name ) const
{
	return m_nameToTrophyIdMap.KeyExist( name );
}

void CUserProfileManagerOrbis::MapAchievementInit( Uint32 numAchievements )
{
	Uint32 totalNumberOfAchievements = m_nameToTrophyIdMap.Capacity() + numAchievements;
	m_nameToTrophyIdMap.Reserve( totalNumberOfAchievements );
}

void CUserProfileManagerOrbis::MapAchievement( const CName& name, const String& platform, const String& id )
{
	if( platform == TXT( "ps4" ) )
	{
		SceNpTrophyId sceId;

		FromString( id, sceId );

		m_nameToTrophyIdMap.Set( name, sceId );
	}
}

void CUserProfileManagerOrbis::LockAchievement( const CName& )
{
	RED_LOG_ERROR( CUserProfileManagerOrbis, TXT( "You cannot unlock trophies/achievements in the game on PS4, you must instead go through the system menu" ) );
}

void CUserProfileManagerOrbis::SetActiveUserDefault()
{
	OnActiveUserLost();

	CInputDeviceManagerOrbis* inputManager = static_cast< CInputDeviceManagerOrbis* >( GEngine->GetInputDeviceManager() );
	RED_FATAL_ASSERT( inputManager, "Cannot call SetActiveUserDefault() if Input device manager does not exist" );

	// Get initial user
	ORBIS_SYS_CALL( sceUserServiceGetInitialUser( &m_userId ) );

	if( inputManager->SetActiveUser( m_userId, true ) )
	{
		OnActiveUserAcquired( m_userId );
	}
}

void CUserProfileManagerOrbis::SetActiveUserPromiscuous()
{
	OnActiveUserLost();

	ToggleInputProcessing( true, CUserProfileManager::eAPDR_Startup );

	CInputDeviceManagerOrbis* inputManager = static_cast< CInputDeviceManagerOrbis* >( GEngine->GetInputDeviceManager() );

	if( inputManager )
	{
		TOrbisUserEvent callback( std::bind( &CUserProfileManagerOrbis::OnActiveUserAcquired, this, std::placeholders::_1 ) );

		inputManager->SetActiveUserPromiscuous( callback );
	}
}

void CUserProfileManagerOrbis::OnActiveUserAcquired( SceUserServiceUserId userId )
{
	m_userId = userId;

	QueueEvent( EUserEvent::UE_SignedIn );

	InitializeTrophySystem();
	CheckPermissions();

	if( userId != SCE_USER_SERVICE_USER_ID_INVALID )
	{
		UpdateSaveInfos();
		if ( m_userSettingsExists )
		{
			m_shouldMountSettingsForReading = true;
			m_loadSaveReadyEventSent = false;
		}
		else
		{
			// user settings don't exist, so we don't need to mount them first, and that means we're ready now
			m_loadSaveReadyEventSent = true;
			QueueEvent( EUserEvent::UE_LoadSaveReady );
		}
	}
}

void CUserProfileManagerOrbis::OnActiveUserLost()
{
	if( m_userId != SCE_USER_SERVICE_USER_ID_INVALID )
	{
		ShutdownTrophySystem();

		QueueEvent( EUserEvent::UE_SignedOut );

		m_userId = SCE_USER_SERVICE_USER_ID_INVALID;

		m_shouldMountSettingsForReading = false;
	}
}

Bool CUserProfileManagerOrbis::GetSafeArea( Float& x, Float& y )
{
	SceSystemServiceDisplaySafeAreaInfo info;
	ORBIS_SYS_CALL_RET( sceSystemServiceGetDisplaySafeAreaInfo( &info ) );

	x = y = info.ratio;

	return true;
}

void CUserProfileManagerOrbis::DoUserPresence( const CName& name )
{
	if( m_hasOnlinePermission.GetValue() )
	{
		Uint32 presenceStringId = 0;
		if( m_nameToPresenceIdMap.Find( name, presenceStringId ) )
		{
			COrbisPresenceTask* presenceTask = new( CTask::Root ) COrbisPresenceTask( m_userId, m_webApiContextId, presenceStringId );

			RED_FATAL_ASSERT( GTaskManager, "Task manager has not been initialised yet" );
			GTaskManager->Issue( *presenceTask, TSP_Normal, TSG_Service );

			// Fire and forget
			presenceTask->Release();
		}
	}
}

void CUserProfileManagerOrbis::MapPresenceInit( Uint32 numEntries )
{
	const Uint32 size = m_nameToPresenceIdMap.Capacity() + numEntries;
	m_nameToPresenceIdMap.Reserve( size );
}

void CUserProfileManagerOrbis::MapPresence( const CName& name, const String& platform, const String& id )
{
	if( platform == TXT( "ps4" ) )
	{
		Uint32 localizedStringId = 0;

		if( FromString( id, localizedStringId ) )
		{
			m_nameToPresenceIdMap.Set( name, localizedStringId );
		}
	}
}

String CUserProfileManagerOrbis::GetActiveUserDisplayNameRaw() const
{
	const size_t size = SCE_USER_SERVICE_MAX_USER_NAME_LENGTH + 1;
	AnsiChar name[ size ];

	if( ORBIS_SYS_CALL( sceUserServiceGetUserName( m_userId, name, size ) ) )
	{
		return String::Printf( TXT( "%hs" ), name );
	}

	return String::EMPTY;
}

template< typename TSystemDialog, typename TParam >
Bool CUserProfileManagerOrbis::DisplayDialog( const TParam& param )
{
	TSystemDialog* userProfileDialog = new TSystemDialog();

	if( userProfileDialog->Activate( param ) )
	{
		m_systemDialog = userProfileDialog;

		return true;
	}
	else
	{
		userProfileDialog->Shutdown();
		delete userProfileDialog;

		return false;
	}
}

void CUserProfileManagerOrbis::DisplayUserProfileSystemDialog()
{
	if( m_userId == SCE_USER_SERVICE_USER_ID_INVALID )
	{
		return;
	}

	if( !m_systemDialog )
	{
		SceNpProfileDialogParam param;
		sceNpProfileDialogParamInitialize( &param );

		param.mode = SCE_NP_PROFILE_DIALOG_MODE_NORMAL;
		param.userId = m_userId;
		SCE_NP_IN_GAME_MESSAGE_ERROR_NOT_SIGNED_IN;

		Int32 onlineIdErrorCode = -1;
		ORBIS_SYS_CALL( onlineIdErrorCode = sceNpGetOnlineId( m_userId, &param.targetOnlineId ) );
		if( onlineIdErrorCode == SCE_OK )
		{
			DisplayDialog< COrbisUserProfileDialog >( param );
		}
		else
		{
			RED_FATAL_ASSERT( onlineIdErrorCode != SCE_NP_ERROR_INVALID_ARGUMENT, "Invalid parameter passed to sceNpGetOnlineId()" );

			SceErrorDialogParam param;
			sceErrorDialogParamInitialize( &param );
			param.errorCode = onlineIdErrorCode;
			param.userId = m_userId;

			DisplayDialog< COrbisErrorDialog >( param );
		}
	}
}

void CUserProfileManagerOrbis::DisplayHelp()
{
	if( !m_systemDialog )
	{
		SceWebBrowserDialogParam param;
		sceWebBrowserDialogParamInitialize( &param );

		param.mode = SCE_WEB_BROWSER_DIALOG_MODE_DEFAULT;
		param.userId = m_userId;
		param.url = "http://en.cdprojektred.com/support/";

		DisplayDialog< COrbisWebBrowserDialog >( param );
	}
}

void CUserProfileManagerOrbis::DisplayStore()
{
	if( !m_systemDialog )
	{
		SceNpCommerceDialogParam param;
		sceNpCommerceDialogParamInitialize( &param );

		// Show top-category
		param.mode = SCE_NP_COMMERCE_DIALOG_MODE_CATEGORY;
		param.targets = nullptr;
		param.numTargets = 0;

		DisplayDialog< COrbisCommerceDialog >( param );
	}
}

Bool CUserProfileManagerOrbis::HasActiveUser() const
{
	return m_userId != SCE_USER_SERVICE_USER_ID_INVALID;
}

void CUserProfileManagerOrbis::ToggleInputProcessing( Bool enabled, EInputProcessingDisabledReason reason )  
{
	CUserProfileManager::ToggleInputProcessing( enabled, reason );

	CInputDeviceManagerOrbis* inputManager = static_cast< CInputDeviceManagerOrbis* >( GEngine->GetInputDeviceManager() );

	if( inputManager )
	{
		inputManager->EnableUserSelection( !m_disableInput );
	}
}

void CUserProfileManagerOrbis::LoadContentRestrictionXML( CXMLReader* xml )
{
	const Char*	ROOT_NODE = TXT( "redxml" );

	if( xml && xml->BeginNode( ROOT_NODE ) )
	{
		const Char*	CONTENT_RESTRICTION_NODE = TXT( "ocr" );
		if( xml->BeginNode( CONTENT_RESTRICTION_NODE ) )
		{
			Int32 defaultAge = 0;
			xml->AttributeT( TXT( "default" ), defaultAge );

			const Uint32 totalRegions = xml->GetChildCount();

			SceNpAgeRestriction* regions = new SceNpAgeRestriction[ totalRegions ];
			Red::System::MemoryZero( regions, sizeof( SceNpAgeRestriction ) * totalRegions );

			Uint32 numValidRegions = 0;

			const String COUNTRY_CODE_ATTRIBUTE( TXT( "code" ) );
			const String AGE_ATTRIBUTE( TXT( "age" ) );

			for( Uint32 i = 0; i < totalRegions; ++i )
			{
				xml->BeginNextNode();

				String name;
				RED_VERIFY( xml->GetNodeName( name ), TXT( "Node has no name!" ) );

				if( name == TXT( "region" ) )
				{
					StringAnsi countryCode;
					Int32 age;

					if( xml->AttributeT( COUNTRY_CODE_ATTRIBUTE, countryCode ) && xml->AttributeT( AGE_ATTRIBUTE, age ) )
					{
						if( countryCode.GetLength() == 2 )
						{
							regions[ numValidRegions ].countryCode.data[ 0 ] = countryCode[ 0 ];
							regions[ numValidRegions ].countryCode.data[ 1 ] = countryCode[ 1 ];
							regions[ numValidRegions ].age = age;

							++numValidRegions;
						}
						else
						{
							RED_LOG_ERROR( PS4ContentRatingXML, TXT( "Invalid country code (Length must be 2): %hs" ), countryCode.AsChar() );
						}
					}
					else
					{
						RED_LOG_ERROR( PS4ContentRatingXML, TXT( "Invalid Region tag" ) );
					}
				}
				else
				{
					RED_LOG_ERROR( PS4ContentRatingXML, TXT( "Unrecognised tag: \"%ls\"" ), name.AsChar() );
				}

				xml->EndNode();
			}

			SceNpContentRestriction contentRestriction;
			Red::System::MemoryZero( &contentRestriction, sizeof( SceNpContentRestriction ) );

			contentRestriction.ageRestriction			= regions;
			contentRestriction.ageRestrictionCount		= numValidRegions;
			contentRestriction.defaultAgeRestriction	= defaultAge;
			contentRestriction.size						= sizeof( SceNpContentRestriction );

			ORBIS_SYS_CALL( sceNpSetContentRestriction( &contentRestriction ) );

			delete [] regions;

			xml->EndNode();
		}
		else
		{
			RED_LOG_ERROR( PS4ContentRatingXML, TXT( "Missing \"ocr\" root node" ) );
		}

		xml->EndNode();
	}
	else
	{
		RED_LOG_ERROR( PS4ContentRatingXML, TXT( "Missing \"redxml\" base root node" ) );
	}
}

void CUserProfileManagerOrbis::SetIgnoreSavingUserSettings(Bool value)
{
	m_ignoreSaveUserSettingsRequest.SetValue( value );
}
