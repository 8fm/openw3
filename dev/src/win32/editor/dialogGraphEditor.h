#pragma once

class CEdSceneEditor;
class CEdSceneEditorScreenplayPanel;
class CStoryScene;
class CStorySceneSection;
class CStorySceneSectionBlock;
class CStorySceneControlPart;
class CUndoDialogGraphBlockExistance;

enum ESceneTemplateType
{
	STT_None,
	STT_Gameplay,
	STT_Normal,
	STT_FullVoiceSet,
};

class CEdSceneGraphEditor : public CEdGraphEditor
{
	friend class CUndoDialogGraphBlockExistance;

	DECLARE_EVENT_TABLE()

	class CSceneMenuInfo
	{
	protected:
		wxString			m_menuText;
		const CClass*		m_blockClass;
		const CClass*		m_objectClass;
		wxString			m_templateName;
		ESceneTemplateType	m_templateType;

	public:
		CSceneMenuInfo( const wxString& menuText, const CClass* blockClass, const CClass* objectClass, const wxString& templateName, ESceneTemplateType templateType )
			: m_menuText( menuText )
			, m_blockClass( blockClass )
			, m_objectClass( objectClass )
			, m_templateName( templateName )
			, m_templateType( templateType )
		{}
		RED_INLINE const wxString&	GetMenuText() const			{ return m_menuText; }
		RED_INLINE const CClass*		GetBlockClass() const		{ return m_blockClass; }
		RED_INLINE const CClass*		GetObjectClass() const		{ return m_objectClass; }
		RED_INLINE const wxString&	GetTemplateName() const		{ return m_templateName; }
		RED_INLINE ESceneTemplateType	GetTemplateType() const		{ return m_templateType; }

		RED_INLINE Bool				IsBlock() const				{ return !!m_blockClass; }
		RED_INLINE Bool				IsTemplate() const			{ return m_templateType > STT_None; }
	};

public:
	CEdSceneGraphEditor( wxWindow* parent, CEdSceneEditor* sceneScriptEditorPage );
	~CEdSceneGraphEditor(void);

	virtual void UpdateBlockLayout( CGraphBlock* block ) override;
	virtual void DrawBlockLayout( CGraphBlock* block ) override;
	virtual void AdjustLinkColors( CGraphSocket* source, CGraphSocket* destination, Color& linkColor, Float& linkWidth ) override;
	virtual void AdjustSocketCaption( CGraphSocket* socket, String& caption, wxColor& color );

	virtual void ConnectSockets( CGraphSocket* srcSocket, CGraphSocket* destSocket ) override;
	
	virtual Bool IsBlockActivated( CGraphBlock* block ) const override;
	virtual Float GetBlockActivationAlpha( CGraphBlock* block ) const override;

public:
	void SelectSectionBlock( CStorySceneSection* section, Bool select );
	void CenterOnSectionBlock( CStorySceneSection* section );
	
	void MarkDebugControlPart( const CStorySceneControlPart* controlPart );
	void SetContolPartActive( const CStorySceneControlPart* controlPart );

public:
	void OnAddSection( wxCommandEvent& event );
	void OnAddCutscene( wxCommandEvent& event );

	void OnEditCopy( wxCommandEvent& event );
	void OnEditCut( wxCommandEvent& event );
	void OnEditPaste( wxCommandEvent& event );

protected:
	CEdSceneEditorScreenplayPanel*	HACK_GetScreenplayPanel() const;
	CEdSceneEditor*					GetSceneEditor() const;
	CStoryScene*					HACK_GetStoryScene() const;

	void SetBlockPositionAtMouseCursor( CGraphBlock* createdBlock, const wxPoint* mousePosition = nullptr );
	void CreateAddBlockUndoStep( CGraphBlock* createdBlock );

	CStorySceneGraphBlock* FindControlPartBlock( CStorySceneControlPart* controlPart );
	void GetSelectedSceneBlocks( TDynArray< CStorySceneGraphBlock* >& selectedSceneBlocks );

	void DispatchEditorEvent( const CName& name, IEdEventData* data );

	void OpenCutscenePreview( CStorySceneCutsceneSectionBlock* csBlock );

	void DeleteBlock( CGraphBlock* block );
	void CopyBlockConnections( CGraphBlock* from, CGraphBlock* to );

	void OnCheckConsistency( Bool doNotShowInfoIfItsOK = false, Bool doNotShowInfoIfItsNotOK = false );
	void OnPaste( const Vector* pos );

protected:
	virtual wxColor GetCanvasColor() const { return DIALOG_EDITOR_BACKGROUND; }
	wxRect GetOutputSelectionRect( const CGraphBlock* block ) const;
	wxRect GetInputSelectionRect( const CGraphBlock* block ) const;

	virtual void InitLinkedBlockContextMenu( CGraphBlock *block, wxMenu &menu );
	virtual void InitLinkedSocketContextMenu( CGraphSocket *block, wxMenu &menu );
	virtual void InitLinkedDefaultContextMenu( wxMenu& menu );
	virtual void MouseClick( wxMouseEvent& event );

protected:
	void OnAddSceneBlock( wxCommandEvent& event );
	void OnAddSceneTemplate( wxCommandEvent& event );
	void OnCheckConsistency( wxCommandEvent& event );
	void OnPasteHere( wxCommandEvent& event );

	void OnDeleteBlock( wxCommandEvent& event );

	void OnKeyDown( wxKeyEvent& event );
	void OnKeyUp( wxKeyEvent& event );
	void OnKillFocus( wxFocusEvent& event );
	void OnLeftClick( wxMouseEvent& event );
	void OnDoubleClick( wxMouseEvent& event );

	void OnOpenCutscenePreview( wxCommandEvent& event );

	void OnCreateDialogsetNew( wxCommandEvent& event );
	void OnCreateDialogsetFromFile( wxCommandEvent& event );
	void OnCreateDialogsetFromPreviousSection( wxCommandEvent& event );
	void OnCreateDialogsetFromSelectedSection( wxCommandEvent& event );
	void OnChangeDialogset( wxCommandEvent& event );
	void OnVerifySection( wxCommandEvent& event );

	virtual const Char * ClipboardChannelName() const { return TXT("CEdSceneGraphEditor") ; } 

	void AddSceneBlock( const CClass* blockClass, const wxPoint* mousePosition = nullptr );

	void AddSceneTemplate( const String& templateName, const wxPoint* mousePosition = nullptr );
	void AddSceneTemplate( ESceneTemplateType templateType, const wxPoint* mousePosition = nullptr );
	void AddSceneTemplate( Bool gameplay, wxPoint blockPos, const Char* title = NULL );

	void FillBlockAndTemplateInfo();
	void LoadKeyboardShortcuts();

private:
	CEdSceneEditor*			m_sceneEditor;
	wxPoint					m_mousePosition;
	CEdSceneEditor*			m_mediator;
	Int32					m_pressedKeyCode;
	THashMap< Char, TPair< CClass*, String > >	m_shortcuts;
	TStaticArray< CSceneMenuInfo, 20 >			m_sceneMenuInfo;
};
