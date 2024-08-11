#pragma once

#include "dialogGraphEditor.h"
#include "dialogEditorSceneCtrl.h"
#include "dialogEditorCameraCtrl.h"
#include "dialogEditorWorldCtrl.h"
#include "dialogEditorKeyframeCtrl.h"
#include "dialogEditorDebugger.h"
#include "dialogEditorActorsProvider.h"
#include "dialogEditorModificationCtrl.h"
#include "propertiesPageComponent.h"
#include "dialogEditorHelperEntitiesCtrl.h"
#include "dialogEditorLocCtrl.h"
#include "dialogEditorLocAnalyser.h"
#include "patToolPanel.h"
#include "dialogEditorFlowCtrl.h"
#include "timelineLayout.h"
#include "redUserPrivileges.h"
#include "../../common/engine/soundsystem.h"

#include "../../common/game/storyScenePlayer.h"
#include "../../common/game/storySceneItems.h"
#include "../../common/engine/controlRigIncludes.h"
#include "sceneValidator.h"
#include "wxAUIRed.h"

class CEdSceneEditorScreenplayPanel;
class CEdSceneEditorDialogsetPanel;
class CGridEditor;
class CEdDialogTimeline;
class CEdDialogPreview;
class CAnimatedComponent;
class CStorySceneGraphBlock;
class CEdStorySceneEventGeneratorSetupDialog;
class CEdComponentProperties;
class CEdAnimationTreeBrowser;
class CEdControlRigPanel;
enum EEditStringId;
class CStorySceneEventLightProperties;
class ITimelineItem;

namespace SceneEditor
{
	class CMainControlPanel;
}

//#define PROFILE_DIALOG_EDITOR_TICK

//////////////////////////////////////////////////////////////////////////

class CEdStringDbConnectionTimer : public wxTimer
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Editor );

	Bool m_notified;

public:
	CEdStringDbConnectionTimer() 
		: m_notified( false )
	{}

	static int CONNECTION_CHECK_TIME ; // in ms

	void Notify();
};

//////////////////////////////////////////////////////////////////////////

class CEdSceneRunnable
{
public:
	CEdSceneRunnable() {}
	virtual ~CEdSceneRunnable() {}
	virtual void Run()=0;
};

template < typename FunctT >
class SceneRunnableFunctorWrapper : public CEdSceneRunnable
{
public:
	SceneRunnableFunctorWrapper( FunctT funct ) 
		: m_functor( funct )
	{ }

	void Run() override
	{
		m_functor();
	}

private:
	FunctT m_functor;
};

//////////////////////////////////////////////////////////////////////////

class CEdSceneEditor_Hacks;

