#pragma once

#include "dialogHandlerFactory.h"
#include "dialogEditor.h"

class CEdSceneSectionPanel;
class CEdStorySceneChoiceLinePanel;
class CStoryScene;
class CStorySceneSection;
class CStorySceneElement;
class CEdSceneGraphEditor;
class CEdDialogTimeline;
class CEdDialogPreview;

class CStoryScenePlayer;

class CEdSceneEditorScreenplayPanel : public wxPanel, public IEdEventListener
{
	DECLARE_CLASS( CEdSceneEditorScreenplayPanel );
	DECLARE_EVENT_TABLE();

private:
	CEdStorySceneHandlerFactory m_handlerFactory;
	
	CEdSceneEditor*								m_sceneEditor;
	CEdSceneEditor*								m_mediator;
	
	wxScrolledWindow*							m_dialogTextPanel;
	wxStaticText*								m_statusLabel;

	TDynArray< CEdSceneSectionPanel* >			m_sectionPanels;
	CEdSceneSectionPanel*						m_currentSectionPanel;

	TDynArray< CEdStorySceneChoiceLinePanel* >	m_choiceLines;

	THashSet< CEdSceneSectionPanel* >			m_selectedSections;
	CEdSceneSectionPanel*						m_sectionPanelHandlingElementSelection;

	Uint32										m_changeElementsCounter;
	Int32										m_fontSize;
	
	Bool										m_isInitialized;
	Bool										m_isRefreshInProgress;
	Bool										m_shouldIgnoreLinkUpdateOnElementDelete;

	Bool										m_showOnlyScriptTexts;

	CEdTimer*									m_wordCountTimer;

	CEdUndoManager*								m_undoManager;

public:
	CEdSceneEditorScreenplayPanel( wxWindow* parent, CEdSceneEditor* sceneEditor );
	~CEdSceneEditorScreenplayPanel(void);

	void CommitChanges();

	void AddSectionPanel( CStorySceneSection* section );
	void RemoveSectionPanel( CEdSceneSectionPanel* sectionPanel );

	void SetCurrentSection( CStorySceneSection* section );

	CEdSceneSectionPanel* GetSectionPanel( Uint32 index );
	CEdSceneSectionPanel* FindSectionPanel( const CStorySceneSection* section );
	CEdSceneSectionPanel* GetSectionPanelAfter( CEdSceneSectionPanel* sectionPanel );
	CEdSceneSectionPanel* GetSectionPanelBefore( CEdSceneSectionPanel* sectionPanel );
	Int32 GetSectionPanelIndex( CEdSceneSectionPanel* sectionPanel );
	Bool IsFirstSectionPanel( CEdSceneSectionPanel* sectionPanel );
	Bool IsLastSectionPanel( CEdSceneSectionPanel* sectionPanel );
	Bool ShiftSectionPanel( CEdSceneSectionPanel* sectionPanel, Bool isMovingDown );
	Bool MoveSectionPanel( CEdSceneSectionPanel* sectionPanel, Uint32 newSectionPanelIndex );
	Bool MoveSectionPanel( const CStorySceneSection* section, Uint32 newSectionPanelIndex );

	void RegisterChoiceLine( CEdStorySceneChoiceLinePanel* choiceLine );
	void UnregisterChoiceLine( CEdStorySceneChoiceLinePanel* choiceLine );
	void UpdateAvailableSections();
	void AnalizeSection( CStorySceneSection* section );

	void ReloadScreenplay();

	void FocusOnFirstSection();
	void RefreshDialog();
	void ReinitializeSceneScript();
	Bool RefreshDialogObject( CObject* objectToRefresh );
	void RefreshHelperData();

	void PreChangeSceneElements();
	void PostChangeSceneElements( wxWindow* windowToRefreshLayout = NULL );
	void ScrollDialogTextToShowElement( wxWindow* dialogTextElement );

	void SetSectionPanelHandlingElementSelection( CEdSceneSectionPanel* sectionPanel );	

	void HandleSectionSelection( CEdSceneSectionPanel* sectionPanel );
	void ClearSectionSelection();

	void ChangeWindowFontSize( wxWindow* window, Int32 sizeChange );
	void ChangeFontSize( Int32 sizeChange );
	void ToggleShowOnlyScriptTexts();
	void EnableShowOnlyScriptTexts( Bool enable /* = true */ );
	Bool IsShowOnlyScriptTextsEnabled() const;
	void RefreshWordCount();

	
	RED_INLINE Int32 GetFontSize() const { return m_fontSize; }


	void OnAddSection( wxCommandEvent& event );
	void OnAddCutscene( wxCommandEvent& event );

	void SetUndoManager( CEdUndoManager* undoManager ) { m_undoManager = undoManager; }

	void RemoveSection( CStorySceneSection* section );

public:
	RED_INLINE CEdStorySceneHandlerFactory*	GetHandlerFactory() { return &m_handlerFactory; }
	RED_INLINE wxScrolledWindow*				GetDialogTextPanel() const { return m_dialogTextPanel; }
	RED_INLINE CEdSceneEditor*				GetSceneEditor() const { return m_sceneEditor; }
	RED_INLINE CStoryScene*					HACK_GetStoryScene() const { return m_sceneEditor->HACK_GetStoryScene(); }
	RED_INLINE CEdSceneGraphEditor*			HACK_GetSceneGraphEditor() const { return m_sceneEditor->HACK_GetSceneGraphEditor(); }
	RED_INLINE CEdDialogPreview*				HACK_GetPreview() const { return m_sceneEditor->HACK_GetScenePreview();}

public:
	RED_INLINE Bool IsInitialized() const { return m_isInitialized; }
	RED_INLINE Bool ShouldIgnoreLinkUpdateOnElementDelete() const { return m_shouldIgnoreLinkUpdateOnElementDelete; }
	RED_INLINE Bool ShouldShowOnlyScriptTexts() const { return m_showOnlyScriptTexts; }

	void OnViewportTick( IViewport* view, Float timeDelta );

	void UpdateTimeline();
	CName GetPrevSpeakerName( const CStorySceneElement* currElement );
	const CStorySceneLine* GetPrevLine( const CStorySceneElement* currElement );

public:
	void OnElementPanelFocus( const CStorySceneSection* section, CStorySceneElement* e );
	void OnChoiceLineChildFocus( const CStorySceneSection* section, CStorySceneChoiceLine* line );
	void OnSectionPanelSectionFocus( const CStorySceneSection* section );
	void OnSectionPanelSectionChildFocus( CStorySceneSection* section );
	void OnChoicePanelClick( CStorySceneChoice* ch );

protected:
	void DispatchEditorEvent( const CName& name, IEdEventData* data );

	void OnTextPanelClick( wxMouseEvent& event );
	void OnTextPanelContextMenu( wxMouseEvent& event );

	void OnNextSection( wxCommandEvent& event );
	void OnPrevSection( wxCommandEvent& event );
	void OnChangeSection( wxCommandEvent& event );
	
	void OnExit( wxCommandEvent& event );

	void OnRefreshDialog( wxCommandEvent& event );
	void OnResetStringDbIds( wxCommandEvent& event );

	void OnWordCountTimer( wxTimerEvent& event );

	void OnBtnApplyScreenplayChanges( wxCommandEvent& event );

public:
	void DoEditDelete();
	void OnEditDelete( wxCommandEvent& event );
	void OnEditCopy( wxCommandEvent& event );
	void OnEditCut( wxCommandEvent& event );
	void OnEditPaste( wxCommandEvent& event );

	void OnGenerateSpeakingToData( wxCommandEvent& event );
private:
	void UpdateScene( Float dt );
};
