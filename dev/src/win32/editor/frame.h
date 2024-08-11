#ifndef EDITORFRAME_H
#define EDITORFRAME_H

class CEdAssetBrowser;
class CEdAssetBrowserFrame;
class CEdSelectionProperties;
class CEdSceneExplorer;
class CEdWorldEditPanel;
class CEdToolsPanel;
class CEdFilterPanel;
class CEdBrushPanel;
class CEdWorldProperties;
class CSoundBankLoader;
class CEdMoveEntity;
class CEdMapPinValidatorWindow;
class CEdRewardEditor;
class CEdJournalEditor;
class CEdCharacterDBEditor;
class CEdWorldEnvironmentTool;
class CJournalPath;
class IDataErrorReporterWindow;
class CEdDataErrorReporterWindow;
class CEdWorldSceneDebugger;
class CEdUndoHistoryFrame;
class CEntitiesBrowser;
class CEdConfigPropertiesPage;

#include "shortcut.h"
#include "directoryWatcher.h"
#include "../../common/engine/selectionManager.h"

struct GridSettings
{
	Bool	m_usePositionGrid;
	Bool	m_usePositionGridLength;
	Float	m_positionGrid;
	Bool	m_useRotationGrid;
	Float	m_rotationGrid;

	GridSettings()
		: m_useRotationGrid( true )
		, m_rotationGrid( 15.0f )
		, m_usePositionGrid( true )
		, m_usePositionGridLength( false )
		, m_positionGrid( 0.1f )
	{};

	inline Float Snap( Float value, Float grid ) const
	{
		if ( grid > 0.0f )
		{
			Float val = ( value > 0.0f ) ? ( value + grid * 0.5f ) : ( value - grid * 0.5f );
			return (Int32)( val / grid ) * grid;
		}
		else
		{
			return value;
		}
	}

	inline Vector Snap( const Vector& pos, const Vector& mask ) const
	{
		if ( m_usePositionGrid || m_usePositionGridLength )
		{
			Vector ret;
			ret.X = mask.X ? Snap( pos.X, m_positionGrid ) : pos.X;
			ret.Y = mask.Y ? Snap( pos.Y, m_positionGrid ) : pos.Y;
			ret.Z = mask.Z ? Snap( pos.Z, m_positionGrid ) : pos.Z;
			return ret;
		}
		else
		{
			return pos;
		}
	}

	inline Vector SnapLength( const Vector& v ) const
	{
		if ( m_usePositionGrid || m_usePositionGridLength )
		{
			Float len = v.Mag3();
			return len > 0.0f ? Vector::Div3( v, len ) * Snap( len, m_positionGrid ) : Vector::ZEROS;
		}
		else
		{
			return v;
		}
	}

	inline EulerAngles Snap( const EulerAngles& pos, const EulerAngles& mask ) const
	{
		if ( m_useRotationGrid )
		{
			EulerAngles ret;
			ret.Pitch = mask.Pitch ? Snap( pos.Pitch, m_rotationGrid ) : pos.Pitch;
			ret.Yaw = mask.Yaw ? Snap( pos.Yaw, m_rotationGrid ) : pos.Yaw;
			ret.Roll = mask.Roll ? Snap( pos.Roll, m_rotationGrid ) : pos.Roll;
			return ret;
		}
		else
		{
			return pos;
		}
	}
};

class CEdFrame;

class CEdFrameTimer : public CEdTimer
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Editor );
	bool			m_first;
	CEdFrame*		m_owner;
public:
	CEdFrameTimer( CEdFrame* owner );
	void NotifyOnce();
	void Pause();
	void Unpause();
};



class CEdConfig : public wxSmartLayoutPanel
{
	DECLARE_EVENT_TABLE()

public:
	CEdConfig( wxWindow* parent );
	~CEdConfig();

	CEdPropertiesPage* m_propertiesEditor;
	CEdPropertiesPage* m_propertiesGameplay;
	CEdPropertiesPage* m_propertiesEncounter;

