
/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "r6Game.h"
#include "r6GuiManager.h"

#include "r6DialogDisplayManager.h"
#include "combatUtils.h"
#include "teamManager.h"
#include "crowdManager.h"
#include "r6aiSystem.h"

#include "../../common/game/storySceneSystem.h"
#include "../../common/game/factsDB.h"
#include "../../common/game/communitySystem.h"
#include "../../common/game/interactionsManager.h"
#include "../../common/game/attitudeManager.h"
#include "../../common/game/questsSystem.h"
#include "../../common/game/commonGameResource.h"
#include "../../common/core/gatheredResource.h"
#include "../../common/engine/occlusionSystem.h"
#include "../../common/core/depot.h"
#include "../../common/core/scriptingSystem.h"

#include "../../games/r6/idSystem.h"
#include "../../games/r6/r6QuestSystem.h"
#include "../../common/engine/debugPageManagerBase.h"
#include "../../common/engine/viewport.h"


CGatheredResource resR6DefaultGameDef( TXT("game\\cp2077base.redgame"), RGF_Startup );

void InitializeR6GameSystems( CCommonGame *game )
{
	game->m_gameSystems.Resize( GSR6_MAX );

	game->AddSystem( CreateObject< CInteractionsManager > ( game ) );
	game->AddSystem( CreateObject< CR6QuestSystem > ( game ) );
	game->AddSystem( CreateObject< CStorySceneSystem > ( game ) );
	game->AddSystem( CreateObject< CFactsDB > ( game ) );
	game->AddSystem( CreateObject< CCommunitySystem > ( game ) );
	game->AddSystem( CreateObject< CAttitudeManager > ( game ) );
	game->AddSystem( CreateObject< CR6DialogDisplay > ( game ) );
	game->AddSystem( CreateObject< CInteractiveDialogSystem > ( game ) );
	game->AddSystem( CreateObject< CCrowdManager > ( game ) );
	game->AddSystem( CreateObject< CR6AISystem > ( game ) );

	game->OnGameplaySystemsInitialize();
}

void ShutdownR6GameSystems( CCommonGame *game )
{
	game->OnGameplaySystemsShutdown();
}

IMPLEMENT_ENGINE_CLASS( CR6Game );

CR6Game::CR6Game()
	: m_tempLoadingScreen( nullptr )
{
	m_aiObjectLooseTime = 300.0f;

#ifndef NO_DEBUG_PAGES
	m_idialogDebugPage = NULL;
#endif
}

void CR6Game::Init()
{
	TBaseClass::Init();

	// tempTest();

	// Register game in scripting system
	R6_ASSERT( GScriptingSystem );
	GScriptingSystem->RegisterGlobal( CScriptingSystem::GP_GAME, this );

	InitializeR6GameSystems( this );

	SetupGameResource();
	
	
	m_guiManager						= CreateObject< CR6GuiManager >( this ); // NOTE: GC is handled in CCommonGame OnSerialize

	m_combatUtils						= CreateObject< CCombatUtils >( this );	
	m_coversManager						= CreateObject< CCoversManager >( this );
	//m_crowdManager						= CreateObject< CCrowdManager >( this );

	m_combatUtils						->Initialize();
	m_guiManager						->Initialize();	

	#ifndef NO_DEBUG_PAGES
		// init debug pages
		extern IDebugPage* CreateDebugPageInteractiveDialogs();
		m_idialogDebugPage = CreateDebugPageInteractiveDialogs();
	#endif
}

void CR6Game::OnGameplaySystemsGameStart( const CGameInfo& info )
{	
	m_combatUtils	= CreateObject< CCombatUtils >( this );	
	m_coversManager	= CreateObject< CCoversManager >( this );
	
	m_combatUtils	->Initialize();
	m_coversManager	->Initialize();
	
	m_gameplayTime	= 0;

	TBaseClass::OnGameplaySystemsGameStart( info );
}

