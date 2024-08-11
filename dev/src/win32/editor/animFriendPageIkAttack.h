
#pragma once

#include "animFriendPreview.h"
#include "animFriendPage.h"
#include "../../common/engine/animationSelectors.h"
#include "../../common/engine/animationGameParams.h"
#include "../../common/engine/animationTrajectory.h"
#include "../../common/engine/animationTrajectoryPlayer.h"
#include "animationParamPanel.h"

class CEdAnimationFriendIkAttackPage;

class CEdIkAttackDummyItem : public IPreviewItem
{
	IPreviewItemContainer*	m_container;

public:
	CEdIkAttackDummyItem( IPreviewItemContainer* ic );

	virtual IPreviewItemContainer* GetItemContainer() const;

	virtual Bool IsValid() const { return true; }
	virtual void Refresh() {}

	virtual void SetPositionFromPreview( const Vector& prevPos, Vector& newPos ) {}
	virtual void SetRotationFromPreview( const EulerAngles& prevRot, EulerAngles& newRot ) {}
};

class CEdIkAttackPointItem : public IPreviewItem
{
	IPreviewItemContainer*			m_container;
	CEdAnimationFriendIkAttackPage* m_page;

public:
	CEdIkAttackPointItem( IPreviewItemContainer* ic, CEdAnimationFriendIkAttackPage* page );

	virtual IPreviewItemContainer* GetItemContainer() const;

	virtual Bool IsValid() const { return true; }
	virtual void Refresh();

	virtual void SetPositionFromPreview( const Vector& prevPos, Vector& newPos );
	virtual void SetRotationFromPreview( const EulerAngles& prevRot, EulerAngles& newRot );

	virtual Bool HasCaption() const { return false; }
};

//////////////////////////////////////////////////////////////////////////

class CEdAnimationAttackTrajectoryParamInitializer : public CEdAnimationParamInitializer
{
public:
	Bool Initialize( ISkeletalAnimationSetEntryParam* param, const CSkeletalAnimationSetEntry* animation, const CAnimatedComponent* animatedComponent ) const;
};

//////////////////////////////////////////////////////////////////////////

class CEdAnimationFriendIkAttackPage  : public CEdAnimationFriendSimplePage, public CEdAnimationParamPanelListener
{
	DECLARE_EVENT_TABLE()

	Bool						m_autoAttack;

	AnimationTrajectoryPlayer*	m_player;
	Int32							m_isPlayingAttack;

	CEdIkAttackPointItem*		m_pointItem;

	static const String			TARGET_NAME;
	Vector						m_targetPosition;

	struct AnimPoint
	{
		CEdIkAttackDummyItem*							m_item;
		const CSkeletalAnimationSetEntry*				m_animation;
		const CSkeletalAnimationAttackTrajectoryParam*	m_trajectory;

		AnimPoint() : m_item( NULL ), m_animation( NULL ), m_trajectory( NULL ) {}
	};

	TDynArray< AnimPoint >		m_animPoints;
	Int32							m_selectedAnimPoint;
	Int32							m_bestAnimPoint;
	Int32							m_nearestAnimPoint;
	//tAnimationTrajectory_Trajectory*	m_nearestTrajectoryL;
	//tAnimationTrajectory_Trajectory*	m_nearestTrajectoryR;

	Bool						m_syncBreak;
	Float						m_syncBreakDuration;

	//CPlayedSkeletalAnimation*	m_playedAnimation;
	//Bool						m_entityInZeroPoint;

	CEdAnimationParamPanel*		m_paramPanel;

public:
	CEdAnimationFriendIkAttackPage( CEdAnimationFriend* owner );
	~CEdAnimationFriendIkAttackPage();

	virtual wxAuiPaneInfo	GetPageInfo() const;

public:
	virtual void OnTick( Float dt );
	virtual void OnSelectPreviewItem( IPreviewItem* item );
	virtual void OnDeselectAllPreviewItems();
	virtual void OnGenerateFragments( CRenderFrame *frame );
	virtual void OnLoadPreviewEntity( CAnimatedComponent* component );

	virtual void OnAnimationStarted( const CPlayedSkeletalAnimation* animation );
	virtual void OnAnimationFinished( const CPlayedSkeletalAnimation* animation );
	virtual void OnAnimationStopped( const CPlayedSkeletalAnimation* animation );

public: // CEdAnimationParamPanelListener
	virtual void OnAnimationParamAddedToAnimation( const CSkeletalAnimationSetEntry* animation, const ISkeletalAnimationSetEntryParam* param );
	virtual void OnAnimationParamRemovedFromAnimation( const CSkeletalAnimationSetEntry* animation, const ISkeletalAnimationSetEntryParam* param );

public:
	void OnAttackPointChanged( const Vector& point );

private:
	CEdIkAttackDummyItem* CreateDummyItem( IPreviewItemContainer* ic, const CSkeletalAnimationSetEntry* anim, const CSkeletalAnimationAttackTrajectoryParam* traj );
	void CreatePointerItem();

	void RebuildDummyItems();

	void SetDummyItemNamesVisible( Bool flag );
	void DrawTrajectory( Int32 index, Float time, CRenderFrame *frame ) const;

	void OnSelectedItemChanged();

	void SelectAnimAsBest( const CSkeletalAnimationSetEntry* anim, Int32& index );
	const AnimationTrajectoryData* GetTrajectoryDataFromAnimation( const CSkeletalAnimationSetEntry* animation, Bool left ) const;

	void ShiftEntityToZeroPoint();

	void FireAttack();

	void BlockFireButton();
	void UnblockFireButton();

	void UpdateSyncBreak();

protected:
	void OnGhostsToggle( wxCommandEvent& event );
	void OnAttack( wxCommandEvent& event );
	void OnAutoAttack( wxCommandEvent& event );
	void OnSyncBreak( wxCommandEvent& event );
};

