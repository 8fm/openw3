/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "skeletonPreview.h"
#include "displayItemDlg.h"
#include "animBrowserPreview.h"
#include "propertiesPageHavok.h"
#include "../../common/game/inventoryEditor.h"
#include "restartable.h"
#include "undoTimeLine.h"
#include "../../common/engine/behaviorGraphAnimationMixerSlot.h"
#include "../../common/engine/playedAnimation.h"
#include "../../common/engine/skeletalAnimationContainer.h"

class CEdAnimEventsTimeline;
class CEdDisplayItemDialog;
class CMemoryFileReader;

BEGIN_DECLARE_EVENT_TYPES()
	DECLARE_EVENT_TYPE( wxEVT_ANIM_CONFIRMED, wxEVT_USER_FIRST + 1 )
	DECLARE_EVENT_TYPE( wxEVT_ANIM_ABANDONED, wxEVT_USER_FIRST + 2 )
END_DECLARE_EVENT_TYPES()

enum EAnimBrowserMode
{
	ABM_Normal_Anim,
	ABM_Normal_Animset,
	ABM_Select
};

class IAnimBrowserPageInterface
{
public:
	virtual void DestroyPanel() = 0;
	virtual void EnablePanel( Bool flag ) = 0;

	virtual void OnSelectedAnimation() {}
	virtual void OnRefreshAnimation() {}

	virtual void OnItemSelected( CNode* itemNode ) {}

	virtual void Tick( Float timeDelta ) {}
	virtual void HandleSelection( const TDynArray< CHitProxyObject* >& objects ) {}

	virtual void OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame ) {}

	virtual void Save( CUserConfigurationManager& config ) {}
	virtual void Load( CUserConfigurationManager& config ) {}
	virtual void Reset( CUserConfigurationManager& config ) {};
};

