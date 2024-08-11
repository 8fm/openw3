/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "questsDebugger.h"
#include "questEditor.h"
#include "../../common/game/questGraphBlock.h"
#include "../../common/game/questGraphSocket.h"
#include "../../common/game/quest.h"
#include "../../common/game/questsSystem.h"
#include "../../common/game/questInteractionDialogBlock.h"
#include "../../common/game/questExternalScenePlayer.h"
#include "../../common/engine/graphConnection.h"


#define wxID_QUESTDEBUGGER_STOPBLOCK						4500
#define wxID_QUESTDEBUGGER_STOPQUEST						4501
#define wxID_QUESTDEBUGGER_RESTARTQUEST						4502
#define wxID_QUESTDEBUGGER_TOGGLEBREAKPOINT					4503
#define wxID_QUESTDEBUGGER_PAUSESYSTEM						4504

///////////////////////////////////////////////////////////////////////////////

namespace // anonymous
{

	// ------------------------------------------------------------------------

	class IQuestsDebuggerCommand : public wxObject
	{
	public:
		virtual ~IQuestsDebuggerCommand() {}
		
		virtual void Execute() {}
	};

	// ------------------------------------------------------------------------

	class CStopBlockCommand : public IQuestsDebuggerCommand
	{
	private:
		wxTreeCtrl*			m_activeThreadsTree;
		wxTreeItemId		m_itemClicked;

	public:
		CStopBlockCommand( wxTreeCtrl* activeThreadsTree, wxTreeItemId& itemClicked )
			: m_activeThreadsTree( activeThreadsTree )
			, m_itemClicked( itemClicked )
		{ }

		void Execute()
		{
			if ( m_itemClicked.IsOk() == false )
			{
				return;
			}

			wxTreeItemId parentItem = m_activeThreadsTree->GetItemParent( m_itemClicked );
			if ( parentItem.IsOk() == false )
			{
				return;
			}

			wxTreeItemData* itemData = m_activeThreadsTree->GetItemData( m_itemClicked );
			wxTreeItemData* parentItemData = m_activeThreadsTree->GetItemData( parentItem );

			SBlockData* blockItem = dynamic_cast< SBlockData* >( itemData );
			SThreadData* threadItem = dynamic_cast< SThreadData* >( parentItemData );
			if ( !threadItem || !blockItem )
			{
				return;
			}

			threadItem->data->DeactivateBlock( blockItem->data );
		}

	};

	// ------------------------------------------------------------------------

	class CStopQuestCommand : public IQuestsDebuggerCommand
	{
	private:
		CQuestsSystem*		m_system;
		wxTreeCtrl*			m_activeThreadsTree;
		wxTreeItemId		m_itemClicked;

	public:
		CStopQuestCommand( CQuestsSystem* system,
							wxTreeCtrl* activeThreadsTree, 
							wxTreeItemId& itemClicked )
			: m_system( system )
			, m_activeThreadsTree( activeThreadsTree )
			, m_itemClicked( itemClicked )
		{ }

		void Execute()
		{
			if ( m_itemClicked.IsOk() == false )
			{
				return;
			}

			wxTreeItemData* itemData = m_activeThreadsTree->GetItemData( m_itemClicked );
			SQuestData* questItem = dynamic_cast< SQuestData* >( itemData );
			if ( !questItem )
			{
				return;
			}

			m_system->Stop( questItem->data );
		}
	};

	// ------------------------------------------------------------------------

	class CPauseQuestSystem : public IQuestsDebuggerCommand
	{
	private:
		CQuestsSystem*		m_system;
		Bool				m_pause;

	public:
		CPauseQuestSystem( CQuestsSystem* system, Bool pause )
			: m_system( system )
			, m_pause( pause )
		{ }

		void Execute()
		{
			m_system->Pause( m_pause );
		}
	};

	// ------------------------------------------------------------------------

	class CToggleBreakpointCommand : public IToolCommandWrapper
	{
	private:
		CQuestsDebugger& m_parent;

	public:
		CToggleBreakpointCommand( CQuestsDebugger& parent ) 
			: m_parent( parent ) 
		{}

