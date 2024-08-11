#include "build.h"
#include "questScenesDebugger.h"
#include "questEditor.h"
#include "../../common/game/actor.h"
#include "../../common/game/questsSystem.h"
#include "../../common/game/questScenePlayer.h"
#include "../../common/game/storyScenePlayer.h"


CQuestScenesDebugger::CQuestScenesDebugger()
: m_activeScenes( NULL )
, m_scheduledScenes( NULL )
, m_scenesMgr( NULL )
, m_host( NULL )
{
}

CQuestScenesDebugger::~CQuestScenesDebugger()
{
	OnDetach();
}

void CQuestScenesDebugger::OnAttach( CEdQuestEditor& host, wxWindow* parent )
{
	m_host = &host;

	wxXmlResource::Get()->LoadPanel( this, parent, wxT("QuestScenesDebugger") );
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( CQuestScenesDebugger::OnClose ), 0, this );

	// initialize widgets
	m_scheduledScenes = XRCCTRL( *this, "scheduledScenes", wxListCtrl );
	m_activeScenes = XRCCTRL( *this, "activeScenes", wxListCtrl );
	m_completedScenes = XRCCTRL( *this, "completedScenes", wxListCtrl );
	m_lockedTags = XRCCTRL( *this, "lockedTags", wxListBox );

	// set the columns at the 'm_scheduledScenes' control
	m_scheduledScenes->InsertColumn( 0, wxT( "Scene" ) );
	m_scheduledScenes->SetColumnWidth( 0, 200 );
	m_scheduledScenes->InsertColumn( 1, wxT( "Input" ) );
	m_scheduledScenes->SetColumnWidth( 1, 110 );
	m_scheduledScenes->Layout();

	// set the columns at the 'm_activeScenes' control
	m_activeScenes->InsertColumn( 0, wxT( "Scene" ) );
	m_activeScenes->SetColumnWidth( 0, 200 );
	m_activeScenes->InsertColumn( 1, wxT( "Input" ) );
	m_activeScenes->SetColumnWidth( 1, 110 );
	m_activeScenes->Layout();

	// set the columns at the 'm_completedScenes' control
	m_completedScenes->InsertColumn( 0, wxT( "Time" ) );
	m_completedScenes->SetColumnWidth( 0, 110 );
	m_completedScenes->InsertColumn( 1, wxT( "Scene" ) );
	m_completedScenes->SetColumnWidth( 1, 200 );
	m_completedScenes->InsertColumn( 2, wxT( "Input" ) );
	m_completedScenes->SetColumnWidth( 2, 110 );
	m_completedScenes->InsertColumn( 3, wxT( "Output" ) );
	m_completedScenes->SetColumnWidth( 3, 110 );
	m_completedScenes->InsertColumn( 4, wxT( "Termination reason" ) );
	m_completedScenes->SetColumnWidth( 4, 110 );
	m_completedScenes->InsertColumn( 5, wxT( "Was played?" ) );
	m_completedScenes->SetColumnWidth( 5, 110 );
	m_completedScenes->InsertColumn( 6, wxT( "SceneTime" ) );
	m_completedScenes->SetColumnWidth( 6, 110 );
	m_completedScenes->InsertColumn( 7, wxT( "Last section name" ) );
	m_completedScenes->SetColumnWidth( 7, 1000 );
	m_completedScenes->Layout();

	// attach listener to the scenes manager
	m_scenesMgr = host.GetQuestsSystem()->GetStoriesManager();
	if ( m_scenesMgr )
	{
		m_scenesMgr->AttachListener( *this );
	}
}

void CQuestScenesDebugger::OnDetach()
{
	m_host = NULL;

	if ( m_scenesMgr )
	{
		m_scenesMgr->DetachListener( *this );
		m_scenesMgr = NULL;
	}
	m_activeScenes = NULL;
	m_scheduledScenes = NULL;
}

wxPanel* CQuestScenesDebugger::GetPanel()
{
	return this;
}

void CQuestScenesDebugger::Notify( CQuestScenesManager& mgr )
{
	if ( !m_scheduledScenes || !m_activeScenes || !m_lockedTags )
	{
		return;
	}

	m_scheduledScenes->DeleteAllItems();
	m_activeScenes->DeleteAllItems();
	m_lockedTags->Clear();

	// refresh info about scheduled scenes
	const TDynArray< CQuestScenePlayer* >* scenes = &mgr.GetScheduledScenes();
	Uint32 count = scenes->Size();
	for ( Uint32 i = 0; i < count; ++i )
	{
		CQuestScenePlayer* scene = (*scenes)[ i ];

		if ( scene->IsLoaded() )
		{
			Int32 insertPos = m_scheduledScenes->InsertItem( i, scene->GetSceneName().AsChar() );
			m_scheduledScenes->SetItem( insertPos, 1, scene->GetInputName().AsChar() );
		}
	}

	// refresh info about running scenes
	scenes = &mgr.GetActiveScenes();
	count = scenes->Size();
	for ( Uint32 i = 0; i < count; ++i )
	{
		CQuestScenePlayer* scene = (*scenes)[ i ];

		if ( scene->IsLoaded() )
		{
			Int32 insertPos = m_activeScenes->InsertItem( i, scene->GetSceneName().AsChar() );
			m_activeScenes->SetItem( insertPos, 1, scene->GetInputName().AsChar() );
		}
	}

	// refresh info about locked tags
	const TDynArray< CName >& tags = mgr.GetActiveTags().GetTags();
	count = tags.Size();
	for ( Uint32 i = 0; i < count; ++i )
	{
		m_lockedTags->Append( tags[ i ].AsString().AsChar() );
	}
}