	CEdConfigPropertiesPage* m_propertiesEngine;

	// ISavableToConfig interface
	virtual void SaveOptionsToConfig();
	virtual void LoadOptionsFromConfig();

	// Menu
	void OnSave( wxCommandEvent &event );
	void OnExit( wxCommandEvent &event );
	void OnClose( wxCloseEvent &event );

protected:
	void OnGameplayPropertiesChanged( wxCommandEvent &event );

private:
	void Save();
};

struct SEditorUserConfig
{
	SEditorUserConfig();

	const static Bool	DEFAULT_DISPLAY_CAMERA_TRANSFORM;
	const static Float	DEFAULT_RENDERING_PANEL_CAMERA_SPEED;
	const static Float	DEFAULT_RENDERING_PANEL_CAMERA_ACCELERATION;
	const static Bool	DEFAULT_ALLOW_TERRAIN_SYNC_LOADING;

	void ResetToDefault();
	void Load();
	void Save();

	Bool	m_displayCameraTransform;
	Float   m_renderingPanelCameraSpeed;
	Float	m_renderingPanelCameraAcceleration;
	Bool	m_allowTerrainSyncLoading;

	DECLARE_RTTI_SIMPLE_CLASS( SEditorUserConfig ); 
};

BEGIN_CLASS_RTTI( SEditorUserConfig );
	PROPERTY_EDIT( m_displayCameraTransform, TXT("Should the editor display current camera transform?") )
	PROPERTY_EDIT_RANGE( m_renderingPanelCameraSpeed, TXT("Initial speed for camera in Scene Editor"), 0.f, 500.f )
	PROPERTY_EDIT_RANGE( m_renderingPanelCameraAcceleration, TXT("Acceleration for camera in Scene Editor"), 0.f, 500.f )
	PROPERTY_EDIT( m_allowTerrainSyncLoading, TXT("Allow synchronous loading of terrain data in editor") );
END_CLASS_RTTI();

class CEdFrame : public wxFrame, public IEdEventListener, public ISavableToConfig, public ISmartLayoutWindow, public IDirectoryChangeListener
{
	friend class CEdFrameTimer;
	friend class CEditorEngine;

protected:
	wxAuiManager					m_auiMgr;			//!< AUI Manager
	wxNotebook*						m_solution;			//!< Solution side bar
	CEdSelectionProperties*			m_properties;		//!< Properties panel
	CEdWorldProperties*				m_worldProperties;	//!< Properties of the current world
    CEdUndoManager*					m_undoManager;      //!< Undo manager
	CEdAssetBrowser*				m_assetBrowser;		//!< Asset browser
	CEdSceneExplorer*				m_sceneExplorer;	//!< Scene explorer
	CEdFileDialog					m_mapDialog;		//!< Map opening dialog
	CEdFileReloadDialog*			m_fileReloadDialog;	//!< Files reload dialog
	CEdWorldEditPanel*				m_viewport;			//!< Rendering panel
	CEdToolsPanel*					m_tools;			//!< Tools panel
	CEntitiesBrowser*				m_entitiesBrowser;	//!< Entities browser
	CEdBrushPanel*					m_brushPanel;		//!< Brush edition panel
	wxToolBar*						m_widgets;			//!< Widget toolbar
	wxToolBar*						m_widgetP4;			//!< Widget toolbar for P4
	TDynArray<Bool>					m_cyclableWidgets;	//!< Cyclable widgets
	wxToolBar*						m_mainToolbar;		//!< Main frame toolbar
	TDynArray< CPerfCounter* >		m_perfCounters;		//!< Performance counters
	GridSettings					m_gridSettings;		//!< Editor grid settings
	THashMap<Int32, String>			m_recent;			//!< Recently opened worlds
	CEdFilterPanel*					m_filter;			//!< Filters
	wxSmartLayoutPanel*				m_filterSmartPanel;
	CEdFrameTimer*					m_timer;			//!< Timer for saving options routine
	CEdMoveEntity*					m_moveEntityTool;	//!< Tool for moving entities
	wxStatusBar*					m_statusBar;		//!< Status bar
	CEdTimer*						m_statusBarTimer;	//!< Timer for status bar message
	wxString						m_statusBarMsg;		//!< Status bar message
	TDynArray< wxWindow* >			m_windowsStack;		//!< Stack of visible windows (session store purpose)
	wxPanel*						m_emptyPanel;
	String							m_commandLine;	
	THashMap< String, String >		m_commandLineValue;
	CEdDataErrorReporterWindow*		m_dataErrorReporterWindow;
	TEdShortcutArray				m_shortcuts;
	CEdMapPinValidatorWindow*		m_mapPinValidator;
#ifdef REWARD_EDITOR
	CEdRewardEditor*				m_rewardsEditor;
#endif
	CEdJournalEditor*				m_journalEditor;
	CEdCharacterDBEditor*			m_characterDBeditor;
	CEdWorldEnvironmentTool*		m_worldEnvironmentEditor;
	CEdWorldSceneDebugger*			m_worldSceneDebuggerWindow;
	CEdUndoHistoryFrame*			m_undoHistoryFrame;
	wxToggleButton*					m_profilerOnOffToggle;
	class CEdAutoScriptWindow*		m_autoScriptWin;

