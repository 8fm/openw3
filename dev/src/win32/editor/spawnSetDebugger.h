/**
 * Copyright © 2009 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#if 0

#include "../../games/witcher3/npcSpawnSetComponent.h"
#include "../../games/witcher3/actionPointComponent.h"

class CSpawnSetDebugger : public wxSmartLayoutPanel, public IEdEventListener
{
	DECLARE_EVENT_TABLE()

public:
	CSpawnSetDebugger( wxWindow* parent );
	~CSpawnSetDebugger();

	void SaveOptionsToConfig();
	void LoadOptionsFromConfig();

protected:
	void DispatchEditorEvent( const CName& name, IEdEventData* data );

private:
	void OnTimer( wxCommandEvent& event );
	void OnToolBar( wxCommandEvent &event );
	void OnSpawnSetListBoxItemSelected( wxCommandEvent &event );
	void OnSpawnSetListBoxItemDoubleClicked( wxCommandEvent &event );
	void OnSpawnSetEntryListBoxItemDoubleClicked( wxCommandEvent &event );
	void OnSpawnSetEntryListBoxItemSelected( wxCommandEvent &event );
	void OnActorsListBoxItemSelected( wxCommandEvent &event );
	void OnActorsListBoxItemDoubleClicked( wxCommandEvent &event );

	void UpdateGUIData();
	void UpdateListBox( wxListBox *listBox, const wxArrayString &data );
	void UpdateTextCtrl( wxTextCtrl *listCtrl, const wxString &data );

	// Do not call any GUI methods from Process* members
	void ProcessSpawnSetComponents();
	void ProcessSpawnSetEntries( int index );
	void ProcessActors( int indexSpawnSet, int indexSpawnSetEntry );
	void ProcessTimers( int indexSpawnSet, int indexSpawnSetEntry );
	void ProcessActor( int indexSpawnSet, int indexActor );

	String GetFriendlySpawnSetActorStateName( CNPCSpawnSetActorState actorState );
	String GetFriendlyActionCategoriesName( const TDynArray< CNPCActionCategory > &actionCategories );
	String GetFriendlyTagsArrayName( const TDynArray< TagList > &tagListArray );
	Bool IsTimeActive( const TDynArray< CSpawnSetTimetableSpawnEntry > &sstse, const GameTime &gameTime );

private:
	// GUI elements
	wxToolBar    *m_toolBar;
	wxListBox    *m_spawnSetListBox;
	wxListBox    *m_spawnSetEntryListBox;
	wxListBox    *m_actorsListBox;
	wxListBox    *m_timersListBox;
	wxListBox    *m_spawnInfoListBox;
	wxTextCtrl   *m_infoTextCtrl;
	wxStaticText *m_label1StaticText;
	wxStaticText *m_label2StaticText;
	wxStaticText *m_label3StaticText;
	wxStaticText *m_label4StaticText;
	wxStaticText *m_label5StaticText;
	wxStaticText *m_label6StaticText;
	const int     m_idStartTool;
	const int     m_idStopTool;

	// GUI data
	wxArrayString m_spawnSetListBoxData;
	wxArrayString m_spawnSetEntryListBoxData;
	wxArrayString m_actorsListBoxData;
	wxArrayString m_timersListBoxData;
	wxArrayString m_spawnInfoListBoxData;
	wxString      m_infoTextCtrlData;

	// Data associated with GUI
	TDynArray< const CNPCSpawnSetComponent::NPCControl * > m_actorsListBoxDataNPCControls;

	CEdTimer *m_timer;
};

#endif