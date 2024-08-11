#pragma once

#include "dialogEditorElement.h"

class CEdStorySceneInjectedChoiceLinePanel;
class CEdStorySceneChoiceLinePanel;
class CStorySceneChoice;

class CEdStorySceneChoicePanel : public CEdStorySceneElementPanel
{
	DECLARE_EVENT_TABLE()

public:
	typedef TDynArray< CEdStorySceneChoiceLinePanel* > TChoiceLinePanelArray;

private:
	TChoiceLinePanelArray	m_choiceLines;
	
	CStorySceneChoice*		m_storySceneChoice;
	CQuestGraph*			m_questGraphWithInjectedChoices;
	wxBitmapButton*			m_findContextChoicesBtn;
	wxStaticText*			m_choiceElementLabel;
	wxSizer*				m_injectedChoiceLinesSizer;
	wxSizer*				m_regularChoiceLinesSizer;

public:
	CEdStorySceneChoicePanel( wxWindow* parent, CEdSceneSectionPanel* sectionPanel, CEdUndoManager* undoManager );
	~CEdStorySceneChoicePanel();

	Bool IsEmpty();
	virtual void RefreshData();
	virtual Bool RefreshDialogObject( CObject* objectToRefresh );

	void SetFocus();

	virtual void SetStorySceneElement( CStorySceneElement* storySceneElement );

	virtual CStorySceneElement* GetDialogElement();

	CStorySceneChoiceLine* AddChoiceLine( Int32 index = wxNOT_FOUND, Bool after = true );
	Int32 AddChoiceLinePanel( Int32 index, Bool after, CEdStorySceneChoiceLinePanel* choiceLine );
	void CreateAndAddChoiceLine( Int32 index, CStorySceneChoiceLine* choiceLine );
	void RemoveChoiceLine( CEdStorySceneChoiceLinePanel* choiceLine );
	void RemoveChoiceLine( Int32 index );
	
	void UpdateChoiceLineIndices();
	Int32 FindChoiceLineIndex( CEdStorySceneChoiceLinePanel* choiceLine );
	Bool MoveChoiceLine( CEdStorySceneChoiceLinePanel* choiceLinePanel, Bool moveDown );
	

	virtual void ChangeFontSize( Int32 sizeChange );
	
	virtual void OnSelected();
	virtual void OnDeselected();

	virtual wxWindow* GetFirstNavigableWindow();
	virtual wxWindow* GetLastNavigableWindow();

	void OnChoiceLineChanged( wxCommandEvent& event );

	RED_INLINE CEdSceneSectionPanel* GetSectionPanel() const { return m_sectionPanel; }
	RED_INLINE const TChoiceLinePanelArray& GetChoiceLines() const { return m_choiceLines; }
	RED_INLINE TChoiceLinePanelArray& GetChoiceLines() { return m_choiceLines; }

	virtual void GetWordCount( Uint32& contentWords, Uint32& commentWords ) const;

public:
	void NavigateToNextLine( CEdStorySceneChoiceLinePanel* currentLine );
	void NavigateToPrevLine( CEdStorySceneChoiceLinePanel* currentLine );

protected:
	virtual EStorySceneElementType NextElementType();
	virtual void FillContextMenu( wxMenu& contextMenu );

	void OnFindContextChoices( wxCommandEvent& event );
	void OnAddChoiceLine( wxCommandEvent& event );
	void OnNavigate( wxNavigationKeyEvent& event );
	void OnClick( wxMouseEvent& event );

private:
	void UpdateQuestGraphSelection();
	void RefreshInjectedDialogs();
	void FindContextDialogBlocks( CQuestGraph& questGraph, TDynArray< CQuestContextDialogBlock* >& outBlocks ) const;

	virtual void ImplCommitChanges() override;
};
