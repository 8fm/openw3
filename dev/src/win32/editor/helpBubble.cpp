#include "build.h"
#include "helpBubble.h"

IMPLEMENT_CLASS( CEdHelpBubble, wxPopupWindow );

CEdHelpBubble::CEdHelpBubble( wxWindow* parent, const String& helpText, wxPoint position /*= wxDefaultPosition */ )
:	wxPopupWindow( parent, wxBORDER )
{
	wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
	SetSizer( sizer );

	wxPanel* mainPanel = new wxPanel;

	VERIFY( wxXmlResource::Get()->LoadPanel( mainPanel, this, wxT( "HelpBubble" ) ) );

	sizer->Add( mainPanel );

	m_helpTextCtrl = XRCCTRL( *this, "HelpText", wxStaticText );
	ASSERT( m_helpTextCtrl != NULL, TXT( "HelpText ctrl doesn't exist in HelpBubble in editor_tools XRC" ) );

	SetPosition( position );

	parent->Bind( wxEVT_MOVING, &CEdHelpBubble::OnParentMoved, this );
	parent->Bind( wxEVT_SHOW, &CEdHelpBubble::OnParentShown, this );

	SetLabel( helpText );
}

CEdHelpBubble::~CEdHelpBubble()
{
}

void CEdHelpBubble::OnParentShown( wxShowEvent& event )
{
	if( !event.IsShown() )
	{
		Hide();
	}

	event.Skip();
}

void CEdHelpBubble::OnParentMoved( wxMoveEvent& event )
{
	Hide();

	event.Skip();
}

void CEdHelpBubble::SetLabel( const String& helpText )
{
	m_helpTextCtrl->SetLabel( helpText.AsChar() );

	Fit();
}

void CEdHelpBubble::SetPosition( wxPoint position /*= wxDefaultPosition */ )
{
	if( position == wxDefaultPosition )
	{
		wxSize size = GetParent()->GetSize();
		position = wxPoint( size.GetX(), 0 ) + GetParent()->GetPosition();
	}

	Move( position, wxSIZE_AUTO | wxSIZE_FORCE );
}
