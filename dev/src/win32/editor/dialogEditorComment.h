#pragma once

#include "dialogEditorElement.h"
#include "textControl.h"

class CStorySceneComment;

class CEdStorySceneCommentPanel : public CEdStorySceneElementPanel, public IEdDialogHandlerAware
{
	wxDECLARE_EVENT_TABLE();
	wxDECLARE_CLASS( CEdStorySceneCommentPanel );

private:
	CStorySceneComment* m_comment;

protected:
	CEdTextControl* m_commentField;

public:
	CEdStorySceneCommentPanel( wxWindow* parent, CEdSceneSectionPanel* sectionPanel, CEdUndoManager* undoManager, CStorySceneComment* dialogComment = NULL, Bool readOnly = false );

	void InitializeElementHandling();
	~CEdStorySceneCommentPanel(void);

	virtual Bool IsEmpty();
	virtual void SetFocus();

	virtual void SetStorySceneElement( CStorySceneElement* storySceneElement );
	virtual CStorySceneElement* GetDialogElement();
	virtual void RefreshData();

	virtual void ChangeFontSize( Int32 sizeChange );

	virtual wxWindow* GetFirstNavigableWindow() { return m_commentField; }
	virtual wxWindow* GetLastNavigableWindow() { return m_commentField; }

	virtual void GetWordCount( Uint32& contentWords, Uint32& commentWords ) const;

	virtual void OnSelected();
	virtual void OnDeselected();

protected:
	virtual EStorySceneElementType NextElementType();
	virtual void ConnectHandlers( CEdStorySceneHandlerFactory* handlerFactory );
	virtual void FillContextMenu( wxMenu& contextMenu );

	virtual void OnCommentChar( wxKeyEvent& event );
	virtual void OnCommentPaste( wxCommandEvent& event );
	virtual void OnCommentCut( wxCommandEvent& event );
	virtual void OnCommentFocusLost( wxFocusEvent& event );

	void OnMakeCopyUnique( wxCommandEvent& event );

	virtual bool SetBackgroundColour( const wxColour& colour );
	void UpdateColors();

private:
	virtual void ImplCommitChanges() override;
};