	wxPanel*						m_gamePanel;
	wxMenuBar*						m_gameMenu;
	wxMenuBar*						m_clonedMenu;
	wxSmartLayoutAuiNotebook*		m_notebook;
	wxSizer*						m_notebookSizer;
	wxChoice*						m_languageChoice;

	CDirectoryWatcher*				m_assetDirWatcher;
	Int32							m_lastDisplayMode;

	class CEdXMLErrorsDisplayer*	m_xmlErrorsDisplayer;

	class CEdInGameConfigurationDlg* m_inGameConfigDialog;

protected:
	Bool							m_isSolutionTabDrag;
	wxPoint							m_solutionTabDragStartPoint;

	SEditorUserConfig				m_userConfig;

	Bool							m_isViewportFloating;
	wxFrame*						m_fullScreenFrame;

	struct CToolWindowState
	{
		bool isVisible;
		bool isFloat;
		wxWindow *widget;
		wxString caption;

		void Set( wxWindow *_widget, wxString _caption, bool _isVisible, bool _isFloat )
		{
			widget = _widget;
			caption = _caption;
			isVisible = _isVisible;
			isFloat = _isFloat;
		}
	};

	TSortedMap< Int32, CToolWindowState >	m_toolWindowState;

	struct SStatusBarInfo
	{
		Uint32 m_selObjsNum; // the number of selected objects
		SStatusBarInfo() : m_selObjsNum( 0 ) {}
	};
	SStatusBarInfo m_secondStatusBarInfo;

public:
	CEdFrame( wxWindow *parent = NULL, int id = -1 );

	~CEdFrame();

	Bool HasCommandLineParameter( const String &name );
	Bool GetCommandLineParameter( const String &name, String &value );

	RED_INLINE const GridSettings& GetGridSettings() const { return m_gridSettings; }
	
	RED_INLINE SEditorUserConfig& GetEditorUserConfig() { return m_userConfig; }

	void SetFrameTitle();
    wxMenuBar*			GetMenuBarToMerge()	{ return m_clonedMenu; }
    wxAuiNotebook*		GetEditorsBar ()	{ return m_notebook; }
	wxNotebook*			GetSolutionBar();
	CEdAssetBrowser*	GetAssetBrowser()	{ return m_assetBrowser; }
	CEdFilterPanel*		GetFilterPanel()	{ return m_filter; }
	wxPanel*			GetGamePanel()		{ return m_gamePanel; }
	CEdToolsPanel*		GetToolsPanel()		{ return m_tools; }
	CEdWorldEditPanel*	GetWorldEditPanel();
	CEdSceneExplorer*	GetSceneExplorer()	{ return m_sceneExplorer; };
	CEdUndoManager*		GetUndoManager()	{ return m_undoManager; }