class CEdSceneEditor	: public wxSmartLayoutPanel
						, public CSoundAdditionalListener
						, public IEdEventListener
						, public IPatToolCallback
{
	DECLARE_CLASS( CEdSceneEditor );
	DECLARE_EVENT_TABLE();

	friend class CEdSceneEditor_Hacks;
	friend class CSceneRecorder;

	// Increment SCENE_EDITOR_LAYOUT_VERSION whenever panels, windows etc. 
	// in the editor change. This way old layout data wont be applied 
	// to new panels & windows etc.
	enum { SCENE_EDITOR_LAYOUT_VERSION = 2 };

	struct ControlRequest
	{
		const CStorySceneSection*	m_section;
		
		Bool						m_timeRequest;
		Float						m_timeValue;

		CObject*					m_propertyBrowserObject;

		Bool						m_stateToRebuildRequest;
		ScenePlayerInputState		m_stateToRebuild;

		Bool						m_rebuildCurrentStateRequest;
		Bool						m_refreshCurrentStateRequest;
		Bool						m_refreshTimelineRequest;
		Bool						m_freeze;

		/*
		Request to set scene line as a background or non-background line.
		*/
		struct SceneLineStateRequest
		{
			void Reset()
			{
				m_active = false;
				m_isBackgroundLine = false;
				m_line = nullptr;
			}

			Bool m_active;				// indicates whether request is active or not
			Bool m_isBackgroundLine;	// desired state
			CStorySceneLine* m_line;	// line on which to operate
		};
		SceneLineStateRequest		m_sceneLineStateRequest;

		//...

		const CStorySceneEvent*		m_eventDuringChange;
		Uint64						m_eventDuringChangeTimestamp;

		//...

		void RequestSection( const CStorySceneSection* s )	{ m_section = s; }
		void RequestTime( Float time )						{ m_timeRequest = true; m_timeValue = Max( time, 0.f ); }
		void RequestPropBrowserObject( CObject* o )			{ m_propertyBrowserObject = o; }
		void RequestRefreshTimeline()						{ m_refreshTimelineRequest = true; }
		void RequestFreeze()								{ m_freeze = true; }

		/*
		Requests that specified scene line is set as a background or non-background line.

		\param line Scene line to operate on. Must not be nullptr.
		\param state True - line is to be set as a background line, false - line is to be set as a non-background line.
		*/
		void RequestSetAsBackgroundLine( CStorySceneLine* line, Bool state )
		{
			m_sceneLineStateRequest.m_line = line;
			m_sceneLineStateRequest.m_isBackgroundLine = state;
			m_sceneLineStateRequest.m_active = true;
		}

		void RequestRebuild( const ScenePlayerInputState& s) 
		{ 
			m_stateToRebuildRequest = true;
			m_stateToRebuild = s; 
		}

		void RequestRebuild()								
		{ 
			m_rebuildCurrentStateRequest = true; 
		}

		void RequestRefresh()								
		{ 
			m_refreshCurrentStateRequest = true; 
		}

		void InfoStartEventChanging( const CStorySceneEvent* e ) { m_eventDuringChange = e; m_eventDuringChangeTimestamp = GEngine->GetCurrentEngineTick(); }

		ControlRequest() 
		{
			ResetAllRequests();
		}

		void ResetAllRequests()
		{
			m_section = NULL;
			m_timeRequest = false;
			m_timeValue = 0.f;
			m_propertyBrowserObject = NULL;
			m_stateToRebuildRequest = false;
			m_eventDuringChange = NULL;
			m_eventDuringChangeTimestamp = 0;
			m_refreshTimelineRequest = false;
			m_rebuildCurrentStateRequest = false;
			m_freeze = false;
			m_sceneLineStateRequest.Reset();
		}
	};

	class RebuildPlayerTask : public CEdRunnable
	{
		CEdSceneEditor*		m_editor;

	public:
		RebuildPlayerTask( CEdSceneEditor* e )
			: m_editor( e )
		{
			TriggerAfter( 0.5f );
		}

		virtual void Run()
		{
			m_editor->RebuildPlayer();
		}
	};

	struct LightPreset
	{
		String							m_presetName;
		String							m_lightID;

		ELightType						m_type;

		Color							m_color;
		Float							m_radius;
		Float							m_brightness;

		Float							m_innerAngle;
		Float							m_outerAngle;
		Float							m_softness;

		ELightShadowCastingMode			m_shadowCastingMode;
		Float							m_shadowFadeDistance;
		Float							m_shadowFadeRange;

		EngineTransform					m_transform;
	};

private:
	CStoryScene*					m_storyScene;

	Bool							m_useApprovedVoDurations;	// True - use durations of approved VO, false - use durations of local VO.

	CEdSceneCtrl					m_controller;
	CEdSceneCameraCtrl				m_camera;
	Bool							m_isCameraLocked; // Prevents camera update
	CEdSceneWorldCtrl				m_worldCtrl;
	CEdSceneModCtrl					m_modCtrl;
	CEdSceneWorldProxyRelinker		m_worldRelinker;
	CEdSceneActorsProvider*			m_actorsProvider;
	CEdSceneHelperEntitiesCtrl		m_helperEntitiesCtrl;
	CEdSceneKeyframeCtrl			m_keyframeCtrl;
	CEdSceneFlowCtrl				m_flowCtrl;

	ControlRequest					m_controlRequest;
	TDynArray< CEdSceneRunnable* >	m_runnables;
	Bool							m_frozen;

	CRedUserPrivileges				m_redUserPrivileges;					// Privileges possessed by current Red User.

private:
	SceneEditor::CMainControlPanel* m_mainControlPanel;
	CEdSceneEditorScreenplayPanel*	m_screenplayPanel;
	CEdSceneEditorDialogsetPanel*	m_dialogsetPanel;
	wxPanel*						m_previewPanel;
	TDynArray<wxWindow*>			m_movedWindows;
	wxString						m_oldPerspective;
	CEdComponentProperties*			m_propertiesBrowser;
	CGridEditor*					m_startingConditionsGrid;
	CGridEditor*					m_actorsDefinitionsGrid;
	CGridEditor*					m_propsDefinitionsGrid;
	CGridEditor*					m_effectsDefinitionsGrid;
	CGridEditor*					m_lightDefinitionsGrid;
	CEdSceneGraphEditor*			m_sceneGraphEditor;
	CEdDialogTimeline*				m_timeline;
	CEdDialogPreview*				m_preview;
	CEdPropertiesPage*				m_cameraProperties;
	wxListCtrl*						m_customCamerasList;
	CEdSceneDebugger				m_debugger;
	CEdDialogFCurveEditor*			m_curveEditor;
	CEdControlRigPanel*				m_controlRigPanel;
	CPatToolPanel*					m_mimicsControlRig;
	TDynArray< Float >				m_mimicsControlRigBuffer;
	CStorySceneEventPoseKey*		m_mimicsControlRigEvt;
	Bool							m_mimicsControlRigAutoRefresh;
	CEdAnimationTreeBrowser*		m_animTreeBody;
	CName							m_animTreeBodySelectedEntity;
	CEdAnimationTreeBrowser*		m_animTreeMimics;
	CName							m_animTreeMimicsSelectedEntity;
	CEdSceneLocalizationCtrl		m_locCtrl;
	CEdSceneLocAnalyser				m_locAnalyzer;
	String							m_locPrevLang;
	CEdSceneScreenshotCtrl			m_screenshotCtrl;

	CEdAuiManager					m_auiManager;
	CEdAuiNotebook*					m_detailsTabs;
	CEdAuiNotebook*					m_mainTabs;
	CEdAuiNotebook*					m_controlTabs;

	CEdUndoManager*					m_undoManager;
	CEdStringDbConnectionTimer*		m_databaseTimer;
	CEdStorySceneEventGeneratorSetupDialog* m_generatorDialog;

	TDynArray<wxToolBarToolBase*>	m_previewTools;
	TDynArray<LightPreset>			m_lightPresets;

	CTimelineLayoutMgr				m_timelineLayoutMgr;

	CName							m_oldCameraNameCache;

	Bool							m_recordingInViewport;
	Bool							m_recordingPreviousClipForceValue;
	struct SRecordingInfo*			m_recordingInfo;

	static const Uint32				PAGE_PROPERTY = 0;
	static const Uint32				PAGE_CAMERA = 1;
	static const Uint32				PAGE_BODY_ANIMS = 4;
	static const Uint32				PAGE_MIMICS_ANIMS = 5;
	static const Uint32				PAGE_LIGHTS = 6;

public:
	CEdSceneEditor( wxWindow* parent, CStoryScene* scene );
	CEdSceneEditor( wxWindow* parent, CStorySceneController* controller );
	~CEdSceneEditor();

	void SetGridDataFromProperty( CGridEditor* gridEditor, CObject* object, const CName& propertyName );

	void SaveOptionsToConfig();
	void LoadOptionsFromConfig();

	Bool GetConfigUseApprovedVoDurations() const;
	void SetConfigUseApprovedVoDurations( Bool state );

	Bool TimelineEditingEnabled() const;

	void StoreTimelineLayout( Uint32 sectionId, const CTimelineLayout& trackLayout );
	const CTimelineLayout* GetTimelineLayout( Uint32 sectionId ) const;

	void ReloadSceneInputsTable();

	const CRedUserPrivileges& GetRedUserPrivileges() const;

public:
	CWorld* GetWorld() const;

	CStoryScenePreviewPlayer* Hack_GetPlayer() { return m_controller.HACK_GetPlayer(); }
	CEdSceneFlowCtrl* HACK_GetFlowCtrl() { return &m_flowCtrl; }

	IStorySceneDebugger* GetStorySceneDebugger();

	CActor* GetSceneActor( const CName& actorName );
	CEntity* GetSceneEntity( const CName& actorName );
	const CEntity* GetSceneEntity( const CName& actorName ) const;

	IControllerSetupActorsProvider* ResetAndGetSceneContextActorsProvider();
	void ClearAllSceneDataForActors();

public: // IPatToolCallback
	virtual void OnPatToolControlsPostChanged() override;
	virtual void OnPatToolControlsChanging() override;

public: // main control panel
	const CStorySceneSection* OnMainControlPanel_GetCurrentSection() const;
	void OnMainControlPanel_RequestSectionVariant( CStorySceneSectionVariantId variantId );
	void OnMainControlPanel_SetVariantBase( CStorySceneSectionVariantId variantId, Uint32 localeId );
	void OnMainControlPanel_SetVariantForcedInEditor( CStorySceneSectionVariantId variantId );
	void OnMainControlPanel_SetVariantAsDefault( CStorySceneSectionVariantId variantId );
	void OnMainControlPanel_RequestCreateEmptyVariant( Uint32 baseLocaleId );
	CStorySceneSectionVariantId OnMainControlPanel_RequestCloneVariant( CStorySceneSectionVariantId variantId );
	void OnMainControlPanel_RequestDeleteVariant( CStorySceneSectionVariantId variantId );
	void OnMainControlPanel_RequestDeleteAllEvents( CStorySceneSectionVariantId variantId );
	void OnMainControlPanel_SetLocaleVariantMapping( Uint32 localeId, CStorySceneSectionVariantId variantId );
	void OnMainControlPanel_UpdateCurrentVariantBase();
	Int32 OnMainControlPanel_CreateUnscaledVariantFromCurrentView();
	Bool OnMainControlPanel_LocalVoMatchApprovedVoInCurrentSectionVariant() const;
	void OnMainControlPanel_RequestChangeLocale( Uint32 localeId );
	void OnMainControlPanel_RequestRebuildImmediate();
	void OnMainControlPanel_SetConfigUseApprovedDurations( Bool state );

public: // timeline
	void OnTimeline_AddEvent( const CStorySceneSection* section, CStorySceneEvent* e, CStorySceneSectionVariantId variantId, Bool loadDataFromPreviousEvent = true );
	void OnTimeline_RemoveEvent( const CStorySceneSection* section, CStorySceneEvent* e );
	void OnTimeline_AddSceneElement( const CStorySceneSection* section, CStorySceneElement* e, Uint32 place );
	void OnTimeline_ApproveElementDuration( const CStorySceneSection* section, CStorySceneSectionVariantId variantId, CStorySceneElement* e, Float duration );
	void OnTimeline_RemoveElement( const CStorySceneSection* section, CStorySceneElement* e );
	void OnTimeline_EventChanged( const CStorySceneSection* section, CStorySceneEvent* event, const CName& propertyName );
	void OnTimeline_EventChanged( const CStorySceneSection* section, CStorySceneEvent* e );
	void OnTimeline_SelectionChanged( const CStorySceneSection* section, ITimelineItem* item, Bool goToEvent = false );
	void OnTimeline_RequestSetTime( wxCommandEvent& event );
	void OnTimeline_RequestRebuild();
	void OnTimeline_RequestRefreshTimeline();
	void OnTimeline_GetCurrentCameraDefinition( StorySceneCameraDefinition& cameraDef );
	Bool OnTimeline_CreateCustomCameraFromView();
	Bool OnTimeline_CreateCustomCameraEnhancedBlendInterpolated();
	Bool OnTimeline_CreatePlacementOverrideBlendInterpolated();
	Float OnTimeline_GetAnimationDurationFromEvent( const CStorySceneEventAnimClip* animClip  );
	Float OnTimeline_GetBodyAnimationDuration( const CName& actorVoicetag, const CName& animationName );
	Float OnTimeline_GetMimicAnimationDuration( const CName& actorVoicetag, const CName& animationName );
	Float OnTimeline_GetCameraAnimationDuration( const CName& animationName );
	Bool OnTimeline_GetActorPlacementLS( const CName& actorVoicetag, EngineTransform& outPlacement ) const;
	Bool OnTimeline_GetActorPelvisPlacementLS( const CName& actorVoicetag, EngineTransform& outPlacement ) const;
	EngineTransform OnTimeline_GetScenePlacement() const;
	void OnTimeline_StartEventChanging( const CStorySceneSection* section, CStorySceneEvent* e );
	void OnTimeline_StateChanged();
	Bool OnTimeline_GetCurrentActorAnimationState( const CName& actor, SStorySceneActorAnimationState& out ) const;
	CStorySceneDialogsetSlot* OnTimeline_GetAndChangeActorDialogsetSlot( const CName& actor ) const;
	const CStorySceneLight* OnTimeline_GetLightDefinition( const CName& lightId ) const;
	CStorySceneLight* OnTimeline_GetAndChangeLightDefinition( const CName& lightId ) const;
	CName OnTimeline_GetPrevSpeakerName( CStorySceneElement* currElement ) const;
	void OnTimeline_GetEventTime( const CStorySceneEvent* e, Float& start, Float& duration ) const;
	Float OnTimeline_GetEventInstanceDuration( const CStorySceneEvent& e ) const;
	void OnTimeline_SetEventInstanceDuration( const CStorySceneEvent& e, Float duration ) const;
	Float OnTimeline_GetEventInstanceStartTime( const CStorySceneEvent& e ) const;
	void OnTimeline_SetEventInstanceStartTime( const CStorySceneEvent& e, Float startTime ) const;
	Float OnTimeline_GetEventScalingFactor( const CStorySceneEvent& e ) const;
	void OnTimeline_GetDisableDialogLookatEventsPositions( CName actorName, CName animName, TDynArray< TPair< Float, Float > >& dialogAnimEvents, Bool mimicAnim );
	void OnTimeline_GetKeyPoseMarkersEventsPositions( CName actorName, CName animName, TDynArray< Float >& dialogAnimEvents, Bool mimicAnim );
	void OnTimeline_GetKeyPoseDurationsEventsPositions( CName actorName, CName animName, TDynArray< TPair< Float, Float > >& dialogAnimEventsTrans, TDynArray< TPair< Float, Float > >& dialogAnimEventsPoses, Bool mimic );
	Bool OnTimeline_GetVoiceDataPositions( CStorySceneLine* forLine, TDynArray< TPair< Float, Float > >* maxAmpPos, Float* voiceStartPos = NULL , Float* voiceEndPos = NULL);
	const CAnimatedComponent* OnTimeline_GetBodyComponentForActor( const CName& actor );
	const CAnimatedComponent* OnTimeline_GetMimicComponentForActor( const CName& actor );
	CWorld* OnTimeline_GetWorld() const;
	const StorySceneCameraDefinition* OnTimeline_GetCameraDefinition( CName cameraName ) const;
	void OnTimeline_SetAsBackgroundLine( CStorySceneLine* line, Bool state );
	void OnTimeline_CameraBlendNewPlot( const CStorySceneCameraBlendEvent* e, const TDynArray< String >& tracks );
	void OnTimeline_GetMimicIdleData( TDynArray<CName>& data, Float& poseWeight, CName actor ) const;
	void OnTimeline_TrackSelectionChanged( const String& trackName );
	Bool OnTimeline_GetPreviousActorAnimationState( const CName& actorName, SStorySceneActorAnimationState& state ) const;
	Bool OnTimeline_UseLocColors() const;
	wxColor OnTimeline_FindLocColor( Uint32 stringId ) const;
	const IStorySceneElementInstanceData* OnTimeline_FindElementInstance( const CStorySceneElement* element ) const;
	Bool OnTimeline_IsTimelineEditingEnabled() const;
	Bool OnTimeline_AdjustCameraForActorHeight( const StorySceneCameraDefinition& def, EngineTransform* outAdjusted ) const;
	void OnTimeline_ToggleTimePause();
	void OnTimeline_GoToTime( Float time );
	void OnTimeline_InterpolationEventPlot( const CStorySceneEventInterpolation* interpolationEvent, const TDynArray< String >& tracks );
	void OnTimeline_OnItemPropertyChanged( wxCommandEvent& event );
	void OnTimeline_RefreshPlayer();
	void OnTimeline_RebuildPlayer();
	Vector OnTimeline_CalcLightSpawnPositionSS() const;
	Vector OnTimeline_CalcLightSpawnPositionWS() const;
	void OnTimeline_GetVoiceTagsForCurrentSetting( TDynArray< CName >& vt ) const;
	void OnTimeline_GetCurrentLightState( CName lightId, SStorySceneAttachmentInfo& out, EngineTransform& outPos ) const;
	void OnTimeline_ToggleIsInteractiveEntityHelper();

public: // preview
	void OnPreview_ViewportTick( Float timeDelta );
	Bool OnPreview_CalculateCamera( IViewport* view, CRenderCamera &camera ) const;
	void OnPreview_GenerateFragments( IViewport *view, CRenderFrame *frame );
	Bool OnPreview_ViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data, Bool& moveViewportCam );
	void OnPreview_FreeCameraMode();
	void OnPreview_PreviewCameraMode();
	void OnPreview_EditCameraMode();
	EScenePreviewCameraMode OnPreview_GetCameraMode();
	void OnPreview_CameraMoved();
	Bool OnPreview_CreateCustomCameraFromView();
	Bool OnPreview_PlayInMainWorld( Bool ingame );
	void OnPreview_OpenCutscene();
	void OnPreview_AddLight();
	void OnPreview_GenerateFragmentsForCurrentCamera( IViewport *view, CRenderFrame *frame );

	// TODO
	void OnPreviewCameraPreviewMode( wxCommandEvent& event );
	void OnPreviewCameraFreeMode( wxCommandEvent& event );
	void OnPreviewCameraEditMode( wxCommandEvent& event );
	void OnNewKeyframe( wxCommandEvent& event );
	void OnPreviewPlayToggle( wxCommandEvent& event );
	void OnPreviewPlayFromStart( wxCommandEvent& event );

	void OnRecord( wxCommandEvent& event );

