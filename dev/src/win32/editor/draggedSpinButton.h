#pragma once
#include "wx/geometry.h"

class CEdDraggedSpinButton: public wxSpinButton
{
	enum
	{
		MODE_NORMAL	= 0,
		MODE_CHECKMOUSE, 
		MODE_DRAGGED
	};

	Uint32		m_mode;
	wxPoint		m_mousePressedPosition;
	Float		m_floatValue;
	Float		m_precision;

public:

	CEdDraggedSpinButton(wxWindow *parent,
		wxWindowID id = wxID_ANY,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = wxSP_VERTICAL | wxSP_ARROW_KEYS,
		const wxString& name = wxSPIN_BUTTON_NAME);

protected:

	bool MSWOnNotify( int WXUNUSED(idCtrl), WXLPARAM lParam, WXLPARAM *result );
	bool MSWOnScroll( int orientation, WXWORD wParam, WXWORD pos, WXHWND control );
	void OnMouseEvent( wxMouseEvent& event );

public:
	void SetPrecision( Float precision ) { m_precision = precision; };

	DECLARE_EVENT_TABLE();
};