class CEdAnimBrowser : public wxFrame
					 , public IPlayedAnimationListener
					 , public ISavableToConfig
					 , public IEdEventListener
					 , public ISmartLayoutWindow
					 , public ISkeletonPreviewControl
					 , public CItemDisplayer
					 , public IAnimBrowserPreviewListener
					 , public IRestartable
{
	DECLARE_EVENT_TABLE()

	friend class CEdAnimBrowserPreview;
	friend class CAnimBrowserCompressionPage;
	friend class CAnimBrowserBehaviorPage;
	friend class CAnimBrowserTrajectoryPage;
	friend class CUndoAnimBrowserAnimChange;
	friend class CAnimBrowserDialogParamsPage;
	friend struct SUndoAnimBrowserPasteEventsEntryInfo;
	friend struct SUndoAnimBrowserFindAndRemoveEventsEntryEventInfo;
	friend class CUndoAnimBrowserFindAndMoveEvents;

	static const unsigned int		PAGE_ALL_ANIMATIONS = 0;
	static const unsigned int		PAGE_ANIM_SETS = 1;
	static const unsigned int		PAGE_ANIMATIONS = 2;
	static const unsigned int		PAGE_ANIM_PROPERTIES = 3;
	static const unsigned int		PAGE_PROPERTIES = 4;
	static const unsigned int		PAGE_HAVOK = 5;
	static const unsigned int		PAGE_CUSTOM = 6;
	static const unsigned int		PAGE_COMPRESSION = 6;
	static const unsigned int		PAGE_BEHAVIOR = 7;
	static const unsigned int		PAGE_TRAJECTORY = 8;

	static const unsigned int		PAGE_COMPR_INFO = 0;
	static const unsigned int		PAGE_COMPR_COMPRESS = 1;

	static const Uint32				CLONE_COMPRESSED_POSE = 1;

	struct PlayedAnimation
	{
		Bool						m_bodyMimicFlag;
		CPlayedSkeletalAnimation*	m_playedAnimation;
		CBehaviorMixerSlotInterface	m_slot;

		PlayedAnimation();

		void Set( CAnimatedComponent* ac, CSkeletalAnimationSetEntry* aniamtion, IPlayedAnimationListener* l ) ;
		void Reset();

		Bool IsPaused() const;
		void Pause();
		void Unpause();

		Bool IsEqual( const CSkeletalAnimationSetEntry* rhs ) const;
		Bool IsValid() const;
		Float GetTime() const;
		void SetTime( Float t );
		Float GetDuration() const;
		CName GetName() const;
		const CSkeletalAnimationSetEntry* GetAnimationEntry();

		void ForceCompressedPose();

		Bool IsBody() const;
		void SetMimic( CMimicComponent* mimicComponent );
	};

	struct DebugPose
	{
		String								m_name;
		Color								m_color;
		TDynArray< Matrix >					m_pose;
		THandle< CSkeleton >				m_skeleton;
		const CSkeletalAnimationSetEntry*	m_id;
	};

	enum EExtraAnimInfo
	{
		I_None,
		I_Durations,
		I_Size,
		I_ComprPose,
		I_Streaming,
		I_Virtual
	};

	EAnimBrowserMode						m_mode;						// operating mode
	EExtraAnimInfo							m_extraAnimInfo;

	wxBitmap								m_playIcon;
	wxBitmap								m_pauseIcon;

	//! Filter
	enum EBrowserAnimFilterMode
	{
		BAFM_BeginsWith,
		BAFM_Contain
	};

	EBrowserAnimFilterMode					m_filterMode;
	String									m_filterText;

	//! global properties
	CEntity									*m_entity;					// entity viewed
	CEntityTemplate							*m_entityTemplate;			// entity template
	CAnimatedComponent						*m_animatedComponent;		// animated component used
	PlayedAnimation*						m_playedAnimation;			// animation being played
	Bool									m_forceCompressedPose;		// force compressed pose
	Bool									m_motionExType;
	Bool									m_mimicCor;
	Uint32									m_tPoseConstraint;

	CResource								*m_resource;				// resource edited

	// access to all interesting controls 
	CEdAnimBrowserPreview					*m_previewPanel;			// preview panel
	CEdAnimEventsTimeline					*m_timeline;				// timeline panel
	wxNotebook								*m_editorsNotebook;			// editor notebook
	CDropTargetAnimBrowserNotebook			*m_editorsNotebookDropTarget; // editor notebook drop target
	wxPanel									*m_timelinePanel;

	// 'all animations' page
	wxTreeCtrl								*m_animsList;				// tree containing all animations for given entiry
	CSkeletalAnimationSetEntry				*m_selectedAnimation;		// animation selected in tree control

	static const Float						ANIM_DUR_NORMAL;
	static const Float						ANIM_DUR_MID;
	static const Float						ANIM_DUR_LARGE;

	static const Float						ANIM_SIZE_NORMAL;
	static const Float						ANIM_SIZE_MID;
	static const Float						ANIM_SIZE_LARGE;

	static const wxColour					ANIM_COLOR_NORMAL;
	static const wxColour					ANIM_COLOR_MID;
	static const wxColour					ANIM_COLOR_LARGE;

	// 'entity animsets' page
	wxListBox								*m_entityAnimSetsList;		// list box with entity's animsets
	CSkeletalAnimationSet					*m_selectedAnimSet;			// animset selected on list

	// 'animset animations' page
	wxChoice								*m_entityAnimSets;			// combo with animset names
	wxListBox								*m_animSetAnimationList;	// listbox with animset animations

	// 'animation' page
	wxChoice								*m_animSetAnimations;		// combo with animations in given animset
	CEdPropertiesPage						*m_animProperties;

	// 'properties' page
	CEdPropertiesPage						*m_propertiesPage;

	// 'compression' page
	TDynArray< IAnimBrowserPageInterface* >	m_pages;

	// 'havok' page
#ifdef USE_HAVOK_ANIMATION
	CEdHavokPropertiesPage*					m_hkAnimProp;
#endif
	// item display dialog
	CEdDisplayItemDialog					*m_itemDialog;

	Bool									m_addDynamicAnimset;
	Bool									m_useMotionExtraction;

	Bool									m_showBBox;
	Bool									m_showFloor;
	CEntity*								m_floor;

	Red::Threads::CAtomic< Int32 >			m_refershIconsRequest;

	Uint32									m_ghostCount;
	PreviewGhostContainer::EGhostType		m_ghostType;
	TDynArray< DebugPose >					m_debugPoses;

	Bool									m_resettingConfig;
	Bool									m_refreshing;
	CEdUndoManager*							m_undoManager;

public:
	CEdAnimBrowser( wxWindow *parent );
	CEdAnimBrowser( wxWindow *parent, CResource* res ) ;

	~CEdAnimBrowser();

	void SetupWidget( wxWindow *parent );
	void ShowForSelection();

	Bool IsPaused() const;
	void Pause( Bool flag );

	void LoadEntity( const String &entName, const String &animName );
	void CloneEntityFromComponent( const CComponent *originalComponent );
	void UnloadEntity();
	void ShowEntity( Bool flag );

	void SetMode( EAnimBrowserMode mode );
	void SelectAnimation(const String& name);
	void ResetPlayedAnimation();
	void ReselectCurrentAnimation();
	void RefreshBrowser() { RefreshPage(); }

	CSkeletalAnimationSet* GetSelectedAnimSet() const;
	CSkeletalAnimationSetEntry* GetSelectedAnimationEntry( Int32 selIndex = -1, Int32 * selCount = nullptr ) const; 
	String GetSelectedAnimation() const; 

	RED_INLINE CAnimatedComponent* GetAnimatedComponent() const { return m_animatedComponent; }

	void AddAnimations( TDynArray<CSkeletalAnimation*> anims );

public:
	void SaveOptionsToConfig();
	void LoadOptionsFromConfig();
	void ResetConfig();

	wxToolBar* GetSkeletonToolbar();

	void Tick( Float timeDelta );
	void SelectItem( CNode* itemNode );
	void HandleSelection( const TDynArray< CHitProxyObject* >& objects );
	virtual void OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame );

