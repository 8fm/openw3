//////////////////////////////////////
//          Inferno Engine          //
// Copyright (C) 2002-2007 by Team  //
//////////////////////////////////////

#include "build.h"
#include "inputBoxDlg.h"

/// Input box

CEdInputBox::CEdInputBox( wxWindow* parent, const String& title, const String &caption, const String &edit, Bool multiline /* = false */ )
	: wxDialog( parent, wxID_ANY, title.AsChar(), wxDefaultPosition, wxSize( -1, 125 ) )
	, m_editText( edit )
{
	// Center
	SetSizeHints( wxDefaultSize, wxDefaultSize );
	Centre( wxBOTH );

	// Base sizer
	wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );

	// Caption
	wxStaticText* text = new wxStaticText( this, wxID_ANY, caption.AsChar(), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE );
	text->Wrap( -1 );
	sizer->Add( text, 0, wxALL|wxEXPAND, 5 );

	// Edit box
	m_editBox = new wxTextCtrlEx( this, wxID_ANY, edit.AsChar(), wxDefaultPosition, wxSize( 300,-1 ), multiline ? wxTE_MULTILINE : 0 );
	sizer->Add( m_editBox, 0, wxALL|wxEXPAND, 5 );

	// Sizer for buttons
	wxBoxSizer* sizer2 = new wxBoxSizer( wxHORIZONTAL );
	sizer2->Add( 0, 0, 1, wxEXPAND, 5 );

	// OK button
	wxButton *ok = new wxButton( this, wxID_ANY, wxT("&OK"), wxDefaultPosition, wxDefaultSize, 0 );
	ok->SetDefault(); 
	sizer2->Add( ok, 0, wxALL, 5 );

	// Cancel button
	wxButton* cancel = new wxButton( this, wxID_ANY, wxT("&Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
	sizer2->Add( cancel, 0, wxALL, 5 );

	// Finalize
	sizer2->Add( 0, 0, 1, wxEXPAND, 5 );
	sizer->Add( sizer2, 2, wxEXPAND, 5 );

	// Connect Events
	ok->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdInputBox::OnOK ), NULL, this );
	cancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdInputBox::OnCancel ), NULL, this );

	// Update layout
	SetSizer( sizer );
	Layout();
}

VOID CEdInputBox::OnOK( wxCommandEvent& event )
{
	m_editText = m_editBox->GetValue().wc_str();
	EndDialog( 1 );
}

VOID CEdInputBox::OnCancel( wxCommandEvent& event )
{
	EndDialog( 0 );
}

/// Input box for file name

CEdInputBoxFileName::CEdInputBoxFileName(wxWindow* parent, const String& title, const String &caption, const String &edit, const String &fileExtension ) 
:CEdInputBox( parent, title, caption, edit )
, m_fileExtension( fileExtension )
{
}

VOID CEdInputBoxFileName::OnOK( wxCommandEvent& event )
{
	m_editText = m_editBox->GetValue().wc_str();
	m_editText = m_editText.ToLower();

	if ( ValidateFileName() )
	{
		m_editText = m_editText + TXT(".") + m_fileExtension;
		EndDialog( 1 );
	}
	else
	{
		GFeedback->ShowError( TXT("File name contains forbidden character") );
	}
}

Bool CEdInputBoxFileName::ValidateFileName() const
{
	// allow only alphanumeric and underscroe chars
	for ( Uint32 i = 0; i < m_editText.Size() - 1; ++i )
	{
		const Char& ch = m_editText[i];
		if ( ( ch < '0' || ch > '9' ) && ( ch < 'a' || ch > 'z' ) && ch != '_' )
		{
			return false;
		}
	}

	return true;
}

/// Input double box

