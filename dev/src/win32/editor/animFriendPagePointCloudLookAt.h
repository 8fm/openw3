
#pragma once

#include "animFriendPage.h"
#include "animationParamPanel.h"
#include "animationTrajectoryParamInitializer.h"
#include "../../common/engine/animPointCloudLookAtParam.h"

class CEdAnimPointCloudLookAtParamInitializer : public CEdAnimationParamInitializer
{
public:
	Bool Initialize( ISkeletalAnimationSetEntryParam* param, const CSkeletalAnimationSetEntry* animation, const CAnimatedComponent* animatedComponent ) const;
};

//////////////////////////////////////////////////////////////////////////

class CEdAnimationPointCloudLookAtPlayerPage  : public CEdAnimationFriendSimplePage, public CEdAnimationParamPanelListener
{
	DECLARE_EVENT_TABLE()

	CEdAnimationParamPanel*				m_paramPanel;

	Int32								m_targetMode;
	const static Int32					TARGET_FIRST = 0;
	const static Int32					TARGET_MANUAL = 0;
	const static Int32					TARGET_AUTO = 1;
	const static Int32					TARGET_LAST = 2;

	Float								m_lastDt;
	AnimVector4							m_lastPos;
	Vector								m_lastPosOffset;
	Vector								m_speed;
	Vector								m_speedAng;

	Vector								m_targetWS;

	const CSkeletalAnimationSetEntry*	m_selectedAnimation;
	Int32								m_cachedBoneIndex;

public:
	CEdAnimationPointCloudLookAtPlayerPage( CEdAnimationFriend* owner );
	~CEdAnimationPointCloudLookAtPlayerPage();

public:	// CEdAnimationFriendSimplePage
	virtual wxAuiPaneInfo	GetPageInfo() const;

	virtual void OnTick( Float dt );
	virtual void OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data );
	virtual void OnGenerateFragments( CRenderFrame *frame );

	virtual void OnLoadPreviewEntity( CAnimatedComponent* component );
	virtual void OnUnloadPreviewEntity();

public: // CEdAnimationParamPanelListener
	virtual void OnAnimationParamSelectedAnimation( const CSkeletalAnimationSetEntry* animation );
	virtual void OnAnimationParamAddedToAnimation( const CSkeletalAnimationSetEntry* animation, const ISkeletalAnimationSetEntryParam* param );
	virtual void OnAnimationParamRemovedFromAnimation( const CSkeletalAnimationSetEntry* animation, const ISkeletalAnimationSetEntryParam* param );

private:
	void CacheBoneIndex();
};
