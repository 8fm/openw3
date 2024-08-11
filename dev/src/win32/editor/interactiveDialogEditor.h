/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

class CInteractiveDialog;
class CIDTopic;
class CEdInteractiveDialogGraphEditor;
struct SIDLineStub;
class CEdScreenplayControl;

#include "idBlockOp.h"

#include "wxAUIRed.h"

class CEdInteractiveDialogEditor 
	: public wxSmartLayoutPanel
	, public IEdEventListener
	, public GraphEditorHook
{
	DECLARE_CLASS( CEdInteractiveDialogEditor );
	DECLARE_EVENT_TABLE();

	friend class CEdScreenplayControl;

private:
	CInteractiveDialog*					m_dialog;
	CIDTopic*							m_dialogTopic;
	CEdPropertiesPage*					m_dialogProperties;
	CEdPropertiesPage*					m_elementProperties;
	CEdInteractiveDialogGraphEditor*	m_graphEditor;
	Uint32								m_numTopics;

	THandle< IReferencable >			m_editedObject;

	wxButton*							m_upBtn;
	wxButton*							m_downBtn;
	wxButton*							m_addBtn;
	wxButton*							m_removeBtn;
	wxButton*							m_renameBtn;
	wxListBox*							m_topicsListBox;
	



	// < Dockable Layout >
	// **************************************************************
private:

	CEdAuiManager						m_auiManager;
	CEdAuiNotebook*						m_propertiesNotebook;
	CEdAuiNotebook*						m_editorNotebook;
	CEdAuiNotebook*						m_topicNotebook;
	CEdAuiNotebook*						m_graphNotebook;


	void OnLoadLayoutFromFile(	wxCommandEvent& event );
	void OnSaveLayoutToFile(	wxCommandEvent& event );

	// < / Dockable Layout >
	// **************************************************************


private:

	static const size_t					PAGE_INDEX_DIALOG_PROPERTIES;
	static const size_t					PAGE_INDEX_ITEM_PROPERTIES;
	static const size_t					PAGE_INDEX_SCREENPLAY;
	static const size_t					PAGE_INDEX_TIMELINE;

	TDynArray< SIDBlockEditorOp >		m_pendingBlockOperations;

	// < Screenplay >
	// **************************************************************
	CEdScreenplayControl*				m_screenplayCtrl;

	String								m_lastScreenplayContent;
	String								m_currentScreenplayContent;
	String								m_screenplayUnrecogizedText;
	
	Int32								m_screenplayCaretPos;
	Int32								m_screenplayLinePos;

	Bool								m_fillingScreenplayNow;				

	static const wxString				TOKEN_TEXT_BLOCK_BEGIN[];
	static const wxString				TOKEN_CHOICE_BLOCK_BEGIN[];
	static const wxString				PROPERTY_GUID_NAME;

	// < /Screenplay >
	// **************************************************************


public:
	CEdInteractiveDialogEditor( wxWindow* parent, CInteractiveDialog* dialog = NULL );

	~CEdInteractiveDialogEditor();

	virtual void DispatchEditorEvent( const CName& name, IEdEventData* data );
	void SetPropertiesObject( IReferencable* obj, Bool switchTab = true );

	static String AskForVOPath( wxWindow* editor );
	static void GenerateAllAudioAndLipsyncFiles();

	// GraphEditorHook
	virtual void OnGraphStructureWillBeModified( IGraphContainer* graph ) {}
	virtual void OnGraphStructureModified( IGraphContainer* graph );
	virtual void OnGraphSelectionChanged();

	void OnBlockSelected( CIDGraphBlock* block );

protected:
	void SaveOptionsToConfig();
	void LoadOptionsFromConfig();

	void OnDialogPropertyPanelChanged( wxCommandEvent& event );

	void OnTopicsListChanged();

	void OnDialogPropertyPanelPropertySelected( wxCommandEvent& event );
	void OnElementPropertyPanelChanged( wxCommandEvent& event );
	void OnElementPropertyPanelPropertySelected( wxCommandEvent& event );

	void OnMenuGenAudio	( wxCommandEvent& event );
	void OnMenuGenLipsync( wxCommandEvent& event );
	void OnMenuSave		( wxCommandEvent& event );
	void OnMenuExit		( wxCommandEvent& event );
	void OnMenuCopy		( wxCommandEvent& event );
	void OnMenuPaste	( wxCommandEvent& event );
	void OnMenuCut		( wxCommandEvent& event );
	void OnMenuDelete	( wxCommandEvent& event );
	void OnMenuUndo		( wxCommandEvent& event );

	void OnTopicAdd( wxCommandEvent& event );
	void OnTopicRemove( wxCommandEvent& event );
	void OnTopicRename( wxCommandEvent& event );
	void OnTopicUp( wxCommandEvent& event );
	void OnTopicDown( wxCommandEvent& event );
	void OnTopicSelect( wxCommandEvent& event );

	void OnClose( wxCloseEvent &event );

	void ChangeTopic( CIDTopic* newTopic );
	void RefreshTopicsList();

	void OnTopicSelectionChanged( Uint32 idx );

	void EnsureBlockNameUniqueness();

	void HiliteBlockUnderScreenplayCaret();

	void FillScreenplay();
	void FillScreenplayBlock( const wxRichTextProperties& props, const CIDGraphBlockText* block, Int32& blockLineIndex );
	void FillScreenplayBlock( const wxRichTextProperties& props, const CIDGraphBlockChoice* block, Int32& blockLineIndex );
	void WriteScreenplayText( const wxString& text, const wxRichTextProperties& props );
	void WriteScreenplayComment( const String& comment, CName blockName, Int32& blockLineIndex, const wxRichTextProperties& props );

	void OnBeginScreenplayPasting();
	void OnEndScreenplayPasting();
	
	void FetchScreenplayContent();
	Bool TestForScreenplayChange() const;
	void HandleSceenplayUserInput();
	void ApplyScreenplayChanges();
	void ApplyScreenplayChangesIfNeeded();

	void UpdateTextBlock( const CGUID& guid, const CName& name, const TDynArray< SIDLineStub >& stubs, const String& blockComment ); 
	void UpdateChoiceBlock( const CGUID& guid, const CName& name, const SIDOptionStub* stubs, const String& blockComment ); 
	
	Bool ValidateUpdate(); // this returns true if it's safe to do pending ops. This can clear pending ops table, depending on user's decision.
	void DoPendingBlockOperations();

	CIDGraphBlockText* DoAddTextBlock();
	CIDGraphBlockChoice* DoAddChoiceBlock();
	void OnDoCreateBlock( CIDGraphBlock* createdBlock );

	void DoUpdateTextBlockLines( CIDGraphBlockText* block, const TDynArray< SIDLineStub > &stubs );
	void DoUpdateChoiceBlockOptions( CIDGraphBlockChoice* block, const SIDOptionStub* stubs );
	void DoUpdateBlockComment( CIDGraphBlock* block, const String& blockComment );
	void DoUpdateBlockName( CIDGraphBlock* block, const CName& name );
	void DoDeleteBlock( CIDGraphBlock* block );

	void HandleScreenplayUserInput();
	void OnScreenplayCaretPositionChange( Int32 oldPos );
	void CheckRelativeCaretPosition( CName blockName, Int32 linePos );

	static void GenerateAudioFiles( const CInteractiveDialog* dialog, const String& path, Bool cookerMode );
	static void GenerateLipsyncFiles( const CInteractiveDialog* dialog, const String& path, Bool cookerMode );	
	static void GatherAllTextBlocks( const CInteractiveDialog* dialog, TDynArray< const CIDGraphBlockText* > &blockArray, Uint32 &totalLines );
};
