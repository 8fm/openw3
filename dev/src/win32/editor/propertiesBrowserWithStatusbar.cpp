/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "propertiesBrowserWithStatusbar.h"

CEdPropertiesBrowserWithStatusbar::CEdPropertiesBrowserWithStatusbar( wxWindow* parent, const PropertiesPageSettings& settings, CEdUndoManager* undoManager )
	: wxPanel( parent, wxID_ANY, wxDefaultPosition, wxSize(340,400), wxWANTS_CHARS ) // get rid of wxWANTS_CHARS if something goes wrong
{
	int height = GetSize().GetHeight();

	wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
	SetSizer( sizer );

	wxSplitterWindow* splitter = new wxSplitterWindow( 
		this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3D|wxSP_LIVE_UPDATE 
		);
	sizer->Add( splitter, 1, wxEXPAND, 0 );
	splitter->SetSashGravity( 1.0 ); // keep the status at the bottom
	splitter->SetMinimumPaneSize( 20 );

	// -- props browser --

 	wxPanel* propsPanel = new wxPanel( splitter, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSTATIC_BORDER );
 	wxBoxSizer* propsPanelSizer = new wxBoxSizer( wxVERTICAL );
 	propsPanel->SetSizer( propsPanelSizer );

	m_propertiesBrowser = new CEdPropertiesPage( propsPanel, settings, undoManager );
	propsPanelSizer->Add( m_propertiesBrowser, 1, wxEXPAND, 0 );

	// -- status panel & text --

	wxPanel* statusPanel = new wxPanel( splitter, 0, wxDefaultPosition, wxDefaultSize, wxSTATIC_BORDER );
	wxBoxSizer* statusPanelSizer = new wxBoxSizer( wxVERTICAL );
	statusPanel->SetSizer( statusPanelSizer );

	m_statusBar = new wxStaticText( 
		statusPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT | wxST_NO_AUTORESIZE
		);

	wxSizerItem* statusSizerItem = statusPanelSizer->Add( m_statusBar, 1, wxEXPAND|wxLEFT, 5 );

	splitter->SplitHorizontally( propsPanel, statusPanel, height-20 );
	Layout();

	m_propertiesBrowser->SetStatusBar( m_statusBar );
}

