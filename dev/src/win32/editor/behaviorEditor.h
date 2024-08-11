/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../../common/engine/behaviorGraphStack.h"

#include "../../common/game/lookAtTypes.h"
#include "../../common/game/inventoryEditor.h"
#include "smartLayout.h"

#include "displayItemDlg.h"
#include "skeletonPreview.h"
#include "graphEditor.h"
#include "undoBehaviorGraph.h"

class CBehaviorGraphInstance;
class CEdBehaviorDebugger;
class CEdBehaviorEditorPanel;
class CEdBehaviorEditorProperties;
class CEdBehaviorGraphControlPanel;
class CEdBehaviorGraphEditor;
class CEdBehaviorPreviewPanel;

class CEdBehaviorEditor : public wxSmartLayoutPanel
						, public GraphEditorHook
						, public CItemDisplayer

{
	wxDECLARE_EVENT_TABLE();
	wxDECLARE_CLASS( CEdBehaviorEditor );

protected:
	CBehaviorGraph*		m_behaviorGraph;
	CBehaviorGraph*		m_originalBehaviorGraph;
	CName				m_behaviorInstanceName;
	CName				m_originalBehaviorGraphInstance;

	CWorld*								m_world;
	THandle< CEntity >					m_entity;
	THandle< CAnimatedComponent >		m_animatedComponent;

	TDynArray< CAnimatedComponent* >	m_entityAnimComponents;
	TDynArray< CAnimatedComponent* >	m_clones;

	wxAuiManager		m_mgr;
	TEdShortcutArray    m_shortcuts;
	String				m_perspective;
	String				m_entityFileName;
	TDynArray< String >	m_entityHistory;

	Bool				m_isPaused;
	Int32				m_playOneFrame;
	Float				m_timeFactor;

	Bool				m_movingCamera;
	Bool				m_eyeCamera;

	Bool				m_extractMotion;
	Bool				m_extractTrajectory;

	EBehaviorLod		m_lodLevel;
	Int32				m_meshLodLevel;

	CEntity*			m_floor;

	Bool				m_inputConnected;

	Bool				m_dispGlobalSkeletonBones;
	Bool				m_dispGlobalSkeletonBoneNames;
	Bool				m_dispGlobalSkeletonBoneAxis;

	Bool				m_activationAlphaEnabled;
	Bool				m_autoTrackingEnabled;
	Bool				m_debugMode;
	Bool				m_debugUnsafeMode;
	
	TDynArray< CEdBehaviorEditorPanel* > m_toolPanels;
	TDynArray< CEdBehaviorEditorPanel* > m_activePanels;

	CEdUndoManager* m_undoManager;

public:
	CEdBehaviorEditor( wxWindow* parent, CBehaviorGraph* behaviorGraph );
	CEdBehaviorEditor( wxWindow* parent, CBehaviorGraphInstance* behaviorInstance );
	~CEdBehaviorEditor();

	TEdShortcutArray* GetAccelerators();

protected:
	void SetupEditor();

	Bool HasPanel( const wxString& name );

	void CreatePanel( CEdBehaviorEditorPanel* panel );
	void CreateToolPanel( CEdBehaviorEditorPanel* panel );

	wxAuiNotebook* CreateNotebook();
	void CreateNotebookPanel( CEdBehaviorEditorPanel* panel, wxAuiNotebook* n );

	void ShowTool( const wxString& panelName, Bool canBeClose = true );
	void HideTool( const wxString& panelName );	

	void AddSeparatorToMenuBar();
	void AddToolToMenuBar( CEdBehaviorEditorPanel* panel, Uint32 id );

	CEdBehaviorEditorPanel*	GetToolPanel( const wxString& panelName ) const;

	void CheckPanelsActivation();

	void ShowDefaultNode();

public:
	Bool IsToolActive( const CEdBehaviorEditorPanel* panel );

public:
	void LoadEntity( const String& fileName, const String& component = String::EMPTY, const String& defaultComponent = String::EMPTY );
	void AnalyzeEntity();
	void UnloadEntity();

	void Tick( Float timeDelta );

	Bool RequiresCustomTick() const;
	//! Do custom tick, returns true if it was actually ticked
	void CustomTick( Float timeDelta );

	void OnDebuggerPostSamplePose( const SBehaviorGraphOutput& pose );

	void RelinkToInstance( CBehaviorGraphInstance* behaviorInstance );
	void FocusOnBehaviorNode( CBehaviorGraphNode* node );

public:
	void OnLoadEntity();
	void OnUnloadEntity();
	void OnPreInstanceReload();
	void OnInstanceReload();
	void OnGraphModified();
	void OnNodesSelect( const TDynArray< CGraphBlock* >& nodes );
	void OnNodesDeselect();
	void OnReset();
	void OnDebug( Bool flag );
	void OnTick( Float dt );
	void OnCustomTick( Float dt );
	void OnPrintNodes( CEdGraphEditor* graphCanvas );

public:
	RED_INLINE CBehaviorGraph*			GetBehaviorGraph() const			{ return m_behaviorGraph; }
	RED_INLINE CBehaviorGraphInstance*	GetBehaviorGraphInstance() const	{ CAnimatedComponent *ac = m_animatedComponent.Get(); return ac && ac->GetBehaviorStack() ? ac->GetBehaviorStack()->GetBehaviorGraphInstance( m_behaviorInstanceName ) : NULL; }
	RED_INLINE CAnimatedComponent*		GetAnimatedComponent() const		{ return m_animatedComponent.Get(); }
	RED_INLINE CEntity*					GetEntity() const					{ return m_entity.Get(); }
	RED_INLINE const TDynArray< CAnimatedComponent* >& GetAllAnimatedComponents() const { return m_entityAnimComponents; }
	RED_INLINE const CName&				GetOriginalInstanceName() const		{ return m_originalBehaviorGraphInstance; }

public:
	CEdBehaviorEditorProperties*	GetStaticPropertyBrowser();
	CEdBehaviorGraphEditor*			GetGraphEditor();
	CEdBehaviorDebugger*			GetDebugger();
	CEdBehaviorGraphControlPanel*	GetControlPanel();
	CEdBehaviorPreviewPanel*		GetPreviewPanel();

public:
	void SetPause( Bool flag )					{ m_isPaused = flag; }
	Bool IsPaused() const						{ return m_isPaused; }

	Bool IsDebugMode() const					{ return m_debugMode; }

	void SetExtractedMotion( Bool flag );
	void SetExtractedTrajectory( Bool flag );
	Bool UseExtractedMotion() const				{ return m_extractMotion; }
	Bool UseExtractedTrajectory() const			{ return m_extractTrajectory; }

	void ConnectInputs( Bool flag )				{ m_inputConnected = flag; }
	Bool IsInputConnected() const				{ return m_inputConnected; }

	void SetEyeCamera( Bool flag );
	void SetMovingCamera( Bool flag );
	Bool UseEyeCamera() const					{ return m_eyeCamera; }
	Bool UseMovingCamera() const				{ return m_movingCamera; }

	void SetTimeFactor( Float factor )			{ m_timeFactor = factor; }
	Float GetTimeFactor() const					{ return m_timeFactor; }

	void PlayOneFrame( Bool flag = true )		{ m_playOneFrame = flag ? 1 : 0; }
	Bool HasPlayOneFrameFlag() const			{ return m_playOneFrame == 1; }
	void PlayOneFrameBack( Bool flag = true )	{ m_playOneFrame = flag ? -1 : 0; }
	Bool HasPlayOneFrameBackFlag() const		{ return m_playOneFrame == -1; }

	void EnableActivationAlpha( Bool flag );
	Bool IsActivationAlphaEnabled() const		{ return m_activationAlphaEnabled; }

	void EnableAutoTracking( Bool flag );
	Bool IsAutoTrackingEnabled() const			{ return m_autoTrackingEnabled; }

	void ToggleSkeletonBones();
 	void ToggleSkeletonBoneNames();
 	void ToggleSkeletonBoneAxis();
 	Bool IsSkeletonBonesVisible() const			{ return m_dispGlobalSkeletonBones; }
 	Bool IsSkeletonBoneNamesVisible() const		{ return m_dispGlobalSkeletonBoneNames; }
 	Bool IsSkeletonBoneAxisVisible() const		{ return m_dispGlobalSkeletonBoneAxis; }

	void ToggleDebuggerUnsafeMode();
	void OnToggleFloor();
	void OnDynTarget( Bool flag );

	void SetLodLevel( EBehaviorLod lod )		{ m_lodLevel = lod; }
	EBehaviorLod GetLodLevel() const			{ return m_lodLevel; }

	void SetMeshLodLevel( Int32 lod );

	void SetEditorWorld( CWorld* world )		{ m_world = world; }

	const TDynArray< String >& GetEntityHistory() { return m_entityHistory; }

public:
	virtual void OnGraphSelectionChanged();
	virtual void OnGraphStructureWillBeModified( IGraphContainer* graph );
	virtual void OnGraphStructureModified( IGraphContainer* graph );

public:	
	void OnSave( wxCommandEvent& event );
	void OnClose( wxCloseEvent &event );

	void OnEditUndo( wxCommandEvent& event );
	void OnEditRedo( wxCommandEvent& event );
	void OnEditCopy( wxCommandEvent& event );
	void OnEditCut( wxCommandEvent& event );
	void OnEditDelete( wxCommandEvent& event );
	void OnEditPaste( wxCommandEvent& event );
	void OnMenuReset( wxCommandEvent& event );
	void OnMenuPlayOne( wxCommandEvent& event );
	void OnMenuPlayOneBack( wxCommandEvent& event );
	void OnMenuPlay( wxCommandEvent& event );
	void OnMenuCameraEye( wxCommandEvent& event );
	void OnMenuCameraFollow( wxCommandEvent& event );
	void OnMenuMotionEx( wxCommandEvent& event );
	void OnMenuShowSkeleton( wxCommandEvent& event );
	void OnMenuShowNames( wxCommandEvent& event );
	void OnMenuShowActivations( wxCommandEvent& event );

	void OnSavePerspectives( wxCommandEvent& event );
	void OnSaveAsPerspectives( wxCommandEvent& event );
	void OnLoadPerspectives( wxCommandEvent& event );
	void OnReloadPerspectives( wxCommandEvent& event );
	void OnResetPerspectives( wxCommandEvent& event );

	void OnRemoveUnusedVariablesAndEvents( wxCommandEvent& event );

	void OnViewDebugActivations( wxCommandEvent& event );
	void OnViewConditions( wxCommandEvent& event );
	void OnViewConditionTests( wxCommandEvent& event );

	void OnMenuTool( wxCommandEvent& event );

	void OnBehaviorGraphValidatorLinkClicked( wxHtmlLinkEvent& event );

public:
	void SaveOptionsToConfig();
	void LoadOptionsFromConfig();

	void SaveSession( CConfigurationManager &config );
	void RestoreSession( CConfigurationManager &config );

	void EnumPerspectives( TDynArray< String >& list );

	void SavePerspective( const String& data );
	void LoadPerspective();
	void ResetPerspective();

	void EnumInputs( TDynArray< CName >& inputs );

public:
	void SetBehaviorGraph( CBehaviorGraph *graph );

	void UseBehaviorInstance( Bool flag );
	void RecreateBehaviorGraphInstance();

	void BehaviorGraphModified();

	Bool CanBeModify();

	void AttachToSelectedEntity();

	void GetSelectedNodes( TDynArray< CGraphBlock* >& nodes );

protected:
	void CheckGraphEngineValues();
	void CheckStateMachines( IFeedbackSystem* sys );
	void VaildateGraphNodes();
	void ValidateBehaviorGraph();
	void HackSaftySave();

	void OnGraphOpenInEditor();

	void CreateFloor();
	void DestroyFloor();

	void ActiveAnimComponentInApp();
	Bool CheckAnimatedComponentForDebugging( CAnimatedComponent* component ) const;

	void RestorePanels();

public:
	// CItemDisplayer
	//! Get an actor this displayer attaches items to
	virtual CActor*	GetActorEntity();
};
