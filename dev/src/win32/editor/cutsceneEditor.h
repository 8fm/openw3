/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "previewPanel.h"
#include "previewItem.h"
#include "utils.h"
#include "../../common/game/inventoryEditor.h"
#include "wx/socket.h"
#include "../../common/engine/behaviorGraphAnimationMixerSlot.h"
#include "../../common/engine/cutscene.h"
#include "../../common/engine/selectionManager.h"

class CEdPropertiesPage;
class CEdCutsceneEditor;
class CEdPropertiesBrowserWithStatusbar;
class CEdCutsceneEditorPreview;
class CEdCutsceneTimeline;
struct SCutsceneActorLine;
class CEdEffectEditor;

//////////////////////////////////////////////////////////////////////////

class CEffectPreviewItem : public IPreviewItem
{
public:
	CEffectPreviewItem( CEdCutsceneEditor* csEditor );

	virtual Bool IsValid() const override;

	virtual void Refresh() override;

	virtual void SetPositionFromPreview( const Vector& prevPos, Vector& newPos ) override;
	virtual void SetRotationFromPreview( const EulerAngles& prevRot, EulerAngles& newRot ) override;

	virtual IPreviewItemContainer* GetItemContainer() const override;

	virtual void Init( const String& name ) override;

protected:
	const CExtAnimCutsceneEffectEvent* GetEvent() const;
	CEntity* GetEffect() const;

protected:
	CEdCutsceneEditor*	m_editor;

private:
	Bool				m_isRefreshing;
};

//////////////////////////////////////////////////////////////////////////

class CEdCutsceneEditorPreview	: public CEdPreviewPanel
								, public IEdEventListener
{
	CEdCutsceneEditor*	m_csEditor;
	Bool				m_enabled;
	Bool				m_showSkeletons;

public:
	CEdCutsceneEditorPreview( wxWindow* parent, CEdCutsceneEditor *csEditor );
	~CEdCutsceneEditorPreview();

	void SetEnabled( Bool flag );
	Bool ToggleDispSkeletons();

	ERPWidgetMode GetWidgetMode() const;
	void SetWidgetMode( ERPWidgetMode mode );

	virtual void DispatchEditorEvent( const CName& name, IEdEventData* data );
	virtual void HandleSelection( const TDynArray< CHitProxyObject* >& objects );

	virtual void OnViewportCalculateCamera( IViewport* view, CRenderCamera &camera );
	virtual void OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame );
	virtual void OnViewportTick( IViewport* view, Float timeDelta );

protected:
	void DisplayBBox( CRenderFrame *frame );
	void DisplaySkeletons( CRenderFrame *frame );
	void DisplaySkeleton( CRenderFrame *frame, const CAnimatedComponent* component, Color color );


};

//////////////////////////////////////////////////////////////////////////