	void SaveOptionsToConfig();
	void LoadOptionsFromConfig();

    void SaveSession( CConfigurationManager &config );
	void RestoreSession( CConfigurationManager &config );

	void SetStatusBarText( wxString text, int ms );
	void SetSecondStatusBarText( wxString text );
	void DisplayCameraTransform( wxString text );
	void UpdateSecondStatusBarText();

	void PushWindow( wxTopLevelWindow *w );
	void WindowCreated( wxTopLevelWindow *w );
	void WindowDestroyed( wxTopLevelWindow *w );

	void AddToIsolated( TDynArray< CEntity* > &entities );
	void AddToIsolated( CEntity* entitity );

	Bool OpenWorld( const String &depotPath );
	
	void ToggleFullscreen();
	Bool IsInFullscreenMode() const;

	void ConnectToStringDb();

	void OnCameraMoved( const Vector& cameraPosition, const EulerAngles& cameraRotation );

	void OpenJournalEditor( THandle< CJournalPath > path = NULL );

	void OpenCharacterDBEditor( CCharacterResource* characterResource = nullptr );

	void OpenWorldEnvironmentEditor();
	void EditEnvironmentParams( CObject* e );

	void PauseConfigTimer();
	void ResumeConfigTimer();

	void OnWhatsNew();
	void OnXMLErrors();

	void ShowDataErrorReporter();
	CEdDataErrorReporterWindow* GetDataErrorReporter();

	CEdUndoHistoryFrame* GetUndoHistoryFrame();
	void SetUndoHistoryFrameManager( CEdUndoManager* undoManager, const String& title = String::EMPTY );
	void ShowUndoHistoryFrame();

	void UpdateGameResourceNameField();

	void OnSelectAll					( wxCommandEvent& event );
	void OnUnselectAll					( wxCommandEvent& event );
	void OnInvertSelection		( wxCommandEvent& event );

	void OnSelectByTheSameEntityTemplate( wxCommandEvent& event );
	void OnSelectByTheSameTag			( wxCommandEvent& event );

	void OnSelectionOnActiveLayer		( wxCommandEvent& event );
	void OnSelectionOnMultiLayer		( wxCommandEvent& event );

	void OnCenterOnSelected				( wxCommandEvent& event );

	void OnHideLayersWithSelectedObjects( wxCommandEvent& event );
	void OnUnhideAllLoadedLayers		( wxCommandEvent& event );

	void OnDisconnectPerforce	( wxCommandEvent& event );

	// FUNCTIONS FOR LOOTING OPERATIONS
	void OnMergeLootWithEntity( wxCommandEvent& event );
	void OnReplaceWithLootable( wxCommandEvent& event );
	void OnAddLootOptions( wxCommandEvent& event );

	Bool DisplayErrorsList( const String& header, const String& footer, const TDynArray< String >& errors, const TDynArray< String >& descriptions = TDynArray< String >(), Bool cancelBtn = false ) const;

	void GetDebugFlags( TDynArray< EShowFlags > &debugFlags );

private:
	void CreateToolbars();
	void BuildToolsMenu( wxMenu* parent, String path );

	void OnEraseBackground( wxEraseEvent& event );
	void OnSize( wxSizeEvent& event );
	void OnMove( wxMoveEvent& event );

	bool BeforeCloseQuery();
	void RelaunchEditor();
	void OnCloseWindow( wxCloseEvent& event );
	void OnActivateWindow( wxActivateEvent& event );
	void OnExit( wxCommandEvent& event );
	void OnAbout( wxCommandEvent& event );
	void OnWhatsNewForced( wxCommandEvent& event );
	void OnLaunchTool( wxCommandEvent& event );
	void OnAssetBrowser( wxCommandEvent& event );
	void OnReloadScripts( wxCommandEvent& event ); 
	void OnReloadResourceDefinitions( wxCommandEvent& event ); 
	void OnReloadGameDefinitions( wxCommandEvent& event ); 
	void OnGameDefinitionChoice( wxCommandEvent& event );
	void OnRefreshVoices( wxCommandEvent& event ); 
	void OnSelectDLC( wxCommandEvent& event );

