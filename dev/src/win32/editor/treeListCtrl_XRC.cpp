/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

IMPLEMENT_DYNAMIC_CLASS( wxTreeListCtrl_XmlHandler, wxXmlResourceHandler ) 

wxTreeListCtrl_XmlHandler::wxTreeListCtrl_XmlHandler()
{              
	AddWindowStyles();
}

wxObject* wxTreeListCtrl_XmlHandler::DoCreateResource()
{
	XRC_MAKE_INSTANCE(window, wxTreeListCtrl );

	window->Create(m_parentAsWindow, GetID(), GetPosition(), GetSize(),
		wxTR_HIDE_ROOT | wxTR_HAS_BUTTONS | wxTR_ROW_LINES | wxTR_COLUMN_LINES | wxTR_FULL_ROW_HIGHLIGHT, 
		wxDefaultValidator, GetName() );
	SetupWindow(window);
	return window;
}

bool wxTreeListCtrl_XmlHandler::CanHandle(wxXmlNode *node)
{
	return IsOfClass(node, wxT("wxTreeListCtrl"));
}

