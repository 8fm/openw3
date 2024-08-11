#pragma once

#include "dialogEditorPage.h"
#include "dialogEditorHandlerAware.h"

class CEdStorySceneLinePanel;
class CEdStorySceneElementPanel;
class CStorySceneSection;
class CStorySceneElement;
enum EStorySceneElementType;

class CEdSceneSectionPanel : public wxPanel, public IEdDialogHandlerAware
{
	DECLARE_EVENT_TABLE();

private:
	wxPanel* m_dialogPanel;
	wxPanel* m_linkPanel;
	wxTextCtrl* m_sectionNameField;
	wxStaticText* m_sectionFlagsStatic;
	wxStaticText* m_collapsePseudoButton;

	wxStaticText* m_expandPseudoButton;
	wxPanel*	m_postLinkPanel;

	CEdSceneEditorScreenplayPanel* m_storySceneEditor;
	CStorySceneSection* m_dialogSection;

	TDynArray< CEdStorySceneElementPanel* > m_dialogElements;
	TDynArray< wxControl* > m_controlsToThis;

	TDynArray< CEdSceneSectionPanel* > m_connectedSections;
	THashMap< CEdSceneSectionPanel*, wxControl* > m_previousSectionsLinks;
	THashMap< CEdSceneSectionPanel*, wxControl* > m_nextSectionsLinks;

	THashSet< CEdStorySceneElementPanel* > m_selectedElements;
	CEdStorySceneElementPanel* m_firstSelectedElement;

	Bool m_hasChoice;
	Bool m_hasQuestChoiceLine;

	CEdUndoManager* m_undoManager;
	
public:
	CEdSceneSectionPanel( wxWindow* parent, CEdSceneEditorScreenplayPanel* dialogEditor, CStorySceneSection* dialogSection, CEdUndoManager* undoManager );
	~CEdSceneSectionPanel();

	void CommitChanges();

	void RemovePreLink( CEdSceneSectionPanel* linkedSection );

	void AddPostLink( CEdSceneSectionPanel* linkedSection );
	void AddPreLink( CEdSceneSectionPanel* linkedSectionPanel, wxWindow* linkWindow );
	void AddOutputMarker( const String& outputName );

	CAbstractStorySceneLine* AddDialogLine( Int32 index = wxNOT_FOUND, Bool after = true );
	CStorySceneQuestChoiceLine* AddQuestChoice();
	CStorySceneComment* AddComment( Int32 index = wxNOT_FOUND, Bool after = true );
	CStorySceneScriptLine* AddScriptLine( Int32 index = wxNOT_FOUND, Bool after = true );
	CStorySceneChoice* AddChoice();

	Bool CanAddDialogLine( Int32 index, Bool after ) const;
	Bool CanAddRandomLine( Int32 index, Bool after ) const;
	Bool CanAddQuestChoice( Int32 index, Bool after ) const;
	Bool CanAddComment( Int32 index, Bool after ) const;
	Bool CanAddScriptLine( Int32 index, Bool after ) const;
	Bool CanAddChoice();

	Bool CanRemoveDialogElement( CEdStorySceneElementPanel* element ) const;
	Bool CanRemoveDialogElement( Uint32 elementIndex ) const;

	void RemoveDialogElement( Uint32 elementIndex );
	void RemoveDialogElement( CEdStorySceneElementPanel* element );
	void ChangeDialogElementType( CEdStorySceneElementPanel* element, EStorySceneElementType type );
	
	CEdStorySceneElementPanel* CreateAndAddStorySceneElementPanel( CStorySceneElement* element );
	CEdStorySceneElementPanel* CreateAndAddStorySceneElementPanel( CStorySceneElement* element, Int32 addLocationIndex );

	void RegisterControlToThis( wxControl* control );
	void UnregisterControlToThis( wxControl* control );

	virtual void SetFocus();

	void OnContextMenu( wxContextMenuEvent& event );
	void OnAddDialogChoice( wxCommandEvent& event );
	void OnRemoveSection( wxCommandEvent& event );

	Int32 FindDialogElementIndex( wxWindow* storySceneElement );
	Bool IsLastElementChild( wxWindow* window );
	Bool IsFirstElementChild( wxWindow* window );


	wxString GetSectionName();

	void SetDialogSection( CStorySceneSection* section );

	void RefreshData();
	void RefreshHelperData();
	Bool RefreshDialogObject( CObject* objectToRefresh );

	void CollapseSection();
	void ExpandSection();

