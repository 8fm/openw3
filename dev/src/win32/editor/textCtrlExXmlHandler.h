#pragma once

#include <wx/xrc/xmlres.h>
#include "textCtrlEx.h"

class wxTextCtrlExXmlHandler : public wxTextCtrlXmlHandler
{
public:
    virtual wxObject *DoCreateResource()
	{
		XRC_MAKE_INSTANCE(text, wxTextCtrlEx)

		text->Create(m_parentAsWindow,
					 GetID(),
					 GetText(wxT("value")),
					 GetPosition(), GetSize(),
					 GetStyle(),
					 wxDefaultValidator,
					 GetName());

		SetupWindow(text);

		if (HasParam(wxT("maxlength")))
			text->SetMaxLength(GetLong(wxT("maxlength")));

		return text;
	}
};