		void OnBlock( CQuestGraphBlock* block )
		{
			m_parent.ToggleBreakpoint( block );
		}

		void OnSocket( CQuestGraphBlock* block, const CQuestGraphSocket* socket )  {}
	};

	// ------------------------------------------------------------------------

	class CToolDeactivateBlockCommand : public IToolCommandWrapper
	{
	private:
		CQuestsDebugger& m_parent;

	public:
		CToolDeactivateBlockCommand( CQuestsDebugger& parent ) 
			: m_parent( parent ) 
		{}

		void OnBlock( CQuestGraphBlock* block )
		{	
			if ( !m_parent.GetObservedQuestThread() || !block )
			{
				return;
			}
			m_parent.GetObservedQuestThread()->DeactivateBlock( block );
		}

		void OnSocket( CQuestGraphBlock* block, const CQuestGraphSocket* socket )  {}
	};

	// ------------------------------------------------------------------------

	class CContinueFromCommand : public IToolCommandWrapper
	{
	private:
		CQuestsDebugger& m_parent;

	public:
		CContinueFromCommand( CQuestsDebugger& parent ) 
			: m_parent( parent ) 
		{}

		void OnSocket( CQuestGraphBlock* block, const CQuestGraphSocket* socket )  
		{
			m_parent.ContinueFrom( block, socket );
		}

		void OnBlock( CQuestGraphBlock* block ) {}
	};

	class CStartInteractionDialogCommand : public IToolCommandWrapper
	{
	private:
		CQuestsDebugger& m_parent;
	public:
		CStartInteractionDialogCommand( CQuestsDebugger& parent ) 
			: m_parent( parent )
		{}
		void OnSocket( CQuestGraphBlock* block, const CQuestGraphSocket* socket ){}
		void OnBlock( CQuestGraphBlock* block ) 
		{
			const CQuestInteractionDialogBlock* intDial = Cast< CQuestInteractionDialogBlock >( block );
			if ( intDial )
			{
				CQuestsSystem* questsSystem = GCommonGame->GetSystem< CQuestsSystem >();
				const TDynArray< CName >& tags = intDial->GetActorTags().GetTags();

				Bool started = false;
				const Uint32 count = tags.Size();
				for ( Uint32 i = 0; i < count; ++i )
				{
					started = questsSystem->GetInteractionDialogPlayer()->StartDialog( tags[ i ] );
					if ( started )
					{
						break;
					}
				}	
			}		
		}
	};


} // anonymous

///////////////////////////////////////////////////////////////////////////////

// Event table
BEGIN_EVENT_TABLE( CQuestsDebugger, wxPanel )
END_EVENT_TABLE()

///////////////////////////////////////////////////////////////////////////////

CQuestsDebugger::CQuestsDebugger()
	: m_host( NULL )
	, m_system( NULL )
	, m_debugImages( 16, 16, true, QDG_MAX )
	, m_observedQuestThread( NULL )
	, m_observedGraph( NULL )
	, m_gameEnding( false )
{
	SEvents::GetInstance().RegisterListener( CNAME( GameEndRequested ), this );
	SEvents::GetInstance().RegisterListener( CNAME( GameEnded ), this );
}

CQuestsDebugger::~CQuestsDebugger()
{
	SEvents::GetInstance().UnregisterListener( this );
	OnDetach();
}

