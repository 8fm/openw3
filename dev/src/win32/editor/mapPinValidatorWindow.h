/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#pragma once

class CEdMapPinValidatorWindow : public wxSmartLayoutPanel
{
	DECLARE_EVENT_TABLE();

public:

	CEdMapPinValidatorWindow( wxWindow* parent );
	~CEdMapPinValidatorWindow();

protected:

	wxListBox*		m_invalidList;
	wxStaticText*	m_foundText;

	void OnBeginSearching( wxCommandEvent& event );
	void OnItemDoubleClicked( wxCommandEvent& event );
	void OnShowDetails( wxCommandEvent& event );

	void ClearData();
};