protected:
	void OnPageChanged( wxNotebookEvent& event );
	void OnPlayPause( wxCommandEvent& event );
	void OnReset( wxCommandEvent& event );	

	Bool IsLoopingEnabled() const;
	void UpdatePlayPauseIcon();	
	void UpdateMotionExTypeIcon();
	void UpdateMotionExTypeForAnim();

	void OnOk( wxCommandEvent& event );
	void OnCancel( wxCommandEvent& event );
	void OnResetConfig( wxCommandEvent& event );
	void OnClose( wxCloseEvent& event );

	void OnAnimSelected( wxTreeEvent &event );
	void OnAnimDblClick( wxTreeEvent &event );

	void AnalizeEntity();
	void AddDynamicAnimset();
	void RemoveDynamicAnimset();
	void UseMotionExtraction( Bool flag );
	void UseTrajectoryExtraction( Bool flag );

protected:
	void OnRequestSetTime( wxCommandEvent &event );

protected:
	void RefreshPage();
	void RefreshPageNow();
	void ChangePage(const unsigned int page);
	void ChangeCompressionPage(const unsigned int page);

	void ShowPageAllAnimations();
	void AllAnimationFillAnimsList();
	wxColour GetAnimColor( Float animDuration ) const;

	void ShowPageAnimSets();
	void EntityAnimsetFillList();

	void ShowPageAnimation();
	void AnimsetFillList();

	void ShowPageAnimProperties();
	void DispAnimMotionEx();
	void AnimProprertiesFillAnimationsList();

	void ShowCustomPagePanel( unsigned int page );
	void DestroyCustomPages();
	void RefreshAnimationForCustomPages();
	void SelectAnimationForCustomPages();

	void ShowPageProperties();
	void ShowPageHavok();
	void ShowPageTrajectory();

	void ClearPagesData();

	Bool FilterAnimName( const CName& animName ) const;

	// Select functions
	void SelectAnimation(CSkeletalAnimationSetEntry *anim);
	void SelectAnimSet(CSkeletalAnimationSet *animSet);

	// Check functions
	Bool IsAnimationValid();
	Bool IsAnimsetValid();
	void CheckAnimsetContent();
	Bool CheckUniqueAnimNamesInAnimset(CSkeletalAnimationSet* animset);

	// Events
	void OnEntityAnimSetSelected( wxCommandEvent &event );
	void OnEntityAnimSetDblClick( wxCommandEvent &event );
	void OnAnimSetChanged( wxCommandEvent &event );
	void OnAnimSetAnimationSelected( wxCommandEvent &event );
	void OnAnimSetAnimationSelectedDblClick( wxCommandEvent &event );
	void OnAnimChanged( wxCommandEvent &event );
	void OnAnimMotionExChanged( wxCommandEvent &event );
	void OnSaveAnimation( wxCommandEvent &event );
	void OnRestoreAnimation( wxCommandEvent &event );
	void OnMenuUndo( wxCommandEvent& event );
	void OnMenuRedo( wxCommandEvent& event );
	void OnMenuReloadEntity( wxCommandEvent& event );
	void OnRemoveAnimationFromAll( wxCommandEvent &event );
	void OnRemoveAnimationFromAnimList( wxCommandEvent &event );
	void OnLoadEntity( wxCommandEvent& event );
	void OnTakeScreenshot( wxCommandEvent& event );
	void OnChangeTrackName( wxCommandEvent& event );
	void OnChangeTrackComment( wxCommandEvent& event );
	void OnSuckAnimaitons( wxCommandEvent &event );
	void OnImportAnimations( wxCommandEvent &event );
	void OnImportNormalAnimations( wxCommandEvent &event );
	void OnImportAddAnimations1( wxCommandEvent &event );
	void OnImportAddAnimations2( wxCommandEvent &event );
	void OnImportAddAnimations3( wxCommandEvent &event );
	void OnImportAddAnimations4( wxCommandEvent &event );
	void OnImportMSAnimations( wxCommandEvent &event );
	void OnRecompressAnimations( wxCommandEvent &event );
	void OnRecompressAnimation( wxCommandEvent &event );
	void OnRecompressAnimationsChoosePreset( wxCommandEvent &event );
	void OnRecompressAnimationChoosePreset( wxCommandEvent &event );
	void OnExportAnimation( wxCommandEvent &event );
	void OnExportAnimationWithoutTrajectory( wxCommandEvent &event );
	void OnReimportAnimations( wxCommandEvent &event );
	void OnReimportAnimation( wxCommandEvent &event );
	void OnReimportNormalAnimation( wxCommandEvent &event );
	void OnReimportAddAnimation1( wxCommandEvent &event );
	void OnReimportAddAnimation2( wxCommandEvent &event );
	void OnReimportAddAnimation3( wxCommandEvent &event );
	void OnReimportAddAnimation4( wxCommandEvent &event );
	void OnReimportMSAnimation( wxCommandEvent &event );
	void OnDuplicateAsAdd1( wxCommandEvent &event );
	void OnDuplicateAsAdd2( wxCommandEvent &event );
	void OnRenameAnimation( wxCommandEvent &event );
	void OnCharHook( wxKeyEvent &event );
	void OnGenerateBoundingBox( wxCommandEvent& event );
	void OnGenerateBoundingBoxForAll( wxCommandEvent& event );
	void OnAddEventFile( wxCommandEvent& event );
	void OnDeleteEventFile( wxCommandEvent& event );
	void OnEventFileSelected( wxListEvent& event );
	void OnEventFileActivated( wxListEvent& event );
	void OnTimeMultiplierChanged( wxScrollEvent& event );
	void OnTimelineResized( wxCommandEvent& event );
	void OnGenMemRepSet( wxCommandEvent& event );
	void OnGenMemRepAnim( wxCommandEvent& event );
	void OnDoNotClick( wxCommandEvent& event );
	void OnSortAnimset( wxCommandEvent& event );
	void OnShowAnimDurations( wxCommandEvent& event );
	void OnShowAnimMems( wxCommandEvent& event );
	void OnShowAnimComprPoses( wxCommandEvent& event );
	void OnShowAnimStreaming( wxCommandEvent& event );
	void OnShowVirtualAnim( wxCommandEvent& event );
	void OnAnimFilter( wxCommandEvent& event );
	void OnAnimFilterCheck( wxCommandEvent& event );
	void OnAnimPlot( wxCommandEvent& event );
	void OnAnimMSPlot( wxCommandEvent& event );
	void OnMimicAnimPlot( wxCommandEvent& event );
	void OnCalcBoneMS( wxCommandEvent& event );
	void OnCalcBoneWS( wxCommandEvent& event );
	void OnCalcBoneWithMotionMS( wxCommandEvent& event );
	void OnCalcBoneWithMotionWS( wxCommandEvent& event );
	void OnAnimCompressMotion( wxCommandEvent& event );
	void OnRecreateCompressedMotionExtraction( wxCommandEvent& event );
	void OnRecreateBBox( wxCommandEvent& event );
	void OnRecreateAllCompressedMotionExtraction( wxCommandEvent& event );
	void OnCheckCorruptedAnimations( wxCommandEvent& event );
	void OnCheckAllCorruptedAnimations( wxCommandEvent& event );
	void OnCheckPartStreambleStats( wxCommandEvent& event );
	void OnRecreateAllBBox( wxCommandEvent& event );
	void OnRecreateComprPose( wxCommandEvent& event );
	void OnRecreateComprPoses( wxCommandEvent& event );
	void OnAnimCopy( wxCommandEvent& event );
	void OnAnimPaste( wxCommandEvent& event );
	void OnPasteTimelineEventsForAnimations( wxCommandEvent& event );
	void OnPasteTimelineEventsForAnimset( wxCommandEvent& event );
	void OnPasteTimelineEventsRelativeToOtherEventForAnimations( wxCommandEvent& event );
	void OnPasteTimelineEventsRelativeToOtherEventForAnimset( wxCommandEvent& event );
	void OnFindAndRemoveEventsForAnimations( wxCommandEvent& event );
	void OnFindAndRemoveEventsForAnimset( wxCommandEvent& event );
	void OnFindAndMoveEventsForAnimations( wxCommandEvent& event );
	void OnFindAndMoveEventsForAnimset( wxCommandEvent& event );
	void OnFindAndEditEventsForAnimations( wxCommandEvent& event );
	void OnFindAndEditEventsForAnimset( wxCommandEvent& event );
	void OnAnimStreamingTypeChanged( wxCommandEvent& event );
	void OnCreateVirtualAnimation( wxCommandEvent& event );
	void OnEditVirtualAnimation( wxCommandEvent& event );
	void OnSyncEventsTo( wxCommandEvent& event );
	void OnCopyAllAnimationsToClipboard( wxCommandEvent& event );

	void RecompressAnimations( SAnimationBufferBitwiseCompressionPreset* preset = nullptr );
	void RecompressAnimation( SAnimationBufferBitwiseCompressionPreset* preset = nullptr );

	void PlayCurrentAnimation();

	void EntityChanged();

	Bool CreateDebugPose( Float time, const CSkeletalAnimationSetEntry* anim, DebugPose& outPose ) const;

	void GetAnimsets(TSkeletalAnimationSetsArray& animSets);
	void GetAnimationsFromAnimset(CSkeletalAnimationSet* animset, TDynArray<CSkeletalAnimationSetEntry*>& anims);
	String GetAnimSetName( CSkeletalAnimationSet *animSet ) const;
	CSkeletalAnimationSet* FindAnimSetContainingAnimation( CSkeletalAnimationSetEntry *anim );
	wxTreeItemId FindAnimationTreeItem( CSkeletalAnimationSetEntry *anim );

	// Context Menu events
	void OnAllAnimsRClick( wxMouseEvent &event );
	void OnAnimSetsRClick( wxMouseEvent &event );
	void OnAnimsRClick( wxMouseEvent &event );
	void OnEventsRClick( wxMouseEvent &event );

	// Context Menu
	void OpenAllAnimsContextMenu();
	void OpenAnimSetsContextMenu();
	void OpenAnimsContextMenu();
	void OpenEventsContextMenu();

	void ImportAnimation( Bool overrideAnimType = false, ESkeletalAnimationType animType = SAT_Normal, EAdditiveType type = AT_Local );
	void ExportAnimation( CSkeletalAnimationSetEntry *anim, Bool exportTrajectory );

	void OnToggleBBox( wxCommandEvent& event );
	void OnToggleFloor( wxCommandEvent& event );
	void OnTogglePlayerCameraMode( wxCommandEvent& event );
	void OnTogglePlayerMotionTrack( wxCommandEvent& event );
	void OnToggleExtractTrajectory( wxCommandEvent& event );
	void OnDisplayItem( wxCommandEvent& event );
	void OnDisplayItemUndo( wxCommandEvent& event );
	void OnCompressedPose( wxCommandEvent& event );
	void OnMotionExTypeChanged( wxCommandEvent& event );
	void OnMimicCor( wxCommandEvent& event );
	void OnTPoseCor( wxCommandEvent& event );
	void OnToggleGhosts( wxCommandEvent& event );
	void OnConfigureGhosts( wxCommandEvent& event );
	void OnConfigureGhostsOK( wxCommandEvent& event );
	void OnZoomExtentsPreview( wxCommandEvent& event );

	void OnSelectCompressedPose( wxCommandEvent& event );
	void OnEditCompressedPose( wxCommandEvent& event );
	void OnPlayCompressedPose( wxCommandEvent& event );
	void OnAddCompressedPose( wxCommandEvent& event );
	void OnRemoveCompressedPose( wxCommandEvent& event );
	void OnPrevCompressedPose( wxCommandEvent& event );
	void OnNextCompressedPose( wxCommandEvent& event );
	void OnCloneWithCompressedPose( wxCommandEvent& event );
	void OnCloneFlipPose( wxCommandEvent& event );
	void OnCloneWithCompressedPoseColor( wxCommandEvent& event );
	void OnCloneWithCompressedPoseOffset( wxCommandEvent& event );
	void OnAddDebugPoseStart( wxCommandEvent& event );
	void OnAddDebugPoseEnd( wxCommandEvent& event );
	void OnAddDebugPoseTime( wxCommandEvent& event );
	void OnRemoveDebugPose( wxCommandEvent& event );

	//void OnLoopOverflow( Float deltaTime, Uint32 overflows );
	//void OnLoopUnderflow( Float deltaTime, Uint32 underflows );

	virtual void OnAnimationFinished( const CPlayedSkeletalAnimation* animation );

	void OnMenuResetConfig( wxCommandEvent& event );
	void OnModelessClose( wxCommandEvent& event );

