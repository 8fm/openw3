#include "build.h"
#include "animBrowserFindEventDialog.h"


BEGIN_EVENT_TABLE( CAnimBrowserFindEventDialog, wxDialog )
	EVT_BUTTON( XRCID("OKBtn"), CAnimBrowserFindEventDialog::OnOK )
	EVT_BUTTON( XRCID("CancelBtn"), CAnimBrowserFindEventDialog::OnCancel )
END_EVENT_TABLE()

CAnimBrowserFindEventDialog::CAnimBrowserFindEventDialog( wxWindow* parent, String const & title )
{
	wxXmlResource::Get()->LoadDialog( this, parent, wxT("FindEventDialog") );

	m_eventNameBox = XRCCTRL( *this, "EventNameBox", wxTextCtrl );

	Layout();

	if ( ! title.Empty() )
	{
		SetTitle( title.AsChar() );
	}
}

CAnimBrowserFindEventDialog::~CAnimBrowserFindEventDialog()
{
}

void CAnimBrowserFindEventDialog::OnOK( wxCommandEvent& event )
{
	m_eventName = CName( m_eventNameBox->GetValue().wc_str() );
	if ( ! m_eventName.Empty() )
	{
		EndDialog(1);
	}
}

void CAnimBrowserFindEventDialog::OnCancel( wxCommandEvent& event )
{
	EndDialog(0);
}