class CEdCutsceneEditor : public wxFrame
						, public ISavableToConfig
						, public ISmartLayoutWindow
						, public IViewportHook
						, public IEdEventListener
						, public IPreviewItemContainer
{
	DECLARE_EVENT_TABLE()

public:
	enum ECutsceneEditorMode { M_Preview, M_Game, M_None };

	typedef TPair< Float, String > tCsEdDialogLine;

protected:
	ECutsceneEditorMode					m_mode;

	CCutsceneTemplate*					m_csTemplate;
	CCutsceneInstance*					m_csInstance;

	Bool								m_repeat;
	Float								m_timeMul;
	Bool								m_showFloor;

	CCameraComponent*					m_csCamera;
	IViewportHook*						m_prevHook;

	CutsceneImporterParams				m_importParams;

	Int32								m_selectEvent;
	CName								m_selectAnimation;

	TDynArray< tCsEdDialogLine >		m_dialogLines;
	TDynArray< SCutsceneActorLine >		m_actorLines;

	TDynArray< CItemEntity* >			m_itemsInPreview;

	TDynArray< TPair< StringAnsi, TPair< CBehaviorMixerSlotInterface, CBehaviorMixerSlotInterface > > > m_mixers;

	Bool								m_recordingInViewport;

	static const Int32					DIALOG_LIST_COL_ACTOR = 0;
	static const Int32					DIALOG_LIST_COL_TEXT = 1;
	static const Int32					DIALOG_LIST_COL_SOUND = 2;
	static const Int32					DIALOG_LIST_COL_INDEX = 3;

	static const Int32					SCENE_LIST_FILE_NAME = 0;
	static const Int32					SCENE_LIST_FILE_PATH = 1;

	TEdShortcutArray					m_shortcuts;
	wxAuiNotebook*						m_auiNotebook;
	wxToolBar*							m_timelineToolbar;
	wxToolBar*							m_cutsceneToolbar;
	CEdCutsceneEditorPreview*			m_previewPanel;
	CEdCutsceneTimeline*				m_newTimeline;
	wxSlider*							m_timeMulSlider;
	wxTextCtrl*							m_timeMulEdit;
	wxChoice*							m_animPanelChoice;
	CEdPropertiesPage*					m_animProperties;
	wxToggleButton*						m_playPreviewButt;
	wxToggleButton*						m_playGameButt;
	wxToggleButton*						m_loadWorldButt;
	wxBitmap							m_playIcon;
	wxBitmap							m_pauseIcon;
	wxChoice*							m_cameraChoice;
	wxTreeCtrl*							m_animTree;
	CEdPropertiesBrowserWithStatusbar*	m_properties;
	wxStaticText*						m_fovText;
	CEdTimer							m_updateTimer;
	wxListCtrl*							m_dialogList;
	wxListCtrl*							m_sceneList;
	wxPanel*							m_timelinePanel;
	wxCheckBox*							m_fovOverride;
	wxCheckBox*							m_dofOverride;
	wxCheckBox*							m_silenceEnvironment;
	wxStaticText*						m_partitionName;
	wxListBox*							m_effectsList;
	CEdEffectEditor*					m_effectEditor;
	wxSocketServer*						m_socketServer;
	wxSocketClient*						m_socketClient;
	String								m_defaultLayout;

	Vector								m_hackPreviousCameraPosition;

public:
	CEdCutsceneEditor( wxWindow *parent ); // dummy constructor for shortcuts only
	CEdCutsceneEditor( wxWindow *parent, CCutsceneTemplate* res, const TDynArray< SCutsceneActorLine >* lines = NULL );
	~CEdCutsceneEditor();

	TEdShortcutArray* GetAccelerators();

	CCameraComponent* GetCsCamera() { return m_csCamera; }
	void UpdateCs( Float dt );

	void DispatchEditorEvent( const CName& name, IEdEventData* data );

	const CCutsceneInstance* GetCsInstance() const	{ return m_csInstance; }
	const CCutsceneTemplate* GetCsTemplate() const	{ return m_csTemplate; }

	CCutsceneTemplate* GetCsTemplate()	{ return m_csTemplate; }
	CCutsceneInstance* GetCsInstance() 	{ return m_csInstance; }

	const CAnimatedComponent* GetSelectedActor() const;
	const TDynArray< tCsEdDialogLine >& GetEdDialogLines() const { return m_dialogLines; }

	RED_INLINE Bool IsRecordingInViewport() const { return m_recordingInViewport; }

	void CsToGameplayRequest();

	void OnPlayPause();

protected:
	Bool CreateCutsceneInstance( ECutsceneEditorMode mode, struct SRecordingInfo* recordingInfo );
	void DestroyCutsceneInstance();
	void SetTime( Float time );
	void ApplyPreviewActorsEquipments();

	void ActiveMode( CEdCutsceneEditor::ECutsceneEditorMode mode );
	void ActiveCsCamera();
	void ActiveCsCamera( CCameraComponent* camera );
	void DeactiveCsCamera();

	void UpdateWidgets();
	void FillCameraCombo();
	void FillDialogPanel();
	void FillScenePanel();
	void UpdateCameraIcon();
	void UpdatePlayPauseButton();
	void UpdateFOV();
	void UpdateManualCameraOptions();
	void UpdatePointDesc( Bool show, const Matrix& point = Matrix::IDENTITY, const String& desc = TXT(""), Uint32 points = 0 );
	Bool ReloadCutscene();
	Bool Validate();
	void ResetImportParams();

	void ShowCutscenePage();
	void ShowTemplatePage();
	void SetupAnimationPage();
	void ShowDialogPage();
	void ShowScenesPage();
	void ShowCameraPage();

	void RefreshPage();
	void ChangePage(const unsigned int page);

	void SelectAnimation( const CName& animationName, Float duration );
	void FillTree();

	CLayer* GetLayer( ECutsceneEditorMode mode ) const;
	Bool CanUseMode( ECutsceneEditorMode mode ) const;
	Uint32 GetCsPoint( Matrix& point, ECutsceneEditorMode mode, String& desc ) const;
	CWorld* GetWorld() const;
	IViewport* GetViewport() const;
	void TakeScreenshot( const String &destinationFile ) const;
	void CreateAvi( const String& avsName , const String& folderName, Uint32 frameNumber ) const;
	String GetLine( Int32 num ) const;
	String GetLineText( Int32 num ) const;
	String GetLineActor( Int32 num ) const;
	StringAnsi GetSoundEventName( Int32 num ) const;
	Uint32 GetLineIndex( Int32 num ) const;
	CActor* FindLineActor( const String& actorVoicetag ) const;
	Float GetLineTime( const String& line ) const;
	void ImportLinesFromSceneFile( const String& filePath );
	Float GetManualFov() const;
	SDofParams GetManualDof() const;

	void CheckTemplate();
	void OnCheckActorsTemplates( wxCommandEvent& event );
	String FindErrors( Bool deep );
	void CheckAnimData( String& msg, const TDynArray< CSkeletalAnimationSetEntry* >& animations ) const;
	void CheckMotionEx( String& msg, const TDynArray< CSkeletalAnimationSetEntry* >& animations ) const;
	void CheckComponents( String& msg ) const;
	void CheckTrajectories( String& msg, const TDynArray< CSkeletalAnimationSetEntry* >& animations ) const;
	void CheckAndFillActors( String& msg ) const;
	void CheckBBox( String& msg, const TDynArray< CSkeletalAnimationSetEntry* >& animations ) const;
	Bool GenerateBBox();

	void SetCachets();
	void ResetCachets();

	void UpdateEffectList();
	CFXDefinition* DoAddEffect();
	void EditEffect( CFXDefinition* effect );

	void ConnectNetwork();
	void DisconnectNetwork();
	wxAuiManager					m_auiManager;
	
	void OnResetLayout( wxCommandEvent& event );
	String SaveDefaultLayout( wxCommandEvent& event );
	void SaveCustomLayout( wxCommandEvent& event );
	void LoadCustomLayout( wxCommandEvent& event );

private:
	CBehaviorMixerSlotInterface* FindMixer( const AnsiChar* actorName, Bool bodyOrMimics );
	void CreateMixers( const AnsiChar* actorName );
	Bool DestroyMixer( const AnsiChar* actorName );
	void DestroyAllMixers();

public:
	// IPreviewItemContainer
	virtual void OnSelectItem( IPreviewItem* item ) override;
	virtual void OnDeselectAllItem() override;
	virtual void OnItemTransformChangedFromPreview( IPreviewItem* item ) override;
	virtual CWorld* GetPreviewItemWorld() const override { return GetWorld(); }

	void RecreateEffectPreviewItems();

public:
	virtual void OnViewportCalculateCamera( IViewport* view, CRenderCamera &camera ) override;
	virtual void OnViewportTick( IViewport* view, Float timeDelta ) override;
	virtual void OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame ) override;

