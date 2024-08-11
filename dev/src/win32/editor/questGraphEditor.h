/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CEdQuestEditor;
class CVersionControlIconsPainter;
class IQuestBlockWithScene;
class CEdQuestGraphUndoManager;

class CEdQuestGraphEditor : public CEdGraphEditor
{
	friend class CEdQuestGraphUndoManager;

	DECLARE_EVENT_TABLE()
	
private:
	IGraphContainer*				m_nullGraph;

	CEdQuestEditor*					m_editor;
	CVersionControlIconsPainter*	m_vciPainter;
	wxPoint							m_mousePosition;

	TDynArray< CQuestGraph* >		m_phasesStack;

	bool							m_showFlowSequence;

	Int32							m_pressedKeyCode;
	Uint32							m_unusedBlocks;
	Bool							m_checkedForErrors;
	THashMap< Char, TPair< CClass*, CClass* > >	m_shortcuts;

public:
	CEdQuestGraphEditor( wxWindow* parent, CEdQuestEditor *editor );
	~CEdQuestGraphEditor();

	RED_INLINE const TDynArray< CQuestGraph* >& GetPhasesStack() const { return m_phasesStack; }

	CQuestGraph* GetTopLevelGraph();
	CObject* GetCurrentPhaseRoot();
	void GetAllPhases( CQuestGraph *graph, TDynArray< CQuestPhase * >& phasesArray );

	void SetQuestGraph( CQuestGraph* graph );

	// Focuses the editor on the selected block. In case of a phase block,
	// the second param allows to decide whether the block should be 
	// focused on or entered.
	Bool OpenBlock( const CGUID& blockGuid, Bool enterPhase = true );
	Bool OpenBlock( CQuestGraphBlock* block, Bool enterPhase = true );

	// Opens a stack of blocks that can be obtained using the 'FindBlock' method
	void OpenBlock( TDynArray< CQuestGraphBlock* >& stack );

	// Returns a stack of blocks leading to the specified blocks.
	void FindBlock( CQuestGraphBlock* block, TDynArray< CQuestGraphBlock* >& stack ) const;

	// Toggles the display of a graph flow sequence
	void ToggleFlowSequence();

	// Allows to look for certain blocks in the edited graph
	void FindInGraph();

	// Updates GUIDs of all the blocks in the edited graph's hierarchy
	void UpdateGUIDs();

	// Check and update GUIDs
	void CheckAndUpdateGUIDs( Bool performUpdate );

	// Updates GUIDs of all the spawnsets in the Depot
	void OnUpdateCommunityGUIDs();

	// Upgrades the structure of a quest (if there's an upgrade available)
	void OnUpgrade();

	// Count and remove unused GraphBlocks
	Uint32 CountUnusedBlocks();
	
	void DeleteUnusedBlocks();
	Bool DeleteUnusedBlock( CQuestGraphBlock* block );
	void GetUnusedBlocks(TDynArray< CQuestGraphBlock* >& unusedBlocks);
	void CreateDepFromGraph();

	void SetScaleAndOffset( Float scale, const wxPoint& offset );

protected:
	//! CEdGraphEditor interface
	virtual wxColor GetCanvasColor() const;
	virtual void PaintCanvas( Int32 width, Int32 height );
	virtual void CalcTitleIconArea( CGraphBlock* block, wxSize &size );
	virtual void DrawTitleIconArea( CGraphBlock* block, const wxRect& rect );
	virtual void CalcBlockInnerArea( CGraphBlock* block, wxSize& size );
	virtual void DrawBlockInnerArea( CGraphBlock* block, const wxRect& rect );
	virtual void AdjustLinkColors( CGraphSocket* source, CGraphSocket* destination, Color& linkColor, Float& linkWidth );
	virtual void AdjustLinkCaps( CGraphSocket* source, CGraphSocket* destination, Bool& srcCapArrow, Bool& destCapArrow );
	virtual void InitLinkedBlockContextMenu( CGraphBlock *block, wxMenu &menu );
	virtual void InitLinkedSocketContextMenu( CGraphSocket *block, wxMenu &menu );
	virtual void InitLinkedDefaultContextMenu( wxMenu& menu );
	virtual void AdjustBlockColors( CGraphBlock* block, Color& borderColor, Float& borderWidth );
	virtual void DrawBlockLayout( CGraphBlock* block );
	virtual void AnnotateConnection( const CGraphConnection* con, const wxPoint &src, const wxPoint& srcDir, const wxPoint &dest, const wxPoint &destDir, float width = 1.0f );
	virtual void MouseClick( wxMouseEvent& event );