void CQuestScenesDebugger::NotifySceneCompleted( CQuestScenePlayer& player, SceneCompletionReason reason )
{ 
	if ( !player.IsLoaded() )
		return;

	// set info about the completed scene
	String sceneName = player.GetSceneName();
	String sceneInput = player.GetInputName();
	String sceneOutput = player.GetOutput().AsString();

	// set info about termination reason
	EMsgType msgType;
	String reasonStr;
	switch ( reason )
	{
	case SCR_NONE:
		{
			reasonStr = TXT( "SCENE NOT ENDED YET" );
			msgType = QSD_INFO;
			break;
		}

	case SCR_SCENE_ENDED:
		{
			reasonStr = TXT( "SCENE ENDED" );
			msgType = QSD_INFO;
			break;
		}

	case SCR_KILLED_BY_QUEST:
		{
			reasonStr = TXT( "SCENE WAS KILLED BY QUEST" );
			msgType = QSD_INFO;
			break;
		}

	case SCR_WORLD_UNLOADING:
		{
			reasonStr = TXT( "SCENE KILLED BY UNLOADING WORLD" );
			msgType = QSD_WARNING;
			break;
		}

	case SCR_ACTOR_KILLED:
		{
			reasonStr = TXT( "SCENE ACTOR HAS BEEN KILLED" );
			msgType = QSD_WARNING;
			break;
		}

	case SCR_ACTOR_DESPAWN:
		{
			reasonStr = TXT( "SCENE ACTOR HAS BEEN DESPAWNED" );
			msgType = QSD_WARNING;
			break;
		}

	case SCR_ACTOR_STOLEN:
		{
			reasonStr = TXT( "SCENE ACTOR HAS BEEN STOLEN" );
			msgType = QSD_WARNING;
			break;
		}

	case SCR_ACTOR_REACTING:
		{
			reasonStr = TXT( "SCENE STOPPED BY ACTOR REACTION" );
			msgType = QSD_WARNING;
			break;
		}

	case SCR_ACTOR_IN_COMBAT:
		{
			reasonStr = TXT( "SCENE STOPPED BY ACTOR COMBAT" );
			msgType = QSD_WARNING;
			break;
		}

	case SCR_TIMEDOUT:
		{
			reasonStr = TXT( "TIMEDOUT ON START ATTEMPT" );
			msgType = QSD_ERROR;
			break;
		}

	case SCR_DIED:
		{
			reasonStr = TXT( "DIED DURING EXECUTION" );
			msgType = QSD_ERROR;
			break;
		}

	case SCR_INTERRUPTED:
		{
			reasonStr = TXT( "INTERRUPTED" );
			msgType = QSD_WARNING;
			break;
		}

	case SCR_INVALID_SCENE:
		{
			reasonStr = TXT( "INVALID SCENE" );
			msgType = QSD_ERROR;
			break;
		}

	case SCR_PLAYER_DESTROYED:
		{
			reasonStr = TXT( "SCENE BLOCK DEACTIVATED" );
			msgType = QSD_INFO;
			break;
		}

	case SCR_STOPPED:
		{
			reasonStr = TXT( "SCENE STOPPED" );
			msgType = QSD_INFO;
			break;
		}

	case SCR_SYSTEM_RESET:
		{
			reasonStr = TXT( "SYSTEM RESET" );
			msgType = QSD_INFO;
			break;
		}

	case SCR_CANNOT_START:
		{
			reasonStr = TXT( "SCENE STARTING CONDITIONS WERE NOT MET" );
			msgType = QSD_ERROR;
			break;
		}

	default:
		{
			ASSERT( false, TXT( "Invalid termination reason" ) );
			msgType = QSD_ERROR;
			break;
		}
	}

	// add info whether the scene was played at all or not
	String wasPlayedStr;
	if ( player.IsPlaying() || player.IsOver() )
	{
		wasPlayedStr = TXT( "PLAYED" );
	}
	else
	{
		wasPlayedStr = TXT( "NOT PLAYED" );
	}

	// refresh info about completed scenes
	Int32 insertPos = m_completedScenes->InsertItem( 0, ToString( (Float)GGame->GetEngineTime() ).AsChar() );
	m_completedScenes->SetItem( insertPos, 1, sceneName.AsChar() );
	m_completedScenes->SetItem( insertPos, 2, sceneInput.AsChar() );
	m_completedScenes->SetItem( insertPos, 3, sceneOutput.AsChar() );
	m_completedScenes->SetItem( insertPos, 4, reasonStr.AsChar() );
	m_completedScenes->SetItem( insertPos, 5, wasPlayedStr.AsChar() );
	m_completedScenes->SetItem( insertPos, 6, ToString( player.GetTotalTime() ).AsChar() );
	m_completedScenes->SetItem( insertPos, 7, player.GetLastSectionName().AsChar() );

	// set colour
	switch ( msgType )
	{
	case QSD_INFO:
		{
			m_completedScenes->SetItemBackgroundColour( insertPos, *wxWHITE );
			break;
		}

	case QSD_WARNING:
		{
			m_completedScenes->SetItemBackgroundColour( insertPos, wxColour( 255, 165, 48 ) );
			break;
		}

	case QSD_ERROR:
		{
			m_completedScenes->SetItemBackgroundColour( insertPos, *wxRED );
			m_host->SetToolErrorIndicator( *this );
			break;
		}
	}
}

void CQuestScenesDebugger::OnClose( wxCloseEvent& event )
{
	Hide();
}
