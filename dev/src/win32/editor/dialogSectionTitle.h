#pragma once
#include "textControl.h"

class CEdDialogSectionTitle : public CEdTextControl
{
public:
	CEdDialogSectionTitle
		(
		wxWindow* parent,
		wxWindowID id,
		const wxString& value			= wxEmptyString,
		const wxPoint& pos				= wxDefaultPosition,
		const wxSize& size				= wxDefaultSize,
		long style						= 0,
		const wxValidator& validator	= wxDefaultValidator,
		const wxString& name			= wxTextCtrlNameStr
		) 
		:CEdTextControl(parent,id,value,pos,size,style,validator,name)
	{}

	virtual Bool CustomArrowTraverseRule( wxKeyEvent &event );
};
