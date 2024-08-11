#pragma once

#include "textControl.h"
#include "..\..\common\game\storySceneAbstractLine.h"

class CStoryScene;

class CEdStorySceneLineSpeakerInput : public CEdTextControl
{
	wxDECLARE_CLASS( CEdStorySceneLineSpeakerInput );

public:
	CEdStorySceneLineSpeakerInput( CStoryScene* scene, wxWindow* parent, long style );
	~CEdStorySceneLineSpeakerInput();

	wxListBox* CreateNewAutoCompleteBox();
	void CloseAutoComplete();
	void FilterContents( const wxString& prefix, wxArrayString& filteredContents );
	wxPoint CalculateAutoCompleteBoxPosition();
	void ChangeAutoCompleteBoxSelection( Bool selectNext ); 
	void RefreshAutoCompleteBoxContents();
	void TriggerAutoCompleteBoxSelectionEvent();
	void OnStringUpdated( wxCommandEvent& event );
	void OnAutoCompleteBoxFocusLost( wxEvent& event );
	void OnCharPressed( wxKeyEvent& event );
	void OnAutoCompleteBoxSelectionChange( wxCommandEvent& event );
	Bool CanAppendCharacter( wxChar character );
	Bool IsAlwaysAllowedKey( Int32 keycode );
	void OnAutoCompleteMouseWheel( wxMouseEvent& event );
	virtual void GetAutoCompleteContents( wxArrayString& autoCompleteContents );
	
	virtual Bool CustomArrowTraverseRule( wxKeyEvent & event);

	RED_INLINE Bool IsAutoCompleteBoxOpened() { return m_autoCompleteBox != NULL; }

protected:
	CStoryScene* m_scene;

	wxListBox*	m_autoCompleteBox;
	wxDialog*	m_autoCompleteDialog;
	
	Bool m_isDeleting;
	Int32 m_lastCursorPosition;
};

class CEdStorySceneLineSpeakingToInput : public CEdStorySceneLineSpeakerInput
{
public:
	CEdStorySceneLineSpeakingToInput( const CAbstractStorySceneLine* dialogLine, CStoryScene* scene, wxWindow* parent, long style )
		: CEdStorySceneLineSpeakerInput( scene, parent, style )
		, m_dialogLine(dialogLine)
	{}
	const CAbstractStorySceneLine* m_dialogLine;
	virtual void GetAutoCompleteContents( wxArrayString& autoCompleteContents );
};

