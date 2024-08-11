/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CEdResources
{

protected:
	wxPanel*				m_XResourcesPanel;	//!< Resources container

protected:

public:
	CEdResources();
	wxBitmap LoadBitmap( const wxString& name );

};

typedef TSingleton<CEdResources> SEdResources;