protected:
	virtual void DispatchEditorEvent( const CName& name, IEdEventData* data );

	void OnPasteTimelineEventsRelativeToOtherEventForAnimationsPerformOn( CMemoryFileReader & reader, CName const & relativeToEvent = CName::NONE, Float relPos = 0.0f );
	void OnPasteTimelineEventsRelativeToOtherEventForAnimsetPerformOn( CMemoryFileReader & reader, CName const & relativeToEvent = CName::NONE, Float relPos = 0.0f );

	void PasteTimelineEventsToAnimation( CMemoryFileReader & reader, CSkeletalAnimationSetEntry* animEntry, CName const & eventName = CName::NONE, Float relPos = 0.0f, TDynArray< CExtAnimEvent* >* events = nullptr );

	void FindAndRemoveEvents( CSkeletalAnimationSetEntry* animEntry, CName const & eventName );
	void FindAndMoveEvents( CSkeletalAnimationSetEntry* animEntry, CName const & eventName, Float relPos );
	void FindEventsToEdit( CSkeletalAnimationSetEntry* animEntry, CName const & eventName, TDynArray< CExtAnimEvent* > & events );
	void EditEvents( TDynArray< CExtAnimEvent* > & events );

private:
	Bool FindEntity(String& entName, const String& filePlace);
	CDirectory* FindParentDir(CDirectory* childDir, const String& parentDirName);

	void PlayPauseAnimation();
	void ResetAnimation( bool setToEnd = false );
	void UpdateCompressedPose();

	void LoadGUILayout();
	void SaveGUILayout();

	void OnSaveAnimset( CSkeletalAnimationSet* set );

	String GetConfigPath( bool perFile ) const;
	void RefreshSecondToolbar();
	void RefreshCompressedPoseWidgets();
	void FillCompressedPoseChoice();
	void SelectCompressedPose( const String& poseName );
	void DeselectCompressedPose();
	String GetSelectedCompressedPose() const;

	void SetAnimExtraInfo( EExtraAnimInfo info );
	void EnableAnimExtraInfo();
	void ApplyAnimListItemExtraData( wxString& data, const CSkeletalAnimationSetEntry *anim );
	void ApplyAnimListItemColor( wxTreeItemId& item, const CSkeletalAnimationSetEntry *anim );

	void RecreateCloneWithCompressedPose();

	Bool SelectExtraAnimation( CSkeletalAnimationSetEntry*& animation ) const;

	void OnReloadEntity();

	// IRestartable interface
	virtual bool ShouldBeRestarted() const;

	void LogMemoryStats();

public:
	// CItemDisplayer implementation
	//! Get an actor this displayer attaches items to
	virtual CActor*	GetActorEntity();
};

/////////////////////////////////////////////////////////////////////////

class PoseComparator
{
public:
	Float CalcSimilarity( const SBehaviorGraphOutput* poseA, const SBehaviorGraphOutput* poseB ) const;

	Float CalcSimilarity( const CSkeleton* skeleton, const CSkeletalAnimation* animA, Float timeA, const CSkeletalAnimation* animB, Float timeB ) const;

};

//////////////////////////////////////////////////////////////////////////