public: // screenplay panel
	void OnScreenplayPanel_ElementPanelFocus( const CStorySceneSection* section, CStorySceneElement* e );
	void OnScreenplayPanel_ChoiceLineChildFocus( const CStorySceneSection* section, CStorySceneChoiceLine* line );
	void OnScreenplayPanel_SectionPanelSectionFocus( const CStorySceneSection* section );
	void OnScreenplayPanel_SectionPanelSectionChildFocus( CStorySceneSection* section );
	void OnScreenplayPanel_ChoicePanelClick( CStorySceneChoice* ch );
	void OnScreenplayPanel_PostChangeSceneElements();
	void OnScreenplayPanel_RemoveSection( CStorySceneSection* section );
	CName OnScreenplayPanel_GetPrevSpeakerName( const CStorySceneElement* currElement ) const;
	const CStorySceneLine* OnScreenplayPanel_GetPrevLine( const CStorySceneElement* currElement );
	void OnScreenplayPanel_ApplyChanges();
	void OnScreenplayPanel_RequestRebuildImmediate();
	
public: // Definitions
	void OnDefinitions_PropertiesChanged( wxCommandEvent& event );

public: //EvtGenerator
	const CStorySceneLine* OnEvtGenerator_GetPrevLine( const CStorySceneElement* currElement );
	CActor* OnEvtGenerator_GetSceneActor( CName actorName ){ return GetSceneActor( actorName ); }
	void OnEvtGenerator_GetMarkedLockedEvents( TDynArray< CStorySceneEvent*>& marked, TDynArray< CStorySceneEvent*>& locked );
	void OnEvtGenerator_RunEventsGenerator( CDialogEventGeneratorConfig & dialogConfig );
	Bool OnEvtGenerator_GetVoiceDataPositions( CStorySceneLine* forLine, TDynArray< TPair< Float, Float > >* maxAmpPos, Float* voiceStartPos = NULL , Float* voiceEndPos = NULL);
	Bool OnEvtGenerator_IsTimelineTrackLocked( const String& trackName ) const;
	EngineTransform OnEvtGenerator_GetCurrentScenePlacement();
	void OnEvtGenerator_GetMimicIdleData( TDynArray<CName>& data, Float& poseWeight, CName actor ) const;
	void OnEvtGenerator_PinTimelineTrack( const String& track );

