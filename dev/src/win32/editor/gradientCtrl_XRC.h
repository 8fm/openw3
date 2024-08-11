/**
 * Copyright © 2008 CD Projekt Red. All Rights Reserved.
 */
#pragma once

class wxGradientCtrl_XmlHandler : public wxXmlResourceHandler
{
public:
	DECLARE_DYNAMIC_CLASS(wxGradientCtrl_XmlHandler)

public:
	wxGradientCtrl_XmlHandler();

	virtual wxObject *DoCreateResource();
	virtual bool CanHandle(wxXmlNode *node);
};