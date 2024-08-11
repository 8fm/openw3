/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "aboutDlg.h"


CEdAboutDlg::CEdAboutDlg( wxWindow* parent )
{
	// Load layout from XRC
	wxXmlResource::Get()->LoadDialog( this, parent, wxT("AboutDlg") );
	
	// Set the version label.
	wxStaticText* m_staticVersion = XRCCTRL(*this, "m_staticVersion", wxStaticText);
	m_staticVersion->SetLabel( m_staticVersion->GetLabel() + wxT(" ") + APP_VERSION_NUMBER + wxT("\n") + wxVERSION_STRING );

	// Resize to fit the contents.
	this->GetSizer()->SetSizeHints( this );
}

