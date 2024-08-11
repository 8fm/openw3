
#pragma once

#include "animationPreviewGhost.h"
#include "animationPreviewPostprocess.h"
#include "animationPreviewItems.h"
#include "../../common/engine/behaviorGraphAnimationMixerSlot.h"

class CEdAnimationPreviewListener
{
public:
	virtual void OnPreviewTick( Float dt ) = 0;
	virtual void OnPreviewGenerateFragments( CRenderFrame *frame ) = 0;
	virtual void OnLoadPreviewEntity( CAnimatedComponent* component ) = 0;
	virtual void OnPreviewSelectPreviewItem( IPreviewItem* item ) {}
	virtual void OnPreviewDeselectAllPreviewItems() {}
	virtual void OnPreviewHandleSelection( const TDynArray< CHitProxyObject* >& object ) {}
	virtual void OnPreviewViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data ) {}
};

class CEdAnimationPreview : public CEdPreviewPanel, public CItemDisplayer, public IEdEventListener
{
	struct PlayedAnimation
	{
		Bool						m_bodyMimicFlag;
		CPlayedSkeletalAnimation*	m_playedAnimation;
		CBehaviorMixerSlotInterface	m_slot;

		PlayedAnimation();

		void Set( CAnimatedComponent* ac, CSkeletalAnimationSetEntry* aniamtion ) ;
		CPlayedSkeletalAnimation* SetBody( CAnimatedComponent* ac, CSkeletalAnimationSetEntry* aniamtion );
		void Reset();

		Bool IsBody() const;
		void SetMimic( CMimicComponent* mimicComponent );
	};

protected:
	CEdAnimationPreviewListener* m_listener;

	PlayedAnimation				m_playedAnimation;
	CAnimatedComponent*			m_component;
	Bool						m_animationGraphEnabled;

	PreviewGhostContainer		m_ghostContainer;

	Float						m_timeMul;
	Bool						m_pause;
	Int32						m_forceOneFrame;

	Bool						m_cameraSnapping;
	Bool						m_usePositionWraping;
	Vector						m_prevPosition;

	static const Vector			CAMERA_POS_OFFSET;
	static const EulerAngles	CAMERA_ROT_OFFSET;

	wxMenu*						m_contextMenu;
	wxSlider*					m_timeSilder;
	wxStaticText*				m_timeText;

	TDynArray< CEdAnimationPreviewPostProcess* > m_postprocesses;

public:
	CEdAnimationPreview( wxWindow* parent, Bool timeOptions = false, CEdAnimationPreviewListener* listener = NULL );
	~CEdAnimationPreview();

	virtual void HandleSelection( const TDynArray< CHitProxyObject* >& object );
	virtual void HandleContextMenu( Int32 x, Int32 y );
	virtual void OnViewportTick( IViewport* view, Float timeDelta );
	virtual void OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame );
	virtual Bool OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data );

	virtual void SaveSession( CConfigurationManager &config );
	virtual void RestoreSession( CConfigurationManager &config );

public:
	// Entity and components
	void LoadEntity( const String& fileName, const String& component = String::EMPTY );
	void UnloadEntity();

	void CloneAndUseAnimatedComponent( const CAnimatedComponent* animatedComponent );

	CAnimatedComponent* GetAnimatedComponent() const;

	void RefreshWorld();

	// Animations
	void SetAnimation( CSkeletalAnimationSetEntry* animation );
	CPlayedSkeletalAnimation* SetBodyAnimation( CSkeletalAnimationSetEntry* animation );
	void ClearAnimation();

	void SetAnimationGraphEnabled( Bool state );

	// Preview control
	void UseCameraSnapping( Bool flag );

	void AddPostProcess( CEdAnimationPreviewPostProcess* process );

	// Time control
	void Pause( Bool flag );
	Bool IsPaused() const;

	void SetTimeMul( Float factor );

	void ForceOneFrame();
	void ForceOneFrameBack();

	// Ghosts
	void ShowGhosts( Uint32 number, PreviewGhostContainer::EGhostType type );
	void HideGhosts();
	Bool HasGhosts() const;

	// Menu
	void SetDefaultContextMenu();
	void SetCustomContextMenu( wxMenu* contextMenu );

public: // CItemDisplayer
	virtual CActor*	GetActorEntity();

public: // IEdEventListener
	virtual void DispatchEditorEvent( const CName& name, IEdEventData* data );

public:
	void AddRotationWidgets();
	void AddTranslationWidgets();
	void AddScaleWidgets();

	void SetTranslationWidgetMode();
	void SetRotationWidgetMode();
	void ToggleWidgetMode();

	void SetWidgetModelSpace();
	void SetWidgetLocalSpace();

	void EnableWidgets();
	void DisableWidgets();

protected:
	void SetInitialCameraPosition();
	void SnapCamera();

	Bool ShouldBackToDefaultPosition() const;
	void BackToDefaultPosition();

	void CreateTimeOptions();
	void CreateFloor();
	void DestroyFloor();

	void CloneEntityFromComponent( const CAnimatedComponent *originalComponent );

	void SetupAndSetEntity( CAnimatedComponent *component );

protected:
	void OnLoadPreviewEntity( wxCommandEvent& event );
	void OnLoadPreviewEntityItem( wxCommandEvent& event );
	void OnTimeSliderChanged( wxCommandEvent& event );
	void OnBtnTime( wxCommandEvent& event );
};

//////////////////////////////////////////////////////////////////////////

class CEdAnimationPreviewWithPreviewItems : public CEdAnimationPreview, public IPreviewItemContainer
{
public:
	CEdAnimationPreviewWithPreviewItems( wxWindow* parent, Bool timeOptions = false, CEdAnimationPreviewListener* listener = NULL );
	~CEdAnimationPreviewWithPreviewItems();

public:
	virtual void HandleSelection( const TDynArray< CHitProxyObject* >& objects );

	virtual CWorld* GetPreviewItemWorld() const;

	virtual void OnSelectItem( IPreviewItem* item );
	virtual void OnDeselectAllItem();
};

//////////////////////////////////////////////////////////////////////////
