/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

//////////////////////////////////////////////////////////////////////////
CEdResources::CEdResources()
:m_XResourcesPanel( NULL )
{
	m_XResourcesPanel = new wxPanel();
	wxXmlResource::Get()->LoadPanel( m_XResourcesPanel, wxTheFrame, wxT("XResources") );
	m_XResourcesPanel->Hide();
}

//////////////////////////////////////////////////////////////////////////
wxBitmap CEdResources::LoadBitmap( const wxString& name )
{
	wxStaticBitmap* bitmap = NULL;
	wxWindow* bitmapWindow = m_XResourcesPanel->FindWindow( wxXmlResource::GetXRCID( name ) );
	if ( bitmapWindow )
	{
		bitmap = wxStaticCast( bitmapWindow, wxStaticBitmap );
	}
	

	return ( bitmap != NULL ) ? bitmap->GetBitmap() : wxBitmap();
}
