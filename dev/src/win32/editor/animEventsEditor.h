/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "undoManager.h"
#include "animBrowserPreview.h"
#include "restartable.h"
#include "undoAnimEventsEditor.h"
#include "../../common/engine/playedAnimation.h"
#include "../../common/engine/soundAdditionalListener.h"

class CEdAnimBrowserPreview;
class CEdAnimEventsTimeline;
class CUndoAnimEventsAnimChange;
class CEdDisplayItemDialog;

class CEdAnimEventsEditor : public wxFrame
						  , public IPlayedAnimationListener
						  , public ISavableToConfig
	 					  , public ISmartLayoutWindow
						  , public ISkeletonPreviewControl
						  , public CItemDisplayer
						  , public IAnimBrowserPreviewListener
						  , public IRestartable
						  , public CSoundAdditionalListener
{
	friend class CUndoAnimEventsAnimChange;

	DECLARE_EVENT_TABLE();

public:

	CEdAnimEventsEditor( wxWindow* parent, CExtAnimEventsFile* eventsFile );
	virtual ~CEdAnimEventsEditor();

	Bool InitWidget();

	virtual void OnAnimationFinished( const CPlayedSkeletalAnimation* animation );
	virtual wxToolBar* GetSkeletonToolbar();

private:
	const static String CONFIG_PATH;
	const static String CONFIG_ANIMSET;
	const static String CONFIG_ENTITY;

	CExtAnimEventsFile*			m_eventsFile;
	CSkeletalAnimationSet*		m_animationSet;
	Bool						m_updatingGui;
	CAnimatedComponent*			m_animatedComponent;
	CPlayedSkeletalAnimation*	m_playedAnimation;
	CEntity*					m_previewEntity;
	CEdDisplayItemDialog*		m_itemDialog;
	Uint32						m_tPoseConstraint;
	Bool						m_resettingConfig;

	// widgets
	CEdAnimBrowserPreview*		m_preview;
	CEdAnimEventsTimeline*		m_timeline;
	wxStaticText*				m_entityNameLabel;
	wxTextCtrl*					m_animationSearchField;
	wxButton*					m_clearSearchButton;
	wxListCtrl*					m_animationsList;
	wxSlider*					m_speedSlider;
	wxBitmap					m_playIcon;
	wxBitmap					m_pauseIcon;
	wxToolBar*					m_controlToolbar;
	wxPanel*					m_timelinePanel;
	wxCheckBox*                 m_autoPlayCheckBox;

	Red::Threads::CAtomic< Int32 >	m_refershIconsRequest;

	CEdUndoManager*				m_undoManager;

	// Events handlers
	void OnAnimationChanged( wxListEvent& event );
	void OnSpeedChanged( wxScrollEvent& event );
	void OnPlayPause( wxCommandEvent& event );
	void OnReset( wxCommandEvent& event );
	void OnSave( wxCommandEvent& event );
	void OnResetConfig( wxCommandEvent& event );
	void OnClose( wxCloseEvent& event );
	void OnMenuClose( wxCommandEvent& event );
	void OnMenuUndo( wxCommandEvent& event );
	void OnMenuRedo( wxCommandEvent& event );
	void OnRequestSetTime( wxCommandEvent& event );
	void OnRefreshPreview( wxCommandEvent& event );
	void OnTimelineResized( wxCommandEvent& event );
	void OnUseSelectedEntity( wxCommandEvent& event );
	void OnToggleCarButton( wxCommandEvent& event );
	void OnMassActions( wxCommandEvent& event );
	void OnAnimationSearchField( wxCommandEvent& event );
	void OnClearSearchButton( wxCommandEvent& event );
	void OnItemDisplayToolButton( wxCommandEvent& event );
	void OnUndoItemDisplayToolButton( wxCommandEvent& event );
	void OnTPoseButton( wxCommandEvent& event );
	void OnCharHook( wxKeyEvent &event );
	void OnRenameAnimations( wxCommandEvent& event );

	void UpdateAnimListSelection();
	void UpdateGui();

	void SaveGUILayout();
	void LoadGUILayout();

	virtual void LoadOptionsFromConfig();
	virtual void SaveOptionsToConfig();
	void ResetConfig();

	void LoadEntity( const String &entName );
	void UnloadEntity();

	void SelectAnimation( CSkeletalAnimationSetEntry* animationEntry, CName const & animName, Bool doNotPlay = false );
	void SelectAnimation( CSkeletalAnimationSetEntry* animationEntry, Bool doNotPlay = false ) { SelectAnimation( animationEntry, animationEntry? animationEntry->GetName() : CName(), doNotPlay); }
	void PlayPauseAnimation();
	void ResetAnimation( bool setToEnd = false );
	void UpdatePlayPauseIcon();
	String GetConfigPath( bool perFile ) const;

	Bool IsLoopingEnabled() const;

	virtual CActor*	GetActorEntity();

	void Tick( Float timeDelta );

	// IRestartable interface
	virtual bool ShouldBeRestarted() const;

};