public: // graph editor
	void OnGraph_AddSceneBlock( CStorySceneGraphBlock* block );
	void OnGraph_AddSceneBlocks( const TDynArray< CStorySceneGraphBlock* >& blocks );
	void OnGraph_RemoveSceneBlocks( const TDynArray< CStorySceneGraphBlock* >& blocks );
	Int32 OnGraph_GetNumBlocks( const CClass* blockClass ) const;
	Bool OnGraph_IsBlockActivated( const CStorySceneControlPart* cp ) const;
	Float OnGraph_GetBlockActivationAlpha( const CStorySceneControlPart* cp ) const;
	void OnGraph_ToggleSelectedInputLinkElement();
	void OnGraph_ToggleSelectedOutputLinkElement();
	CName OnGraph_GetCurrentDialogsetName() const;

public: // temporary lipsync dialog
	const CStorySceneSection* OnTempLipsyncDlg_GetCurrentSection() const;
	CStoryScene* OnTempLipsyncDlg_GetScene() const;
	void OnTempLipsyncDlg_RefreshLipsyncs();

public: // edit mimics dialog
	const THashMap< CName, THandle< CEntity > >& OnEditMimicsDialog_GetActorMap() const;
	CActor* OnEditMimicsDialog_GetSceneActor( CName actorName );