void CQuestsDebugger::OnAttach( CEdQuestEditor& host, wxWindow* parent )
{
	m_host = &host;

	wxXmlResource::Get()->LoadPanel( this, parent, wxT("QuestsDebugger") );
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( CQuestsDebugger::OnClose ), 0, this );

	// Load icons
	m_icons[ QDG_ICON_BLOCK ] = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_BDI_ACTION") );
	m_icons[ QDG_ICON_THREAD ] = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_TOOL") );
	m_icons[ QDG_ICON_QUEST ] = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_WORLD16") );
	m_icons[ QDG_ICON_PAUSED ] = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_CONTROL_PAUSE") );
	m_icons[ QDG_ICON_RUNNING ] = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_CONTROL_PLAY") );

	Int32 iconIdx;
	for ( Uint32 i = 0; i < QDG_MAX; ++i )
	{
		iconIdx = m_debugImages.Add( m_icons[ i ] );
	}

	// initialize tree widget
	m_activeThreadsTree = XRCCTRL( *this, "activeThreadsTree", wxTreeCtrl );
	m_activeThreadsTree->SetImageList( &m_debugImages );
	m_activeThreadsTree->Connect( wxEVT_COMMAND_TREE_ITEM_RIGHT_CLICK, wxTreeEventHandler( CQuestsDebugger::ShowContextMenu ), NULL, this );
	m_activeThreadsTree->Connect( wxEVT_COMMAND_TREE_ITEM_ACTIVATED, wxTreeEventHandler( CQuestsDebugger::ShowInEditor ), NULL, this );

	m_rootItem = m_activeThreadsTree->AddRoot( wxT( "Active quests" ) );

	// attach to the quests system
	if ( m_system != NULL )
	{
		m_system->DetachListener( *this );
	}

	m_system = host.GetQuestsSystem();
	m_system->AttachListener( *this );

	m_host->SetDebugInfo( *this );
}

void CQuestsDebugger::OnDetach()
{
	m_activeThreadsTree = NULL;
	m_host = NULL;
	m_observedQuestThread = NULL;
	m_observedGraph = NULL;

	if ( m_system != NULL )
	{
		m_system->DetachListener( *this );
		m_system = NULL;
	}
}

void CQuestsDebugger::OnCreateBlockContextMenu( TDynArray< SToolMenu >& subMenus, const CQuestGraphBlock* atBlock )
{
	if ( !m_observedQuestThread || !m_observedGraph || !atBlock )
		return;

	const CQuestInteractionDialogBlock* intDial = Cast< CQuestInteractionDialogBlock >( atBlock );
	Bool isGraphActive =  IsGraphInActive( m_observedGraph );
	Bool isBlockActive =  m_observedQuestThread->IsBlockActive( atBlock );
	
	if( m_observedQuestThread->IsBlockActive( atBlock ) && intDial )
	{
		subMenus.PushBack( SToolMenu( TXT( "Start interaction dialog"), new CStartInteractionDialogCommand( *this ), SToolMenu::AT_BLOCK ) );
	}

	subMenus.PushBack( SToolMenu( TXT( "Toggle breakpoint"), new CToggleBreakpointCommand( *this ), SToolMenu::AT_BLOCK ) );

	if( isBlockActive )
	{
		subMenus.PushBack( SToolMenu( TXT( "Kill signal"), new CToolDeactivateBlockCommand( *this ), SToolMenu::AT_BLOCK ) );
	}

	if ( isGraphActive )
	{
		subMenus.PushBack( SToolMenu( TXT( "Continue from"), new CContinueFromCommand( *this ), SToolMenu::AT_OUT_SOCKET | SToolMenu::AT_IN_SOCKET  ) );
	}
}

void CQuestsDebugger::OnGraphSet( CQuestGraph& graph )
{
	m_observedGraph = &graph;

	// set the observed thread
	static TDynArray< CQuestThread* > runningThreads;
	runningThreads.ClearFast();
	m_system->GetAllRunningThreads( runningThreads );
	for ( TDynArray< CQuestThread* >::const_iterator it = runningThreads.Begin(); it != runningThreads.End(); ++it )
	{
		CQuestThread* thread = *it;
		if ( thread->DoesWorkOnGraph( graph ) )
		{
			m_observedQuestThread = thread;
			break;
		}
	}
}

void CQuestsDebugger::OnClose( wxCloseEvent& event )
{
	Hide();
}

void CQuestsDebugger::ShowContextMenu( wxTreeEvent& event )
{ 
	if ( !m_activeThreadsTree )
		return;

	wxTreeItemId itemClicked = event.GetItem();

	if ( itemClicked == m_rootItem )
	{
		ShowSystemContextMenu();
	}
	else
	{
		wxTreeItemData* data = m_activeThreadsTree->GetItemData( itemClicked );

		SBlockData* blockItem = dynamic_cast< SBlockData* >( data );
		if ( blockItem )
		{
			ShowBlockContextMenu( itemClicked );
			return;
		}

		SQuestData* questItem = dynamic_cast< SQuestData* >( data );
		if ( questItem )
		{
			ShowQuestContextMenu( itemClicked );
			return;
		}
	}
}

