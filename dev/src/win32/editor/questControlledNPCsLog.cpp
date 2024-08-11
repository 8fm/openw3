#include "build.h"
#include "questControlledNPCsLog.h"
#include "questEditor.h"
#include "../../games/r4/r4QuestSystem.h"


CQuestControlledNPCsLog::CQuestControlledNPCsLog()
: m_log( NULL )
, m_system( NULL )
, m_host( NULL )
{

}

CQuestControlledNPCsLog::~CQuestControlledNPCsLog()
{
	OnDetach();
}

void CQuestControlledNPCsLog::OnAttach( CEdQuestEditor& host, wxWindow* parent )
{
	if ( GCommonGame->IsA< CR4Game > () )
	{
		m_host = &host;

		// initialize UI
		Create( parent );

		wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );

		m_log = new wxListCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT );
		sizer->Add( m_log, 1, wxALL|wxEXPAND, 5 );

		m_log->InsertColumn( 0, wxT( "Time" ) );
		m_log->SetColumnWidth( 0, 110 );
		m_log->InsertColumn( 1, wxT( "Type" ) );
		m_log->SetColumnWidth( 1, 110 );
		m_log->InsertColumn( 2, wxT( "Description" ) );
		m_log->SetColumnWidth( 2, 500 );
		m_log->Layout();


		SetSizer( sizer );
		Layout();
		sizer->Fit( m_log );

		// attach to the quests system
		if ( m_system != NULL )
		{
			m_system->GetNPCsManager()->DetachListener( *this );
		}

		m_system = static_cast< CR4QuestSystem*>( host.GetQuestsSystem() );
		m_system->GetNPCsManager()->AttachListener( *this );
	}
}

void CQuestControlledNPCsLog::OnDetach()
{
	if ( m_system != NULL )
	{
		m_system->GetNPCsManager()->DetachListener( *this );
		m_system = NULL;
	}

	m_host = NULL;
}

void CQuestControlledNPCsLog::OnClose( wxCloseEvent& event )
{
	Hide();
}

void CQuestControlledNPCsLog::NotifyError( const String& errMsg ) const
{
	Int32 insertPos = m_log->InsertItem( 0, ToString( (Float)GGame->GetEngineTime() ).AsChar() );

	m_log->SetItem( insertPos, 1, wxT( "ERROR" ) );
	m_log->SetItemBackgroundColour( insertPos, *wxRED );
	m_host->SetToolErrorIndicator( *this );
	m_log->SetItem( insertPos, 2, errMsg.AsChar() );
}

void CQuestControlledNPCsLog::NotifySuccess( const String& msg ) const
{
	Int32 insertPos = m_log->InsertItem( 0, ToString( (Float)GGame->GetEngineTime() ).AsChar() );

	m_log->SetItem( insertPos, 1, wxT( "INFO" ) );
	m_log->SetItemBackgroundColour( insertPos, *wxWHITE );
	m_log->SetItem( insertPos, 2, msg.AsChar() );
}