protected:
	void OnClose( wxCloseEvent& event );
	void OnSave( wxCommandEvent &event );
	void OnModeMove( wxCommandEvent &event );
	void OnModeRotate( wxCommandEvent &event );
	void OnModeChange( wxCommandEvent &event );
	void OnRestart( wxCommandEvent& event );
	void OnChangeRepeat( wxCommandEvent& event );
	void OnPlayPause( wxCommandEvent& event );
	void OnReloadCutscene( wxCommandEvent& event );
	void OnRecompressAnimations( wxCommandEvent& event );
	void OnImportEvents( wxCommandEvent& event );
	void OnImportFootStepEvents( wxCommandEvent& event );
	void OnCreateRelaxEvents( wxCommandEvent& event );
	void OnDeleteRelaxEvents( wxCommandEvent& event );
	void OnTimeMulSliderUpdate( wxCommandEvent& event );
	void OnTimeSliderMulUpdating( wxCommandEvent& event );
	void OnTimeMulEditUpdate( wxCommandEvent& event );
	void OnCreateDestroyGame( wxCommandEvent& event );
	void OnLoadWorld( wxCommandEvent& event );
	void OnCreateDestroyPreview( wxCommandEvent& event );
	void OnMenuCreateDestroyGame( wxCommandEvent& event );
	void OnMenuCreateDestroyPreview( wxCommandEvent& event );
	void OnSetEndTime( wxCommandEvent& event );
	void OnCsDoAll( wxCommandEvent& event );
	void OnCsDoCheck( wxCommandEvent& event );
	void OnCsCamera( wxCommandEvent& event );
	void OnCameraChoiceChanged( wxCommandEvent &event );
	void OnAnimTreeDblClick( wxTreeEvent &event );
	void OnAnimTreeSelected( wxTreeEvent &event );
	void OnErrorDesc( wxCommandEvent& event );
	void OnPropertiesChanged( wxCommandEvent& event );
	void OnEventPropertiesChanged( wxCommandEvent& event );
	void OnDumpCutscene( wxCommandEvent& event );
	void OnReimportAndConvToStreamed( wxCommandEvent& event );
	void OnGameTimer( wxTimerEvent& event );
	void OnDispSkeletons( wxCommandEvent& event );
	void OnEffectReset( wxCommandEvent& event );
	void OnDofIntCheck( wxCommandEvent& event );
	void OnDofOverCheck( wxCommandEvent& event );
	void OnPageChanged( wxNotebookEvent& event );
	void OnGenerateBBox( wxCommandEvent& event );
	void OnToggleFloor( wxCommandEvent& event );
	void OnRequestSetTime( wxCommandEvent &event );
	void OnEventSelected( wxCommandEvent& event );
	void OnSceneColRightClick( wxListEvent& event );
	void OnSceneListShow( wxCommandEvent& event );
	void OnSceneListOpen( wxCommandEvent& event );
	void OnSceneListLines( wxCommandEvent& event );
	void OnSceneListRemove( wxCommandEvent& event );
	void OnDumpAllVoiceoversToFile( wxCommandEvent& event );
	void OnAnimChanged( wxCommandEvent &event );

	void OnEffectChange( wxCommandEvent& event );
	void OnAddEffect   ( wxCommandEvent& event );
	void OnAddEffectEmitter( wxCommandEvent& event );
	void OnAddEffectCustom( wxCommandEvent& event );
	void OnRemoveEffect( wxCommandEvent& event );
	void OnCopyEffect  ( wxCommandEvent& event );
	void OnPasteEffect ( wxCommandEvent& event );

	void OnNetConnected( wxCommandEvent& event );
	void OnSocketServerEvent( wxSocketEvent& event );
	void OnSocketClientEvent( wxSocketEvent& event );

	void OnUpdateUI( wxUpdateUIEvent& event );

	SLIDER_AND_EDIT_DEFINE( CEdCutsceneEditor::OnFovEdit, sliderCameraFov, editCameraFov, 1, 160 );
	SLIDER_AND_EDIT_WITH_RANGE_DEFINE( CEdCutsceneEditor::OnDofParam1, sliderDof1, editDof1, editDof1Min, editDof1Max );
	SLIDER_AND_EDIT_DEFINE( CEdCutsceneEditor::OnDofParam2, sliderDof2, editDof2, 0, 100 );
	SLIDER_AND_EDIT_DEFINE( CEdCutsceneEditor::OnDofParam3, sliderDof3, editDof3, 0, 100 );
	SLIDER_AND_EDIT_DEFINE( CEdCutsceneEditor::OnDofParam4, sliderDof4, editDof4, 0, 100 );
	SLIDER_AND_EDIT_DEFINE( CEdCutsceneEditor::OnDofParam5, sliderDof5, editDof5, 0, 100 );
	SLIDER_AND_EDIT_DEFINE( CEdCutsceneEditor::OnDofParam6, sliderDof6, editDof6, 0, 100 );

private:
	void ProcessNetworkPacket_Camera( wxSocketBase *sock, wxTextCtrl* text );
	void TryMatchingActors();
	void InsertSwitchesRelaxEvents();

public:
	void OnNetwork_SetCamera( const Matrix& cam, Float fov );
	void OnNetwork_SetTime( Float t );
	void OnNetwork_SetPose( const AnsiChar* actorName, Int32 actorNameSize, const SAnimationMappedPose& pose );
	
	CActor* m_preSpawnedActor;
	void HACK_DepawnGeralt( CLayer* layer );
	void HACK_SpawnGeralt( CLayer* layer, CCutsceneTemplate* csTemplate, const Vector& position );
};
