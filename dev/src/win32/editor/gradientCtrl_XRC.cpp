/**
 * Copyright © 2008 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

IMPLEMENT_DYNAMIC_CLASS( wxGradientCtrl_XmlHandler, wxXmlResourceHandler ) 

wxGradientCtrl_XmlHandler::wxGradientCtrl_XmlHandler()
{              
	AddWindowStyles();
}

wxObject* wxGradientCtrl_XmlHandler::DoCreateResource()
{
	XRC_MAKE_INSTANCE( window, CEdGradientPicker );

	window->Create(m_parentAsWindow, GetID(), GetPosition(), GetSize(),
		wxTR_HIDE_ROOT | wxTR_HAS_BUTTONS | wxTR_ROW_LINES | wxTR_COLUMN_LINES | wxTR_FULL_ROW_HIGHLIGHT, 
		wxDefaultValidator, GetName() );
	SetupWindow(window);
	return window;
}

bool wxGradientCtrl_XmlHandler::CanHandle(wxXmlNode *node)
{
	return IsOfClass(node, wxT("wxGradientCtrl"));
}