void CQuestsDebugger::ShowSystemContextMenu()
{
	if ( !m_host )
		return;

	wxMenu* menu = new wxMenu();

	if ( m_host->GetQuestsSystem()->IsPaused() )
	{
		menu->Append( wxID_QUESTDEBUGGER_PAUSESYSTEM, wxT("Play") );
		menu->Connect( wxID_QUESTDEBUGGER_PAUSESYSTEM, 
			wxEVT_COMMAND_MENU_SELECTED, 
			wxCommandEventHandler( CQuestsDebugger::OnBlockCommand ), 
			new CPauseQuestSystem( m_host->GetQuestsSystem(), false ), 
			this );
	}
	else
	{
		menu->Append( wxID_QUESTDEBUGGER_PAUSESYSTEM, wxT("Pause") );
		menu->Connect( wxID_QUESTDEBUGGER_PAUSESYSTEM, 
			wxEVT_COMMAND_MENU_SELECTED, 
			wxCommandEventHandler( CQuestsDebugger::OnBlockCommand ), 
			new CPauseQuestSystem( m_host->GetQuestsSystem(), true ), 
			this );
	}

	PopupMenu( menu );
}

void CQuestsDebugger::ShowQuestContextMenu( wxTreeItemId& itemClicked )
{
	if ( !m_system || !m_activeThreadsTree )
		return;

	wxMenu* menu = new wxMenu();

	menu->Append( wxID_QUESTDEBUGGER_STOPQUEST, wxT("Stop") );
	menu->Connect( wxID_QUESTDEBUGGER_STOPQUEST, 
		wxEVT_COMMAND_MENU_SELECTED, 
		wxCommandEventHandler( CQuestsDebugger::OnBlockCommand ), 
		new CStopQuestCommand( m_system, m_activeThreadsTree, itemClicked ), 
		this );

	PopupMenu( menu );
}

void CQuestsDebugger::ShowBlockContextMenu( wxTreeItemId& itemClicked )
{
	if ( !m_activeThreadsTree )
		return;

	wxMenu* menu = new wxMenu();

	menu->Append( wxID_QUESTDEBUGGER_STOPBLOCK, wxT("Stop") );
	menu->Connect( wxID_QUESTDEBUGGER_STOPBLOCK, 
		wxEVT_COMMAND_MENU_SELECTED, 
		wxCommandEventHandler( CQuestsDebugger::OnBlockCommand ), 
		new CStopBlockCommand( m_activeThreadsTree, itemClicked ), 
		this );

	PopupMenu( menu );
}

void CQuestsDebugger::OnBlockCommand( wxCommandEvent& event )
{
	IQuestsDebuggerCommand* command = dynamic_cast< IQuestsDebuggerCommand * >( event.m_callbackUserData );
	if ( command )
	{
		command->Execute();
	}
} 

void CQuestsDebugger::ShowInEditor( wxTreeEvent& event )
{
	if ( !m_activeThreadsTree )
		return;

	wxTreeItemId itemClicked = event.GetItem();
	FocusEditorOnItem( itemClicked );

	// set the new instance of the thread we're observing
	wxTreeItemData* itemData = m_activeThreadsTree->GetItemData( itemClicked );
	SThreadData* threadData = dynamic_cast< SThreadData*>( itemData );
	if ( threadData )
	{
		// we clicked a thread entry
		m_observedQuestThread = threadData->data;
	}
	else if ( dynamic_cast< SBlockData*>( itemData ) )
	{
		// we clicked a block entry - get the pointer to its parent
		wxTreeItemId parentItem = m_activeThreadsTree->GetItemParent( itemClicked );
		itemData = m_activeThreadsTree->GetItemData( parentItem );
		threadData = dynamic_cast< SThreadData*>( itemData );
		ASSERT( threadData, TXT( "A block item can't be a child only to a thread item" ) );

		m_observedQuestThread = threadData->data;
	}
}

