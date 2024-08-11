#include "build.h"
#include "floatValueEditor.h"


BEGIN_EVENT_TABLE( CEdFloatValueEditor, wxDialog )
EVT_SET_FOCUS( CEdFloatValueEditor::OnFocus )
EVT_BUTTON( XRCID("btnOK"), CEdFloatValueEditor::OnOK )
EVT_BUTTON( XRCID("btnCancel"), CEdFloatValueEditor::OnCancel )
END_EVENT_TABLE()


CEdFloatValueEditor::CEdFloatValueEditor( wxWindow *parent, Float defaultValue )
 : m_defaultValue ( defaultValue )
 , m_value ( defaultValue )
{
	// Load dialog
	wxXmlResource::Get()->LoadDialog( this, parent, TEXT("FloatValueDialog") );	
	
	// Build default value string
	m_defaultValueString.Printf( TXT("%f"), (Float)defaultValue );

	// Write default value to dialog control
	wxTextCtrl* text = XRCCTRL( *this, "valueText", wxTextCtrl );
	text->SetValue( m_defaultValueString );

	// Set focus
	SetFocus();
	Refresh();
}

CEdFloatValueEditor::~CEdFloatValueEditor()
{
	// empty
}

void CEdFloatValueEditor::OnOK( wxCommandEvent& event )
{
	wxTextCtrl* text = XRCCTRL( *this, "valueText", wxTextCtrl );
	wxString currValueString = text->GetValue();
	if ( currValueString != m_defaultValueString )
	{
		double valueTemp = m_defaultValue;
		if ( currValueString.ToDouble( &valueTemp ) )
		{
			m_value = (Float)valueTemp;
		}
	}

	EndModal( wxID_OK );
}

void CEdFloatValueEditor::OnCancel( wxCommandEvent& event )
{
	ASSERT( m_value == m_defaultValue );
	Close();
}

void CEdFloatValueEditor::OnFocus(wxFocusEvent& event)
{
	event.Skip();
}