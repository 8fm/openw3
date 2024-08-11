#pragma once

#include "undoQuestGraph.h"
#include "questEdTool.h"

class CEdGraphEditor;
class CEdQuestGraphEditor;
class CQuestPhase;
class IQuestEdTool;
class CQuestsSystem;
class IQuestDebugInfo;

class CQuestEditorClipboard: public IGraphEditorClipboard
{
public:
	virtual void Copy( CEdGraphEditor* editor );
	virtual void Paste( CEdGraphEditor* editor );

	Bool ParseQuestViewString( const String& viewString, String& filename, TDynArray< CGUID >& blockGuids, Float& scale, Float& offsetX, Float& offsetY ) const;
	Bool OpenQuestEditor( const String& filename, const TDynArray< CGUID >& blockGuids, Float scale, Float offsetX, Float offsetY );
};

class CEdQuestEditor : public wxFrame, public GraphEditorHook, public IEdEventListener
{
	wxDECLARE_EVENT_TABLE();
	wxDECLARE_CLASS( CEdQuestEditor );

private:
	enum EToolStateIndicator
	{
		QDT_IMG_OK,
		QDT_IMG_ERROR,
	};

private:
	// UI elements
	CEdPropertiesPage*				m_propertiesBrowser;
	CEdQuestGraphEditor*			m_questGraphEditor;
	wxSplitterWindow*				m_mainSplitter;
	wxPanel*						m_topPanel;

	wxNotebook*						m_toolsNotebook;

	// Internal elements
	CQuestPhase*					m_questPhase;
	CEdUndoManager*					m_undoManager;

	TDynArray< IQuestEdTool* >		m_tools;
	IQuestDebugInfo*				m_defaultDebugInfo;
	IQuestDebugInfo*				m_debugInfo;
	
	Bool							m_setFocus;

	static CQuestEditorClipboard	m_questClipboard;

public:

	CEdQuestEditor( wxWindow* parent, CQuestPhase* questPhase, const TDynArray< IQuestEdTool* >& tools = TDynArray< IQuestEdTool* >() );
	~CEdQuestEditor();

	// IEdEventListener
	virtual void DispatchEditorEvent( const CName& name, IEdEventData* data );

	CQuestsSystem* GetQuestsSystem();

	const CQuestPhase* GetQuestPhase() { return m_questPhase; }
	void SetQuestPhase( CQuestPhase* questPhase );
	Bool OpenBlock( const CGUID& blockGuid, Bool enterPhase = true );
	void OpenBlock( CQuestGraphBlock* block, Bool enterPhase = true );

	void OnCreateBlockContextMenu( TDynArray< SToolMenu >& subMenus, const CQuestGraphBlock* atBlock );

	// Sets a new debug info provider
	RED_INLINE void SetDebugInfo( IQuestDebugInfo& info ) { m_debugInfo = &info; }

	// Retrieves a reference to the currently set debug info provider
	RED_INLINE IQuestDebugInfo* GetDebugInfo() { return m_debugInfo; }

	// Immediately repaints the editor's canvas
	void Repaint();

	// A tool for listing all editable block classes
	static void GetEditableBlockClasses( const CQuestGraph* parentGraph, TDynArray< CClass* >& outClasses );

	// -------------------------------------------------------------------------
	// Notifications
	// -------------------------------------------------------------------------
	// Called when the user enters a different graph level
	void OnGraphSet( CQuestGraph& graph );

	// A notification from a registered tool that asks the editor 
	// to somehow indicate that the tab contains an error information
	void SetToolErrorIndicator( const IQuestEdTool& tool );

	// War user about test blocks
	void ShowTestQuestWarning();

	void SetGraphEditorScaleAndOffset( Float scale, const wxPoint& offset );
	RED_INLINE void ScheduleSetFocus() { m_setFocus = true; }

	static CQuestEditorClipboard& GetQuestEditorClipboard() { return m_questClipboard; }

	// check if all of the socket IDs of inputs are unique and fix them if not
	void RepairDuplicateInputs();

protected:

	void SaveOptionsToConfig();
	void LoadOptionsFromConfig();

	//! GraphEditorHook interface
	virtual void OnGraphSelectionChanged();
	virtual void OnGraphStructureModified( IGraphContainer* graph );

	// Event handlers
	void OnPropertiesChanged( wxCommandEvent& event );
	void OnSave( wxCommandEvent& event );
	void OnSaveAll( wxCommandEvent& event );
	void OnExit( wxCommandEvent& event );
	void OnEditUndo( wxCommandEvent& event );
	void OnEditRedo( wxCommandEvent& event );
	void OnEditCopy( wxCommandEvent& event );
	void OnEditPaste( wxCommandEvent& event );
	void OnEditCut( wxCommandEvent& event );
	void OnFlowSequence( wxCommandEvent& event );
	void OnFind( wxCommandEvent& event );
	void OnCopyQuestView( wxCommandEvent& event );
	void OnPasteQuestView( wxCommandEvent& event );
	void OnCleanUnusedBlocks( wxCommandEvent& event );
	void OnUpdateGUIDs( wxCommandEvent& event );
	void OnUpdateCommunityGUIDs( wxCommandEvent& event );
	void OnUpgrade( wxCommandEvent& event );
	void OnCleanSourceData( wxCommandEvent& event );
	void OnToolsPageChanged( wxNotebookEvent& event );
	void OnSaveDependencyInfo( wxCommandEvent& event );
	void OnCookSceneData( wxCommandEvent& event );

	void CheckForDuplicateSockets() const;
};
