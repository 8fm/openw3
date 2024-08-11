#pragma once

#include <wx/textctrl.h>

class wxTextCtrlEx : public wxTextCtrl
{
public:
	wxTextCtrlEx() : wxTextCtrl() {}
    wxTextCtrlEx(wxWindow *parent, wxWindowID id,
               const wxString& value = wxEmptyString,
               const wxPoint& pos = wxDefaultPosition,
               const wxSize& size = wxDefaultSize,
               long style = 0,
               const wxValidator& validator = wxDefaultValidator,
               const wxString& name = wxTextCtrlNameStr)
		: wxTextCtrl( parent, id, value, pos, size, style, validator, name )
    {}

protected:
	virtual bool MSWShouldPreProcessMessage(WXMSG* msg)
	{
		if ( msg->message == WM_KEYDOWN )
		{
			const WPARAM vkey = msg->wParam;

			bool isUndoRedoPressed = ( ( vkey == 'Z' || vkey == 'Y' ) && wxIsCtrlDown() );

			if ( vkey != VK_RETURN && !isUndoRedoPressed )
			{
				return false;
			}
		}

		return wxTextCtrl::MSWShouldPreProcessMessage( msg );
	}
};