	void OnNewWorld( wxCommandEvent& event );
	void OnRestoreWorld( wxCommandEvent& event );
	void OnReloadSoundbanks( wxCommandEvent& event );
	void OnApexDestructionStatistics( wxCommandEvent& event );
	void OnApexClothStatistics( wxCommandEvent& event );
	void OnGeneratePhysxGeometryStatisticsFromCollisionCache( wxCommandEvent& event );
	void OnFillCollisionCacheWithEveryMeshInLocalDepotCollision( wxCommandEvent& event );
	void OnOpenWorld( wxCommandEvent& event );
	void OnCloseWorld( wxCommandEvent& event );
	void OnOpenRecent( wxCommandEvent &event );
	void OnSaveWorld( wxCommandEvent& event );
	void OnSaveWorldModifiedOnly( wxCommandEvent& event );
	void OnClientSettings( wxCommandEvent &event );
	void OnDepartmentTags( wxCommandEvent &event );
	void OnRestartWithGame( wxCommandEvent &event );

    void OnUndo( wxCommandEvent& event );
    void OnRedo( wxCommandEvent& event );
	void OnUndoHistory( wxCommandEvent& event );
	void OnUndoTrackSelection( wxCommandEvent& event );

	void OnCopy( wxCommandEvent& event );
	void OnCut( wxCommandEvent& event );
	void OnPaste( wxCommandEvent& event );
	void OnDelete( wxCommandEvent& event );

	void OnCopyCameraView( wxCommandEvent& event );
	void OnPasteCameraView( wxCommandEvent& event );
	void OnPasteQuestView( wxCommandEvent& event );

	void OnSpaceForeign( wxCommandEvent& event );
	void OnSpaceLocal( wxCommandEvent& event );
	void OnSpaceWorld( wxCommandEvent& event );
	void OnSpaceChange( wxCommandEvent& event );
	void OnWidgetPick( wxCommandEvent& event );
	void OnWidgetMove( wxCommandEvent& event );
	void OnWidgetRotate( wxCommandEvent& event );
	void OnWidgetScale( wxCommandEvent& event );
	void OnWidgetChange( wxCommandEvent& event );
	void OnPlayGame( wxCommandEvent& event );
	void OnPlayGameFast( wxCommandEvent& event );
	void OnPlayGameFromSave( wxCommandEvent& event );

	void OnRegenerateRenderProxies( wxCommandEvent& event );
	void OnEnvironmentModifierChanged( wxCommandEvent& event );

	void OnEditorStreaming( wxCommandEvent& event );

    void OnOpenShortcutsEditor( wxCommandEvent &event );    

	void OnMeshColoringNone( wxCommandEvent &event );
	void OnMeshColoringType( wxCommandEvent &event );
	void OnMeshCollisionType( wxCommandEvent &event );
	void OnMeshSoundMaterial( wxCommandEvent &event );
	void OnMeshSoundOccl( wxCommandEvent &event );
	void OnMeshShadows( wxCommandEvent &event );
	void OnMeshTextureDensity( wxCommandEvent &event );
	void OnMeshRenderingLod( wxCommandEvent &event );
	void OnMeshStreamingLod( wxCommandEvent &event );
	void OnMeshLayerBuildTag( wxCommandEvent &event );
	void OnMeshChunks( wxCommandEvent &event );
	
