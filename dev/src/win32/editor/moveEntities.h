#pragma once

/// Special tool for moving entities
class CEdMoveEntity : public wxSmartLayoutPanel
{
	DECLARE_EVENT_TABLE();

public:
	CEdMoveEntity( wxWindow* parent );

public:
	void OnMove( wxCommandEvent& event );
	void OnCancel( wxCommandEvent& event );
};