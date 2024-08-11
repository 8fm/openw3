/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "commonBaseDialog.h"

wxIMPLEMENT_CLASS( CSSCommonBaseDialog, wxDialog );

CSSCommonBaseDialog::CSSCommonBaseDialog( wxWindow* parent, const wxString& xrcName )
:	m_cancelled( false )
{
	wxXmlResource::Get()->LoadDialog( this, parent, xrcName );

	Bind( wxEVT_COMMAND_BUTTON_CLICKED, &CSSCommonBaseDialog::OnCancel, this, wxID_CANCEL );
}

CSSCommonBaseDialog::~CSSCommonBaseDialog()
{

}

void CSSCommonBaseDialog::OnCancel( wxCommandEvent& event )
{
	m_cancelled = true;
	event.Skip();
}