	void RefreshConnectedSections();

	void ClearPreLinks();
	void ClearPostLinks();
	void RemoveLink( CEdSceneSectionPanel* nextSection );

	Bool IsElementSelected( CEdStorySceneElementPanel* element );
	void HandleElementSelection( CEdStorySceneElementPanel* element, Bool mouseMode = false );
	void ClearSelectedElements();

	void EnterElementSelectionMode( CEdStorySceneElementPanel* initialElement );
	void LeaveElementSelectionMode();
	RED_INLINE Bool IsInElementSelectionMode() { return m_selectedElements.Empty() == false; }

	CEdStorySceneElementPanel* GetElementAfter( CEdStorySceneElementPanel* element );
	CEdStorySceneElementPanel* GetElementBefore( CEdStorySceneElementPanel* element );
	CEdStorySceneElementPanel* GetPanelByElement( CStorySceneElement* element );

	wxWindow* GetFirstNavigableWindow() { return m_sectionNameField; }
	wxWindow* GetLastNavigableWindow();

	void MarkSectionAsSelected();
	void UnmarkSectionAsSelected();

	void ChangeFontSize( Int32 sizeChange );
	virtual bool SetBackgroundColour( const wxColour& colour );

	void GetWordCount( Uint32& totalWords, Uint32& commentWords ) const;

	RED_INLINE CStorySceneSection* GetSection() const { return m_dialogSection; }
	RED_INLINE CEdSceneEditorScreenplayPanel* GetStorySceneEditor() const { return m_storySceneEditor; }
	RED_INLINE Bool HasChoice() const { return m_hasChoice; }
	RED_INLINE Bool HasQuestchoiceLine() const { return m_hasQuestChoiceLine; }

	CName GenerateSpeakingToForLine( const CAbstractStorySceneLine* line ,Uint32 index ) const;
	void GenerateSpeakingToForSection();

protected:
	wxWindow* GetNextNavigableWindow( wxWindow* currentWindow );
	wxWindow* GetPrevNavigableWindow( wxWindow* currentWindow );

	// Debug visualization methods
public:
	void MarkDebugElement( const CStorySceneElement* element );

public:
	void RemoveSelectedElements();
	void OnCopyElements( wxCommandEvent& event );
	void OnCutElements( wxCommandEvent& event );
	void OnPasteElements( wxCommandEvent& event );

protected:
	Int32 AddDialogLineElement( CEdStorySceneElementPanel* storySceneElement, Int32 index, Bool after );	
	
	void RemoveControl( wxWindow* control );

	void CopyElements( Bool generateNewIDs = true );
	void CutElements();

	
	void PasteElementsAtLocation( Int32 pasteLocationIndex );

	void OnCommentEnter( wxCommandEvent& event );

	void OnAddDialogLine( wxCommandEvent& event );
	void OnAddComment( wxCommandEvent& event );
	void OnAddQuestChoice( wxCommandEvent& event );
	void OnAddScriptLine( wxCommandEvent& event );

	void OnSectionNameEnter( wxCommandEvent& event );
	void OnCharPressed( wxKeyEvent& event );
	void OnSectionNamePaste( wxClipboardTextEvent& event );
	
	void OnMaxFieldLength( wxCommandEvent& event );
	void OnNameFieldLostFocus( wxFocusEvent& event );

	void OnSectionFocus( wxFocusEvent& event );
	void OnSectionChildFocus( wxChildFocusEvent& event );

	void OnDelegatedMouseScroll( wxMouseEvent& event );
	
	void OnNameChanged( wxCommandEvent& event );

	void OnChildFocus( wxChildFocusEvent& event );

	void OnCollapseClicked( wxCommandEvent& event );
	void OnExpandClicked( wxCommandEvent& event );

	void OnCopySection( wxCommandEvent& event );
	void OnCutSection( wxCommandEvent& event );
	void OnPasteSection( wxCommandEvent& event );

	void OnShiftSectionPanelDown( wxCommandEvent& event );
	void OnShiftSectionPanelUp( wxCommandEvent& event );

	void OnNavigationRequest( wxNavigationKeyEvent& event );

	CEdStorySceneElementPanel* GetParentStorySceneElementPanel( wxWindow* window );
	void OnDoubleClick( wxMouseEvent& event );

public:
	virtual void OnSceneElementAdded( CStorySceneElement* element, CStorySceneSection* section );

private:
	void PutSelectionIntoUndoSteps( Bool finalize );
};
