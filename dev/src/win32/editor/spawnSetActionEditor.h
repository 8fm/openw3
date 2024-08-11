/**
 * Copyright © 2007 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#if 0
BEGIN_DECLARE_EVENT_TYPES()
	DECLARE_EVENT_TYPE( wxEVT_SPAWNSETACTIONEDITOR_OK,     wxEVT_USER_FIRST + 1 )
	DECLARE_EVENT_TYPE( wxEVT_SPAWNSETACTIONEDITOR_CANCEL, wxEVT_USER_FIRST + 2 )
END_DECLARE_EVENT_TYPES()

class ISpawnSetTimetableAction;

class CEdSpawnSetActionEditor : public wxSmartLayoutDialog
{
	DECLARE_EVENT_TABLE()

private:
	CNPCSpawnSet              *m_sstActionParent;
	ISpawnSetTimetableAction  *m_sstAction;

	// GUI elements
	wxComboBox   *m_guiActionType;
	wxTextCtrl   *m_guiTextCtrl0;
	wxTextCtrl   *m_guiTextCtrl1;
	wxStaticText *m_guiTextCtrl0Label;
	wxStaticText *m_guiTextCtrl1Label;
	wxPanel      *m_guiPanelMain;
	
	void ChangeToActionCategory();

	// GUI events
	void OnOK( wxCommandEvent& event );
	void OnCancel( wxCommandEvent& event );
	void OnComboBox( wxCommandEvent& event );

public:
	CEdSpawnSetActionEditor( wxWindow* parent, CNPCSpawnSet *actionParent, ISpawnSetTimetableAction *sstActionInit );
	~CEdSpawnSetActionEditor();

	// This method transfers the ownership of the m_sstActionEntry
	// Use this method only after OnOK(), not after OnCancel()
	ISpawnSetTimetableAction* GetSSTAction();
};
#endif