CEdInputDoubleBox::CEdInputDoubleBox( wxWindow* parent, const String& title, const String &caption, String &editA, String &editB, Bool multiline /* = false */, Bool useConfig /* = false */ )
	: wxDialog( parent, wxID_ANY, title.AsChar(), wxDefaultPosition, wxSize( -1, 125 ) )
	, m_editTextA( editA )
	, m_editTextB( editB )
	, m_useConfig( useConfig )
{
	// Center
	SetSizeHints( wxDefaultSize, wxDefaultSize );
	Centre( wxBOTH );

	if ( m_useConfig )
	{
		ReadDefaultValues();
	}

	// Base sizer
	wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );

	// Caption
	wxStaticText* text = new wxStaticText( this, wxID_ANY, caption.AsChar(), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE );
	text->Wrap( -1 );
	sizer->Add( text, 0, wxALL|wxEXPAND, 5 );

	// Edit boxes
	wxBoxSizer* sizerEdit = new wxBoxSizer( wxHORIZONTAL );

	m_editBoxA = new wxTextCtrlEx( this, wxID_ANY, editA.AsChar(), wxDefaultPosition, wxSize( 150,-1 ), multiline ? wxTE_MULTILINE : 0 );
	m_editBoxA->SetLabel( m_editTextA.AsChar() );
	sizerEdit->Add( m_editBoxA, 1, wxALL|wxEXPAND, 5 );

	m_editBoxB = new wxTextCtrlEx( this, wxID_ANY, editB.AsChar(), wxDefaultPosition, wxSize( 150,-1 ), multiline ? wxTE_MULTILINE : 0 );
	m_editBoxB->SetLabel( m_editTextB.AsChar() );
	sizerEdit->Add( m_editBoxB, 1, wxALL|wxEXPAND, 5 );

	sizer->Add( sizerEdit, 0, wxEXPAND, 5 );

	// Sizer for buttons
	wxBoxSizer* sizer2 = new wxBoxSizer( wxHORIZONTAL );
	sizer2->Add( 0, 0, 1, wxEXPAND, 5 );

	// OK button
	wxButton *ok = new wxButton( this, wxID_ANY, wxT("&OK"), wxDefaultPosition, wxDefaultSize, 0 );
	ok->SetDefault(); 
	sizer2->Add( ok, 0, wxALL, 5 );

	// Cancel button
	wxButton* cancel = new wxButton( this, wxID_ANY, wxT("&Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
	sizer2->Add( cancel, 0, wxALL, 5 );

	// Finalize
	sizer2->Add( 0, 0, 1, wxEXPAND, 5 );
	sizer->Add( sizer2, 2, wxEXPAND, 5 );

	// Connect Events
	ok->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdInputDoubleBox::OnOK ), NULL, this );
	cancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdInputDoubleBox::OnCancel ), NULL, this );

	// Update layout
	SetSizer( sizer );
	Layout();
}

void CEdInputDoubleBox::OnOK( wxCommandEvent& event )
{
	m_editTextA = m_editBoxA->GetValue().wc_str();
	m_editTextB = m_editBoxB->GetValue().wc_str();

	if ( m_useConfig )
	{
		SaveDefaultValues();
	}

	EndDialog( 1 );
}

void CEdInputDoubleBox::OnCancel( wxCommandEvent& event )
{
	EndDialog( 0 );
}

void CEdInputDoubleBox::ReadDefaultValues()
{
	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
	String configPath = String::Printf( TXT("/CEdInputDoubleBox/%s/"), GetTitle().wc_str() ); 
	CConfigurationScopedPathSetter pathSetter( config, configPath );

	m_editTextA = config.Read( TXT("valueA"), String::EMPTY );
	m_editTextB = config.Read( TXT("valueB"), String::EMPTY );
}

void CEdInputDoubleBox::SaveDefaultValues()
{
	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
	String configPath = String::Printf( TXT("/CEdInputDoubleBox/%s/"), GetTitle().wc_str() );
	CConfigurationScopedPathSetter pathSetter( config, configPath );

	config.Write( TXT("valueA"), m_editTextA );
	config.Write( TXT("valueB"), m_editTextB );
}

const Int32 CEdInputMultiBox::LINE_Y_SIZE( 36 );
const Int32 CEdInputMultiBox::INITIAL_Y_SIZE( 120 );
const Int32 CEdInputMultiBox::MARGIN( 5 );