	void OnViewMoveToTerrainLevel( wxCommandEvent &event );
	void OnViewLoadLayersAroundCamera( wxCommandEvent& event );
	void OnViewUnloadLayersAroundCamera( wxCommandEvent& event );
	void OnViewLockFreeSpaceVisualization( wxCommandEvent& event );
	void OnViewShowFreeSpaceAroundSelection( wxCommandEvent& event );
	void OnViewToggleDisplayMode( wxCommandEvent& event );
	
	TEdShortcutArray *GetAccelerators();
	void OnAccelFilter( wxCommandEvent& event );

	void OnUpdateUI( wxUpdateUIEvent& event );
	Bool CheckStartGameConditions() const;
	Bool StartGame( Bool fast, const String& gameSaveFileName = String::EMPTY, const String& loadingVideoToPlay = String::EMPTY, const TDynArray< CName >& playGoChunksToActivate = TDynArray<CName>() );
	Bool LoadGame( const SSavegameInfo& info );
	void UpdateWidgetUI();
	void CheckWidgetUI( Int32 id, Bool state );
	void ToggleWidgetCyclability( ERPWidgetMode widget );

	void OnTogglePerfCounter( wxCommandEvent& event );
	void OnShowAllPerfCounters( wxCommandEvent& event );
	void OnHideAllPerfCounters( wxCommandEvent& event );
	void UpdateProfilerMenu();

	void OnPositionGridToggle( wxCommandEvent& event );
	void OnPositionGridChange( wxCommandEvent& event );
	void OnPositionGridLengthToggle( wxCommandEvent& event );
	void OnRotationGridToggle( wxCommandEvent& event );
	void OnRotationGridChange( wxCommandEvent& event );
	void CaptureGridSettings();
	void UpdateGridWidgets();

	void DispatchEditorEvent( const CName& name, IEdEventData* data );
	void OnKeyDown(wxKeyEvent& event);

	void OnWorldSceneDebuggerTool		( wxCommandEvent& event );
	void OnCloseWorldSceneDebuggerWindow( wxCloseEvent& event );
	void OnMemWalkerTool				( wxCommandEvent& event );
	void OnCollisionMemUsageTool		( wxCommandEvent& event );
	void OnToolMoveEntities				( wxCommandEvent& event );

	void OnStamper( wxCommandEvent& event );
	void OnVDB( wxCommandEvent& event );
	void OnSimulate( wxCommandEvent& event );
	void OnVDBMenu( wxCommandEvent& event );
	void OnSnapChange( wxCommandEvent& event );

	void OnSnapNone( wxCommandEvent& event );
	void OnSnapTerrainVisual( wxCommandEvent& event );
	void OnSnapTerrainPhysical( wxCommandEvent& event );
	void OnSnapCollision( wxCommandEvent& event );

	void OnSnapPivot( wxCommandEvent& event );
	void OnSnapBoundingVolume( wxCommandEvent& event );

	void OnDebugSoundRangeMin( wxCommandEvent& event );
	void OnDebugSoundRangeMax( wxCommandEvent& event );
	void OnSpawnSetDebugger( wxCommandEvent& event );
	void OnQuestsDebugger( wxCommandEvent& event );
	void OnBehaviorDebugger( wxCommandEvent& event );
	void OnBehaviorGroupDebugger( wxCommandEvent& event );
	void OnAnimationFriend( wxCommandEvent& event );

	void ExportGlobalMappins( wxCommandEvent& event );
	void ExportEntityMappins( wxCommandEvent& event );
	void ExportEntityMappinsForEP1( wxCommandEvent& event );
	void ExportEntityMappinsForEP2( wxCommandEvent& event );
	void ExportQuestMappins( wxCommandEvent& event );
	void UpdateMapPinEntities( wxCommandEvent& event );
	void TEMP_CheckEntitiesThatUseEntityHandles( wxCommandEvent& event );

