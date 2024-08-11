/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

class CEdDraggableFloatValueEditor : public wxEvtHandler
{
	enum EMode
	{
		MODE_INACTIVE,
		MODE_CHECKING,
		MODE_DRAGGING
	};

protected:
	wxTextCtrl*				m_text;
	Float*					m_data;
	CObject*				m_parentObject;

	wxPoint					m_mousePressedPosition;
	Float					m_precision;
	EMode					m_mode;

public:
	CEdDraggableFloatValueEditor() 
		: m_data( NULL )
		, m_precision( 0.1f )
		, m_mode( MODE_INACTIVE ) 
		, m_parentObject( NULL )
	{}

	void Init( wxTextCtrl* textCtrl, Float* data, CObject* parentObject = NULL );

	void OnTextChanged( wxCommandEvent& event );
	void OnTextFocusLost( wxFocusEvent& event );

	void OnMouseMotion( wxMouseEvent& event );
	void OnLeftDown( wxMouseEvent& event );
	void OnLeftUp( wxMouseEvent& event );
};