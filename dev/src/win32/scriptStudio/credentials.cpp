/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "credentials.h"
#include "app.h"

wxIMPLEMENT_CLASS( CSSVersionControlCredentialsDialog, wxDialog );

CSSVersionControlCredentialsDialog::CSSVersionControlCredentialsDialog( wxWindow* parent )
:	wxDialog()
{
	// Load layout from XRC
	wxXmlResource::Get()->LoadDialog( this, parent, wxT( "credentials" ) );

	unsigned int count = wxTheSSApp->GetVersionControl()->GetCredentialsCount();
	const char** credentials = wxTheSSApp->GetVersionControl()->GetCredentials();

	wxGridSizer* sizer = new wxGridSizer( Col_Max );

	int idStart = wxID_HIGHEST;

	for( unsigned int i = 0; i < count; ++i )
	{
		int id = idStart + static_cast< int >( i );

		wxString key = credentials[ i ];
		wxString existingValue = wxTheSSApp->GetVersionControl()->GetCredential( credentials[ i ] );

		m_wxIdToCredentialKey[ id ] = key;
		m_keyToValue[ key ] = existingValue;

		wxStaticText* label = new wxStaticText( this, wxID_ANY, credentials[ i ] );
		sizer->Add( label, 1, wxALL|wxEXPAND, 5 );

		wxTextCtrl* field = new wxTextCtrl( this, id, existingValue );
		sizer->Add( field, 1, wxALL|wxEXPAND, 5 );

		field->Bind( wxEVT_COMMAND_TEXT_UPDATED, &CSSVersionControlCredentialsDialog::OnCredentialsChanged, this );
	}

	GetSizer()->Insert( 0, sizer, 1, wxALL|wxEXPAND, 5 );

	Bind( wxEVT_COMMAND_BUTTON_CLICKED, &CSSVersionControlCredentialsDialog::OnOK, this, wxID_OK );

	Fit();
	Layout();
}

CSSVersionControlCredentialsDialog::~CSSVersionControlCredentialsDialog()
{

}

void CSSVersionControlCredentialsDialog::OnCredentialsChanged( wxCommandEvent& event )
{
	int id = event.GetId();

	wxString key = m_wxIdToCredentialKey[ id ];

	m_keyToValue[ key ] = event.GetString();

	event.Skip();
}

void CSSVersionControlCredentialsDialog::OnOK( wxCommandEvent& event )
{
	for( map< wxString, wxString >::iterator iter = m_keyToValue.begin(); iter != m_keyToValue.end(); ++iter )
	{
		wxTheSSApp->GetVersionControl()->SetCredential( iter->first.mb_str(), iter->second.mb_str() );
	}

	CCredentialsChangedEvent* newEvent = new CCredentialsChangedEvent( m_keyToValue );

	QueueEvent( newEvent );

	event.Skip();
}

//////////////////////////////////////////////////////////////////////////

wxDEFINE_EVENT( ssEVT_CREDENTIALS_CHANGED_EVENT, CCredentialsChangedEvent );
wxIMPLEMENT_DYNAMIC_CLASS( CCredentialsChangedEvent, wxEvent );

CCredentialsChangedEvent::CCredentialsChangedEvent( wxEventType commandType, int winid )
:	wxEvent( commandType, winid )
{

}

CCredentialsChangedEvent::CCredentialsChangedEvent( const map< wxString, wxString >& credentials )
:	wxEvent( wxID_ANY, ssEVT_CREDENTIALS_CHANGED_EVENT )
{
	m_credentials = new map< wxString, wxString >( credentials );
	m_referenceCount = new Red::Threads::CAtomic< Red::System::Uint32 >( 1 );
}

CCredentialsChangedEvent::CCredentialsChangedEvent( const CCredentialsChangedEvent* other )
:	wxEvent( wxID_ANY, ssEVT_CREDENTIALS_CHANGED_EVENT )
,	m_credentials( other->m_credentials )
{
	m_referenceCount->Increment();
}

inline CCredentialsChangedEvent::~CCredentialsChangedEvent()
{
	if( m_referenceCount->Decrement() == 0 )
	{
		delete m_credentials;
		delete m_referenceCount;

		m_credentials = nullptr;
		m_referenceCount = nullptr;
	}
}