	void UpdateFocusEntityTemplates( wxCommandEvent& event );
	void DumpCommunityAgents( wxCommandEvent& event );
	void AIGlobalRefactor_IAITree( wxCommandEvent& event );
	void GlobalSpawnTreesResaving( wxCommandEvent& event );
	void GlobalEncounterLayersResaving( wxCommandEvent& event );
	void AIGlobalRefactor_CBehTreeTask( wxCommandEvent& event );
	void OnQuestBlocksValidator( wxCommandEvent& event );
	void CheckGraph( class CQuestGraph* graph, THashMap< CGUID, TDynArray< class CQuestGraphBlock* > >& duplicatedGUIDs, THashMap< CQuestPhase*, TDynArray< CQuest* > >& questRefs, CQuest* currQuest );

	void OnMaraudersMap( wxCommandEvent& event );
	void OnLocalizedStringsEditor( wxCommandEvent& event );
	void OnLocalizationTools( wxCommandEvent& event );
	void OnResourceSorterStart( wxCommandEvent& event );
	void OnSceneValidator( wxCommandEvent& event );
	void OnEntitiesBrowser( wxCommandEvent& event );
	void OnSoundsDebugger( wxCommandEvent& event );
	void OnDataErrorReporter( wxCommandEvent& event );	
	void OnAnimationsReport( wxCommandEvent& event );
	void OnLipsyncPreview( wxCommandEvent& event );
	void OnRewardsEditor( wxCommandEvent& event );
	void OnJournalEditor( wxCommandEvent& event );
	void OnJournalEditorClosed( wxEvent& event );
	void OnCharacterDBEditor( wxCommandEvent& event );
	void OnCharacterDBEditorClosed( wxEvent& event );
	void OnWorldEnvironmentEditor( wxCommandEvent& event );
	void OnMinimapGenerator( wxCommandEvent& event );
	void OnScreenshotEditor( wxCommandEvent& event );
	void OnExportAnimationList( wxCommandEvent& event );

	void OnStatusBarTimeout( wxCommandEvent& event );
	void OnViewChanged( wxCommandEvent& event );
	void OnPopupNotificationLocationChanged( wxCommandEvent& event );
	void OnViewportCachetAspectRatioChanged( wxCommandEvent& event );
	void OnViewFOV( wxCommandEvent& event );
	void OnOverlayVertexSprites( wxCommandEvent& event );
	void OnListEntitiesAroundCamera( wxCommandEvent& event );

	void OnGameEditorPageShow( wxAuiNotebookEvent& event );
	void OnGameEditorClosePage( wxAuiNotebookEvent& event );
	
	void OnSolutionDragStart( wxMouseEvent &event );
	void OnSolutionDragMove ( wxMouseEvent &event );
	void OnSolutionDragEnd  ( wxMouseEvent &event );
	void OnSolutionDockWidget( wxCommandEvent &event );

	void OnGenerateUmbraScene( wxCommandEvent& event );

	void OnResaveTextures( wxCommandEvent& event );
	void OnShowXMLErrors( wxCommandEvent& event );

	void OnAttitudeEditor( wxCommandEvent& event );
	void OnLootEditor( wxCommandEvent& event );

	void OnCreateEditorAnimCache( wxCommandEvent& event );

	void OnConfig( wxCommandEvent& event );
	void OnCrowdDebugger( wxCommandEvent& event );
	void OnIDGenerator( wxCommandEvent& event );

	void OnShowCookingDialog( wxCommandEvent& event );

	void OnImportTextureSourceData( wxCommandEvent& event );

	void UpdateToolWindowState( Int32 wnd, Bool isVisible, Bool isFloat, bool refresh = true );
	void RefreshToolWindows();
	void OnToolWindowClose( wxCommandEvent& event );

	void ResaveAllFiles( wxCommandEvent& event );

	Bool LoadWorld( const String &depotPath );
	void StoreWorldChoice( String localDepotPath );
	void CloseAllWorldRelatedWindows();

	void AddToProfilerMenu( THashMap< String, TPair< Uint32, CPerfCounter* > > &ps, wxMenu *menu );

	void SaveWindowsStack( CConfigurationManager &config );
	void RestoreWindowsStack( CConfigurationManager &config );