void CR6Game::ShutDown()
{
	if ( m_activeWorld )
	{
		UnloadWorld();
	}

	if ( m_guiManager )
	{
		m_guiManager->Deinitialize();
		m_guiManager->Discard();
		m_guiManager = NULL;
	}

	ShutdownR6GameSystems( this );

	// Unregister game in scripting system
	R6_ASSERT( GScriptingSystem );
	GScriptingSystem->RegisterGlobal( CScriptingSystem::GP_GAME, NULL );

#ifndef NO_DEBUG_PAGES
	m_idialogDebugPage = NULL;
#endif

	TBaseClass::ShutDown();
}

void CR6Game::Tick( Float timeDelta )
{
	TBaseClass::Tick( timeDelta );

	if( m_combatUtils && m_combatUtils->GetTeamManager() )
	{
		m_combatUtils->GetTeamManager()->Tick( timeDelta );
	}

	if( !IsPaused( ) )
	{
		m_gameplayTime += timeDelta;
	}
}

Bool CR6Game::SetupGameResourceFromFile( const String& filePath )
{
	if ( filePath.Empty() == false )
	{
		m_gameResource = LoadResource< CCommonGameResource >( filePath );
		return true;
	}

	R6_ASSERT( m_gameResource.IsValid() );
	return false;
}

void CR6Game::SetupGameResource()
{
	R6_ASSERT( IsActive() == false );

#if 0 // the performance of this makes kittens cry
	TDynArray< String > mainResourcePaths;
	GDepot->FindResourcesByExtension( CCommonGameResource::GetFileExtension(), mainResourcePaths );

	if ( mainResourcePaths.Empty() == true )
	{
		ERR_R6( TXT( "Game main definitions resource was not found!!!" ) );
	}
	if ( mainResourcePaths.Size() > 1 && GIsEditor == false )
	{
		WARN_R6( TXT( "More than one game definition resource found!! Using the first one" ) );
	}
	SetupGameResourceFromFile( mainResourcePaths[ 0 ] );
#endif

	SetupGameResourceFromFile( resR6DefaultGameDef.GetPath().ToString() );
}

void CR6Game::OnGameplaySystemsWorldStart( const CGameInfo& info )
{	
	TBaseClass::OnGameplaySystemsWorldStart( info );
	m_coversManager->Initialize();
}
void CR6Game::OnGameplaySystemsWorldEnd( const CGameInfo& info )
{
	CallEvent( CNAME( OnGameEndedR6 ) );
	TBaseClass::OnGameplaySystemsWorldEnd( info );
}

void CR6Game::funcGetViewPortWidthAndHeight( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( Int32, _WidthI, 0 );
	GET_PARAMETER_REF( Int32, _HeightI, 0 );
	FINISH_PARAMETERS;

	_WidthI		= GGame->GetViewport()->GetWidth();
	_HeightI	= GGame->GetViewport()->GetHeight();
}

void CR6Game::funcEndCurrentDialog( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RED_LOG( Dialog, TXT("Deprecation warning: theGame.EndCurrentDialog() is deprecated, please don't use.") );
}

void CR6Game::funcGetCoverManager( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_HANDLE( CCoversManager, m_coversManager );
}

void CR6Game::funcGetCombatUtils( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_HANDLE( CCombatUtils, m_combatUtils );
}

Bool CR6Game::OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data )
{
#ifndef NO_DEBUG_PAGES
	if ( RIM_IS_KEY_DOWN( IK_Ctrl ) && key == IK_I && action == IACT_Press )
	{
		IDebugPageManagerBase::GetInstance()->SelectDebugPage( m_idialogDebugPage );
		return true;
	}
#endif

	return TBaseClass::OnViewportInput( view, key, action, data );
}

void CR6Game::funcChangeWorld( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, path, String::EMPTY );
	FINISH_PARAMETERS;	

	const TSoftHandle< CWorld >* world = GetGameResource()->FindWorld( path );
	if ( world && !world->GetPath().Empty() )
	{
		SChangeWorldInfo info;
		info.SetWorldName( world->GetPath() );
		ScheduleWorldChange( info );
	}
}

void CR6Game::funcGetGameplayTime( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_FLOAT( m_gameplayTime );
}
