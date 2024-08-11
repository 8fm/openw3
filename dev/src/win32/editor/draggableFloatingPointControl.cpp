/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "draggableFloatingPointControl.h"

void CEdDraggableFloatValueEditor::Init( wxTextCtrl* textCtrl, Float* data, CObject* parentObject /*= NULL*/ )
{
	m_parentObject = parentObject;
	m_text = textCtrl;
	m_data = data;

	wxTextValidator validator( wxFILTER_NUMERIC | wxFILTER_EXCLUDE_LIST );
	wxArrayString excludes;
	excludes.Add( TXT(",") );
	validator.SetExcludes( excludes );

	m_text->SetValidator( validator );
	m_text->SetValue( ToString( *m_data ).AsChar() );
	m_text->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( CEdDraggableFloatValueEditor::OnTextChanged ), NULL, this );
	m_text->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( CEdDraggableFloatValueEditor::OnTextFocusLost ), NULL, this );

	m_text->Connect( wxEVT_MOTION, wxMouseEventHandler( CEdDraggableFloatValueEditor::OnMouseMotion ), NULL, this );
	m_text->Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( CEdDraggableFloatValueEditor::OnLeftDown ), NULL, this );
	m_text->Connect( wxEVT_LEFT_UP, wxMouseEventHandler( CEdDraggableFloatValueEditor::OnLeftUp ), NULL, this );
}

void CEdDraggableFloatValueEditor::OnTextChanged( wxCommandEvent& event )
{
	if ( m_parentObject )
	{
		m_parentObject->MarkModified();
	}
	FromString< Float >( m_text->GetValue().wc_str(), *m_data );
	//RED_LOG( Info, TXT("OnTextChanged") );
}

void CEdDraggableFloatValueEditor::OnTextFocusLost( wxFocusEvent& event )
{
	m_text->ChangeValue( ToString( *m_data ).AsChar() );
	event.Skip();
	//RED_LOG( Info, TXT("OnTextFocusLost") );
}

void CEdDraggableFloatValueEditor::OnMouseMotion( wxMouseEvent& event )
{
	wxPoint newMousePosition = wxGetMousePosition();
	wxPoint mouseDelta =  newMousePosition - m_mousePressedPosition;

	if ( m_mode == MODE_CHECKING )
	{
		if ( wxPoint2DInt( mouseDelta ).GetVectorLength( ) > 10 )
		{
			if ( m_parentObject && !m_parentObject->MarkModified() )
			{
				m_mode = MODE_INACTIVE;
				return;
			}
			m_mode = MODE_DRAGGING;
			m_mousePressedPosition = newMousePosition;
			m_text->SetCursor( wxCURSOR_SIZENS );
			m_text->CaptureMouse();
			//RED_LOG( Info, TXT("OnMouseMotion - start dragging") );
		}
	}
	else if ( m_mode == MODE_DRAGGING )
	{
		m_mousePressedPosition = newMousePosition;

		// 1 percent of current value per pixel
		Float stepPerPix = Abs(*m_data) / 100.0f;
		stepPerPix =  Max <Float> ( stepPerPix, m_precision );

		*m_data -= (Float) mouseDelta.y * stepPerPix;

		// clamp to range
		*m_data = Clamp<Float>( *m_data, 0.0f, 100.0f );		

		m_text->ChangeValue( ToString( *m_data ).AsChar() );

		//RED_LOG( Info, TXT("OnMouseMotion - dragging") );
	}
	event.Skip();
}

void CEdDraggableFloatValueEditor::OnLeftDown( wxMouseEvent& event )
{
	m_mode = MODE_CHECKING;
	m_mousePressedPosition = wxGetMousePosition();
	event.Skip();
	//RED_LOG( Info, TXT("OnLeftDown") );
}

void CEdDraggableFloatValueEditor::OnLeftUp( wxMouseEvent& event )
{
	m_mode = MODE_INACTIVE;
	m_text->ReleaseMouse();
	m_text->ReleaseMouse(); // Hell yeah!
	m_text->SetCursor( wxNullCursor );
	//RED_LOG( Info, TXT("OnLeftUp") );
}