void CQuestsDebugger::FocusEditorOnItem( wxTreeItemId item )
{
	if ( !m_activeThreadsTree )
		return;

	wxTreeItemData* data = m_activeThreadsTree->GetItemData( item );

	SBlockData* blockItem = dynamic_cast< SBlockData* >( data );
	if ( !blockItem )
	{
		// only blocks can be homed-in on in the editor
		return;
	}

	// gather a stack of blocks throughout all active quest phases leading
	// to the one we want to see 
	TDynArray< const CQuestGraphBlock*> blocksStack;
	blocksStack.PushBack( blockItem->data );
	CQuest* quest = NULL;
	InstanceBuffer* instanceData = NULL;

	while ( true )
	{
		wxTreeItemId parentItem = m_activeThreadsTree->GetItemParent( item );
		if ( parentItem.IsOk() == false )
		{
			break;
		}

		wxTreeItemData* parentItemData = m_activeThreadsTree->GetItemData( parentItem );

		SQuestData* questItem = dynamic_cast< SQuestData*>( parentItemData );
		if ( questItem )
		{
			quest = &questItem->quest;
			break;
		}
		else
		{
			SThreadData* threadItem = dynamic_cast< SThreadData* >( parentItemData );
			if ( threadItem && threadItem->data->GetParentBlock() )
			{
				blocksStack.PushBack( threadItem->data->GetParentBlock() );
				instanceData = &threadItem->data->GetInstanceData();

				item = parentItem;
			}
			else
			{
				break;
			}
		}
	}

	// assuming we have our stack of blocks, start unrolling it
	// by opening them in the editor one by one - this will
	// make the editor go deeper into the stack of phases
	// and finally end up centered on the block of our desire
	if ( quest && !blocksStack.Empty() )
	{
		m_host->SetQuestPhase( quest );

		const Uint32 count = blocksStack.Size();
		for ( Uint32 i = count; i > 0; --i )
		{
			Bool enterBlock = ( i > 0 );
			m_host->OpenBlock( const_cast< CQuestGraphBlock* >( blocksStack[ i-1 ] ), enterBlock );
		}
	}
}

void CQuestsDebugger::OnQuestStarted( CQuestThread* thread, CQuest& quest )
{
	if ( !thread || !m_activeThreadsTree )
		return;

	Int32 icon = QDG_ICON_QUEST;
	wxTreeItemId newItem = m_activeThreadsTree->AppendItem( m_rootItem, 
		thread->GetName().AsChar(), 
		icon, icon,
		new SQuestData( thread, quest ) );
}

void CQuestsDebugger::OnQuestStopped( CQuestThread* thread )
{
	m_observedQuestThread = NULL;

	if ( !thread || !m_activeThreadsTree )
		return;

	OnRemoveThread( NULL, thread );
}

void CQuestsDebugger::OnSystemPaused( bool paused )
{
	if ( !m_activeThreadsTree )
		return;

	if ( paused )
	{
		m_activeThreadsTree->SetItemImage( m_rootItem, QDG_ICON_PAUSED );
	}
	else
	{
		m_activeThreadsTree->SetItemImage( m_rootItem, QDG_ICON_RUNNING );
	}
}

void CQuestsDebugger::OnThreadPaused( CQuestThread* thread, bool paused )
{
	if ( !m_activeThreadsTree || !thread )
		return;

	wxTreeItemId threadNode = FindNode< SThreadData >( m_rootItem, thread );
	if ( !threadNode.IsOk() )
	{
		return;
	}
	if ( paused )
	{
		m_activeThreadsTree->SetItemImage( threadNode, QDG_ICON_PAUSED );
	}
	else
	{
		m_activeThreadsTree->SetItemImage( threadNode, QDG_ICON_THREAD );
	}
}

void CQuestsDebugger::OnAddThread( CQuestThread* parentThread, CQuestThread* thread )
{
	if ( !m_activeThreadsTree || !thread || !parentThread )
		return;

	wxTreeItemId parentNode = FindNode< SThreadData >( m_rootItem, parentThread );
	if ( parentNode.IsOk() == false )
	{
		return;
	}
	
	// add a new item to the tree
	Int32 icon = QDG_ICON_THREAD;
	if ( thread->IsPaused() )
	{
		icon = QDG_ICON_PAUSED;
	}
	
	wxTreeItemId newItem = m_activeThreadsTree->AppendItem( parentNode, 
		thread->GetName().AsChar(), 
		icon, icon,
		new SThreadData( thread ) );

	// automatically expand the tree
	m_activeThreadsTree->ExpandAll();

	// refresh the data in the editor that visualizes the active blocks
	RefreshVisualiser();
}

