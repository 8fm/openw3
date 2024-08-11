#include "build.h"
#include "questExecutionLog.h"
#include "../../common/game/quest.h"
#include "../../common/game/questGraphBlock.h"
#include "../../common/game/questGraphSocket.h"
#include "questEditor.h"


CQuestExecutionLog::CQuestExecutionLog()
: m_log( NULL )
, m_system( NULL )
, m_host( NULL )
{
	
}

CQuestExecutionLog::~CQuestExecutionLog()
{
	OnDetach();
}

void CQuestExecutionLog::OnAttach( CEdQuestEditor& host, wxWindow* parent )
{
	m_host = &host;

	// initialize UI
	Create( parent );

	wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );

	m_log = new wxListCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT );
	sizer->Add( m_log, 1, wxALL|wxEXPAND, 5 );

	m_log->InsertColumn( 0, wxT( "Description" ) );
	m_log->SetColumnWidth( 0, 500 );
	m_log->InsertColumn( 1, wxT( "Type" ) );
	m_log->SetColumnWidth( 1, 110 );
	m_log->InsertColumn( 2, wxT( "Time" ) );
	m_log->SetColumnWidth( 2, 110 );
	m_log->Layout();

	// search field
	{
		wxBoxSizer* searchFieldSizer = new wxBoxSizer( wxHORIZONTAL );

		wxBitmap bitmap = SEdResources::GetInstance().LoadBitmap( TXT("IMG_PB_BROWSE") );
		m_findButton = new wxBitmapButton( this, wxID_ANY, bitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW );
		m_findButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CQuestExecutionLog::OnFind ), NULL, this );
		searchFieldSizer->Add( m_findButton, 0, wxALL, 5 );

		m_searchPhrase = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
		searchFieldSizer->Add( m_searchPhrase, 1, wxALL, 5 );
		searchFieldSizer->Layout();

		sizer->Add( searchFieldSizer, 0, wxEXPAND, 5 );
	}

	sizer->Layout();
	SetSizer( sizer );
	Layout();

	// attach to the quests system
	if ( m_system != NULL )
	{
		m_system->DetachListener( *this );
	}

	m_system = host.GetQuestsSystem();
	m_system->AttachListener( *this );
}

void CQuestExecutionLog::OnDetach()
{
	if ( m_system != NULL )
	{
		m_system->DetachListener( *this );
		m_system = NULL;
	}

	m_host = NULL;
}

void CQuestExecutionLog::OnQuestStarted( CQuestThread* thread, CQuest& quest )
{
	AddEntry( QEL_INFO, TXT( "Quest ") + quest.GetFileName() + TXT( " started" ), NULL );
}

void CQuestExecutionLog::OnQuestStopped( CQuestThread* thread )
{
	AddEntry( QEL_INFO, TXT( "Quest ") + ( thread ? thread->GetName() : TXT( "'NULL'" ) ) + TXT( " stopped" ), NULL );
}

void CQuestExecutionLog::OnSystemPaused( bool paused )
{
	String desc = ( paused ? TXT( "paused" ) : TXT( "resumed" ) );
	AddEntry( QEL_INFO, TXT( "Quest ") + desc, NULL );
}

void CQuestExecutionLog::OnThreadPaused( CQuestThread* thread, bool paused )
{
	String desc = TXT( "[" ) + thread->GetName() + TXT( "]" ) + (paused ? TXT( " thread paused ") : TXT( " thread unpaused "));
	Type type = QEL_INFO;
	AddEntry( type, desc, nullptr );
}

void CQuestExecutionLog::OnAddThread( CQuestThread* parentThread, CQuestThread* thread )
{
	String desc = GetThreadDesc( parentThread, thread ) + TXT( " thread started" );
	Type type = QEL_INFO;
	if ( !thread || !parentThread )
	{
		type = QEL_ERROR;
	}
	AddEntry( type, desc, NULL );
}

void CQuestExecutionLog::OnRemoveThread( CQuestThread* parentThread, CQuestThread* thread )
{
	String desc = GetThreadDesc( parentThread, thread ) + TXT( " thread killed" );
	Type type = QEL_INFO;
	if ( !thread || !parentThread )
	{
		type = QEL_ERROR;
	}
	AddEntry( type, desc, NULL );
}

void CQuestExecutionLog::OnAddBlock( CQuestThread* thread, const CQuestGraphBlock* block )
{
	String desc = GetBlockDesc( thread, block ) + TXT( " started in thread " );
	Type type = QEL_INFO;
	if ( !thread || !block )
	{
		type = QEL_ERROR;
	}
	AddEntry( type, desc, block );
}

