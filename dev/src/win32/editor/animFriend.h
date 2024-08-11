
#pragma once

#include "animationTreeBrowser.h"
#include "animBuilderTimeline.h"
#include "htmlLog.h"
#include "animFriendPage.h"
#include "../../common/engine/animationGameParams.h"
#include "../../common/engine/playedAnimation.h"

template< class T >
class CEdAnimationParamFilter : public CEdAnimationTreeBrowserAnimFilter
{
public:
	virtual Bool Test( const CSkeletalAnimationSetEntry* animation )
	{
		return animation->FindParam< T >() != NULL;
	}
};

class CEdAnimationFriend : public wxSmartLayoutPanel, public CEdAnimationPreviewListener, public IPlayedAnimationListener
{
	DECLARE_EVENT_TABLE()

protected:
	CAnimatedComponent*						m_animatedComponent;

	wxAuiManager							m_mgr;

	CEdAnimationPreviewWithPreviewItems*	m_preview;
	CEdAnimationTreeBrowser*				m_animationBrowser;
	CEdHtmlLog*								m_htmlLog;

	TDynArray< CEdAnimationFriendPage* >	m_pages;

	wxToolBar*								m_toolbar;
	wxSlider*								m_timeSilder;
	wxBitmap								m_playIcon;
	wxBitmap								m_pauseIcon;

public:
	CEdAnimationFriend( wxWindow* parent );
	~CEdAnimationFriend();

public: // CEdAnimationPreviewListener
	virtual void OnPreviewTick( Float dt );
	virtual void OnPreviewGenerateFragments( CRenderFrame *frame );
	virtual void OnLoadPreviewEntity( CAnimatedComponent* component );
	virtual void OnPreviewSelectPreviewItem( IPreviewItem* item );
	virtual void OnPreviewDeselectAllPreviewItems();
	virtual void OnPreviewViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data );

public: // IPlayedAnimationListener
	virtual void OnAnimationStarted( const CPlayedSkeletalAnimation* animation );
	virtual void OnAnimationBlendInFinished( const CPlayedSkeletalAnimation* animation );
	virtual void OnAnimationBlendOutStarted( const CPlayedSkeletalAnimation* animation );
	virtual void OnAnimationFinished( const CPlayedSkeletalAnimation* animation );
	virtual void OnAnimationStopped( const CPlayedSkeletalAnimation* animation );

protected: // Base logic
	void LoadEntity( const String& fileName, const String& component = String::EMPTY, const String& defaultComponent = String::EMPTY );
	void UnloadEntity();

	void SetupLoadedEntity( CAnimatedComponent* component );

	void Tick( Float timeDelta );

public: // Editor builder
	void AddPage( CEdAnimationFriendPage* page );

	void SetAnimationFilter( CEdAnimationTreeBrowserAnimFilter* filter );

public: // User control
	void SetPause( Bool flag );
	Bool IsPaused() const;

	void TogglePause();

	void SetTimeMul( Float factor );

	void SetExtractedMotion( Bool flag );
	Bool UseExtractedMotion() const;

public: // functions for pages
	IPreviewItemContainer* GetPreviewItemContainer();

	void CollectAllAnimations( TDynArray< const CSkeletalAnimationSetEntry* >& anims ) const;

	const CAnimatedComponent* GetAnimatedComponent() const;

	CPlayedSkeletalAnimation* PlayAnimation( const CSkeletalAnimationSetEntry* animation );
	CPlayedSkeletalAnimation* PlaySingleAnimation( const CSkeletalAnimationSetEntry* animation );
	CPlayedSkeletalAnimation* GetPlayedAnimation();
	void StopAnimation();
	void PauseAnimation();

	void SetCustomPagePerspective( const wxString& code );

protected:
	void SetupPerspective();
	void SetAnimationTime( Float time );

	void UpdatePlayPauseToolItem();

	void LoadDefaultEntity();

	void Log( const wxString& msg );

protected:
	void OnTimelineTimeDrag( wxCommandEvent& event );
	void OnTimelineChanged( wxCommandEvent& event );
	void OnResetPlayback( wxCommandEvent& event );
	void OnPlayPause( wxCommandEvent& event );
	void OnPlayOneFrame( wxCommandEvent& event );
	void OnPlayBackOneFrame( wxCommandEvent& event );
	void OnTimeSliderChanged( wxCommandEvent& event );
	void OnBtnTime( wxCommandEvent& event );
	void OnToggleExMotion( wxCommandEvent& event );
};