void CQuestsDebugger::OnRemoveThread( CQuestThread* parentThread, CQuestThread* thread )
{
	if ( !m_activeThreadsTree || !thread || !parentThread )
		return;

	if ( m_gameEnding )
	{
		m_threadsToRemove.PushBackUnique( thread );
		return;
	}

	wxTreeItemId node = FindNode< SThreadData >( m_rootItem, thread );
	if ( node.IsOk() == true )
	{
		m_activeThreadsTree->Delete( node );
	}
	
	// automatically expand the tree
	m_activeThreadsTree->ExpandAll();

	// refresh the data in the editor that visualizes the active blocks
	RefreshVisualiser();

	// if that was the thread we were observing - discard it
	if ( m_observedQuestThread == thread )
	{
		m_observedQuestThread = NULL;
	}
}

void CQuestsDebugger::OnAddBlock( CQuestThread* thread, const CQuestGraphBlock* block )
{
	if ( !m_activeThreadsTree || !thread || !block )
		return;

	wxTreeItemId parentNode = FindNode< SThreadData >( m_rootItem, thread );
	if ( parentNode.IsOk() == false )
	{
		return;
	}

	wxString name = CreateBlockName( thread, block );

	Int32 icon = QDG_ICON_BLOCK;
	wxTreeItemId newItem = m_activeThreadsTree->AppendItem( parentNode, 
		name, 
		icon, icon,
		new SBlockData( block ) );

	ExecuteBreakpoints( block );
	
	// automatically expand the tree
	m_activeThreadsTree->ExpandAll();

	// refresh the data in the editor that visualizes the active blocks
	RefreshVisualiser();

	// focus on the activated block, providing it belongs to the currently observed thread
	if ( thread == m_observedQuestThread )
	{
		FocusEditorOnItem( newItem );
	}
}

void CQuestsDebugger::OnRemoveBlock( CQuestThread* thread, const CQuestGraphBlock* block )
{
	if ( !m_activeThreadsTree || !thread || !block )
		return;

	if ( m_gameEnding )
	{
		m_blocksToRemove.PushBackUnique( block );
		return;
	}

	wxTreeItemId node = FindNode< SBlockData >( m_rootItem, block );
	if ( node.IsOk() == true )
	{
		m_activeThreadsTree->Delete( node );
	}

	// automatically expand the tree
	m_activeThreadsTree->ExpandAll();

	// refresh the data in the editor that visualizes the active blocks
	RefreshVisualiser();
}

void CQuestsDebugger::OnBlockInputActivated( CQuestThread* thread, const CQuestGraphBlock* block )
{
	if ( !m_activeThreadsTree || !thread || !block )
		return;

	wxTreeItemId node = FindNode< SBlockData >( m_rootItem, block );
	if ( node.IsOk() == true )
	{
		wxString name = CreateBlockName( thread, block );
		m_activeThreadsTree->SetItemText( node, name );
	}

	RefreshVisualiser();
}

void CQuestsDebugger::RefreshVisualiser()
{
	if ( m_host )
	{
		m_host->Repaint();
	}
}

wxString CQuestsDebugger::CreateBlockName( CQuestThread* thread, const CQuestGraphBlock* block ) const
{
	if ( !thread || !block )
		return wxT( "" );

	wxString name( block->GetCaption().AsChar() );

	// append the names of active inputs
	const TDynArray< CName >* activeInputs = thread->GetActivatedInputs( block );
	if ( activeInputs != NULL )
	{
		name += wxT( "[" );
		const Uint32 count = activeInputs->Size();
		for ( Uint32 i = 0; i < count; ++i )
		{
			if ( i > 0 )
			{
				name += wxT( ", " );
			}
			name += (*activeInputs)[ i ].AsString().AsChar();
		}
		name += wxT( "]" );
	}

	return name;
}