CEdInputMultiBox::CEdInputMultiBox( wxWindow* parent, const String& title, const String& msg, TDynArray< String >& values )
	: wxDialog( parent, wxID_ANY, title.AsChar(), wxDefaultPosition, wxSize( -1, INITIAL_Y_SIZE + LINE_Y_SIZE ), wxTAB_TRAVERSAL | wxDEFAULT_DIALOG_STYLE )
	, m_values( values )
{
	// Center
	SetSizeHints( wxDefaultSize, wxDefaultSize );
	Centre( wxBOTH );

	// Base sizer
	wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
	SetSizer( sizer );

	// Message
	wxStaticText* text = new wxStaticText( this, wxID_ANY, msg.AsChar(), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE );
	text->SetFont( wxFont( 14, wxSWISS, wxNORMAL, wxBOLD, false, wxT("Tahoma") ) );
	text->Wrap( -1 );
	sizer->Add( text, 1, wxALL | wxEXPAND, MARGIN );

	// Lines
	m_linesSizer = new wxBoxSizer( wxVERTICAL );
	sizer->Add( m_linesSizer, 5, wxALL | wxEXPAND, MARGIN ); 

	// Initial line
	InsertLine( 0 );

	// Sizer for buttons
	wxBoxSizer* sizer2 = new wxBoxSizer( wxHORIZONTAL );

	// OK button
	wxButton *ok = new wxButton( this, wxID_ANY, wxT("&OK"), wxDefaultPosition, wxDefaultSize, 0 );
	ok->SetDefault(); 
	sizer2->Add( ok, 1, wxALL, MARGIN );

	// Cancel button
	wxButton* cancel = new wxButton( this, wxID_ANY, wxT("&Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
	sizer2->Add( cancel, 1, wxALL, MARGIN );

	// Finalize
	sizer->Add( sizer2, 1, wxEXPAND, MARGIN );

	// Connect Events
	ok->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdInputMultiBox::OnOK ), NULL, this );
	cancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdInputMultiBox::OnCancel ), NULL, this );

	// Update layout
	Layout();

	// Focus
	m_lines[ 0 ].m_inputBox->SetFocus();
}

void CEdInputMultiBox::OnOK( wxCommandEvent& event )
{
	m_values.Resize( m_lines.Size() );

	for ( Uint32 i = 0; i < m_lines.Size(); ++i )
	{
		m_values[ i ] = m_lines[ i ].m_inputBox->GetValue().wx_str();
	}

	EndDialog( 0 );
}

void CEdInputMultiBox::OnCancel( wxCommandEvent& event )
{
	EndDialog( 1 );
}

void CEdInputMultiBox::OnAdd( wxCommandEvent& event )
{
	wxObject* eventObj = event.GetEventObject();
	for ( Uint32 i = 0; i < m_lines.Size(); ++i )
	{
		if ( m_lines[ i ].m_addBtn == eventObj )
		{
			InsertLine( i + 1 );
			break;
		}
	}

	event.Skip();
}

void CEdInputMultiBox::InsertLine( Uint32 index )
{
	SLine line;

	Freeze();

	String& val = String::EMPTY;
	if ( m_values.Size() > index )
	{
		val = m_values[ index ];
	}

	line.m_sizer = new wxBoxSizer( wxHORIZONTAL );
	line.m_label = new wxStaticText( this, wxID_ANY, String::Printf( TXT("%ld:\t"), index ).AsChar(), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	line.m_inputBox =  new wxTextCtrlEx( this, wxID_ANY, val.AsChar(), wxDefaultPosition, wxSize( -1, LINE_Y_SIZE - ( 2 * MARGIN ) ) );
	line.m_addBtn = new wxButton( this, wxID_ANY, TXT("+"), wxDefaultPosition, wxSize( 24, -1 ) );
	line.m_addBtn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdInputMultiBox::OnAdd ), nullptr, this );

	line.m_sizer->Add( line.m_label, 1, wxLEFT | wxRIGHT | wxSHRINK, MARGIN );
	line.m_sizer->Add( line.m_inputBox, 15, wxLEFT | wxRIGHT , MARGIN );
	line.m_sizer->Add( line.m_addBtn, 1, wxLEFT | wxRIGHT | wxSHRINK, MARGIN );

	m_linesSizer->Add( line.m_sizer, 1, wxALL | wxEXPAND, MARGIN ); 

	m_lines.Insert( index, line );

	OnLayoutChange();
	Thaw();

	line.m_inputBox->SetFocus();
}

void CEdInputMultiBox::OnLayoutChange()
{
	for ( Uint32 i = 0; i < m_lines.Size(); ++i )
	{
		m_lines[ i ].m_label->SetLabel( String::Printf( TXT("%ld:\t"), i ).AsChar() );
	}

	SetSize( wxSize( -1, INITIAL_Y_SIZE + m_lines.Size() * LINE_Y_SIZE ) );
	Layout();
	Refresh();
}