	void OnToolMenu( wxCommandEvent& event );
	void OnAddQuestBlock( wxCommandEvent& event );
	void OnRemoveSelectedBlocks( wxCommandEvent& event );
	void OnConvertToResource( wxCommandEvent& event );
	void OnEmbedGraphFromResource( wxCommandEvent& event );
	void OnVersionControlCommand( wxCommandEvent& event );
	void OnAddInput( wxCommandEvent& event );
	void OnRemoveInput( wxCommandEvent& event );
	void OnAddPatchOutputBlock( wxCommandEvent& event );
	void OnRemovePatchOutputBlock( wxCommandEvent& event );
	void OnRunBlockSpecialOption( wxCommandEvent& event );
	void OnAddTerminationInputBlock( wxCommandEvent& event );
	void OnRemoveTerminationInputBlock( wxCommandEvent& event );
	void OnSaveScopeBlock( wxCommandEvent& event );
	void OnLeftClick( wxMouseEvent& event );
	void OnDoubleClick( wxMouseEvent& event );
	void OnKeyDown( wxKeyEvent& event );
	void OnKeyUp( wxKeyEvent& event );
	void OnKillFocus( wxFocusEvent& event );
	void OnEditorClosed( wxWindowDestroyEvent& event );
	void OnAddOutput( wxCommandEvent& event );
	void OnRemoveOutput( wxCommandEvent& event );
	void OnHandleBehaviorOutcome( wxCommandEvent& event );
	void OnDontHandleBehaviorOutcome( wxCommandEvent& event );
	void OnPasteHere( wxCommandEvent& event );
	void OnRebuildSockets( wxCommandEvent& event );

	// graphs stack management
	void PushGraph( CQuestGraph* graph );
	CQuestGraph* PopGraph();
	CQuestScopeBlock *GetParentScopeBlock( CQuestGraph *graph );
	virtual const Char * ClipboardChannelName() const { return TXT("CEdQuestGraphEditor") ; } 

	Bool ShouldUseDeletionMarkers( const String& path );

private:
	void AddScopeResourceManagementMenuEntries( CQuestScopeBlock* block, wxMenu& menu );
	void AddVersionControlMenuEntries( CQuestScopeBlock* block, wxMenu& menu );
	void EditScene( IQuestBlockWithScene* sceneBlock );
	void LoadKeyboardShortcuts();

	template< typename T >
	Bool SaveResource( const Char* defaultFileName, const Char* filterName, CResource* resource )
	{
		CEdFileDialog dlg;
		dlg.SetMultiselection( false );
		dlg.AddFormat( ResourceExtension< T >(), filterName );
		dlg.SetIniTag( ClassID< T >()->GetDefaultObject<CObject>()->GetFriendlyName() );

		// Get root directory
		String rootDirectory;
		GDepot->GetAbsolutePath( rootDirectory );
		dlg.SetDirectory( rootDirectory );


		if ( dlg.DoSave( (HWND)GetHandle(), defaultFileName, true ) )
		{				
			String localDepotPath;
			if ( !GDepot->ConvertToLocalPath( dlg.GetFile(), localDepotPath ) )
			{
				WARN_EDITOR( TXT("Could not convert '%s' to local depot path."), dlg.GetFile().AsChar() );
				return false;
			}

			CDirectory* directory = GDepot->CreatePath( localDepotPath.AsChar() );

			if ( !directory )
			{
				WARN_EDITOR( TXT("The directory '%s' not found."), localDepotPath.AsChar() );
				return false;
			}

			String filename = localDepotPath;
			if ( localDepotPath.ContainsSubstring( TXT(".") ) )
			{
				filename = localDepotPath.StringAfter(TXT("\\"), true);
			}

			if ( CDiskFile *diskFile = GDepot->FindFile( localDepotPath ) )
			{
				String message = String::Printf( TXT("File '%s' already exists.\nDo you want to replace it?"), localDepotPath.AsChar() );
				if ( wxMessageBox( message.AsChar(), TXT("Confirm file replace"), wxYES_NO | wxCENTER | wxICON_WARNING ) != wxYES )
				{
					return false;
				}
			}

			if ( !resource->SaveAs( directory, filename ) )
			{
				WARN_EDITOR( TXT("Unable to save the resource '%s' to file."), filename.AsChar() );
				return false;
			}

			return true;
		}
		else
		{
			return false;
		}
	}
};
