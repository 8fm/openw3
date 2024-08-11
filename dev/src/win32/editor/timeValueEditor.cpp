#include "build.h"
#include "timeValueEditor.h"
#include "../../common/core/feedback.h"

namespace
{
	const Int32 GAllSeconds = 24 * 60 * 60;	// one day in seconds = 24 hours * 60 minutes * 60 seconds
}


BEGIN_EVENT_TABLE( CEdTimeValueEditor, wxDialog )
EVT_SET_FOCUS( CEdTimeValueEditor::OnFocus )
EVT_BUTTON( XRCID("btnOK"), CEdTimeValueEditor::OnOK )
EVT_BUTTON( XRCID("btnCancel"), CEdTimeValueEditor::OnCancel )
END_EVENT_TABLE()


CEdTimeValueEditor::CEdTimeValueEditor( wxWindow *parent, Float defaultValue )
 : m_defaultValue ( defaultValue )
 , m_value ( defaultValue )
 , m_timePicker( nullptr )
{
	// Load dialog
	wxXmlResource::Get()->LoadDialog( this, parent, TXT("TimeValueDialog") );

	// Write default value to dialog control
	wxPanel* timePanel = XRCCTRL( *this, "timePanel", wxPanel );
	if( timePanel != nullptr )
	{
		wxSizer* sizer = timePanel->GetSizer();
		if( sizer != nullptr )
		{
			m_timePicker = new wxTimePickerCtrl( timePanel, wxID_ANY );
			sizer->Add( m_timePicker, 0, wxALL, 5 );
			m_timePicker->Connect( wxEVT_TIME_CHANGED, wxDateEventHandler( CEdTimeValueEditor::OnTimeChanged ), nullptr, this );

			Int32 seconds = defaultValue * GAllSeconds;
			Int32 hours = seconds / ( 60 * 60 );
			seconds -= ( hours * ( 60 * 60 ) );
			Int32 minutes = seconds / 60;
			seconds -= ( minutes * 60 );

			m_timePicker->SetTime( hours, minutes, seconds );
		}
	}

	// Set focus
	SetFocus();
	Refresh();
}

CEdTimeValueEditor::~CEdTimeValueEditor()
{
	// empty
}

void CEdTimeValueEditor::OnOK( wxCommandEvent& event )
{
	EndModal( wxID_OK );
}

void CEdTimeValueEditor::OnCancel( wxCommandEvent& event )
{
	if( m_value != m_defaultValue )
	{
		if( GFeedback->AskYesNo( TXT("Time has been changed. Do you want to close window and lost new time value?") ) == true )
		{
			m_value = m_defaultValue;
			Close();
		}
	}
	else
	{
		Close();
	}
}

void CEdTimeValueEditor::OnFocus(wxFocusEvent& event)
{
	event.Skip();
}

void CEdTimeValueEditor::OnTimeChanged( wxDateEvent& event )
{
	Int32 hours, minutes, seconds;
	m_timePicker->GetTime( &hours, &minutes, &seconds );
	seconds += ( minutes * 60 ) + ( hours * 60 * 60 );
	m_value = seconds / (Float)GAllSeconds;
}
