#pragma once

#include "dialogEditorElement.h"
#include "textControl.h"

class CStorySceneComment;

class CEdStorySceneQuestChoicePanel : public CEdStorySceneElementPanel, public IEdDialogHandlerAware
{
	DECLARE_EVENT_TABLE()

private:
	CStorySceneComment* m_questChoiceLine;

protected:
	CEdTextControl* m_choiceLineField;
	CEdTextControl*	m_targetScenePathField;

public:
	CEdStorySceneQuestChoicePanel( wxWindow* parent, CEdSceneSectionPanel* sectionPanel, CEdUndoManager* undoManager, CStorySceneComment* dialogComment = NULL );

	void InitializeElementHandling();
	~CEdStorySceneQuestChoicePanel(void);

	Bool IsEmpty();
	virtual void SetFocus();

	virtual void SetStorySceneElement( CStorySceneElement* storySceneElement );
	virtual CStorySceneElement* GetDialogElement();
	virtual void RefreshData();

	virtual void ChangeFontSize( Int32 sizeChange );

	virtual wxWindow* GetFirstNavigableWindow() { return m_choiceLineField; }
	virtual wxWindow* GetLastNavigableWindow() { return m_choiceLineField; }

	virtual void OnSelected();
	virtual void OnDeselected();

protected:
	virtual EStorySceneElementType NextElementType();
	virtual void ConnectHandlers( CEdStorySceneHandlerFactory* handlerFactory );
	virtual void FillContextMenu( wxMenu& contextMenu );

	void OnCommentChar( wxKeyEvent& event );
	void OnCommentLostFocus( wxFocusEvent& event );
	void OnCommentChanged( wxCommandEvent& event );

	void OnMakeCopyUnique( wxCommandEvent& event );
	void OnFindTargetScene( wxCommandEvent& event );
	void OnTargetDoubleClick( wxMouseEvent& event );

	String FindInjectTargetScenePath() const;
	virtual bool SetBackgroundColour( const wxColour& colour );
	void UpdateColors();

private:
	virtual void ImplCommitChanges() override;
};
