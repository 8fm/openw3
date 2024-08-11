/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

class CEdUndoHistoryFrame : public wxFrame, public IEdUndoListener
{
	class CEdAutosizeListCtrl* m_stepsList;
	wxButton*				m_clearButton;
	wxButton*				m_undoButton;
	wxButton*				m_redoButton;
	CEdUndoManager*			m_undoManager;
	Bool					m_historyChangedWhileHidden;
	Int32					m_currentStepIndex;

private:
	void OnStepsListSelected( wxCommandEvent& event );
	void OnStepsListActivated( wxCommandEvent& event );
	void OnClearButtonClicked( wxCommandEvent& event );
	void OnUndoButtonClicked( wxCommandEvent& event );
	void OnRedoButtonClicked( wxCommandEvent& event );
	void OnClose( wxCloseEvent& event );
	void OnShow( wxShowEvent& event );

	Int32 GetSelectedStepIndex() const;
	void UpdateButtonStates();
	void RefreshHistoryNow();

	virtual void OnUndoHistoryChanged() override;

public:
	CEdUndoHistoryFrame( wxWindow* parent );
	virtual ~CEdUndoHistoryFrame();

	void SetUndoManager( CEdUndoManager* undoManager );
	RED_INLINE CEdUndoManager* GetUndoManager() const { return m_undoManager; }

	void RaiseAndFocus();
	void RefreshHistory( Bool ignoreVisibility = false );
};
