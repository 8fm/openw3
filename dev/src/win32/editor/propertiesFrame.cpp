/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

CEdPropertiesFrame::CEdPropertiesFrame( wxWindow* parent, const String& caption, CEdUndoManager* undoManager )
	: wxSmartLayoutFrame( parent, caption )
	//: wxFrame( parent, wxID_ANY, caption.AsChar(), wxDefaultPosition, wxDefaultSize, wxDEFAULT_FRAME_STYLE | wxFRAME_NO_TASKBAR )
{
	wxBoxSizer* sizer = new wxBoxSizer( wxHORIZONTAL );
	PropertiesPageSettings settings;
	m_browser = new CEdPropertiesBrowserWithStatusbar( this, settings, undoManager );
	sizer->Add( m_browser, 1, wxEXPAND, 0 );
	SetSizer( sizer );
	Layout();
}

CEdPropertiesFrame::~CEdPropertiesFrame()
{
}

void CEdPropertiesFrame::SetObject( CObject* object )
{
	m_browser->Get().SetObject( object );
}

void CEdPropertiesFrame::SetObjects( const TDynArray< CObject* > &objects )
{
	m_browser->Get().SetObjects( objects );
}

void CEdPropertiesFrame::SaveOptionsToConfig()
{
	SaveLayout(TXT("/Frames/PropertiesFrame"));
}

void CEdPropertiesFrame::LoadOptionsFromConfig()
{
	LoadLayout(TXT("/Frames/PropertiesFrame"));
}

void CEdPropertiesFrame::SaveSession( CConfigurationManager &config )
{

}

void CEdPropertiesFrame::RestoreSession( CConfigurationManager &config )
{


}
