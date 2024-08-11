
#pragma once

#include "animFriendPreview.h"
#include "animFriendPage.h"
#include "animationParamPanel.h"
#include "../../common/engine/animationSelectors.h"

class CEdAnimationFriendHitReactionPage;

class CEdHitPointItem : public IPreviewItem
{
	IPreviewItemContainer*			m_container;
	CEdAnimationFriendHitReactionPage* m_page;

public:
	CEdHitPointItem( IPreviewItemContainer* ic, CEdAnimationFriendHitReactionPage* page );

	virtual IPreviewItemContainer* GetItemContainer() const;

	virtual Bool IsValid() const { return true; }
	virtual void Refresh();

	virtual void SetPositionFromPreview( const Vector& prevPos, Vector& newPos );
	virtual void SetRotationFromPreview( const EulerAngles& prevRot, EulerAngles& newRot );

	virtual Bool HasCaption() const { return false; }
};

//////////////////////////////////////////////////////////////////////////

class CEdAnimationHitParamInitializer : public CEdAnimationParamInitializer
{
public:
	Bool Initialize( ISkeletalAnimationSetEntryParam* param, const CSkeletalAnimationSetEntry* animation, const CAnimatedComponent* animatedComponent ) const;
};

//////////////////////////////////////////////////////////////////////////

class CEdAnimationFriendHitReactionPage  : public CEdAnimationFriendSimplePage, public CEdAnimationParamPanelListener
{
	DECLARE_EVENT_TABLE()

	AnimationSelector_HitPoint*	m_selector;

	CEdHitPointItem*			m_pointItem;
	Vector						m_hitPointWS;
	Vector						m_hitPointDirectionWS;

	static const String			POINTER_NAME;

	struct AnimPoint
	{
		const CSkeletalAnimationSetEntry*	m_animation;
		const CSkeletalAnimationHitParam*	m_hit;

		AnimPoint() : m_animation( NULL ), m_hit( NULL ) {}
	};

	TDynArray< AnimPoint >	m_animPoints;
	Int32						m_selectedAnimPoint;
	Int32						m_bestAnimPoint;

	CPlayedSkeletalAnimation*	m_playedAnimation;
	Bool					m_entityInZeroPoint;

	CEdAnimationParamPanel* m_paramPanel;

public:
	CEdAnimationFriendHitReactionPage( CEdAnimationFriend* owner );
	~CEdAnimationFriendHitReactionPage();

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
	virtual void OnAnimationParamAddedToAnimation( const CSkeletalAnimationSetEntry* animation, const ISkeletalAnimationSetEntryParam* param ) {}
	virtual void OnAnimationParamRemovedFromAnimation( const CSkeletalAnimationSetEntry* animation, const ISkeletalAnimationSetEntryParam* param ) {}

public:
	void OnHitPointChanged( const Vector& point );

protected:
	void OnButtonHit( wxCommandEvent& event );

private:
	void CreatePointerItem();

	void RebuildDummyItems();

	void SetDummyItemNamesVisible( Bool flag );

	void OnSelectedItemChanged();

	void SelectAnimAsBest( const CSkeletalAnimationSetEntry* anim );

	void ShiftEntityToZeroPoint();

	void FireHit();

	void StopHitAnimation();
};