public: // import W2 strings dialog
	const CStorySceneSection* OnImportW2StringsDlg_GetCurrentSection() const;
	void OnImportW2StringsDlg_RefreshScreenplay() const;

public: // cameras page
	//...

public: // dialogset panel
	const CStoryScene* OnDialogestPanel_GetScene() const { return m_storyScene; }
	void OnDialogsetPanel_SlotPropertyChanged();
	void OnDialogsetPanel_PropertyChanged();

public: // world controller
	void OnWorldCtrl_ViewportTick( Float timeDelta );
	Bool OnWorldCtrl_CalculateCamera( IViewport* view, CRenderCamera &camera ) const;
	void OnWorldCtrl_GenerateFragments( IViewport *view, CRenderFrame *frame );
	void OnWorldCtrl_PreModeChanged();
	void OnWorldCtrl_PostModeChanged();
	void OnWorldCtrl_CameraMoved();
	Bool OnWorldCtrl_ViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data );

public: // debugger
	const THashMap< CName, THandle< CEntity > >& OnDebugger_GetActorMap();
	Bool OnDebugger_GetActorAnimationState( const CName& actor, SStorySceneActorAnimationState& out ) const;
	void OnDebugger_GetActorAnimationNames( const CName& actor, TDynArray< CName >& animations ) const;
	CAnimatedComponent* OnDebugger_GetAnimatedComponentForActor( const CName& actorName );
	Matrix OnDebugger_GetActorPosition( const CName& actorName ) const;
	Matrix OnDebugger_GetDialogPosition() const;

public: // property subeditors
	CAnimatedComponent* OnCameraAnimationSelector_GetCameraComponent();
	CAnimatedComponent* OnMimicsAnimationSelector_GetHeadComponentForActor( const CName& actorName );
	CAnimatedComponent* OnMimicsGestureAnimationSelector_GetHeadComponentForActor( const CName& actorName );
	CAnimatedComponent* OnMimicPoseSelector_GetHeadComponentForActor( const CName& actorName );
	CAnimatedComponent*	OnDialogAnimationFilter_GetBodyComponentForActor( const CName& actorName );
	CAnimatedComponent*	OnDialogAnimationFilter_GetMimicsComponentForActor( const CName& actorName );
	Bool OnDialogAnimationFilter_GetPreviousActorAnimationState( const CName& actorName, SStorySceneActorAnimationState& state ) const;
	const TDynArray<CName> OnVoicetagSelector_GetActorIds( Int32 actorTypes ) const; //See EDialogActorType for actorTypes values
	const CStorySceneSection* OnDialogLineSelector_GetCurrentSection() const;

public: // helper entity
	void OnHelperEntity_RefreshProperty( const CGUID& id, Bool pos, Bool rot, Bool scale = false );
	CWorld* OnHelperEntity_GetWorld();
	void OnHelperEntity_ClearSelection();
	void OnHelperEntity_SelectionChanged();
	void OnHelperEntity_SetEditString( const String& str );
	Bool OnHelperEntity_IsSomethingFromScene( const CComponent* c ) const;
	CNode* OnHelperEntity_SelectObjectFromScene( const CComponent* c );
	CNode* OnHelperEntity_SelectObjectFromWorld( const CComponent* c );
	Bool OnHelperEntity_IsPointOnScreen( const Vector& pointWS, Vector& pointOnScreen ) const;
	Bool OnHelperEntity_FloatingHelpersEnabled() const;  
	void OnHelperEntity_GenerateFragmentsForLight( CName lightId, CRenderFrame* frame );
	void OnHelperEntity_GenerateFragmentsForDimmers( CName lightId, CRenderFrame* frame );

	void OnHelperEntity_GetRigIKTransform( const CGUID& id, Vector& outPos, EulerAngles& outRot );
	void OnHelperEntity_SetRigIKPosition( const CGUID& id, const Vector& prevPos, const Vector& newPos );
	void OnHelperEntity_SetRigIKRotation( const CGUID& id, const EulerAngles& prevRot, const EulerAngles& newRot );

public: // control rig panel
	void OnControlRig_SetPreviewEditString( const String& txt, EEditStringId strid = EEditStringId( 0  /* == EDIT_Default */ ) );
	void OnControlRig_HelperEntityCreate( const CGUID& id, EngineTransform& transformWS, Bool& dirtyFlag, const String& name, const CEdSceneHelperShapeSettings* s );
	void OnControlRig_HelperEntityDestroy( const CGUID& id );
	void OnControlRig_HelperEntitySelect( const CGUID& id );
	Bool OnControlRig_HelperEntityIsSelected( const CGUID& id );
	void OnControlRig_HelperEntityVisible( const CGUID& id );
	void OnControlRig_HelperEntityUpdateTransform( const CGUID& id, const EngineTransform& transformWS );
	void OnControlRig_HelperEntityUpdateTransform( const CGUID& id, const EngineTransform& transformWS, const EngineTransform& refTransformWS );
	CStorySceneEvent* OnControlRig_FindSelectedEvent( const CClass* c, const CName& actorId );
	void OnControlRig_RefreshPlayer();
	Bool OnControlRig_GetCurrIdleAnimationName( CName actorId, CName& animName, Float& animTime ) const;
	Bool OnControlRig_GetParentAnimationName( const CStorySceneEvent* e, CName& animName, Float& animTime ) const;

