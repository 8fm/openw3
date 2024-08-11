
#pragma once

#include "animFriendPage.h"
#include "animationParamPanel.h"
#include "animationTrajectoryParamInitializer.h"
#include "../../games/r4/stepClipAnimationParam.h"

class CEdAnimationStepPlayerParamInitializer : public CEdAnimationParamInitializer
{
public:
	Bool Initialize( ISkeletalAnimationSetEntryParam* param, const CSkeletalAnimationSetEntry* animation, const CAnimatedComponent* animatedComponent ) const;
};

//////////////////////////////////////////////////////////////////////////

class CEdAnimationFriendStepPlayerPage  : public CEdAnimationFriendSimplePage
{
	DECLARE_EVENT_TABLE()

	CEdAnimationParamPanel* m_paramPanel;

	Vector					m_destDirectionWS;
	//StepPlayer*				m_stepPlayer;

public:
	CEdAnimationFriendStepPlayerPage( CEdAnimationFriend* owner );
	~CEdAnimationFriendStepPlayerPage();

	virtual wxAuiPaneInfo	GetPageInfo() const;

	virtual void OnTick( Float dt );
	virtual void OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data );
};
