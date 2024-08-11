#include "build.h"
#include "gameTimeEditor.h"

BEGIN_EVENT_TABLE( CEdGameTimeEditor, wxDialog )
	EVT_BUTTON( XRCID("OK"), CEdGameTimeEditor::OnOK )
	EVT_BUTTON( XRCID("Cancel"), CEdGameTimeEditor::OnCancel )
END_EVENT_TABLE()

CEdGameTimeEditor::CEdGameTimeEditor( wxWindow* parent, GameTime *time, Bool dayPeriodOnly )
	: m_dayPeriodOnly( dayPeriodOnly )
{
	// Load designed dialog from resource
	wxXmlResource::Get()->LoadDialog( this, parent, TEXT("GameTimeEditor") );

	wxTextCtrl* daysCtrl = XRCCTRL( *this, "Days", wxTextCtrl );
	daysCtrl->Enable( !dayPeriodOnly );
	daysCtrl->SetEditable( !dayPeriodOnly );

	wxSize dialogSize = GetSize(); // remember original dialog size
	LoadOptionsFromConfig();
	SetSize( dialogSize ); // restore original dialog size

	m_time = time;
}

CEdGameTimeEditor::~CEdGameTimeEditor()
{
	SaveOptionsToConfig();
}

void CEdGameTimeEditor::SaveOptionsToConfig()
{
	SaveLayout(TXT("/Dialogs/GameTimeEditor"));
}

void CEdGameTimeEditor::LoadOptionsFromConfig()
{
	LoadLayout(TXT("/Dialogs/GameTimeEditor"));
}

void CEdGameTimeEditor::OnOK( wxCommandEvent& event )
{
	Int32 value;
	*m_time = 0;
	if ( FromString(XRCCTRL( *this, "Days", wxTextCtrl )->GetValue().wc_str(), value) )
	{
		*m_time += GameTime::DAY * value;
	}
	if ( FromString(XRCCTRL( *this, "Hours", wxTextCtrl )->GetValue().wc_str(), value) )
	{
		*m_time += GameTime::HOUR * value;
	}
	if ( FromString(XRCCTRL( *this, "Minutes", wxTextCtrl )->GetValue().wc_str(), value) )
	{
		*m_time += GameTime::MINUTE * value;
	}
	if ( FromString(XRCCTRL( *this, "Seconds", wxTextCtrl )->GetValue().wc_str(), value) )
	{
		*m_time += value;
	}

	if ( m_dayPeriodOnly )
	{
		*m_time = GameTime( 0, m_time->Hours(), m_time->Minutes(), m_time->Seconds() );
	}

	EndModal( wxID_OK );
}

void CEdGameTimeEditor::OnCancel( wxCommandEvent& event )
{
	Close();
}