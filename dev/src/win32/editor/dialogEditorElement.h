#pragma once

#include "dialogEditorSection.h"
#include "dialogEditor.h"

enum EStorySceneElementType;

class CEdStorySceneElementPanel : public wxPanel, public IEdEventListener
{
	wxDECLARE_CLASS( CEdStorySceneElementPanel );
	wxDECLARE_EVENT_TABLE();

protected:
	CEdSceneSectionPanel* m_sectionPanel;
	CEdUndoManager*		  m_undoManager;
	wxObject* m_lastContextMenuObject;
	EStorySceneElementType m_elementType;
	
public:
	CEdStorySceneElementPanel( wxWindow* parent, CEdSceneSectionPanel* sectionPanel, CEdUndoManager* undoManager, EStorySceneElementType type );
	virtual ~CEdStorySceneElementPanel(void);

	void CommitChanges();

	void EnableShortcuts( wxWindow* window, Bool addLineOnEnter = true );
	virtual Bool IsEmpty() = 0;
	virtual void RefreshData() = 0;
	virtual void SetStorySceneElement( CStorySceneElement* storySceneElement ) = 0;
	virtual CStorySceneElement* GetDialogElement() = 0;

	virtual void RefreshHelperData() {}

	void OnDeleteLine( wxCommandEvent& event );

	void MarkElementAsSelected();
	void UnmarkElementAsSelected();
	Bool IsSelected();
	virtual void OnSelected() {};
	virtual void OnDeselected() {};

	virtual void GetWordCount( Uint32& contentWords, Uint32& commentWords ) const { contentWords = 0; commentWords = 0; }
	virtual Bool RefreshDialogObject( CObject* objectToRefresh );

	virtual bool SetBackgroundColour( const wxColour& colour );

	inline EStorySceneElementType GetElementType() const { return m_elementType; }
	
	virtual void ChangeFontSize( Int32 sizeChange ) {}
	virtual wxWindow* GetFirstNavigableWindow() { return NULL; }
	virtual wxWindow* GetLastNavigableWindow() { return NULL; }

	virtual bool Destroy();

	CEdSceneSectionPanel* GetSectionPanel();

protected:
	// Variable to prevent recurrent gui updates
	Bool	m_updatingGui;

	virtual EStorySceneElementType NextElementType();
	virtual void FillContextMenu( wxMenu& contextMenu );
	
	Bool ConfirmElementChange();
	Uint32 GetStringWordCount( const String& text ) const;

	void OnDestroy();
	
	void OnCopy( wxCommandEvent& event );
	void OnCut( wxCommandEvent& event );
	void OnPaste( wxCommandEvent& event );

	void OnAddDialogLine( wxCommandEvent& event );
	void OnInsertDialogLine( wxCommandEvent& event );
	void OnAddComment( wxCommandEvent& event );
	void OnInsertComment( wxCommandEvent& event );
	void OnAddScriptLine( wxCommandEvent& event );
	void OnInsertScriptLine( wxCommandEvent& event );
	void OnCycleElementType( wxCommandEvent& event );
	void OnPasteElements( wxCommandEvent& event );

	void OnChangeElementToLine( wxCommandEvent& event );
	void OnChangeElementToComment( wxCommandEvent& event );
	void OnChangeElementToScriptLine( wxCommandEvent& event );
	void OnChangeElementToChoice( wxCommandEvent& event );
	void OnChangeElementToQuestChoiceLine( wxCommandEvent& event );

	void OnCharPressed( wxKeyEvent& event );

	void EmulateMenuEvent( Int32 menuItemId );
	void OnEnterPressed( wxKeyEvent& event );

	void OnElementContextMenu( wxContextMenuEvent& event );
	void OnElementChildFocus( wxChildFocusEvent& event );
	void OnElementLeftClick( wxMouseEvent& event );
	void OnLeftDownInElementChild( wxMouseEvent& event );

	void OnNavigationRequest( wxNavigationKeyEvent& event );
	void OnPanelSelected( wxMouseEvent& event );

private:
	virtual void DispatchEditorEvent( const CName& name, IEdEventData* data ) override;
	virtual void ImplCommitChanges() = 0;
};

RED_INLINE CEdSceneSectionPanel* CEdStorySceneElementPanel::GetSectionPanel()
{
	return m_sectionPanel;
}
