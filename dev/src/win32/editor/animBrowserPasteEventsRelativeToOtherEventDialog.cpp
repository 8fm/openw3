#include "build.h"
#include "animBrowserPasteEventsRelativeToOtherEventDialog.h"


BEGIN_EVENT_TABLE( CAnimBrowserPasteEventsRelativeToOtherEventDialog, wxDialog )
	EVT_BUTTON( XRCID("OKBtn"), CAnimBrowserPasteEventsRelativeToOtherEventDialog::OnOK )
	EVT_BUTTON( XRCID("CancelBtn"), CAnimBrowserPasteEventsRelativeToOtherEventDialog::OnCancel )
END_EVENT_TABLE()

CAnimBrowserPasteEventsRelativeToOtherEventDialog::CAnimBrowserPasteEventsRelativeToOtherEventDialog( wxWindow* parent, String const & title )
{
	wxXmlResource::Get()->LoadDialog( this, parent, wxT("EventAndRelPosDialog") );

	m_eventNameBox = XRCCTRL( *this, "EventNameBox", wxTextCtrl );
	m_relPosBox = XRCCTRL( *this, "RelPosBox", wxTextCtrl );

	m_relPosBox->SetValue( wxT("0.0") );
	Layout();

	if ( ! title.Empty() )
	{
		SetTitle( title.AsChar() );
	}
}

CAnimBrowserPasteEventsRelativeToOtherEventDialog::~CAnimBrowserPasteEventsRelativeToOtherEventDialog()
{
}

void CAnimBrowserPasteEventsRelativeToOtherEventDialog::OnOK( wxCommandEvent& event )
{
	m_eventName = CName( m_eventNameBox->GetValue().wc_str() );
	FromString( String( m_relPosBox->GetValue().wc_str() ), m_relPos );
	if ( ! m_eventName.Empty() )
	{
		EndDialog(1);
	}
}

void CAnimBrowserPasteEventsRelativeToOtherEventDialog::OnCancel( wxCommandEvent& event )
{
	EndDialog(0);
}