	void OnCancelLoad( wxCommandEvent& event );
	void OnProgressDialogTimeout( wxCommandEvent& event );

	void OnAddSoundbank( wxCommandEvent& event );
	void OnRemoveSoundbank( wxCommandEvent& event );
	void OnCloseSoundbankWindow( wxCommandEvent& event );

	void OnPerformancePlatformClicked( wxCommandEvent& event );

	void OnSqlConnectionButton( wxCommandEvent& event );

	void ToggleFloatingViewport();

	void OnMuteSound( wxCommandEvent& event );
	void OnMuteMusic( wxCommandEvent& event );

	void OnMapPinValidator( wxCommandEvent& event );

	void OnSelectGameResource( wxCommandEvent& event );
	void OnPlayGameFromGameResource( wxCommandEvent& event );

	void PathLibEnableGeneration( wxCommandEvent& event );
	void PathLibEnableObstacles( wxCommandEvent& event );
	void PathLibLocalFolder( wxCommandEvent& event );
	void OnResaveAnimationsTool( wxCommandEvent& event );

	void OnAutoScriptWindow( wxCommandEvent& event );

	void ExplorationFinder( wxCommandEvent& event );
	void ExplorationFinderOneLayer( wxCommandEvent& event );
	void ExplorationFinderPrev( wxCommandEvent& event );
	void ExplorationFinderNext( wxCommandEvent& event );
	void ExplorationFinderJumpTo( Bool next );
	void UpdateExplorationFinderIcons();
	void ExplorationFinderToggleSelected( wxCommandEvent& event );
	void ExplorationFinderHide( wxCommandEvent& event );

	void ClearDebugStuff( wxCommandEvent& event );
	void OnForceAllShadowsToggle( wxCommandEvent& event );
	void DebugFlagsContextMenu( wxCommandEvent& event );
	void ChangeDebugFlag( wxCommandEvent& event );

	void SwitchOcclusionCullingUsage( wxCommandEvent& event );
	void OnAutohideToggle( wxCommandEvent& event );

	void FillLanguagesChoiceBox();

	virtual void OnDirectoryChange( const TDynArray< ChangedFileData >& changes ) override;

	void OnProfilerChange( wxCommandEvent& event );

	void RunEditorInExtraMode( const THashMap< String, String >& commandLine );
	void RunEditorInMinimapMode( const THashMap< String, String >& commandLine );
	void RunEditorInTestMode( const String& world );
	void RunEditorInRecorderMode( const THashMap< String, String >& commandLine );

	DECLARE_EVENT_TABLE();

public:
	void OnChangeLanguage( wxCommandEvent& event );
	void ForceNoAutohide( Bool forceNoAutohide, Bool updateToolbarBtn = false );
};

extern CEdFrame* wxTheFrame;

enum ESaveChangedDlgReturn
{
	ESC_Cancel,
	ESC_SaveAll,
	ESC_SkipAll,
	ESC_Custom,
};

class CSaveChangedDlg : public wxDialog
{
	DECLARE_EVENT_TABLE();

protected:
	TDynArray< wxCheckBox* >	m_checkboxSave;
	TDynArray< wxCheckBox* >	m_checkboxSkip;
	Bool						m_custom;
	TDynArray< CDiskFile* >		m_files;
public:
	CSaveChangedDlg( wxWindow* parent, TDynArray< CDiskFile* > &files );
	~CSaveChangedDlg();

	ESaveChangedDlgReturn DoModal();

protected:
	void OnSave( wxCommandEvent& event );
	void OnSkip( wxCommandEvent& event );
	void OnChoose( wxCommandEvent& event );
	void OnCancel( wxCommandEvent& event );
	void OnOk( wxCommandEvent& event );

	void OnCheckboxSave( wxCommandEvent& event );
	void OnCheckboxSkip( wxCommandEvent& event );
};

#endif // EDITORFRAME_H
