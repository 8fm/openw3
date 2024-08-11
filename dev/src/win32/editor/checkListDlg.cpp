
#include "build.h"
#include "checkListDlg.h"

#define ID_BTN_OK 4001
#define ID_BTN_CANCEL 4002

BEGIN_EVENT_TABLE( CEdCheckListDialog, wxDialog )
	EVT_BUTTON( ID_BTN_OK, CEdCheckListDialog::OnOK )
	EVT_BUTTON( ID_BTN_CANCEL, CEdCheckListDialog::OnCancel )
END_EVENT_TABLE()


CEdCheckListDialog::CEdCheckListDialog( wxWindow* parent, const String &title, const TDynArray< String >& options, TDynArray< Bool >& optionStates, TSet< String >& chosen, Bool withCancel /* = false */ )
	: wxDialog( parent, wxID_ANY, title.AsChar() )
	, m_options( options )
	, m_optionStates( optionStates )
	, m_chosenSet( &chosen )
	, m_cancelButtonVisible( withCancel )
{
	Init();
}

CEdCheckListDialog::CEdCheckListDialog( wxWindow* parent, const String &title, const TDynArray< String >& options, TDynArray< Bool >& optionStates, 
									   Bool withCancel /* = false */, const String& desc /*= String::EMPTY*/ )
	: wxDialog( parent, wxID_ANY, title.AsChar() )
	, m_description( desc )
	, m_options( options )
	, m_optionStates( optionStates )
	, m_chosenSet( NULL )
	, m_cancelButtonVisible( withCancel )
{
	Init();
}

void CEdCheckListDialog::Init()
{
	CreateWidget();

	for ( Uint32 i = 0; i < m_options.Size(); i++ )
	{
		m_list->AppendString( m_options[ i ].AsChar() );
	}

	for ( Uint32 i = 0; i < m_optionStates.Size(); i++ )
	{
		if ( m_optionStates[ i ] )
		{
			m_list->Check( i );
		}
	}

	if ( m_chosenSet )
	{
		m_chosenSet->Clear();
	}	
}

void CEdCheckListDialog::OnOK( wxCommandEvent& event )
{
	for ( Uint32 i=0; i<m_options.Size(); i++ )
	{
		if ( m_list->IsChecked( i ) )
		{
			if ( m_chosenSet )
			{
				m_chosenSet->Insert( m_options[ i ] );
			}

			m_optionStates[ i ] = true;
		}
		else
		{
			m_optionStates[ i ] = false;
		}
	}

	EndDialog( wxID_OK );
}

void CEdCheckListDialog::OnCancel( wxCommandEvent& event )
{
	EndDialog( wxID_CANCEL );
}

void CEdCheckListDialog::CreateWidget()
{
	SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bSizer95 = new wxBoxSizer( wxVERTICAL );

	if ( !m_description.Empty() )
	{
		wxStaticText* label = new wxStaticText( this, wxID_ANY, m_description.AsChar() );
		bSizer95->Add( label, 0, wxALL|wxALIGN_LEFT, 5 );
	}

	wxArrayString ChangelistChoices;
	m_list = new wxCheckListBox( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, ChangelistChoices, 0 );
	bSizer95->Add( m_list, 1, wxALL|wxEXPAND, 5 );

	wxBoxSizer* bSizer96 = new wxBoxSizer( wxHORIZONTAL );

	wxButton* OK = new wxButton( this, ID_BTN_OK, wxT("OK"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer96->Add( OK, 0, wxALL, 5 );

	if ( m_cancelButtonVisible )
	{
		wxButton* cancelBtn = new wxButton( this, ID_BTN_CANCEL, wxT("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
		bSizer96->Add( cancelBtn, 0, wxALL, 5 );
	}

	bSizer95->Add( bSizer96, 0, wxALIGN_CENTER_HORIZONTAL, 5 );

	SetSizer( bSizer95 );
	Layout();
}



CEdResourcesOverwriteChecker::CEdResourcesOverwriteChecker( wxWindow* parent, const TDynArray< CResource* >& resources, Bool modifiedOnly /*= false */ )
	: m_parent( parent )
{
	for ( Uint32 i = 0; i < resources.Size(); ++i )
	{
		CResource* res = resources[i];

		if ( res && ( !modifiedOnly || res->IsModified() ) && res->GetFile() )
		{
			m_resources.Insert( res->GetFile()->GetDepotPath(), res );
		}
	}
}

Bool CEdResourcesOverwriteChecker::Execute()
{
	TDynArray< String > options;
	m_resources.GetKeys( options );
	Sort( options.Begin(), options.End() );

	TDynArray< Bool > states( options.Size() );
	for ( Uint32 i = 0; i < states.Size(); ++i )
	{
		states[i] = true;
	}

	CEdCheckListDialog dlg( m_parent, TXT("Overwriting files"), options, states, true, TXT("Would you like to overwrite files?") );
	if ( dlg.ShowModal() == wxID_OK )
	{
		for ( Uint32 i = 0; i < options.Size(); i++ )
		{
			if ( CDiskFile* file = (*m_resources.FindPtr( options[i] ) )->GetFile() )
			{
				if ( states[ i ] )
				{
					file->SetForcedOverwriteFlag( eOverwrite );
				}
				else
				{
					file->SetForcedOverwriteFlag( eCancel );
				}
			}
		}
		return true;
	}
	else
	{
		for ( Uint32 i = 0; i < options.Size(); i++ )
		{
			if ( CDiskFile* file = (*m_resources.FindPtr( options[i] ) )->GetFile() )
			{
				file->SetForcedOverwriteFlag( eAsk );
			}
		}
		return false;
	}
}