void CQuestExecutionLog::OnRemoveBlock( CQuestThread* thread, const CQuestGraphBlock* block )
{
	// check what outputs were activated
	InstanceBuffer& data = thread->GetInstanceData();
	if ( block->WasOutputActivated( data ) )
	{
		// block's output was activated

		CName outputName = block->GetActivatedOutputName( data );
		CQuestGraphSocket* socket = block->FindSocket< CQuestGraphSocket >( outputName );

		if ( !socket )
		{
			// non-existing output was activated!!!
			AddEntry( QEL_ERROR, GetBlockDesc( thread, block ) + TXT( " exited using an invalid output" ), block );
		}
		else if ( socket->GetDirection() != LSD_Output )
		{
			// this is not an output socket
			AddEntry( QEL_ERROR, GetBlockDesc( thread, block ) + TXT( " exited with a non-output socket [" ) + outputName.AsString() + TXT( "]" ), block );
		}
		else if ( !block->IsBlockEnabled( data ) )
		{
			// the block is inactive - this should NEVER happen
			AddEntry( QEL_ERROR, GetBlockDesc( thread, block ) + TXT( " has been deactivated while exiting" ), block );
		}
		else
		{
			// everything seems ok, just make sure the output the block activated
			// is connected to something
			if ( socket->GetConnections().Empty() )
			{
				AddEntry( QEL_WARNING, GetBlockDesc( thread, block ) + TXT( " exited with [" ) + outputName.AsString() + TXT( "], which is NOT CONNECTED to anything" ), block );
			}
			else
			{
				AddEntry( QEL_INFO, GetBlockDesc( thread, block ) + TXT( " correctly exited with [" ) + outputName.AsString() + TXT( "]" ), block );
			}
		}
	}
	else
	{
		// block's output WASN'T activated

		// check if any errors are set
		if ( !block->GetErrorMsg( data ).Empty() )
		{
			AddEntry( QEL_ERROR, GetBlockDesc( thread, block ) + TXT( " has thrown an exception: " ) + block->GetErrorMsg( data ), block );
		}
		else if ( !block->IsBlockEnabled( data ) )
		{
			// the block was cut controlled
			AddEntry( QEL_INFO, GetBlockDesc( thread, block ) + TXT( " was CUT-CONTROLLED" ), block );
		}
		else
		{
			// an output wasn't cut controlled, this indicates that the thread the block
			// was in died - it probably was cut controlled
			AddEntry( QEL_INFO, GetBlockDesc( thread, block ) + TXT( " died, because the phase it was in was CUT_CONTROLLED" ), block );
		}
	}
}

void CQuestExecutionLog::OnBlockInputActivated( CQuestThread* thread, const CQuestGraphBlock* block )
{
	String desc = GetBlockDesc( thread, block ) + TXT( " input activated" );
	Type type = QEL_INFO;
	if ( !thread || !block )
	{
		type = QEL_ERROR;
	}
	AddEntry( type, desc, block );
}

void CQuestExecutionLog::OnClose( wxCloseEvent& event )
{
	Hide();
}

String CQuestExecutionLog::GetThreadDesc( CQuestThread* parentThread, CQuestThread* thread ) const
{
	String parentThreadName = parentThread ? parentThread->GetName() : TXT( "'NULL'" );
	String threadName = thread ? thread->GetName() : TXT( "'NULL'" );
	return TXT( "[" ) + parentThreadName + TXT( ":" ) + threadName + TXT( "]" );
}


String CQuestExecutionLog::GetBlockDesc( CQuestThread* thread, const CQuestGraphBlock* block ) const
{
	String threadName = thread ? thread->GetName() : TXT( "'NULL'" );
	String blockName = block ? block->GetCaption() : TXT( "'NULL'" );
	return TXT( "[" ) + threadName + TXT( "->" ) + blockName + TXT( "]" );
}


void CQuestExecutionLog::AddEntry( Type type, const String& desc, const CQuestGraphBlock* block )
{
	Int32 insertPos = m_log->InsertItem( 0, desc.AsChar() );

	switch ( type )
	{
	case QEL_INFO:
		{
			m_log->SetItem( insertPos, 1, wxT( "INFO" ) );
			m_log->SetItemBackgroundColour( insertPos, *wxWHITE );
			break;
		}
	case QEL_WARNING:
		{
			m_log->SetItem( insertPos, 1, wxT( "WARNING" ) );
			m_log->SetItemBackgroundColour( insertPos, wxColour( 255, 165, 48 ) );
			break;
		}

	case QEL_ERROR:
		{
			m_log->SetItem( insertPos, 1, wxT( "ERROR" ) );
			m_log->SetItemBackgroundColour( insertPos, *wxRED );
			m_host->SetToolErrorIndicator( *this );
			break;
		}
	}

	m_log->SetItem( insertPos, 2, ToString( (Float)GGame->GetEngineTime() ).AsChar() );
	m_log->SetItemPtrData( insertPos, reinterpret_cast< wxUIntPtr >( block ) );
}

void CQuestExecutionLog::OnFind( wxCommandEvent& event )
{
	// deselect all items
	Uint32 count = m_log->GetItemCount();
	if ( count == 0 )
	{
		return;
	}

	// search for the specified phrase
	wxString searchPhrase = m_searchPhrase->GetValue();
	if ( searchPhrase.IsEmpty() )
	{
		return;
	}

	Bool firstItem = true;

	for ( Uint32 i = 0; i < count; ++i )
	{
		wxString text = m_log->GetItemText( i );
		if ( text.Contains( searchPhrase ) )
		{
			m_log->SetItemTextColour( i, wxColour( 100, 100, 255 ) );

			if ( firstItem )
			{
				// focus on the first item found
				m_log->EnsureVisible( i );
				firstItem = false;
			}

		}
		else
		{
			m_log->SetItemTextColour( i, wxColour( 0, 0, 0 ) );
		}
	}
}