public: // keyframe controller
	void OnKeyframeCtrl_HelperEntityCreate( const CGUID& id, EngineTransform& transformWS );
	CEdSceneHelperEntity* OnKeyframeCtrl_FindHelperEntity( const CGUID& id );
	void OnKeyframeCtrl_DeselectAllHelperEntities();
	Bool OnKeyframeCtrl_HasAnyEventNow( const CClass* c, CName id );
	CStorySceneEvent* OnKeyframeCtrl_GetSelectedEvent();
	CStorySceneEvent* OnKeyframeCtrl_CloneEvent( const CStorySceneEvent* e );
	EngineTransform OnHelperEntity_GetSceneToWorld() const;
	const CActor* OnKeyframeCtrl_AsSceneActor( const CEntity* e ) const;
	const CEntity* OnKeyframeCtrl_AsSceneProp( const CEntity* e ) const;
	const CEntity* OnKeyframeCtrl_AsSceneLight( const CEntity* e ) const;

public: // network
	void OnNetwork_SetCamera( const Vector& pos, const EulerAngles& rot, Float fov );

public: // Screenshot panel
	Bool OnScreenshotPanel_ForceMainWorld();
	CWorld* OnScreenshotPanel_GetWorld();
	CStoryScene* OnScreenshotPanel_GetScene();
	CStorySceneDialogsetInstance* OnScreenshotPanel_GetCurrentDialogset();
	EngineTransform OnScreenshotPanel_GetCurrentDialogsetTransform() const;
	void OnScreenshotPanel_AddEntityToScene();

public: // Actors provider
	Vector OnActorsProvider_CalcLightSpawnPositionSS() const;
	EngineTransform OnActorsProvider_GetSceneToWorld() const;
	CWorld* OnActorsProvider_GetWorld() const;
	void OnActorsProvider_AddExtraActors( TDynArray< THandle< CEntity > >& actors, TDynArray< THandle< CEntity > >& props ) const;

public:
	void OnTimeline_LockCamera( Bool lock );
	void ActivateCameraFromList( CName cameraName );
	void ActivateCustomCamera( CStorySceneEventCustomCamera* camera );
	void ActivateCameraFromDefinition( StorySceneCameraDefinition * cameraDef, CStorySceneEvent* cameraDefOwner = NULL );

	void OnTimeline_SaveCameraToDefinition( StorySceneCameraDefinition* cameraDefinition );
	void SaveCameraToDefinition( StorySceneCameraDefinition* cameraDefinition );
	void GetMimicIdleData( TDynArray<CName>& data, Float& poseWeight, CName actor ) const;

	void RefreshBodyAnimTree();
	void RefreshMimicsAnimTree();
	void FreezeBodyAnimTree();
	void FreezeMimicsAnimTree();

	void RefreshMimicKeyEventData();

	void RefreshMainControlPanel();

private:
	void SaveActorHeigthInCameraDef( StorySceneCameraDefinition* cameraDefinition );
	Bool UpdateCameraFromDefinition( StorySceneCameraDefinition* cameraDefinition );
	void OnViewportCameraMoved();
	void ToggleTimePause();
	Bool PlayInWorld( Bool previewWorld, Bool mainWorld, Bool gameplayWorld, Bool record );
	Bool PlayInMainWorld( Bool ingame );
	Bool PlayInGameplayWorld( Bool ingame );

public: // DIALOG_TOMSIN_TODO - ta cxala lista ma byc wywalona - wszystko ma przechodzic przez mediatora
	CStoryScene*					HACK_GetStoryScene() const { return m_storyScene; }
	CEdSceneEditorScreenplayPanel*	HACK_GetScreenplayPanel() const { return m_screenplayPanel; }
	CEdSceneGraphEditor*			HACK_GetSceneGraphEditor() const { return m_sceneGraphEditor; }
	CEdDialogPreview*				HACK_GetScenePreview() const { return m_preview; }

	RED_INLINE wxListCtrl*		HACK_GetCustomCamerasList() const { return m_customCamerasList; }

private:
	CStoryScene*					GetStoryScene() const { return m_storyScene; }
	CEdSceneEditorScreenplayPanel*	GetScreenplayPanel() const { return m_screenplayPanel; }
	CEdSceneGraphEditor*			GetSceneGraphEditor() const { return m_sceneGraphEditor; }
	CEdDialogPreview*				GetScenePreview() const { return m_preview; }

	RED_INLINE wxListCtrl*		GetCustomCamerasList() const { return m_customCamerasList; }

	CEdPropertiesPage*				GetCamerasPage() const { return m_cameraProperties; }

	Float							GetAnimationDurationFromEvent( const CStorySceneEventAnimClip* animClip );
	Float							GetBodyAnimationDuration( const CName& actorVoicetag, const CName& animationName );
	Float							GetMimicAnimationDuration( const CName& actorVoicetag, const CName& animationName );
	Float							GetCameraAnimationDuration( const CName& animationName );

public:
	// DIALOG_TOMSIN_TODO - to nie public
	CAnimatedComponent*		GetAnimatedComponentForActor( const CName& actorName );
	void					RefreshPropertiesPage();

	const CStorySceneEvent* FindPrevEventOfClass( const CStorySceneEvent* evt, const CClass* cls ) const;
	CStorySceneEvent* FindPrevEventOfClass( const CStorySceneEvent* evt, const CClass* cls );
	
	template< typename T >
	const T* FindPrevEventOfType( const T* evt ) const 
	{
		return static_cast< const T*>( FindPrevEventOfClass( evt, evt->GetClass() ) );
	}

	template< typename T >
	T* FindPrevEventOfType( const T* evt )
	{
		return static_cast<T*>( FindPrevEventOfClass( evt, evt->GetClass() ) );
	}

	static CEdSceneEditor*	RetrieveSceneEditorObject( CPropertyItem* propertyItem );

	const CStorySceneDialogsetInstance* GetCurrentDialogsetInstance() const;

public: // Timeline animations
	void SelectItemWithAnimation( const CStorySceneEvent* e );
	CStorySceneEvent* CreateAndSelectEventAndItemWithAnimation( CClass* c, const CName& actorId = CName::NONE );

