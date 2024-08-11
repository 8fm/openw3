/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class wxTreeListCtrl_XmlHandler : public wxXmlResourceHandler
{
public:
	DECLARE_DYNAMIC_CLASS(wxTreeListCtrl_XmlHandler)

public:
	wxTreeListCtrl_XmlHandler();

	virtual wxObject *DoCreateResource();
	virtual bool CanHandle(wxXmlNode *node);
};
