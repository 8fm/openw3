#pragma once

#include "dialogEditorChoice.h"

class CEdStorySceneChoiceLinePanel : public wxPanel, public IEdDialogHandlerAware, public IEdEventListener
{
	wxDECLARE_EVENT_TABLE();
	wxDECLARE_CLASS( CEdStorySceneChoiceLinePanel );

private:
	CEdStorySceneChoicePanel* m_choicePanel;
	CEdUndoManager*			  m_undoManager;
	CEdSceneEditorScreenplayPanel* m_storySceneEditor;

	wxObject* m_lastContextMenuObject;

	wxStaticText* m_indexLabel;
	wxStaticBitmap*	m_choiceActionIcon;
	wxStaticText*	m_choiceActionField;
	wxTextCtrl* m_choiceContentField;
	wxTextCtrl* m_choiceCommentField;
	wxPanel*	m_postLinkPanel;

	TDynArray< CEdSceneSectionPanel* > m_connectedSections;
	THashMap< CEdSceneSectionPanel*, wxControl* > m_nextSectionsLinks;
	
	CStorySceneChoiceLine* m_choiceLine;

public:
	//! Get the line represented by this panel
	RED_INLINE CStorySceneChoiceLine* GetChoiceLine() const { return m_choiceLine; }

public:
	CEdStorySceneChoiceLinePanel( CEdStorySceneChoicePanel* choicePanel, CEdUndoManager* undoManager, Uint32 lineIndex );
	~CEdStorySceneChoiceLinePanel();

	void SetFocus();

	wxString GetChoiceContent() const;
	wxString GetChoiceComment() const;

	void SetChoiceContent( const wxString& text );
	void SetChoiceLineIndex( Uint32 index );

	void SetChoiceLine( CStorySceneChoiceLine* choiceLine );
	void RestoreSectionLink();
	void RefreshData();
	virtual Bool RefreshDialogObject( CObject* objectToRefresh );

	void ChangeFontSize( Int32 sizeChange );
	virtual bool SetBackgroundColour( const wxColour& colour );

	void RefreshConnectedSections();

	void AddPostLink( CEdSceneSectionPanel* linkedSection );

	void OnSelected();
	void OnDeselected();

	wxWindow* GetFirstNavigableWindow() { return m_choiceCommentField->IsShown() ? m_choiceCommentField : m_choiceContentField; }
	wxWindow* GetLastNavigableWindow() { return m_choiceContentField; }

	Bool IsEmpty();

	void UpdateColors();

protected:
	void RemoveControl( wxWindow* control );

	void OnContextMenu( wxMouseEvent& event );
	void OnKeyPressed( wxKeyEvent& event );
	void OnPasteText( wxCommandEvent& event );
	void OnCutText( wxCommandEvent& event );
	void OnChoiceLinePaste( wxClipboardTextEvent& event );
	void OnEnterPressed( wxKeyEvent& event );
	void OnNavigate( wxNavigationKeyEvent& event );

	void OnLeftDown( wxMouseEvent& event );

	void OnAddChoiceLine( wxCommandEvent& event );
	void OnInsertChoiceLine( wxCommandEvent& event );
	void OnDeleteChoiceLine( wxCommandEvent& event );
	void OnMoveChoiceLineUp( wxCommandEvent& event );
	void OnMoveChoiceLineDown( wxCommandEvent& event );

	void OnCopy( wxCommandEvent& event );
	void OnCut( wxCommandEvent& event );
	void OnPaste( wxCommandEvent& event );

	void OnChoiceLineChanged( wxCommandEvent& event );
	void OnCommentLineChanged( wxCommandEvent& event );

	void OnChoiceLineLostFocus( wxFocusEvent& event );
	void OnChoiceLineChildFocus( wxChildFocusEvent& event );

	void OnMakeCopyUnique( wxCommandEvent& event );

	void NavigateDown( wxWindow* currentWindow );
	void NavigateUp( wxWindow* currentWindow );

protected:
	virtual void DispatchEditorEvent( const CName& name, IEdEventData* data );
};