public:
	CStorySceneDialogsetInstance*	CreateNewDialogset();
	CStorySceneDialogsetInstance*	CreateDialogsetFromFile();
	CStorySceneDialogsetInstance*	ReloadDialogsetFromFile( const CName& dialogsetName );
	CStorySceneDialogsetInstance*	CreateDialogsetFromPreviousSection( CStorySceneSection* currentSection );
	CStorySceneDialogsetInstance*	CreateDialogsetFromCurrentSection( CStorySceneSection* currentSection );
	CStorySceneDialogsetInstance*	CreateDialogsetCopy( const CStorySceneDialogsetInstance* dialogsetInstance );

	void SaveDialogsetToFile( const CName& dialogsetName ) ;

public:
	Bool CanModifyScene() const;
	Bool CanModifyEntityFromWorld( const CEntity* e ) const;

	Bool IsCameraUsedInEvent( CName name, String& sectionName ) const;

protected:
	Bool DeleteCameraFromList( CName name );

protected:
	void ProcessEditorState( CEdSceneValidator::SValidationOutput& ret );
	void ProcessSpawnedEntities( CEdSceneValidator::SValidationOutput& ret );

protected:
	void OnMenuExit( wxCommandEvent& event );
	void OnMenuDumpAnimations( wxCommandEvent& event );
	void OnSaveScene( wxCommandEvent& event );
	void OnEditCopy( wxCommandEvent& event );
	void OnEditCut( wxCommandEvent& event );
	void OnEditPaste( wxCommandEvent& event );
	void OnEditDelete( wxCommandEvent& event );
	void DoEditDelete();
	void OnEditUndo( wxCommandEvent& event );
	void OnEditRedo( wxCommandEvent& event );

	void OnLargerFont( wxCommandEvent& event );
	void OnSmallerFont( wxCommandEvent& event );
	void OnEditorClosed( wxCommandEvent& event );

	void OnPlayToggle( wxCommandEvent& event );
	void OnPauseButton( wxCommandEvent& event );
	void OnStopButton( wxCommandEvent& event );
	void OnContinueButton( wxCommandEvent& event );
	void OnPrevButton( wxCommandEvent& event );
	void OnNextButton( wxCommandEvent& event );
	void OnPlayFromStart( wxCommandEvent& event );
	void OnPrevSectionMenu( wxCommandEvent& event );
	void OnNextSectionMenu( wxCommandEvent& event );
	void OnResetButton( wxCommandEvent& event );
	void OnDebugTimelineItems( wxCommandEvent& event );
	void OnPlayOneFrame( wxCommandEvent& event );

	void OnShowVoiceMarkers( wxCommandEvent& event );
	void OnRunEventsGenerator( wxCommandEvent& event );
	void OnClearEventsButton( wxCommandEvent& event );

	void OnShowLocalizationTooltips( wxCommandEvent& event );
	void OnShowStartingConditions( wxShowEvent& event );
	void OnTemporaryLipsync( wxCommandEvent& event );
	void OnUpdateSectionsOrder( wxCommandEvent& event );
	void OnAddPlayerToActors( wxCommandEvent& event );
	void OnEditMimics( wxCommandEvent& event );
	void OnImportW2Strings( wxCommandEvent& event );
	void OnPropagateCameraLights( wxCommandEvent& event );
	void OnDebuggerConnected( wxCommandEvent& event );

	void OnToggleShowOnlyScriptTexts( wxCommandEvent& event );
	void OnEnableDefaultLayout( wxCommandEvent& event );
	void OnEnableWritersLayout( wxCommandEvent& event );
	void OnEnableDesignersLayout( wxCommandEvent& event );
	void OnEnableDirectorsLayout( wxCommandEvent& event );
	void OnResetLayout( wxCommandEvent& event );
	void SaveCustomLayout( wxCommandEvent& event );
	void LoadCustomLayout( wxCommandEvent& event );

	void OnTimelineResized( wxCommandEvent& event );

	void OnResize( wxSizeEvent& event );

	void OnActorListChanged( wxCommandEvent& event );
	void OnPropListChanged( wxCommandEvent& event );
	void OnEffectListChanged( wxCommandEvent& event );
	void OnLightListChanged( wxCommandEvent& event );

	void OnCustomCamerasListDblClick( wxMouseEvent& event );
	void OnCameraDefinitionChanged( const StorySceneCameraDefinition* def );
	void OnCameraDefinitionChanged( wxCommandEvent& event );
	void OnCameraSelectedFromList( wxCommandEvent& event );
	void OnRightClickCameraFromList( wxCommandEvent& event );
	void OnDeleteCameraFromList( wxCommandEvent& event );
	void OnDeleteAllCamerasFromList( wxCommandEvent& event );
	void OnDeleteUnusedCameras( wxCommandEvent& event );
	void OnDuplicateCamera( wxCommandEvent& event );
	void OnGenerateSpeakingToData( wxCommandEvent& event );
	void OnDisableAdjustInCameraDefs( wxCommandEvent& event );
	void OnEnableAdjustInCameraDefs( wxCommandEvent& event );

	void OnMimicsControlRigPanelRefresh( wxCommandEvent& event );
	void OnMimicsControlRigPanelAutoRefresh( wxCommandEvent& event );
	void OnMimicsControlRigPanelWeightRefresh( wxCommandEvent& event );

	void OnDetailsTabsPageChanged( wxAuiNotebookEvent& event );

	void OnToolConvertCameraBlends( wxCommandEvent& event );
	void OnToolFixCameraBlendsEndTime( wxCommandEvent& event );
	void OnToolRemoveScale( wxCommandEvent& event );
	void OnToolLocCtl( wxCommandEvent& event );
	void OnToolScreenshotCtl( wxCommandEvent& event );
	void OnLocAnalyzer( wxCommandEvent& event );
	void OnChangeLanguage( wxCommandEvent& event );
	void OnChangeLanguageToggle( wxCommandEvent& event );
	void OnMuteSpeech( wxCommandEvent& event );
	void OnPreviewGridChanged( wxCommandEvent& event );
	void OnPreviewTrackSun( wxCommandEvent& event );
	void OnFloatingHelpersCheckbox( wxCommandEvent& event );

	void OnLightPresetLoad( wxCommandEvent& event );
	void OnRefreshVoiceovers( wxCommandEvent& event );
	// SCENE_TOMSIN_TODO - tego nie powinno byc - zawsze sa same hacki w srodku takich funckji
	virtual void DispatchEditorEvent( const CName& name, IEdEventData* data );

