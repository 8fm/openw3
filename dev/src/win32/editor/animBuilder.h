
#pragma once

#include "animationTreeBrowser.h"
#include "animBuilderTimeline.h"
#include "../../common/engine/virtualAnimationMixer.h"
#include "animFKPosePreviewEditor.h"
#include "animIKPosePreviewEditor.h"

class CEdAnimationBuilder : public wxSmartLayoutPanel, public CEdAnimationPreviewListener, public VirtualAnimationMixerEdListener
{
	wxDECLARE_EVENT_TABLE();
	wxDECLARE_CLASS( CEdAnimationBuilder );

	typedef TPair< VirtualAnimationID, Color > TDebugAnim;

protected:
	CName								m_animationName;

	CPlayedSkeletalAnimation*			m_playedAnimation;

	TDynArray< TDebugAnim >				m_debugAnims;

	CAnimatedComponent*					m_animatedComponent;
	CVirtualSkeletalAnimation*			m_animation;
	CSkeletalAnimationSetEntry*			m_animationEntry;

	wxAuiManager						m_mgr;

	CEdAnimationPreview*				m_preview;
	CEdAnimBuilderTimeline*				m_timeline;
	CEdAnimationTreeBrowser*			m_animationBrowser;

	AnimFKPosePreviewEditor*			m_fkEditor;
	AnimIKPosePreviewEditor*			m_ikEditor;

	wxToolBar*							m_toolbar;
	wxHtmlWindow*						m_htmlWin;
	wxSlider*							m_timeSilder;
	wxBitmap							m_playIcon;
	wxBitmap							m_pauseIcon;

	Uint32								m_ghostCount;
	PreviewGhostContainer::EGhostType	m_ghostType;

public:
	CEdAnimationBuilder( wxWindow* parent, CSkeletalAnimationSetEntry* animation, const CAnimatedComponent* templateComponent = NULL );
	~CEdAnimationBuilder();

public: // CEdAnimationPreviewListener
	virtual void OnPreviewTick( Float dt );
	virtual void OnPreviewGenerateFragments( CRenderFrame *frame );
	virtual void OnLoadPreviewEntity( CAnimatedComponent* component );
	virtual void OnPreviewHandleSelection( const TDynArray< CHitProxyObject* >& objects );

public: // VirtualAnimationMixerEdListener
	virtual void OnAnimationTracksSampled( Uint32 boneNumIn, Uint32 tracksNumIn, AnimQsTransform* bonesOut, AnimFloat* tracksOut );
	virtual void OnFKTracksSampled( Uint32 boneNumIn, Uint32 tracksNumIn, AnimQsTransform* bonesOut, AnimFloat* tracksOut );
	virtual void OnIKTracksSampled( Uint32 boneNumIn, Uint32 tracksNumIn, AnimQsTransform* bonesOut, AnimFloat* tracksOut );
	virtual void OnAllTracksSampled( Uint32 boneNumIn, Uint32 tracksNumIn, AnimQsTransform* bonesOut, AnimFloat* tracksOut );

protected:
	void LoadEntity( const String& fileName, const String& component = String::EMPTY, const String& defaultComponent = String::EMPTY );
	void UnloadEntity();

	void CloneEntityFrom( const CAnimatedComponent* component );

	void Tick( Float timeDelta );

public:
	void SetPause( Bool flag );
	Bool IsPaused() const;

	void TogglePause();

	void SetTimeMul( Float factor );

	void SetExtractedMotion( Bool flag );
	Bool UseExtractedMotion() const;

public:
	void RequestAddVirtualAnimation( VirtualAnimation& anim, EVirtualAnimationTrack track );
	void RequestVirtualAnimationChange( const VirtualAnimationID& animation, const VirtualAnimation& dest );
	void RequestRemoveVirtualAnimation( const VirtualAnimationID& animation );
	void RequestVirtualAnimationDupication( const VirtualAnimationID& animation );

	void RequestAddVirtualMotion( VirtualAnimationMotion& motion );
	void RequestRemoveVirtualMotion( const VirtualAnimationMotionID& motion );
	void RequestVirtualMotionChange( const VirtualAnimationMotionID& motion, const VirtualAnimationMotion& dest );

	void RequestAddVirtualFK( VirtualAnimationPoseFK& data );
	void RequestRemoveVirtualFK( const VirtualAnimationPoseFKID& dataID );
	void RequestVirtualFKChange( const VirtualAnimationPoseFKID& dataID, const VirtualAnimationPoseFK& data );

	void RequestAddVirtualIK( VirtualAnimationPoseIK& data );
	void RequestRemoveVirtualIK( const VirtualAnimationPoseIKID& dataID );
	void RequestVirtualIKChange( const VirtualAnimationPoseIKID& dataID, const VirtualAnimationPoseIK& data );

	void StartDebuggingAnimation( const VirtualAnimationID& animation, const Color& color );
	void StopDebuggingAnimation( const VirtualAnimationID& animation );
	Bool IsDebuggingAnimation( const VirtualAnimationID& animation ) const;
	Bool IsDebuggingAnimation( const VirtualAnimationID& animation, Color& color ) const;

	void SelectBonesAndWeightsForAnimation( const VirtualAnimationID& animation );

	void SelectAnimationEvent( const VirtualAnimationID& id );
	void SelectMotionEvent( const VirtualAnimationMotionID& id );
	void SelectFKEvent( const VirtualAnimationPoseFKID& id );
	void SelectIKEvent( const VirtualAnimationPoseIKID& id );
	void DeselectEvent();

protected:
	void SetTranslationWidgetMode();
	void SetRotationWidgetMode();

	void SetWidgetModelSpace();
	void SetWidgetLocalSpace();

protected:
	void SetupPerspective();
	void SetAnimationTime( Float time );

	Bool CheckAnimationBeforeSaving();
	
	void UpdatePlayPauseToolItem();

	void LoadDefaultEntity();

	void DrawSkeletons( EVirtualAnimationTrack track, CRenderFrame *frame );

	Bool CheckControlRig();

protected:
	void OnSave( wxCommandEvent& event );
	void OnTimelineRequestSetTime( wxCommandEvent& event );
	void OnTimelineChanged( wxCommandEvent& event );
	void OnResetPlayback( wxCommandEvent& event );
	void OnPlayPause( wxCommandEvent& event );
	void OnPlayOneFrame( wxCommandEvent& event );
	void OnPlayBackOneFrame( wxCommandEvent& event );
	void OnTimeSliderChanged( wxCommandEvent& event );
	void OnBtnTime( wxCommandEvent& event );
	void OnToggleExMotion( wxCommandEvent& event );
	void OnToggleGhosts( wxCommandEvent& event );
	void OnGhostConfig( wxCommandEvent& event );
	void OnConfigureGhostsOK( wxCommandEvent& event );
};
