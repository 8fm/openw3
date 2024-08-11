#include "build.h"
#include "questThreadsDebugger.h"
#include "questEditor.h"
#include "../../common/core/scriptThread.h"
#include "../../common/game/questGraphBlock.h"
#include "../../common/game/questScriptBlock.h"


CQuestThreadsDebugger::CQuestThreadsDebugger()
	: m_threadsInfo( NULL )
	, m_system( NULL )
	, m_updateTimer( NULL)
{
}

CQuestThreadsDebugger::~CQuestThreadsDebugger()
{
	OnDetach();
}

void CQuestThreadsDebugger::OnAttach( CEdQuestEditor& host, wxWindow* parent )
{
	// initialize UI
	Create( parent );

	wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );

	m_threadsInfo = new wxListCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT );
	sizer->Add( m_threadsInfo, 1, wxALL|wxEXPAND, 5 );

	m_threadsInfo->InsertColumn( 0, wxT( "Time" ) );
	m_threadsInfo->SetColumnWidth( 0, 110 );
	m_threadsInfo->InsertColumn( 1, wxT( "Name" ) );
	m_threadsInfo->SetColumnWidth( 1, 200 );
	m_threadsInfo->InsertColumn( 2, wxT( "Total exec. time" ) );
	m_threadsInfo->SetColumnWidth( 2, 110 );
	m_threadsInfo->InsertColumn( 2, wxT( "Curr. latent time" ) );
	m_threadsInfo->SetColumnWidth( 2, 110 );
	m_threadsInfo->Layout();


	SetSizer( sizer );
	Layout();
	sizer->Fit( m_threadsInfo );

	// attach to the quests system
	if ( m_system != NULL )
	{
		m_system->DetachListener( *this );
	}

	m_system = host.GetQuestsSystem();
	m_system->AttachListener( *this );

	// start the update timer
	m_updateTimer = new CEdTimer( this, wxID_ANY );
	Connect( m_updateTimer->GetId(), wxEVT_TIMER, wxTimerEventHandler( CQuestThreadsDebugger::OnTimer ) );
	m_updateTimer->Start( 500 );
}

void CQuestThreadsDebugger::OnDetach()
{
	// delete all data items
	Int32 count = m_threadsInfo->GetItemCount();
	for ( Int32 i = 0; i < count; ++i )
	{
		ItemData* data = reinterpret_cast< ItemData* >( m_threadsInfo->GetItemData( i ) );
		delete data;
	}
	m_threadsInfo->DeleteAllItems();

	if ( m_updateTimer )
	{
		m_updateTimer->Stop();
		delete m_updateTimer;
		m_updateTimer = NULL;
	}

	if ( m_system != NULL )
	{
		m_system->DetachListener( *this );
		m_system = NULL;
	}
}

void CQuestThreadsDebugger::OnThreadPaused( CQuestThread* thread, bool paused )
{
	// not interested in this notification
}

void CQuestThreadsDebugger::OnAddThread( CQuestThread* parentThread, CQuestThread* thread )
{
	// not interested in this notification
}

void CQuestThreadsDebugger::OnRemoveThread( CQuestThread* parentThread, CQuestThread* thread )
{
	// not interested in this notification
}

void CQuestThreadsDebugger::OnAddBlock( CQuestThread* thread, const CQuestGraphBlock* block )
{
	if ( !block->IsA< CQuestScriptBlock >() )
	{
		return;
	}

	const CQuestScriptBlock* scriptBlock = dynamic_cast< const CQuestScriptBlock* >( block );

	// set thread's description
	Int32 insertPos = m_threadsInfo->InsertItem( 0, ToString( (Float)GGame->GetEngineTime() ).AsChar() );
	m_threadsInfo->SetItem( insertPos, 1, scriptBlock->GetCaption().AsChar() );

	// add data to the item that will allow to safely query for the script thread the block is running
	ItemData* data = new ItemData( *thread, *scriptBlock );
	m_threadsInfo->SetItemPtrData( insertPos, reinterpret_cast< wxUIntPtr >( data ) );
}

void CQuestThreadsDebugger::OnRemoveBlock( CQuestThread* thread, const CQuestGraphBlock* block )
{
	if ( !block->IsA< CQuestScriptBlock >() )
	{
		return;
	}

	// locate an item corresponding to the removed block
	const CQuestScriptBlock* scriptBlock = dynamic_cast< const CQuestScriptBlock* >( block );
	Int32 itemIdx = Find( scriptBlock );
	if ( itemIdx < 0 )
	{
		return;
	}

	// dispose of the attached data
	ItemData* data = reinterpret_cast< ItemData* >( m_threadsInfo->GetItemData( itemIdx ) );
	delete data;
	
	// remove the item from the list
	m_threadsInfo->DeleteItem( itemIdx );
}

void CQuestThreadsDebugger::OnBlockInputActivated( CQuestThread* thread, const CQuestGraphBlock* block )
{
	// not interested in this notification
}

void CQuestThreadsDebugger::OnQuestStarted( CQuestThread* thread, CQuest& quest )
{
	// not interested in this notification
}

void CQuestThreadsDebugger::OnQuestStopped( CQuestThread* thread )
{
	// not interested in this notification
}

void CQuestThreadsDebugger::OnSystemPaused( bool paused )
{
	// not interested in this notification
}

void CQuestThreadsDebugger::OnClose( wxCloseEvent& event )
{
	Hide();
}

void CQuestThreadsDebugger::OnTimer( wxTimerEvent& event )
{
	// update all items
	Int32 count = m_threadsInfo->GetItemCount();
	for ( Int32 i = 0; i < count; ++i )
	{
		ItemData* data = reinterpret_cast< ItemData* >( m_threadsInfo->GetItemData( i ) );
		const CScriptThread* const scriptThread = data->GetThread();
		if ( !scriptThread )
		{
			continue;
		}

		// set thread's description
		m_threadsInfo->SetItem( i, 2, String::Printf( TXT( "%f" ), scriptThread->GetTotalTime() ).AsChar() );
		m_threadsInfo->SetItem( i, 3, String::Printf( TXT( "%f" ), scriptThread->GetCurrentLatentTime() ).AsChar() );
	}
}

Int32 CQuestThreadsDebugger::Find( const CQuestScriptBlock* scriptBlock ) const
{
	Int32 count = m_threadsInfo->GetItemCount();
	for ( Int32 i = 0; i < count; ++i )
	{
		ItemData* data = reinterpret_cast< ItemData* >( m_threadsInfo->GetItemData( i ) );

		if ( &data->block == scriptBlock )
		{
			return i;
		}
	}
	return -1;
}

const CScriptThread* const CQuestThreadsDebugger::ItemData::GetThread() const
{
	const CScriptThread* const scriptThread = block.GetScriptThread( questThread.GetInstanceData() );
	return scriptThread;
}