public:
	void OnToolLocLinkClicked( wxHtmlLinkEvent& event );
	void OnToolLocLinkHover( wxHtmlCellEvent& event );
	void OnLocAnalyzerCommonEvt( wxCommandEvent& event );

protected:
	void OnStartBehaviorDebug( wxCommandEvent& event );
	void OnCameraModeChanged();

	void OnSaveLayerPresets( wxCommandEvent& event );
	void OnLoadLayerPresets( wxCommandEvent& event );

	void OnCreateLightPreset( wxCommandEvent& event );

	void MovePreviewWindow( bool previewInEditor );
	void MovePreviewToolbarToTimeline( bool previewInEditor );

	void SelectDetailPage( const wxString& pageName );
	void SelectDetailPage( Uint32 pageId );
	Bool IsDetailPageOpened( Uint32 pageId ) const;

	void ValidateSceneAfterOpen( IFeedbackSystem* f );

protected:
	Bool IsFocusInsideWindow( wxWindow* focusWindow, wxWindow* panelToTest ) const;	//! Helper method

	CStorySceneSection* GetPrecedingSection( const CStorySceneSection* section );
	Bool GetVoiceDataPositions( CStorySceneLine* forLine, TDynArray< TPair< Float, Float > >* maxAmpPos, Float* voiceStartPos = NULL , Float* voiceEndPos = NULL);

	const CActor* AsSceneActor( const CEntity* e ) const;
	const CEntity* AsSceneProp( const CEntity* e ) const;
	const CEntity* AsSceneLight( const CEntity* e ) const;
	String GetEntityIdStr( const CEntity* e ) const;

	void RequestSection( const CStorySceneSection* section );

	void RebuildPlayer();
	void RebuildPlayer( const CStorySceneSection* startFromSection );
	void RebuildPlayer_Lazy();
	void RefreshPlayer();

	void RebuildLights();
	void RefreshLights();

	void FreezeEditor();
	void UnfreezeEditor();
	Bool IsEditorFrozen() const;

	void RefreshCamerasList();
	void DuplicateCamera();
	Bool CreateCustomCameraFromView();
	Bool CreateCustomCameraEnhancedBlendInterpolated();
	void OnToggleDoF( wxCommandEvent& commandEvent );

	void GoToEvent( const CStorySceneEvent* e );

	CAnimatedComponent* GetCameraComponent();
	CAnimatedComponent*	GetHeadComponentForActor( const CName& actorName );
	CActor*				GetActorOnDialogsetSlot( CName slotName, Uint32 slotNumber = 0 );

	void ProcessViewportTick( Float timeDelta );
	void ProcessGenerateFragments( IViewport *view, CRenderFrame *frame );
	Bool ProcessCalculateCamera( IViewport* view, CRenderCamera &camera ) const;
	Bool ProcessViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data, Bool& moveViewportCam );

	void SaveSectionNameToConfig();
	void LoadSectionNameFromConfig();
	void SaveTracksStateToConfig();
	void LoadTracksStateFromConfig();

	void RelinkAllEventsForElement( CStorySceneElement* element );

	CEdSceneHelperEntity* CreateHelperEntityForEvent( CStorySceneEvent* e );
	void UpdateHelperEntityForEvent( CStorySceneEvent* e );
	Bool UpdateHelperEntityForEvent( CStorySceneEvent* e, CEdSceneHelperEntity* helper );

	CStorySceneEvent* FindEditorEvent( const CClass* c, const CName& actorId, const Float* time );
	CStorySceneEvent* FindSelectedEvent( const CClass* c, const CName& actorId );

	void UpdateLightPresets();
	void UpdateScriptBlocks();
	void CheckScenePlacement();
	Vector CalcBestSpawnPositionSS() const;
	Vector CalcBestSpawnPositionWS() const;

	void RemoveEvent( CStorySceneEvent* e );

public:
	void ResetPropertiesBrowserObject()
	{
		if ( m_propertiesBrowser )
		{
			m_propertiesBrowser->SetComponent( nullptr );
			m_propertiesBrowser->SetNoObject();
		}
	}

	template < class T >
	void SetPropertiesBrowserObject( T* object, CComponent* objectOwner )
	{
		if ( m_propertiesBrowser )
		{
			m_propertiesBrowser->SetComponent( objectOwner );
			m_propertiesBrowser->SetObject( object );
		}
	}

	void ResetMimicControlRigObject();
	void SetMimicControlRigObject( CStorySceneEventPoseKey* e );

	void ResetBodyControlRigObject();
	void SetBodyControlRigObject( CStorySceneEventPoseKey* e, CActor* a );

	void ResetFCurvePanel();
	void SetFCurveDataToPlot( const TDynArray< String >& tracks, const TDynArray< TDynArray< TPair< Vector2, Vector > > >& data, Bool setFocus = false );

	Bool SetTreeAnimationSelection( CStorySceneEvent* e );
	Bool IsAnyAnimTreeOpen() const;

	static void GetPrecedingSections( const CStorySceneSection* section, TDynArray< CStorySceneSection* >& prevSections, CStoryScene* storyScene );
	static void GetFollowingSections( const CStorySceneSection* section, TDynArray< TPair< CStorySceneSection*, String > >& nextSections );

public:
	template < typename FunctT >
	void RunLaterOnce( FunctT funct )
	{
		RunLaterOnceEx( new SceneRunnableFunctorWrapper< FunctT >( funct ) );
	}

	void RunLaterOnceEx( CEdSceneRunnable* runnable );
};

RED_INLINE Bool CEdSceneEditor::GetConfigUseApprovedVoDurations() const
{
	return m_useApprovedVoDurations;
}

RED_INLINE void CEdSceneEditor::SetConfigUseApprovedVoDurations( Bool state )
{
	m_useApprovedVoDurations = state;
}

RED_INLINE Bool CEdSceneEditor::TimelineEditingEnabled() const
{
	return m_controller.LocalVoMatchApprovedVoInCurrentSectionVariant() || m_controller.UseApprovedDurations();
}