void CQuestsDebugger::ExecuteBreakpoints( const CQuestGraphBlock* block )
{
	if ( !m_host )
		return;

	const Bool shouldBreak = ( Find( m_breakpoints.Begin(), m_breakpoints.End(), block ) != m_breakpoints.End() );
	if ( shouldBreak )
	{
		m_host->GetQuestsSystem()->Pause( true );
	}
}

void CQuestsDebugger::ToggleBreakpoint( const CQuestGraphBlock* block )
{
	if ( m_breakpoints.Remove( block ) == false )
	{
		m_breakpoints.PushBackUnique( block );
	}
}

void CQuestsDebugger::ContinueFrom( const CQuestGraphBlock* block, const CQuestGraphSocket* socket )
{
	ASSERT( m_observedQuestThread, TXT( "This option should not be invoked without an active thread" ) );
	if ( !m_observedQuestThread || !block || !socket )
		return;

	InstanceBuffer& data = m_observedQuestThread->GetInstanceData();

	if( socket->GetDirection() == LSD_Input )
	{
		m_observedQuestThread->ActivateBlock( *SafeCast< CQuestGraphBlock >( socket->GetBlock() ), socket->GetName() );
	}
	else if( socket->GetDirection() == LSD_Output )
	{
		// get the blocks connected to the specified socket
		TDynArray< SBlockDesc > blocksToActivate;
		const TDynArray< CGraphConnection* >& connectedInputs = socket->GetConnections();
		const Uint32 count = connectedInputs.Size();
		for ( Uint32 i = 0; i < count; ++i )
		{
			if ( connectedInputs[ i ]->IsActive() )
			{
				CGraphSocket* destSocket = connectedInputs[ i ]->GetDestination();
				CQuestGraphBlock* nextBlock = SafeCast< CQuestGraphBlock >( destSocket->GetBlock() );
				blocksToActivate.PushBack( SBlockDesc( nextBlock, destSocket->GetName() ) );
			}
		}

		// activate the blocks
		for ( TDynArray< SBlockDesc >::iterator it = blocksToActivate.Begin(); it != blocksToActivate.End(); ++it )
		{
			m_observedQuestThread->ActivateBlock( *it->block, it->inputName );
		}
	}
}

Bool CQuestsDebugger::IsBreakpointToggled( CQuestGraphBlock* block ) const
{
	return ( Find( m_breakpoints.Begin(), m_breakpoints.End(), block ) != m_breakpoints.End() );
}

Bool CQuestsDebugger::IsBlockActive( CQuestGraphBlock* block ) const
{
	if ( !m_observedQuestThread )
	{
		return false;
	}

	return m_observedQuestThread->IsBlockActive( block );
}

Bool CQuestsDebugger::IsBlockVisited( CQuestGraphBlock* block ) const
{
	if ( !m_observedQuestThread )
	{
		return false;
	}
	return m_observedQuestThread->IsBlockVisited( block );
}

Bool CQuestsDebugger::IsGraphInActive( CQuestGraph* graph ) const
{
	return ( m_observedQuestThread && graph && m_observedQuestThread->DoesWorkOnGraph( *graph ) );
}

void CQuestsDebugger::DispatchEditorEvent(const CName& name, IEdEventData* data)
{
	if ( name == CNAME( GameEndRequested ) )
	{
		m_gameEnding = true;
	}
	else if ( name == CNAME( GameEnded ) )
	{
		for ( const CQuestGraphBlock* block : m_blocksToRemove ) 
		{
			wxTreeItemId node = FindNode< SBlockData >( m_rootItem, block );
			if ( node.IsOk() == true )
			{
				m_activeThreadsTree->Delete( node );
			}
		}
		m_blocksToRemove.Clear();

		for ( CQuestThread* thread : m_threadsToRemove )
		{
			wxTreeItemId node = FindNode< SThreadData >( m_rootItem, thread );
			if ( node.IsOk() == true )
			{
				m_activeThreadsTree->Delete( node );
			}
		}
		m_observedQuestThread = NULL;

		// automatically expand the tree
		m_activeThreadsTree->ExpandAll();

		// refresh the data in the editor that visualizes the active blocks
		RefreshVisualiser();
		m_gameEnding = false;
	}
}
